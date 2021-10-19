/*----------------------------------------------------------------------
  File    : istree.h
  Contents: item set tree management
            (specialized version for FIMI 2003 workshop)
  Author  : Christian Borgelt
  History : 15.08.2003 file created from apriori istree.h
----------------------------------------------------------------------*/
#ifndef __ISTREE__
#define __ISTREE__
#include "tract.h"

/*----------------------------------------------------------------------
  Preprocessor Definitions
----------------------------------------------------------------------*/
/* --- item set filter modes --- */
#define IST_MAXFRQ  0           /* maximally frequent item sets */
#define IST_CLOSED  1           /* closed item sets */

/*----------------------------------------------------------------------
  Type Definitions
----------------------------------------------------------------------*/
typedef struct _isnode {        /* --- item set node --- */
  struct _isnode *parent;       /* parent node */
  struct _isnode *succ;         /* successor node on same level */
  int            id;            /* identifier used in parent node */
  int            chcnt;         /* number of child nodes */
  int            size;          /* size   of counter vector */
  int            offset;        /* offset of counter vector */
  int            cnts[1];       /* counter vector */
} ISNODE;                       /* (item set node) */

typedef struct {                /* --- item set tree --- */
  int    tacnt;                 /* number of transactions counted */
  int    lvlvsz;                /* size of level vector */
  int    lvlcnt;                /* number of levels (tree height) */
  ISNODE **levels;              /* first node of each level */
  int    supp;                  /* minimal support of an item set */
  ISNODE *curr;                 /* current node for traversal */
  int    size;                  /* size of item set/rule/hyperedge */
  ISNODE *node;                 /* item set node for extraction */
  int    index;                 /* index in item set node */
  ISNODE *head;                 /* head item node for extraction */
  int    item;                  /* head item of previous rule */
  int    *buf;                  /* buffer for paths (support check) */
  int    *path;                 /* current path / (partial) item set */
  int    plen;                  /* current path length */
  int    map[1];                /* to collect items of a new node */
} ISTREE;                       /* (item set tree) */

/*----------------------------------------------------------------------
  Functions
----------------------------------------------------------------------*/
extern ISTREE* ist_create  (int itemcnt, int supp);
extern void    ist_delete  (ISTREE *ist);
extern int     ist_itemcnt (ISTREE *ist);

extern void    ist_count   (ISTREE *ist, int *set, int cnt);
extern void    ist_countx  (ISTREE *ist, TATREE *tat);
extern int     ist_settac  (ISTREE *ist, int cnt);
extern int     ist_gettac  (ISTREE *ist);
extern int     ist_check   (ISTREE *ist, char *marks);
extern int     ist_addlvl  (ISTREE *ist);
extern int     ist_height  (ISTREE *ist);

extern void    ist_setcnt  (ISTREE *ist, int item, int cnt);
extern int     ist_getcnt  (ISTREE *ist, int item);
extern int     ist_getcntx (ISTREE *ist, int *set, int cnt);

extern void    ist_filter  (ISTREE *ist, int mode);
extern void    ist_init    (ISTREE *ist);
extern int     ist_set     (ISTREE *ist, int *set, int *supp);

/*----------------------------------------------------------------------
  Preprocessor Definitions
----------------------------------------------------------------------*/
#define ist_itemcnt(t)     ((t)->levels[0]->size)
#define ist_settac(t,n)    ((t)->tacnt = (n))
#define ist_gettac(t)      ((t)->tacnt)
#define ist_height(t)      ((t)->lvlcnt)

#endif
