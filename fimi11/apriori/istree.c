/*----------------------------------------------------------------------
  File    : istree.c
  Contents: item set tree management
            (specialized version for FIMI 2003 workshop)
  Author  : Christian Borgelt
  History : 15.08.2003 file created from apriori istree.c
            02.12.2003 skipping unnecessary subtrees added (_stskip)
----------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <assert.h>
#include "istree.h"
#ifdef STORAGE
#include "storage.h"
#endif

/*----------------------------------------------------------------------
  Preprocessor Definitions
----------------------------------------------------------------------*/
#define BLKSIZE    32           /* block size for level vector */
#define F_SKIP     INT_MIN      /* flag for subtree skipping */

/*----------------------------------------------------------------------
  Auxiliary Functions
----------------------------------------------------------------------*/

static void _count (ISNODE *node, int *set, int cnt, int min)
{                               /* --- count transaction recursively */
  int    i, n;                  /* vector index and size */
  ISNODE **vec;                 /* child node vector */

  assert(node                   /* check the function arguments */
      && (cnt >= 0) && (set || (cnt <= 0)));
  if (node->chcnt == 0) {       /* if this is a new node */
    n = node->offset;           /* get the index offset */
    while ((cnt > 0) && (*set < n)) {
      cnt--; set++; }           /* skip items before first counter */
    while (--cnt >= 0) {        /* traverse the transaction's items */
      i = *set++ -n;            /* compute counter vector index */
      if (i >= node->size) return;
      node->cnts[i]++;          /* if the counter exists, */
    } }                         /* count the transaction */
  else if (node->chcnt > 0) {   /* if there are child nodes */
    vec = (ISNODE**)(node->cnts +node->size);
    n   = vec[0]->id;           /* get the child node vector */
    min--;                      /* one item less to the deepest nodes */
    while ((cnt > min) && (*set < n)) {
      cnt--; set++; }           /* skip items before first child */
    while (--cnt >= min) {      /* traverse the transaction's items */
      i = *set++ -n;            /* compute child vector index */
      if (i >= node->chcnt) return;
      if (vec[i]) _count(vec[i], set, cnt, min);
    }                           /* if the child exists, */
  }                             /* count the transaction recursively */
}  /* _count() */

/*--------------------------------------------------------------------*/

static void _countx (ISNODE *node, TATREE *tat, int min)
{                               /* --- count t.a. tree recursively */
  int    i, k, n;               /* vector index, loop variable, size */
  ISNODE **vec;                 /* child node vector */

  assert(node && tat);          /* check the function arguments */
  if (tat_max(tat) < min)       /* if the transactions are too short, */
    return;                     /* abort the recursion */
  k = tat_size(tat);            /* get the number of children */
  if (k <= 0) {                 /* if there are no children */
    if (k < 0) _count(node, tat_items(tat), -k, min);
    return;                     /* count the normal transaction */
  }                             /* and abort the function */
  while (--k >= 0)              /* count the transactions recursively */
    _countx(node, tat_child(tat, k), min);
  if (node->chcnt == 0) {       /* if this is a new node */
    n = node->offset;           /* get the index offset */
    for (k = tat_size(tat); --k >= 0; ) {
      i = tat_item(tat,k) -n;   /* traverse the items */
      if (i < 0) return;        /* if before first item, abort */
      if (i < node->size)       /* if inside the counter range */
        node->cnts[i] += tat_cnt(tat_child(tat, k));
    } }                         /* count the transaction */
  else if (node->chcnt > 0) {   /* if there are child nodes */
    vec = (ISNODE**)(node->cnts +node->size);
    n   = vec[0]->id;           /* get the child node vector */
    min--;                      /* one item less to the deepest nodes */
    for (k = tat_size(tat); --k >= 0; ) {
      i = tat_item(tat,k) -n;   /* traverse the items */
      if (i < 0) return;        /* if before first item, abort */
      if ((i < node->chcnt) && vec[i])
        _countx(vec[i], tat_child(tat, k), min);
    }                           /* if the child exists, */
  }                             /* count the transaction recursively */
}  /* _countx() */

/*--------------------------------------------------------------------*/

static int _stskip (ISNODE *node)
{                               /* --- set subtree skip flags */
  int    i, r;                  /* vector index, result */
  ISNODE **vec;                 /* child node vector */

  assert(node);                 /* check the function argument */
  if (node->chcnt == 0) return  0;  /* do not skip new leaves */
  if (node->chcnt <  0) return -1;  /* skip marked subtrees */
  vec = (ISNODE**)(node->cnts +node->size);
  for (r = -1, i = node->chcnt; --i >= 0; )
    if (vec[i]) r &= _stskip(vec[i]);
  if (!r) return 0;             /* recursively check all children */
  node->chcnt |= F_SKIP;        /* set the skip flag if possible */
  return -1;                    /* return 'skip subtree' */
}  /* _stskip() */

/*--------------------------------------------------------------------*/

static int _check (ISNODE *node, char *marks, int supp)
{                               /* --- recursively check item usage */
  int    i, r = 0, n;           /* vector index, result of check */
  ISNODE **vec;                 /* child node vector */

  assert(node && marks);        /* check the function arguments */
  if (node->chcnt == 0) {       /* if this is a new node */
    n = node->offset;           /* get the index offset */
    for (i = node->size; --i >= 0; ) {
      if (node->cnts[i] >= supp)
        marks[n+i] = r = 1;     /* mark items in set that satisfies */
    } }                         /* the minimum support criterion */
  else if (node->chcnt > 0) {   /* if there are child nodes */
    vec = (ISNODE**)(node->cnts +node->size);
    for (i = node->chcnt; --i >= 0; )
      if (vec[i]) r |= _check(vec[i], marks, supp);
  }                             /* recursively process all children */
  if ((r != 0) && node->parent) /* if the check succeeded, mark */
    marks[node->id] = 1;        /* the item associated with the node */
  return r;                     /* return the check result */
}  /* _check() */

/*--------------------------------------------------------------------*/

static int _getsupp (ISNODE *node, int *set, int cnt)
{                               /* --- get support of an item set */
  int    i, c;                  /* vector index, number of children */
  ISNODE **vec;                 /* vector of child nodes */

  assert(node && set && (cnt >= 0)); /* check the function arguments */
  while (--cnt > 0) {           /* follow the set/path from the node */
    c = node->chcnt & ~F_SKIP;  /* if there are no children, */
    if (c == 0) return -1;      /* the support is less than minsupp */
    vec = (ISNODE**)(node->cnts +node->size);
    i   = *set++ -vec[0]->id;   /* compute the child vector index */
    if ((i < 0) || (i >= c))    /* if the index is out of range, */
      return -1;                /* abort the recursion */
    node = vec[i];              /* go to the corresponding child */
    if (!node) return -1;       /* if the child does not exists, */
  }                             /* the support is less than minsupp */
  i = *set -node->offset;       /* compute the counter index */
  if ((i < 0) || (i >= node->size))
    return -1;                  /* abort if index is out of range */
  return node->cnts[i];         /* return the item set support */
}  /* _getsupp() */

/*--------------------------------------------------------------------*/

static void _clrsupp (ISNODE *node, int *set, int cnt, int supp)
{                               /* --- clear support of an item set */
  int    i;                     /* vector index */
  ISNODE **vec;                 /* vector of child nodes */

  assert(node && set && (cnt >= 0)); /* check the function arguments */
  while (--cnt > 0) {           /* follow the set/path from the node */
    vec = (ISNODE**)(node->cnts +node->size);
    i   = *set++ -vec[0]->id;   /* compute the child vector index */
    node = vec[i];              /* go to the corresponding child */
  }
  i = *set -node->offset;       /* compute the counter index */
  if ((supp < 0)                /* if to clear unconditionally */
  ||  (node->cnts[i] == supp))  /* or the support is the same, */
    node->cnts[i] = 0;          /* clear the item set support */
}  /* _clrsupp() */

/*--------------------------------------------------------------------*/

static ISNODE* _child (ISTREE *ist, ISNODE *node, int index)
{                               /* --- create child node (extend set) */
  int    i, k, n;               /* loop variables, counters */
  ISNODE *curr;                 /* to traverse the path to the root */
  int    item, cnt;             /* item identifier, number of items */
  int    *set;                  /* next (partial) item set to check */
  int    supp;                  /* support of an item set */

  assert(ist && node            /* check the function arguments */
     && (index >= 0) && (index < node->size));

  /* --- initialize --- */
  supp = node->cnts[index];     /* get support of item set to extend */
  if (supp < ist->supp)         /* if set support is insufficient, */
    return NULL;                /* no child is needed, so abort */
  item = node->offset +index;   /* initialize set for support checks */
  ist->buf[ist->lvlvsz -2] = item;

  /* --- check candidates --- */
  for (n = 0, i = index; ++i < node->size; ) {
    supp = node->cnts[i];       /* traverse the candidate items */
    if (supp < ist->supp)       /* if set support is insufficient, */
      continue;                 /* ignore the corresponding candidate */
    set    = ist->buf +ist->lvlvsz -(cnt = 2);
    set[1] = k = node->offset +i;  /* add the item to the set */
    for (curr = node; curr->parent; curr = curr->parent) {
      supp = _getsupp(curr->parent, set, cnt);
      if (supp < ist->supp)     /* get the item set support and */
        break;                  /* if it is too low, abort the loop */
      *--set = curr->id; cnt++; /* add id of current node to the set */
    }                           /* and adapt the number of items */
    if (!curr->parent)          /* if subset support is high enough */
      ist->map[n++] = k;        /* note the item identifier */
  }
  if (n <= 0) return NULL;      /* if no child is needed, abort */

  /* --- create child --- */
  k = ist->map[n-1] -ist->map[0] +1;
  curr = (ISNODE*)malloc(sizeof(ISNODE) +(k-1) *sizeof(int));
  if (!curr) return (void*)-1;  /* create a child node */
  curr->parent = node;          /* set pointer to parent node */
  curr->succ   = NULL;          /* and clear successor pointer */
  curr->id     = item;          /* initialize the item id. and */
  curr->chcnt  = 0;             /* there are no children yet */
  curr->size   = k;             /* set size of counter vector */
  curr->offset = ist->map[0];   /* note the first item as an offset */
  for (set = curr->cnts +(i = k); --i >= 0; )
    *--set = 0;                 /* clear all counters of the node */
  return curr;                  /* return pointer to created child */
}  /* _child() */

/*----------------------------------------------------------------------
  In the above function the set S represented by the index-th vector
element of the current node is extended only by combining it with the
sets represented by the fields that follow it in the node vector,
i.e. by the sets represented by vec[index+1] to vec[size-1]. The sets
that can be formed by combining the set S and the sets represented by
vec[0] to vec[index-1] are processed in the branches for these sets.
  In the 'check candidates' loop it is checked for each set represented
by vec[index+1] to vec[size-1] whether this set and all other subsets
of the same size, which can be formed from the union of this set and
the set S, have enough support, so that a child node is necessary.
  Note that i +offset is the identifier of the item that has to be
added to set S to form the union of the set S and the set T represented
by vec[i], since S and T have the same path with the exception of the
index in the current node. Hence we can speak of candidate items that
are added to S.
  Checking the support of the other subsets of the union of S and T
that have the same size as S and T is done with the aid of a path
variable. The items in this variable combined with the items on the
path to the current node always represent the subset currently tested.
That is, the path variable holds the path to be followed from the
current node to arrive at the support counter for the subset. The path
variable is initialized to [0]: <item>, [1]: <offset+i>, since the
support counters for S and T can be inspected directly. Then this
path is followed from the parent node of the current node, which is
equivalent to checking the subset that can be obtained by removing
from the union of S and T the item that corresponds to the parent node
(in the path to S or T, resp.).
  Iteratively making the parent node the current node, adding its
corresponding item to the path and checking the support counter at the
end of the path variable when starting from its (the new current node's)
parent node tests all other subsets.
----------------------------------------------------------------------*/

static void _cleanup (ISTREE *ist)
{                               /* --- clean up on error */
  ISNODE *node, *t;             /* to traverse the nodes */

  assert(ist);                  /* check the function argument */
  for (node = ist->levels[ist->lvlcnt]; node; ) {
    t = node; node = node->succ; free(t); }
  ist->levels[ist->lvlcnt] = NULL; /* delete all created nodes */
  for (node = ist->levels[ist->lvlcnt -1]; node; node = node->succ)
    node->chcnt = 0;            /* clear the child node counters */
}  /* _cleanup() */             /* of the deepest nodes in the tree */

/*----------------------------------------------------------------------
  Main Functions
----------------------------------------------------------------------*/

ISTREE* ist_create (int itemcnt, int supp)
{                               /* --- create an item set tree */
  ISTREE *ist;                  /* created item set tree */
  ISNODE **lvl;                 /* vector of tree levels */
  ISNODE *root;                 /* root node of the tree */
  int    *buf;                  /* buffer vector */

  assert((itemcnt >= 0) && (supp >= 1));  /* check function arguments */

  /* --- allocate memory --- */ 
  ist = (ISTREE*)malloc(sizeof(ISTREE) +(itemcnt-1) *sizeof(int));
  if (!ist) return NULL;        /* allocate the tree body */
  ist->levels = lvl = (ISNODE**)malloc(BLKSIZE *sizeof(ISNODE*));
  if (!lvl) { free(ist); return NULL; }
  ist->buf    = buf = (int*)    malloc(BLKSIZE *sizeof(int));
  if (!buf) { free(lvl); free(ist); return NULL; }
  lvl[0] = ist->curr = root =   /* allocate a root node */
    (ISNODE*)calloc(1, sizeof(ISNODE) +(itemcnt-1) *sizeof(int));
  if (!root){ free(buf); free(lvl); free(ist); return NULL; }

  /* --- initialize structures --- */
  ist->lvlvsz  = BLKSIZE;       /* copy parameters to the structure */
  ist->lvlcnt  = 1;
  ist->tacnt   = 0;
  ist->supp    = supp;
  ist_init(ist);                /* initialize item set extraction */
  root->parent = root->succ = NULL;
  root->offset = root->id   = 0;
  root->chcnt  = 0;             /* initialize the root node */
  root->size   = itemcnt;
  return ist;                   /* return created item set tree */
}  /* ist_create() */

/*--------------------------------------------------------------------*/

void ist_delete (ISTREE *ist)
{                               /* --- delete an item set tree */
  int    i;                     /* loop variables */
  ISNODE *node, *t;             /* to traverse the nodes */

  assert(ist);                  /* check the function argument */
  for (i = ist->lvlcnt; --i >= 0; ) {
    for (node = ist->levels[i]; node; ) {
      t = node; node = node->succ; free(t); }
  }                             /* delete all nodes, */
  free(ist->levels);            /* the level vector, */
  free(ist->buf);               /* the path buffer, */
  free(ist);                    /* and the tree body */
}  /* ist_delete() */

/*--------------------------------------------------------------------*/

void ist_count (ISTREE *ist, int *set, int cnt)
{                               /* --- count transaction in tree */
  assert(ist                    /* check the function arguments */
     && (cnt >= 0) && (set || (cnt <= 0)));
  if (cnt >= ist->lvlcnt)       /* recursively count transaction */
    _count(ist->levels[0], set, cnt, ist->lvlcnt);
  ist->tacnt++;                 /* increment the transaction counter */
}  /* ist_count() */

/*--------------------------------------------------------------------*/

void ist_countx (ISTREE *ist, TATREE *tat)
{                               /* --- count transaction in tree */
  assert(ist && tat);           /* check the function arguments */
  _countx(ist->levels[0], tat, ist->lvlcnt);
  ist->tacnt = tat_cnt(tat);    /* recursively count the tree and */
}  /* ist_countx() */           /* set the transaction counter */

/*--------------------------------------------------------------------*/

int ist_check (ISTREE *ist, char *marks)
{                               /* --- check item usage */
  int i, n;                     /* loop variable, number of items */

  assert(ist);                  /* check the function argument */
  for (i = ist->levels[0]->size; --i >= 0; )
    marks[i] = 0;               /* clear the marker vector */
  _check(ist->levels[0], marks, ist->supp);
                                /* recursively check items */
  for (n = 0, i = ist->levels[0]->size; --i >= 0; )
    if (marks[i]) n++;          /* count used items */
  return n;                     /* and return this number */
}  /* ist_check() */

/*--------------------------------------------------------------------*/

int ist_addlvl (ISTREE *ist)
{                               /* --- add a level to item set tree */
  int    i, n;                  /* loop variable, counter */
  ISNODE **ndp;                 /* to traverse the nodes */
  ISNODE *node;                 /* new (reallocated) node */
  ISNODE **end;                 /* end of new level node list */
  ISNODE *cur;                  /* current node in new level */
  ISNODE *frst;                 /* first child of current node */
  ISNODE *last;                 /* last  child of current node */
  ISNODE **vec;                 /* child node vector */
  void   *p;                    /* temporary buffer */

  assert(ist);                  /* check the function arguments */

  /* --- enlarge level vector --- */
  if (ist->lvlcnt >= ist->lvlvsz) { /* if the level vector is full */
    n = ist->lvlvsz +BLKSIZE;   /* compute new vector size */
    p = realloc(ist->levels, n *sizeof(ISNODE*));
    if (!p) return -1;          /* enlarge the level vector */
    ist->levels = (ISNODE**)p;  /* and set the new vector */
    p = realloc(ist->buf,    n *sizeof(int));
    if (!p) return -1;          /* enlarge the buffer vector */
    ist->buf    = (int*)p;      /* and set the new vector */
    ist->lvlvsz = n;            /* set the new vector size */
  }                             /* (applies to buf and levels) */
  end  = ist->levels +ist->lvlcnt;
  *end = NULL;                  /* start a new tree level */

  /* --- add tree level --- */
  for (ndp = ist->levels +ist->lvlcnt -1; *ndp; ndp = &(*ndp)->succ) {
    frst = last = NULL;         /* traverse the deepest nodes */
    for (i = n = 0; i < (*ndp)->size; i++) {
      cur = _child(ist, *ndp, i);
      if (!cur) continue;       /* create a child if necessary */
      if (cur == (void*)-1) { _cleanup(ist); return -1; }
      if (!frst) frst = cur;    /* note first and last child node */
      *end = last = cur;        /* add node at the end of the list */
      end  = &cur->succ; n++;   /* that contains the new level */
    }                           /* and advance end pointer */
    if (n <= 0) {               /* if no child node was created, */
      (*ndp)->chcnt = F_SKIP; continue; }       /* skip the node */
    node = *ndp;                /* decide on the node structure: */
    n = last->id -frst->id +1;  /* always add a pure child vector */
    i = (node->size -1) *sizeof(int) +n *sizeof(ISNODE*);
    node = (ISNODE*)realloc(node, sizeof(ISNODE) +i);
    if (!node) { _cleanup(ist); return -1; }
    node->chcnt = n;            /* add a child vector to the node */
    if ((node != *ndp) && node->parent) {
      last = node->parent;      /* adapt the ref. from the parent */
      vec  = (ISNODE**)(last->cnts +last->size);
      vec[(vec[0] != *ndp) ? node->id -vec[0]->id : 0] = node;
    }                           /* set the new child pointer */
    *ndp = node;                /* set new (reallocated) node */
    vec = (ISNODE**)(node->cnts +node->size);
    while (--n >= 0) vec[n] = NULL;
    i = frst->id;               /* get item identifier of first child */
    for (cur = frst; cur; cur = cur->succ) {
      vec[cur->id -i] = cur;    /* set the child node pointer */
      cur->parent     = node;   /* and the parent pointer */
    }                           /* in the new node */
  }
  if (!ist->levels[ist->lvlcnt])/* if no child has been added, */
    return 1;                   /* abort the function, otherwise */
  ist->lvlcnt++;                /* increment the level counter */
  ist->tacnt = 0;               /* clear the transaction counter and */
  ist->node  = NULL;            /* the item set node for rule extr. */
  _stskip(ist->levels[0]);      /* mark subtrees to be skipped */
  return 0;                     /* return 'ok' */
}  /* ist_addlvl() */

/*--------------------------------------------------------------------*/

void ist_setcnt (ISTREE *ist, int item, int cnt)
{                               /* --- set counter for an item */
  ISNODE *node;                 /* the current node */

  assert(ist && ist->curr);     /* check the function argument */
  node  = ist->curr;            /* get the current node */
  item -= node->offset;         /* get index in counter vector */
  if ((item >= 0) && (item < node->size))
    node->cnts[item] = cnt;     /* set the frequency counter */
}  /* ist_setcnt() */

/*--------------------------------------------------------------------*/

void ist_filter (ISTREE *ist, int mode)
{                               /* --- filter max. freq. item sets */
  int    i, k, n;               /* loop variables */
  ISNODE *node, *curr;          /* to traverse the nodes */
  int    supp;                  /* support of an item set */
  int    *set;                  /* next (partial) item set to process */

  assert(ist);                  /* check the function argument */
  for (n = 1; n < ist->lvlcnt; n++) {
    for (node = ist->levels[n]; node; node = node->succ) {
      for (i = 0; i < node->size; i++) {
        if (node->cnts[i] < ist->supp)
          continue;             /* skip infrequent item sets */
        supp   = (mode == IST_CLOSED) ? node->cnts[i] : -1;
        k      = node->offset +i;
        set    = ist->buf +ist->lvlvsz;
        *--set = k;        _clrsupp(node->parent, set, 1, supp);
        *--set = node->id; _clrsupp(node->parent, set, 1, supp);
        k = 2;                  /* clear counters in parent node */
        for (curr = node->parent; curr->parent; curr = curr->parent) {
          _clrsupp(curr->parent, set, k, supp);
          *--set = curr->id; k++;
        }                       /* climb up the tree and use the */
      }                         /* constructed (partial) item sets */
    }                           /* as paths to find the counters */
  }                             /* that have to be cleared */
}  /* ist_filter() */

/*--------------------------------------------------------------------*/

void ist_init (ISTREE *ist)
{                               /* --- initialize (rule) extraction */
  assert(ist);                  /* check the function argument */
  ist->index = ist->item = -1;
  ist->node  = ist->head = NULL;
  ist->size  = 1;               /* initialize rule extraction */
}  /* ist_init() */

/*--------------------------------------------------------------------*/

int ist_set (ISTREE *ist, int *set, int *supp)
{                               /* --- extract next frequent item set */
  int    i;                     /* loop variable */
  ISNODE *node;                 /* current item set node */

  assert(ist && set && supp);   /* check the function arguments */

  /* --- initialize --- */
  if (ist->size > ist->lvlcnt)  /* if the tree is not high enough */
    return -1;                  /* for the item set size, abort */
  if (!ist->node)               /* on first item set, initialize */
    ist->node = ist->levels[ist->size-1];    /* the current node */
  node = ist->node;             /* get the current item set node */

  /* --- find frequent item set --- */
  while (1) {                   /* search for a frequent item set */
    if (++ist->index >= node->size) { /* if all subsets have been */
      node = node->succ;        /* processed, go to the successor */
      if (!node) {              /* if at the end of a level, go down */
        if (++ist->size > ist->lvlcnt)
          return -1;            /* if beyond the deepest level, abort */
        node = ist->levels[ist->size -1];
      }                         /* get the 1st node of the new level */
      ist->node  = node;        /* note the new item set node */
      ist->index = 0;           /* start with the first item set */
    }                           /* of the new item set node */
    i = node->cnts[ist->index];
    if (i >= ist->supp) break;  /* if the support is sufficient, */
  }                             /* abort the search loop */

  /* --- build frequent item set --- */
  *supp    = i;                 /* note the set support */
  i        = ist->size;         /* get the current item set size and */
  set[--i] = node->offset +ist->index;       /* store the first item */
  while (node->parent) {        /* while not at the root node */
    set[--i] = node->id;        /* add item to the item set */
    node = node->parent;        /* and go to the parent node */
  }
  return ist->size;             /* return item set size */
}  /* ist_set() */
