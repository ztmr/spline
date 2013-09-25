/*
 * $Id: LISP_Core.h 76 2009-09-14 11:51:40Z tmr $
 *
 * Module:  LISP_Core -- LISP Machine interfaces and structures
 * Created: 30-NOV-2007 23:54
 * Author:  tmr
 */

#ifndef _LISP_CORE_H_
#define _LISP_CORE_H_

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define  LISP$MACH_ID   "LISP Machine V1.0 $Rev: 76 $"

#ifndef _HAS_STRL_
#define strlcpy(dst, src, size)  strncpy(dst, src, size)
#define strlcat(dst, src, size)  strncat(dst, src, size)
#endif

#ifdef _VMS_
#include <ssdef.h>
#else
#define SS$_SUCCESS   0
#endif


/******************************************************
 * LISP Definitions
 ******************************************************/

/* List, atom and number table size */
#define LISP$MACH_LSTTABLEN     0x17FF
#define LISP$MACH_ATMTABLEN     0x17FF
#define LISP$MACH_NUMTABLEN     0x17FF

/* Maximal atom name size */
#define LISP$MACH_ATMNAMELEN      64
#define LISP$MACH_INPMAXLEN     1024

/* Dealing with special numbers: +/-Infinite and Not a Number values */
#define LISP$INF         (1.0 / 0.0)                   /* +Inf def.  */
#define LISP$IS_INF(x)   ((double) (x) == (LISP$INF))  /* Is +Inf?   */
#define LISP$IS_NINF(x)  (LISP$IS_INF (-(x)))          /* Is -Inf?   */
#define LISP$NAN         (0.0 / 0.0)                   /* Not a Num. */

/* Macros for manipulating with references */
#define LISP$M_NULLREF               -1
#define LISP$M_setRef(ref,id,type)   ((ref) = (((id) << 2) | (type)))
#define LISP$M_getRefType(ref)       ((ref) & 3)
#define LISP$M_getRefId(ref)         ((ref) >> 2)
#define LISP$M_setRefType(ref,type)  (LISP$M_setRef ((ref), \
                                        LISP$M_getRefId (ref), type))
#define LISP$M_setRefId(ref,id)      (LISP$M_setRef ((ref), id, \
                                        LISP$M_getRefType (ref)))

/* Error name and message maximal lengths */
#define LISP$ERROR_NAMLEN         32
#define LISP$ERROR_MSGLEN        256

/* Error (neg.) and warning (pos.) codes; P=Parser, E=Eval, C=Core   */
#define LISP$_ERR$S_OK             0 /*  : Everything OK             */
#define LISP$_ERR$S_NOEVAL         1 /* P: No result from evaluator  */
#define LISP$_ERR$S_BADSYN         2 /* P: Bad input syntax          */
#define LISP$_ERR$S_LONGTOK        3 /* P: Too long token            */
#define LISP$_ERR$S_IVARGS         4 /* E: Invalid arguments         */
#define LISP$_ERR$S_NOTIMP         5 /* E: Not implemented yet       */
#define LISP$_ERR$S_UNDEF          6 /* E: Atom is not defined       */
#define LISP$_ERR$S_NAOFIL         6 /*  : Unable to open file       */
#define LISP$_ERR$S_ATMFUL        -1 /* C: Atom table is full        */
#define LISP$_ERR$S_LSTFUL        -2 /* C: List table is full        */
#define LISP$_ERR$S_NUMFUL        -3 /* C: Number table is full      */

/* S-Expression helpers */
/* NOTE: The first LISP used M-expressions, so if you like them,
 * feel free to change $S_PAR_OPEN to '[' and $S_PAR_CLOSE to ']' */
#define LISP$S_PAR_OPEN          '('  /* List beginning           */
#define LISP$S_PAR_CLOSE         ')'  /* End of list              */
#define LISP$S_APOSTROPHE       '\''  /* QUOTE 'syntax sugar'     */
#define LISP$S_EOS              '\0'  /* End Of Stream/String     */
#define LISP$S_EOL              '\n'  /* End Of Line              */
#define LISP$S_COMMENT           ';'  /* Comment beginning char   */
#define LISP$S_FLPDLM            '.'  /* Floating-point delimiter */
#define LISP$S_TOKEN_SIZE        256  /* Maximal token length     */

/* Option names */
#define LISP$M_OPT_TRACE "LISP$TRACE" /* Trace when evaluating    */
#define LISP$M_OPT_LOG   "LISP$LOG"   /* Log messages into a file */
#define LISP$M_OPT_GC    "LISP$GC"    /* Collect garbage          */

/* Misc */
#define LISP$ADDRFMT         "0x%04X" /* Something like '%p'       */
#define LISP$PROMPT          "--> "   /* Prompt for the first line */
#define LISP$PROMPT_ALT      "... "   /* Prompt for >1. input line */
#define LISP$PROMPT_RES      "<-- "   /* $S_write's result prompt */


/******************************************************
 * LISP Machine data structures
 ******************************************************/

/* Machine error indicator */
typedef struct {
  int  code;                        /* Error numeric code     */
  char name    [LISP$ERROR_NAMLEN]; /* Error string  code     */
  char message [LISP$ERROR_MSGLEN]; /* Full error description */
} LISP$Error;

/* Table type enumeration */
typedef enum { ListTab, AtomTab, NumberTab } LISP$TableType;

/* Reference to each LISP object in memory               */
/* Note: LISP$Ref was a structure in original design,    *
 * this was changed in V10R32 to better "typed-pointers" */
typedef int LISP$Ref;

/* List table record */
typedef struct {
  LISP$Ref car; /* Contents of Address Register   */
  LISP$Ref cdr; /* Contents of Decrement Register */
} LISP$ListRecord;

/* Atom type enumeration */
typedef enum {
  Undefined,
  Variable,        /* Ordinary atom           */
//  Number,          /* Number   atom           */
//  DottedPair,      /* Non-atomic S-Expression */
  BuiltInFun,
  BuiltInSpecForm,
  UserDefFun,
  UserDefSpecForm,
  UnnamedFun,
  UnnamedSpecForm
} LISP$AtomType;

/* Atom table record */
typedef struct {
  char          name [LISP$MACH_ATMNAMELEN];
  LISP$AtomType type;     /* Type of this AtomRecord */
  LISP$Ref value;    /* Link to list or number value */
  LISP$Ref bindList; /* Bind     list                */
//  void *        propList; /* Property list                */
} LISP$AtomRecord;

typedef enum { Interactive, Batch } LISP$Mode;

/* State of memory node -- used by GC and lst/num/atm lookup/alloc funs *
 * Let's describe the meaning of them:                                  *
 *   Free ... node can be used as storage for a newly created item      *
 *   Used ... node is already used (permanently)                        *
 *   Temp ... superposition of two previous -- node is probably used,   *
 *            but next garbage collection will make it Free             *
 *   Prot ... protected node for internal use                           */
typedef enum { Free, Used, Temp, Prot } LISP$MemState;

/* LISP Machine Instance (LMI) */
typedef struct {
  LISP$Error      error;                        /* LMI error indicator  */

  FILE *          logStream;                    /* Log file stream      */
  FILE *          inputStream;                  /* S-expr input stream  */
  FILE *          outputStream;                 /* S-expr output stream */
#ifdef _HAS_RDLN_
  char *          readLineBuffer;               /* ReadLine's inp. buf. */
#endif
  char            inputBuffer [LISP$MACH_INPMAXLEN]; /* Recently read   */
  int             inputReadPosition;            /* Current getChar pos. */
  int             inputErrorPosition;           /* Incorrect input pos. */

  bool            isReady;                      /* Is initialized?      */
  bool            is2ShutDown;                  /* Is LMI to be halted? */
  bool            is2Process;                   /* Is there st2be read? */
  bool            sexprEnd;                     /* S-expr read "lock"   */

  LISP$Mode       mode;                         /* Mode of processing   */

  LISP$MemState   lstIdx [LISP$MACH_LSTTABLEN]; /* Used lists   index   */
  LISP$MemState   atmIdx [LISP$MACH_ATMTABLEN]; /* Used atom    index   */
  LISP$MemState   numIdx [LISP$MACH_NUMTABLEN]; /* Used numbers index   */

  LISP$ListRecord lstTab [LISP$MACH_LSTTABLEN]; /* List table           */
  LISP$AtomRecord atmTab [LISP$MACH_ATMTABLEN]; /* Atom table           */
  double          numTab [LISP$MACH_NUMTABLEN]; /* Number table         */

  LISP$Ref        QUOTE;                        /* QUOTE reference      */
  LISP$Ref        NIL;                          /* NIL   reference      */
  LISP$Ref        T;                            /* T     reference      */

} LISP$MachIns;


/******************************************************
 * LISP Machine control operations
 ******************************************************/

/* Initialize a new LMI */
LISP$MachIns * LISP$M_init (char * name, FILE * input, FILE * output);

void LISP$M_setStreams (LISP$MachIns * lmi, FILE * input, FILE * output);

/* Destroy specified LMI */
void LISP$M_destroy (LISP$MachIns * lmi);

/* Current stream has been processed up to its end */
void LISP$M_setProcessed (LISP$MachIns * lmi);

/* Check if there are streams to be read */
bool LISP$M_is2Process (LISP$MachIns * lmi);

/* Check if the specified LMI is waiting to be halted */
bool LISP$M_is2ShutDown (LISP$MachIns * lmi);

/* Print message to both log and output stream */
void LISP$M_throwMessage (LISP$MachIns * lmi, const char severity,
                          const char * name, const char * msg, ...);

/* Check for error state */
bool LISP$M_isError (LISP$MachIns * lmi);

/* Report error to the 'output' stream */
void LISP$M_reportError (LISP$MachIns * lmi);

/* Set error */
void LISP$M_setError (LISP$MachIns * lmi, const int code,
                      const char * name, const char * msg, ...);

/* Find a first free node in the specified storage table */
int LISP$M_findFreeNode (LISP$MemState idx [], int length);

/* Allocate a new item in the list area */
LISP$Ref LISP$M_createList (LISP$MachIns * lmi);

/* Settle a new number record in the number table */
LISP$Ref LISP$M_getCreateNumber (LISP$MachIns * lmi, double num);

/* Lookup for specified atom or create a new one */
LISP$Ref LISP$M_getCreateAtom (LISP$MachIns * lmi, char * atomName);

/* Make lookup for the symbol 'option' and check if it's set to T */
bool LISP$M_isOptionEnabled (LISP$MachIns * lmi, const char * option);

/* Return true if the 'atom' is built-in, user def. or unnamed special form */
bool LISP$M_isSpecialForm (LISP$AtomRecord atom);

/* Return true if the 'atom' is built-in, user def. or unnamed function */
bool LISP$M_isFunction (LISP$AtomRecord atom);

/* Check built-in atom's name and argument requirements               *
 * NOTE: to prevent checking argcMin and/or argcMax, set them to -Inf */
bool LISP$M_checkBuiltIn (LISP$MachIns * lmi, const char * name,
                          LISP$AtomRecord bultIn,
                          LISP$ListRecord argv, int argc,
                          int argcMin, double argcMax);

/* Set the 'mark' flag on a memory node specified by the 'ref' */
void LISP$M_markMemNode (LISP$MachIns * lmi, LISP$Ref ref,
                         LISP$MemState mark, bool recurse);

/* Check the specified memory node for a 'mark' memory state */
bool LISP$M_checkMemNodeState (LISP$MachIns * lmi, LISP$Ref ref,
                               LISP$MemState mark);

/* Collect and free previously marked garbage */
void LISP$M_collectGarbage (LISP$MachIns * lmi);

/* Print dump of all memory storages */
void LISP$M_printMemoryDump (LISP$MachIns * lmi, bool full);

/* Implementation of built-in functions and special forms */
LISP$Ref LISP$M_builtInCAR (LISP$MachIns * lmi, LISP$Ref args);
LISP$Ref LISP$M_builtInCDR (LISP$MachIns * lmi, LISP$Ref args);
LISP$Ref LISP$M_builtInSETQ (LISP$MachIns * lmi, LISP$Ref args,
                               int level);
LISP$Ref LISP$M_builtInCONS (LISP$MachIns * lmi, LISP$Ref args);
LISP$Ref LISP$M_builtInArithmetic (LISP$MachIns * lmi, LISP$Ref args,
                                     int argc, const char * oper);
LISP$Ref LISP$M_builtInIS (LISP$MachIns * lmi, LISP$Ref args);
LISP$Ref LISP$M_builtInCOND (LISP$MachIns * lmi, LISP$Ref args,
                               int level);
LISP$Ref LISP$M_builtInLOAD (LISP$MachIns * lmi, LISP$Ref args,
                               int level);
LISP$Ref LISP$M_builtInSHOWMEM (LISP$MachIns * lmi, LISP$Ref args,
                                  int argc);
LISP$Ref LISP$M_builtInGETLPI (LISP$MachIns * lmi, LISP$Ref args,
                               int argc);
LISP$Ref LISP$M_builtInLET (LISP$MachIns * lmi, LISP$Ref args, int level);
LISP$Ref LISP$M_builtInISDEFINED (LISP$MachIns * lmi, LISP$Ref args);
LISP$Ref LISP$M_builtInISATOM (LISP$MachIns * lmi, LISP$Ref args);
LISP$Ref LISP$M_builtInISLIST (LISP$MachIns * lmi, LISP$Ref args);
LISP$Ref LISP$M_builtInLAMBDASPECIAL (LISP$MachIns * lmi, LISP$Ref args,
                                        bool isLambda);

/* Stack operations on list -- useful for example for binding values */
void LISP$M_listPush (LISP$MachIns * lmi, LISP$Ref* list, LISP$Ref item);
LISP$Ref LISP$M_listPop (LISP$MachIns * lmi, LISP$Ref* list);

/* Value binding functions */
void LISP$M_bind (LISP$MachIns * lmi, LISP$Ref atm, LISP$Ref val);
void LISP$M_unBind (LISP$MachIns * lmi, LISP$Ref atm);


/******************************************************
 * S-Expression processing
 ******************************************************/

/* fgetc wrapper with history feature similar to libreadline */
char LISP$S_getChar (LISP$MachIns * lmi);

/* Append char 'c' into the 'token' buffer ans optionally upcase it */
bool LISP$S_buildToken (LISP$MachIns * lmi, char ** token,
                        int i, char c, bool isCaseSensitive);

/* Check if the char 'c' is allowed to be part of atom's name */
bool LISP$S_isValidAtomNameChar (char c, int i);

/* Print a fine prompt */
void LISP$S_makePrompt (LISP$MachIns * lmi, bool primary, bool is2ReadLine);

/* Read token from an input stream and store it into the 'token' buffer; *
 * if the token is a number, set 'isNumeric' to 'true'                   */
char LISP$S_getToken (LISP$MachIns * lmi, char ** token, bool * isNumeric);

/* Helper function for evaluation process tracing */
void LISP$S_trace (LISP$MachIns * lmi, LISP$Ref res, int level);

/* Read S-Expression from the 'input' stream */
LISP$Ref LISP$S_read (LISP$MachIns * lmi);

/* $S_read's internal action */
LISP$Ref LISP$S_readAction (LISP$MachIns * lmi, int * level);

/* Evaluate result of $S_Read call */
LISP$Ref LISP$S_eval (LISP$MachIns * lmi,
                           LISP$Ref read);

/* $S_eval's internal action */
LISP$Ref LISP$S_evalAction (LISP$MachIns * lmi, LISP$Ref read,
                              int level);

/* Write evaluated result to the 'output' stream */
void LISP$S_write (LISP$MachIns * lmi,
                   LISP$Ref evaluated);

/* $S_write's internal action */
void LISP$S_writeAction (LISP$MachIns * lmi, LISP$Ref evaluated,
                         bool isTopLevel);

/* Get human readable string for $AtomType argument 'type' */
char * LISP$S_getFormatAtomType (LISP$AtomType type);

/* Load a script file and returns boolean success */
bool LISP$S_loadFile (LISP$MachIns * lmi, char * fileName);

#endif

// vim: fdm=syntax:fdn=1:tw=74:ts=2:syn=c
