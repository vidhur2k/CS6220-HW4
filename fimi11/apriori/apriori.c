/*----------------------------------------------------------------------
  File    : apriori.c
  Contents: apriori algorithm for finding frequent item sets
            (specialized version for FIMI 2003 workshop)
  Author  : Christian Borgelt
  History : 15.08.2003 file created from normal apriori.c
            16.08.2003 parameter for transaction filtering added
            18.08.2003 dynamic filtering decision based on times added
            21.08.2003 transaction sort changed to heapsort
            20.09.2003 output file made optional
----------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "tract.h"
#include "istree.h"

/*----------------------------------------------------------------------
  Preprocessor Definitions
----------------------------------------------------------------------*/
#define PRGNAME     "fim/apriori"
#define DESCRIPTION "frequent item sets miner for FIMI 2003"
#define VERSION     "version 1.7 (2003.12.02)         " \
                    "(c) 2003   Christian Borgelt"

/* --- error codes --- */
#define E_OPTION    (-5)        /* unknown option */
#define E_OPTARG    (-6)        /* missing option argument */
#define E_ARGCNT    (-7)        /* too few/many arguments */
#define E_SUPP      (-8)        /* invalid minimum support */
#define E_NOTAS     (-9)        /* no items or transactions */
#define E_UNKNOWN  (-18)        /* unknown error */

#ifndef QUIET                   /* if not quiet version */
#define MSG(x)        x         /* print messages */
#else                           /* if quiet version */
#define MSG(x)                  /* suppress messages */
#endif

#define SEC_SINCE(t)  ((clock()-(t)) /(double)CLOCKS_PER_SEC)
#define RECCNT(s)     (tfs_reccnt(is_tfscan(s)) \
                      + ((tfs_delim(is_tfscan(s)) == TFS_REC) ? 0 : 1))
#define BUFFER(s)     tfs_buf(is_tfscan(s))

/*----------------------------------------------------------------------
  Constants
----------------------------------------------------------------------*/
#ifndef QUIET                   /* if not quiet version */

/* --- error messages --- */
static const char *errmsgs[] = {
  /* E_NONE      0 */  "no error\n",
  /* E_NOMEM    -1 */  "not enough memory\n",
  /* E_FOPEN    -2 */  "cannot open file %s\n",
  /* E_FREAD    -3 */  "read error on file %s\n",
  /* E_FWRITE   -4 */  "write error on file %s\n",
  /* E_OPTION   -5 */  "unknown option -%c\n",
  /* E_OPTARG   -6 */  "missing option argument\n",
  /* E_ARGCNT   -7 */  "wrong number of arguments\n",
  /* E_SUPP     -8 */  "invalid minimal support %d\n",
  /* E_NOTAS    -9 */  "no items or transactions to work on\n",
  /*    -10 to -15 */  NULL, NULL, NULL, NULL, NULL, NULL,
  /* E_ITEMEXP -16 */  "file %s, record %d: item expected\n",
  /* E_DUPITEM -17 */  "file %s, record %d: duplicate item %s\n",
  /* E_UNKNOWN -18 */  "unknown error\n"
};
#endif

/*----------------------------------------------------------------------
  Global Variables
----------------------------------------------------------------------*/
#ifndef QUIET
static char    *prgname;        /* program name for error messages */
#endif
static ITEMSET *itemset = NULL; /* item set */
static TASET   *taset   = NULL; /* transaction set */
static TATREE  *tatree  = NULL; /* transaction tree */
static ISTREE  *istree  = NULL; /* item set tree */
static FILE    *in      = NULL; /* input  file */
static FILE    *out     = NULL; /* output file */

/*----------------------------------------------------------------------
  Main Functions
----------------------------------------------------------------------*/

static void error (int code, ...)
{                               /* --- print an error message */
  #ifndef QUIET                 /* if not quiet version */
  va_list    args;              /* list of variable arguments */
  const char *msg;              /* error message */

  assert(prgname);              /* check the program name */
  if (code < E_UNKNOWN) code = E_UNKNOWN;
  if (code < 0) {               /* if to report an error, */
    msg = errmsgs[-code];       /* get the error message */
    if (!msg) msg = errmsgs[-E_UNKNOWN];
    fprintf(stderr, "\n%s: ", prgname);
    va_start(args, code);       /* get variable arguments */
    vfprintf(stderr, msg, args);/* print error message */
    va_end(args);               /* end argument evaluation */
  }
  #endif
  #ifndef NDEBUG                /* if debug version */
  if (istree)  ist_delete(istree);
  if (tatree)  tat_delete(tatree);
  if (taset)   tas_delete(taset, 0);
  if (itemset) is_delete(itemset);
  if (in)      fclose(in);      /* clean up memory */
  if (out)     fclose(out);     /* and close files */
  #endif
  exit(code);                   /* abort the program */
}  /* error() */

/*--------------------------------------------------------------------*/

int main (int argc, char *argv[])
{                               /* --- main function */
  int     i, k, n;              /* loop variables, counters */
  char    *s;                   /* end pointer for conversion */
  char    *fn_in  = NULL;       /* name of input  file */
  char    *fn_out = NULL;       /* name of output file */
  int     tacnt   = 0;          /* number of transactions */
  int     max     = 0;          /* maximum transaction size */
  int     supp;                 /* minimal absolute support */
  int     empty   = 1;          /* number of empty item sets */
  int     *map, *set;           /* identifier map, item set */
  char    *usage;               /* flag vector for item usage */
  clock_t t, tt, tc, x;         /* timer for measurements */

  #ifndef QUIET                 /* if not quiet version */
  prgname = argv[0];            /* get program name for error msgs. */

  /* --- print usage message --- */
  if (argc > 1) {               /* if arguments are given */
    fprintf(stderr, "%s - %s\n", argv[0], DESCRIPTION);
    fprintf(stderr, VERSION); } /* print a startup message */
  else {                        /* if no arguments given */
    printf("usage: %s infile minsupp outfile\n", argv[0]);
    printf("%s\n", DESCRIPTION);
    printf("%s\n", VERSION);
    printf("infile   file to read transactions from\n");
    printf("minsupp  minimum absolute support\n");
    printf("outfile  file to write item sets to\n");
    return 0;                   /* print a usage message */
  }                             /* and abort the program */
  #endif  /* #ifndef QUIET */

  /* --- evaluate the arguments --- */
  if ((argc !=3) && (argc != 4)) error(E_ARGCNT);
  fn_in  = argv[1];             /* get the file names and the support */
  fn_out = (argc > 3) ? argv[3] : NULL;
  supp   = (int)strtol(argv[2], &s, 0);
  if ((supp <= 0) || *s) error(E_SUPP);

  /* --- create item set and transaction set --- */
  itemset = is_create();        /* create an item set and */
  if (!itemset) error(E_NOMEM); /* set the special characters */
  taset = tas_create(itemset);  /* create a transaction set */
  if (!taset) error(E_NOMEM);   /* to store the transactions */
  MSG(fprintf(stderr, "\n"));   /* terminate the startup message */

  /* --- read transactions --- */
  MSG(fprintf(stderr, "reading %s ... ", fn_in));
  t  = clock();                 /* start the timer and */
  in = fopen(fn_in, "r");       /* open the input file */
  if (!in) error(E_FOPEN, fn_in);
  for (tacnt = 0; 1; tacnt++) { /* transaction read loop */
    k = is_read(itemset, in);   /* read the next transaction */
    if (k < 0) error(k, fn_in, RECCNT(itemset), BUFFER(itemset));
    if (k > 0) break;           /* check for error and end of file */
    k = is_tsize(itemset);      /* update the maximal */
    if (k > max) max = k;       /* transaction size */
    if (taset && (tas_add(taset, NULL, 0) != 0))
      error(E_NOMEM);           /* add the loaded transaction */
  }                             /* to the transaction set */
  fclose(in); in = NULL;        /* close the input file */
  n  = is_cnt(itemset);         /* get the number of items */
  MSG(fprintf(stderr, "[%d item(s),", n));
  MSG(fprintf(stderr, " %d transaction(s)] done ", tacnt));
  MSG(fprintf(stderr, "[%.2fs].\n", SEC_SINCE(t)));

  /* --- sort and recode items --- */
  MSG(fprintf(stderr, "sorting and recoding items ... "));
  t   = clock();                /* start the timer */
  map = (int*)malloc(is_cnt(itemset) *sizeof(int));
  if (!map) error(E_NOMEM);     /* create an item identifier map */
  n = is_recode(itemset, supp, 2, map);       /* 2: sorting mode */
  tas_recode(taset, map, n);    /* recode the loaded transactions */
  max = tas_max(taset);         /* get the new maximal t.a. size */
  MSG(fprintf(stderr, "[%d item(s)] ", n));
  MSG(fprintf(stderr, "done [%.2fs].\n", SEC_SINCE(t)));

  /* --- create a transaction tree --- */
  MSG(fprintf(stderr, "creating transaction tree ... "));
  t = clock();                  /* start the timer */
  tatree = tat_create(taset,1); /* create a transaction tree */
  if (!tatree) error(E_NOMEM);  /* (compactify transactions) */
  tt = clock() -t;              /* note the construction time */
  MSG(fprintf(stderr, "done [%.2fs].\n", SEC_SINCE(t)));

  /* --- create an item set tree --- */
  MSG(fprintf(stderr, "checking subsets of size 1"));
  t = clock(); tc = 0;          /* start the timer and */
  istree = ist_create(n, supp); /* create an item set tree */
  if (!istree) error(E_NOMEM);
  for (k = n; --k >= 0; )       /* set single item frequencies */
    ist_setcnt(istree, k, is_getfrq(itemset, k));
  ist_settac(istree, tacnt);    /* set the number of transactions */
  usage = (char*)malloc(n *sizeof(char));
  if (!usage) error(E_NOMEM);   /* create a item usage vector */

  /* --- check item subsets --- */
  while (ist_height(istree) < max) {
    i = ist_check(istree,usage);/* check current item usage */
    if (i < max) max = i;       /* update the maximum set size */
    if (ist_height(istree) >= i) break;
    k = ist_addlvl(istree);     /* while max. height is not reached, */
    if (k <  0) error(E_NOMEM); /* add a level to the item set tree */
    if (k != 0) break;          /* if no level was added, abort */
    MSG(fprintf(stderr, " %d", ist_height(istree)));
    if ((i < n)                 /* check item usage on current level */
    &&  (i *(double)tt < 0.1 *n *tc)) {
      n = i; x = clock();       /* if items were removed and */
      tas_filter(taset, usage); /* the counting time is long enough, */
      tat_delete(tatree);       /* remove unnecessary items */
      tatree = tat_create(taset, 1);
      if (!tatree) error(E_NOMEM);
      tt = clock() -x;          /* rebuild the transaction tree and */
    }                           /* note the new construction time */
    x = clock();                /* start the timer */
    ist_countx(istree, tatree); /* count the transaction tree */
    tc = clock() -x;            /* in the item set tree */
  }                             /* and note the new counting time */
  MSG(fprintf(stderr, " done [%.2fs].\n", SEC_SINCE(t)));

  /* --- filter item sets --- */
  t = clock();                  /* start the timer */
  #ifdef MAXIMAL                /* filter maximal item sets */
  MSG(fprintf(stderr, "filtering maximal item sets ... "));
  ist_filter(istree, IST_MAXFRQ);
  MSG(fprintf(stderr, " done [%.2fs].\n", SEC_SINCE(t)));
  empty = (n <= 0) ? 1 : 0;     /* check whether the empty item set */
  #endif                        /* is maximal */
  #ifdef CLOSED                 /* filter closed item sets */
  MSG(fprintf(stderr, "filtering closed item sets ... "));
  ist_filter(istree, IST_CLOSED);
  MSG(fprintf(stderr, " done [%.2fs].\n", SEC_SINCE(t)));
  for (k = n; --k >= 0; )       /* check for an item in all t.a. */
    if (is_getfrq(itemset, k) == tacnt) break;
  empty = (k <= 0) ? 1 : 0;     /* check whether the empty item set */
  #endif                        /* is closed */

  /* --- print item sets --- */
  for (i = ist_height(istree); --i >= 0; )
    map[i] = 0;                 /* clear the item set counters */
  MSG(fprintf(stderr, "writing %s ... ", (fn_out) ? fn_out : "<none>"));
  t = clock();                  /* start the timer and */
  if (fn_out) {                 /* if an output file is given, */
    out = fopen(fn_out, "w");   /* open the output file */
    if (!out) error(E_FOPEN, fn_out);
    if (empty) fprintf(out, " (%d)\n", tacnt);
  }                             /* report empty item set */
  ist_init(istree);             /* init. the item set extraction */
  set = is_tract(itemset);      /* get the transaction buffer */
  for (n = empty; 1; n++) {     /* extract item sets from the tree */
    k = ist_set(istree, set, &supp);
    if (k <= 0) break;          /* get the next frequent item set */
    map[k-1]++;                 /* count the item set */
    if (fn_out) {               /* if an output file is given */
      for (i = 0; i < k; i++) { /* traverse the items */
        fputs(is_name(itemset, set[i]), out);
        fputc(' ', out);        /* print the name of the next item */
      }                         /* followed by a separator */
      fprintf(out, "(%d)\n", supp);
    }                           /* print the item set's support */
  }
  if (fn_out) {                 /* if an output file is given */
    if (fflush(out) != 0) error(E_FWRITE, fn_out);
    if (out != stdout) fclose(out);
    out = NULL;                 /* close the output file */
  }
  MSG(fprintf(stderr, "[%d set(s)] done ", n));
  MSG(fprintf(stderr, "[%.2fs].\n", SEC_SINCE(t)));

  /* --- print item set statistics --- */
  k = ist_height(istree);       /* find last nonzero counter */
  if ((k > 0) && (map[k-1] <= 0)) k--;
  printf("%d\n", empty);        /* print the numbers of item sets */
  for (i = 0; i < k; i++) printf("%d\n", map[i]);

  /* --- clean up --- */
  #ifndef NDEBUG                /* if this is a debug version */
  free(usage);                  /* delete the item usage vector */
  free(map);                    /* and the identifier map */
  ist_delete(istree);           /* delete the item set tree, */
  if (tatree) tat_delete(tatree);     /* the transaction tree, */
  if (taset)  tas_delete(taset, 0);   /* the transaction set, */
  is_delete(itemset);                 /* and the item set */
  #endif
  return 0;                     /* return 'ok' */
}  /* main() */
