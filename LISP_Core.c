/*
 * $Id: LISP_Core.c 76 2009-09-14 11:51:40Z tmr $
 *
 * Module:  LISP_Core -- LISP Machine Implementation
 * Created: 30-NOV-2007 23:55
 * Author:  tmr
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#ifdef _HAS_RDLN_
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include "LISP_Core.h"


/******************************************************
 * LISP Machine control operations
 ******************************************************/

LISP$MachIns * LISP$M_init (char * name, FILE * input, FILE * output) {

  int  i;
  char logFileName [256];

  LISP$MachIns * lmi = (LISP$MachIns *) malloc (sizeof (LISP$MachIns));

  strlcpy (logFileName, name,   sizeof (logFileName));
  strlcat (logFileName, ".log", sizeof (logFileName));

  lmi->isReady             = false;
  lmi->is2ShutDown         = false;
  lmi->mode                = Interactive;
  lmi->logStream           = fopen (logFileName, "a");
  lmi->inputStream         = NULL;
  lmi->outputStream        = NULL;
  (lmi->error).code        = 0;
  (lmi->error).name    [0] = LISP$S_EOS;
  (lmi->error).message [0] = LISP$S_EOS;
  lmi->inputBuffer     [0] = LISP$S_EOS;
#ifdef _HAS_RDLN_
  lmi->readLineBuffer      = (char *) NULL;
#endif
  lmi->inputErrorPosition  = -1;
  LISP$M_setStreams (lmi, input, output);

  /* Print the initialization message, but not sooner than log is open! */
  LISP$M_throwMessage (lmi, 'I', "STARTUP",
      "Starting up %s... ", LISP$MACH_ID);

  /* Only formally and just for now */
  for (i = 0; i < LISP$MACH_NUMTABLEN; i++)
    lmi->numIdx [i] = Free;

  /* Init atom table */
  for (i = 0; i < LISP$MACH_ATMTABLEN; i++)
    lmi->atmIdx [i] = Free;

  /* Init 'NIL' atom */
  lmi->NIL = LISP$M_getCreateAtom (lmi, "NIL");
  (lmi->atmTab [LISP$M_getRefId (lmi->NIL)]).type     = Variable;
  (lmi->atmTab [LISP$M_getRefId (lmi->NIL)]).value    = lmi->NIL;
  (lmi->atmTab [LISP$M_getRefId (lmi->NIL)]).bindList = lmi->NIL;
  LISP$M_markMemNode (lmi, lmi->NIL, Prot, false);

  /* Init 'T' atom */
  lmi->T = LISP$M_getCreateAtom (lmi, "T");
  (lmi->atmTab [LISP$M_getRefId (lmi->T)]).type  = Variable;
  (lmi->atmTab [LISP$M_getRefId (lmi->T)]).value = lmi->T;
  LISP$M_markMemNode (lmi, lmi->T, Prot, false);

  static char * BuiltInFuns [] = {
    "CAR", "CDR", "CONS", "LIST", "BODY",
    "+", "*", "-", "/", "%%", "<", ">", "=", "<=", ">=", "IS",
    "AND", "OR", "READ", "READLN", "WRITE", "WRITELN",
    "ISATOM", "ISLIST", "ISNUMBER",
    NULL
  };

  static char * BuiltInSpecForms [] = {
    "SETQ", "QUOTE", "EVAL", "COND", "LOAD",
    "LAMBDA", "SPECIAL", "LET", "SHOW-MEM",
    "ISDEFINED", "GETLPI",
    NULL
  };

  LISP$Ref atomRef;

  /* Load all built-in functions */
  for (i = 0; BuiltInFuns [i] != NULL; i++) {
    atomRef = LISP$M_getCreateAtom (lmi, BuiltInFuns [i]);
    (lmi->atmTab [LISP$M_getRefId (atomRef)]).type  = BuiltInFun;
    (lmi->atmTab [LISP$M_getRefId (atomRef)]).value = atomRef;
    LISP$M_markMemNode (lmi, atomRef, Prot, false);
  }

  /* Load all built-in special forms */
  for (i = 0; BuiltInSpecForms [i] != NULL; i++) {
    atomRef = LISP$M_getCreateAtom (lmi, BuiltInSpecForms [i]);
    (lmi->atmTab [LISP$M_getRefId (atomRef)]).type  = BuiltInSpecForm;
    (lmi->atmTab [LISP$M_getRefId (atomRef)]).value = atomRef;
    LISP$M_markMemNode (lmi, atomRef, Prot, false);
  }

  /* Keep 'QUOTE' reference in LMI */
  lmi->QUOTE = LISP$M_getCreateAtom (lmi, "QUOTE");
  LISP$M_markMemNode (lmi, lmi->QUOTE, Prot, false);

  /* Create INF atom -- an infinite number */
  LISP$Ref INF = LISP$M_getCreateNumber (lmi, LISP$INF);
  atomRef = LISP$M_getCreateAtom (lmi, "INF");
  (lmi->atmTab [LISP$M_getRefId (atomRef)]).type  = Variable;
  (lmi->atmTab [LISP$M_getRefId (atomRef)]).value = INF;
  LISP$M_markMemNode (lmi, atomRef, Prot, false);
  LISP$M_markMemNode (lmi, INF, Prot, false);

  /* Create NAN atom */
  LISP$Ref NAN = LISP$M_getCreateNumber (lmi, LISP$NAN);
  atomRef = LISP$M_getCreateAtom (lmi, "NAN");
  (lmi->atmTab [LISP$M_getRefId (atomRef)]).type  = Variable;
  (lmi->atmTab [LISP$M_getRefId (atomRef)]).value = NAN;
  LISP$M_markMemNode (lmi, atomRef, Prot, false);
  LISP$M_markMemNode (lmi, NAN, Prot, false);

  /* Enable GC by default */
  atomRef = LISP$M_getCreateAtom (lmi, LISP$M_OPT_GC);
  (lmi->atmTab [LISP$M_getRefId (atomRef)]).type  = Variable;
  (lmi->atmTab [LISP$M_getRefId (atomRef)]).value = lmi->T;
  LISP$M_markMemNode (lmi, atomRef, Used, false);

  /* Disable TRACE by default */
  atomRef = LISP$M_getCreateAtom (lmi, LISP$M_OPT_TRACE);
  (lmi->atmTab [LISP$M_getRefId (atomRef)]).type  = Variable;
  (lmi->atmTab [LISP$M_getRefId (atomRef)]).value = lmi->NIL;
  LISP$M_markMemNode (lmi, atomRef, Used, false);

  /* Init list table */
  for (i = 0; i < LISP$MACH_LSTTABLEN; i++)
    lmi->lstIdx [i] = Free;

  /* Show memory status */
  LISP$M_printMemoryDump (lmi, false);

  /* Raise READY message */
  LISP$M_throwMessage (lmi, 'I', "STARTUP",
      "READY -- Happy LISPing!");

  lmi->isReady = true;

  return lmi;
}

void LISP$M_setStreams (LISP$MachIns * lmi, FILE * input, FILE * output) {

  /* Flush old streams */
  if (lmi->inputStream  != NULL) fflush (lmi->inputStream);
  if (lmi->outputStream != NULL) fflush (lmi->outputStream);

  /* Set new streams */
  lmi->inputStream  = input;
  lmi->outputStream = output;

  /* Set-up that there are streams to be processed */
  lmi->is2Process = true;
}

void LISP$M_destroy (LISP$MachIns * lmi) {

  int i;
  LISP$Ref ref = LISP$M_NULLREF;

  LISP$M_throwMessage (lmi, 'I', "SHUTDWN", "Shutting down...");

  lmi->isReady = false;
  fclose (lmi->logStream);

  /* Free remaining Used nodes and all internals */
  LISP$M_setRefType (ref, AtomTab);
  for (i = 0; i < LISP$MACH_ATMTABLEN; i++) {
    //if (lmi->atmIdx [i] == Used ||
    //    lmi->atmIdx [i] == Prot) {
    if (lmi->atmIdx [i] == Used) {
      LISP$M_setRefId (ref, i);
      
      /* Change Prot (it's ignored in markMemNode) to Temp */
      lmi->atmIdx [i] = Temp;
      LISP$M_markMemNode (lmi, ref, Temp, true);
    }
  }
  LISP$M_collectGarbage (lmi);

  free (lmi);
}

void LISP$M_setProcessed (LISP$MachIns * lmi) {

  lmi->is2Process = false;
  if (lmi->mode == Interactive)
    lmi->is2ShutDown = true;
}

bool LISP$M_is2Process (LISP$MachIns * lmi) {

  return (!LISP$M_is2ShutDown (lmi) && lmi->is2Process);
}

bool LISP$M_is2ShutDown (LISP$MachIns * lmi) {

  return ((lmi == NULL) ? true : lmi->is2ShutDown);
}

void LISP$M_throwMessage (LISP$MachIns * lmi, const char severity,
                          const char * name, const char * msg, ...) {

  char buf [512];
  va_list args;
  va_start (args, msg);

  char * fmt = "%sLISP-%c-%s, %s\n";
  vsnprintf (buf, sizeof (buf), msg, args);

  /* $M_isOptionEnabled () is based on $M_getCreateAtom () which calls, *
   * this function when the DEBUG_LMI is set, so let's force logging    *
   * to avoid infinite loops                                            */
  #ifndef DEBUG_LMI
  if (LISP$M_isOptionEnabled (lmi, LISP$M_OPT_LOG))
  #endif
    fprintf (lmi->logStream, fmt, "%", severity, name, buf);

  fprintf (stdout, fmt, "\r%", severity, name, buf);
  fflush (lmi->logStream);
}

bool LISP$M_isError (LISP$MachIns * lmi) {

  return ((lmi->error).code != LISP$_ERR$S_OK);
}

void LISP$M_reportError (LISP$MachIns * lmi) {

  LISP$M_throwMessage (lmi, (((lmi->error).code >= 0) ? 'W' : 'E'),
                       (lmi->error).name, (lmi->error).message);

  int i;
  if ((lmi->error).code == LISP$_ERR$S_BADSYN) {
    fprintf (lmi->outputStream, "%s\n", lmi->inputBuffer);
    for (i = 0; i < lmi->inputErrorPosition - 1; i++) {
      fprintf (lmi->outputStream, ".");
    }
    fprintf (lmi->outputStream, "^\n");
    lmi->inputErrorPosition = -1;
  }

  /* Error message passed, let's reset error status */
  (lmi->error).code = LISP$_ERR$S_OK;
}

void LISP$M_setError (LISP$MachIns * lmi, const int code,
                      const char * name, const char * msg, ...) {

  /* Error is already set */
  if (LISP$M_isError (lmi)) return;

  char buf [512];
  va_list args;
  va_start (args, msg);

  vsnprintf (buf, sizeof (buf), msg, args);

  if (code < 0) lmi->is2ShutDown = true;
  (lmi->error).code = code;
  strlcpy ((lmi->error).name, name, sizeof ((lmi->error).name));
  strlcpy ((lmi->error).message, buf, sizeof ((lmi->error).message));
}

int LISP$M_findFreeNode (LISP$MemState idx [], int length) {

  int i;

  for (i = 0; i < length; i++)
    if (idx [i] == Free) return i;

  return -1;
}

LISP$Ref LISP$M_createList (LISP$MachIns * lmi) {

  LISP$Ref res = LISP$M_NULLREF;
  int i = LISP$M_findFreeNode (lmi->lstIdx, LISP$MACH_LSTTABLEN);

  if (i >= 0) {
    /* Create a new list with both CAR and CDR values set to NIL */
    (lmi->lstTab [i]).car = lmi->NIL;
    (lmi->lstTab [i]).cdr = lmi->NIL;
    lmi->lstIdx [i]       = Temp;

    /* Build value reference */
    LISP$M_setRef (res, i, ListTab);
  }
  else {
    /* No free space in list table */
    LISP$M_setError (lmi, LISP$_ERR$S_LSTFUL,
        "LSTFUL", "List table is full!");
    res = LISP$M_NULLREF;
  }

  return res;
}

LISP$Ref LISP$M_getCreateNumber (LISP$MachIns * lmi, double num) {

  LISP$Ref res = LISP$M_NULLREF;
  int i;

  LISP$M_setRefType (res, NumberTab);

  /* Find out if the atom has been already stored */
  for (i = 0; i < LISP$MACH_NUMTABLEN; i++) {
    if (lmi->numIdx [i] != Free &&
        lmi->numTab [i] == num) {
      LISP$M_setRefId (res, i);
      return res;
    }
  }

  /* There's no number like this, so create a new one */
  i = LISP$M_findFreeNode (lmi->numIdx, LISP$MACH_NUMTABLEN);

  if (i >= 0) {
    /* Store number */
    lmi->numIdx [i] = Temp;
    lmi->numTab [i] = num; LISP$M_setRefId (res, i);
    return res;
  }

  /* Cannot create new number since the number table is full */
  LISP$M_setError (lmi, LISP$_ERR$S_NUMFUL,
      "NUMFUL", "Number table is full!");
  return (LISP$M_NULLREF);
}

LISP$Ref LISP$M_getCreateAtom (LISP$MachIns * lmi, char * atomName) {

  int i;
  LISP$Ref res = LISP$M_NULLREF;

  LISP$M_setRefType (res, AtomTab);

  #ifdef DEBUG_LMI
  LISP$M_throwMessage (lmi, 'D', "DEBUG",
      "AtomLookUp (%s)", atomName);
  #endif

  /* Find out if the atom has been already stored */
  for (i = 0; i < LISP$MACH_ATMTABLEN; i++) {
    if (lmi->atmIdx [i] != Free &&
        (lmi->atmTab [i]).name != NULL &&
        !strncmp ((lmi->atmTab [i]).name, atomName,
                  sizeof ((lmi->atmTab [i]).name))) {
      LISP$M_setRefId (res, i);
      #ifdef DEBUG_LMI
      LISP$M_throwMessage (lmi, 'D', "DEBUG",
          "%s found on position: " LISP$ADDRFMT, atomName, i);
      #endif
      return res;
    }
  }

  /* Find free space for a new atom */
  i = LISP$M_findFreeNode (lmi->atmIdx, LISP$MACH_ATMTABLEN);

  /* Nothing found, let's try to settle a new AtomRec */
  if (i >= 0) {
    LISP$M_setRefId (res, i); lmi->atmIdx [i] = Temp;

    strlcpy ((lmi->atmTab [i]).name, atomName,
             sizeof ((lmi->atmTab [i]).name));
    (lmi->atmTab [i]).type     = Undefined;
    (lmi->atmTab [i]).value    = lmi->NIL;
    (lmi->atmTab [i]).bindList = lmi->NIL;
//    (lmi->atmTab [i]).propList = lmi->NIL;

    #ifdef DEBUG_LMI
    LISP$M_throwMessage (lmi, 'D', "DEBUG",
        "%s created on position " LISP$ADDRFMT " (ref="
        LISP$ADDRFMT ")", atomName, i, res);
    #endif

    return res;
  }

  /* Nothing created, table is full */
  LISP$M_setError (lmi, LISP$_ERR$S_ATMFUL,
      "ATMFUL", "Atom table is full!");
  return (LISP$M_NULLREF);
}

bool LISP$M_isOptionEnabled (LISP$MachIns * lmi, const char * option) {

  /* LMI is not initialized yet, so prevent all option lookups      */
  if (!lmi->isReady) return false;

  LISP$Ref ref = LISP$M_getCreateAtom (lmi, (char *) option);

  /* Feature is enabled <=> the option symbol is set to T */
  return ((lmi->atmTab [LISP$M_getRefId (ref)]).value == lmi->T);
}

bool LISP$M_isSpecialForm (LISP$AtomRecord atom) {

  return (atom.type == BuiltInSpecForm ||
          atom.type == UserDefSpecForm ||
          atom.type == UnnamedSpecForm);
}

bool LISP$M_isFunction (LISP$AtomRecord atom) {

  return (atom.type == BuiltInFun ||
          atom.type == UserDefFun ||
          atom.type == UnnamedFun);
}

bool LISP$M_checkBuiltIn (LISP$MachIns * lmi, const char * name,
                          LISP$AtomRecord builtIn,
                          LISP$ListRecord argv, int argc,
                          int argcMin, double argcMax) {

  int len1 = strlen (builtIn.name);
  int len2 = strlen (name);

  /* XXX: just to prevent compiler warning -- could be used later... */
  if (&argv != &argv) return false;

  /* Check the name */
  if (!strncmp (builtIn.name, name, (len1 > len2)? len1 : len2)) {
    /* Name is ok, check the argument vector */
    if (!LISP$IS_NINF (argcMax) && argc < argcMin) {
      LISP$M_setError (lmi, LISP$_ERR$S_IVARGS, "IVARGS",
                       "Too few (%d) arguments for %s!", argc, name);
      return false;
    }
    else if (!LISP$IS_NINF (argcMax) &&
             !LISP$IS_INF  (argcMax) && argc > argcMax) {
      LISP$M_setError (lmi, LISP$_ERR$S_IVARGS, "IVARGS",
                       "Too much (%d) arguments for %s!", argc, name);
      return false;
    }
  }
  else return false;

  return true;
}

void LISP$M_markMemNode (LISP$MachIns * lmi, LISP$Ref ref,
                         LISP$MemState mark, bool recurse) {

  #ifdef DEBUG_LMI
  fprintf (lmi->outputStream, "TRACE of %s for: ",
      (mark == Free)? "FREE" :
        ((mark == Temp)? "TEMP" :
           (mark == Used)? "USED" : "PROT"));
  LISP$S_writeAction (lmi, ref, 0);
  fprintf (lmi->outputStream, "\n");
  #endif

  LISP$Ref tmp;
  switch (LISP$M_getRefType (ref)) {
    case AtomTab:
      /* Do not change protected internal values nor already marked */
      if (lmi->atmIdx [LISP$M_getRefId (ref)] == Prot) return;

      lmi->atmIdx [LISP$M_getRefId (ref)] = mark;
      tmp = (lmi->atmTab [LISP$M_getRefId (ref)]).value;

      /* Skip self-referencing nodes */
      if (recurse && ref != tmp)
        LISP$M_markMemNode (lmi, tmp, mark, true);

      /* It could be lambda or special, so let's touch its bindlist too */
      tmp = (lmi->atmTab [LISP$M_getRefId (ref)]).bindList;
      if (recurse && ref != tmp)
        LISP$M_markMemNode (lmi, tmp, mark, true);
      return;

    case NumberTab:
      lmi->numIdx [LISP$M_getRefId (ref)] = mark;
      return;

    case ListTab:
      if (!recurse) break;

      /* XXX: This COULD skip something marked as Temp deeper in the list!! */
      if (lmi->lstIdx [LISP$M_getRefId (ref)] == mark) return;

      tmp = ref;
      while (LISP$M_getRefType (tmp) == ListTab) {
        LISP$M_markMemNode (lmi,
            (lmi->lstTab [LISP$M_getRefId (tmp)]).car, mark, true);
        lmi->lstIdx [LISP$M_getRefId (tmp)] = mark;
        tmp = (lmi->lstTab [LISP$M_getRefId (tmp)]).cdr;
      }

      /* Last item of CONS */
      if (LISP$M_getRefType (tmp) != ListTab && ref != tmp)
        LISP$M_markMemNode (lmi, tmp, mark, true);

      return;
  }
}

bool LISP$M_checkMemNodeState (LISP$MachIns * lmi, LISP$Ref ref,
                               LISP$MemState mark) {

  if (ref == LISP$M_NULLREF) return false;
  switch (LISP$M_getRefType (ref)) {
    case AtomTab:   return (lmi->atmIdx [LISP$M_getRefId (ref)] == mark);
    case NumberTab: return (lmi->numIdx [LISP$M_getRefId (ref)] == mark);
    case ListTab:   return (lmi->lstIdx [LISP$M_getRefId (ref)] == mark);
    default:        return false;
  }
}

void LISP$M_collectGarbage (LISP$MachIns * lmi) {

  /* If the LMI is up, it's optional, but on destroy, it's required */
  if (lmi->isReady && !LISP$M_isOptionEnabled (lmi, LISP$M_OPT_GC)) return;

  int i;
  LISP$Ref ref = LISP$M_NULLREF;

  /* Find all roots -- symbols marked as Used -- and mark their tree */
  LISP$M_setRefType (ref, AtomTab);
  for (i = 0; i < LISP$MACH_ATMTABLEN; i++) {
    if (lmi->atmIdx [i] == Used) {
      LISP$M_setRefId (ref, i);
      LISP$M_markMemNode (lmi, ref, Used, true);
    }
  }

  /* Mark all Temps as Free and discard their references to others */
  for (i = 0; i < LISP$MACH_LSTTABLEN &&
              i < LISP$MACH_LSTTABLEN &&
              i < LISP$MACH_NUMTABLEN; i++) {
    if (i < LISP$MACH_LSTTABLEN && lmi->lstIdx [i] == Temp)
      lmi->lstIdx [i] = Free;

    if (i < LISP$MACH_ATMTABLEN && lmi->atmIdx [i] == Temp)
      lmi->atmIdx [i] = Free;

    if (i < LISP$MACH_NUMTABLEN && lmi->numIdx [i] == Temp)
      lmi->numIdx [i] = Free;
  }
}

void LISP$M_printMemoryDump (LISP$MachIns * lmi, bool full) {

#define GET_STATUS_STRING(index) \
  ((index) [i] == Temp)? "Temp" : \
    (((index) [i] == Used)? "Used" : "Prot")

#define GET_REF_STRING(ref) \
  (LISP$M_getRefType (ref) == AtomTab)? "ATM" : \
    ((LISP$M_getRefType (ref) == ListTab)? "LST" : "NUM")

#define UPDATE_COUNTER(type) \
  ((lmi->type##Idx [i] == Temp)? type##Temp++ : \
    ((lmi->type##Idx [i] == Used)? type##Used++ : \
      ((lmi->type##Idx [i] == Prot)? type##Prot++ : type##Free++)))

  int i;
  int atmFree, atmUsed, atmTemp, atmProt;
  atmFree = atmUsed = atmTemp = atmProt = 0;
  for (i = 0; i < LISP$MACH_ATMTABLEN; i++) {
    if (full && lmi->atmIdx [i] != Free)
      LISP$M_throwMessage (lmi, 'I', "MEMDUMP", "ATM#" LISP$ADDRFMT
          " [STA:%s  NAM:%-10.10s  VAL:%s#" LISP$ADDRFMT "]",
          i, GET_STATUS_STRING (lmi->atmIdx),
          (lmi->atmTab [i]).name,
          GET_REF_STRING ((lmi->atmTab [i]).value),
          LISP$M_getRefId ((lmi->atmTab [i]).value));
    UPDATE_COUNTER (atm);
  }

  int lstFree, lstUsed, lstTemp, lstProt;
  lstFree = lstUsed = lstTemp = lstProt = 0;
  for (i = 0; i < LISP$MACH_LSTTABLEN; i++) {
    if (full && lmi->lstIdx [i] != Free)
      LISP$M_throwMessage (lmi, 'I', "MEMDUMP", "LST#" LISP$ADDRFMT
          " [STA:%s  CAR:%s#" LISP$ADDRFMT "  CDR:%s#" LISP$ADDRFMT "]",
          i, GET_STATUS_STRING (lmi->lstIdx),
          GET_REF_STRING ((lmi->lstTab [i]).car),
          LISP$M_getRefId ((lmi->lstTab [i]).car),
          GET_REF_STRING ((lmi->lstTab [i]).cdr),
          LISP$M_getRefId ((lmi->lstTab [i]).cdr));
    UPDATE_COUNTER (lst);
  }

  int numFree, numUsed, numTemp, numProt;
  numFree = numUsed = numTemp = numProt = 0;
  for (i = 0; i < LISP$MACH_NUMTABLEN; i++) {
    if (full && lmi->numIdx [i] != Free)
      LISP$M_throwMessage (lmi, 'I', "MEMDUMP", "NUM#" LISP$ADDRFMT
          " [STA:%s  VAL:%26e]", i, GET_STATUS_STRING (lmi->numIdx),
          lmi->numTab [i]);
    UPDATE_COUNTER (num);
  }

  LISP$M_throwMessage (lmi, 'I', "MEMDUMP",
      "ATMTAB: Free %4d; Temp %4d; Used %4d; Prot %4d;",
      atmFree, atmTemp, atmUsed, atmProt);

  LISP$M_throwMessage (lmi, 'I', "MEMDUMP",
      "LSTTAB: Free %4d; Temp %4d; Used %4d; Prot %4d;",
      lstFree, lstTemp, lstUsed, lstProt);

  LISP$M_throwMessage (lmi, 'I', "MEMDUMP",
      "NUMTAB: Free %4d; Temp %4d; Used %4d; Prot %4d;",
      numFree, numTemp, numUsed, lstProt);

#undef UPDATE_COUNTER
#undef GET_REF_STRING
#undef GET_STATUS_STRING
}

LISP$Ref LISP$M_builtInCAR (LISP$MachIns * lmi, LISP$Ref args) {

  LISP$ListRecord list = lmi->lstTab [LISP$M_getRefId (args)];
  if (list.car == lmi->NIL) return lmi->NIL;
  if (LISP$M_getRefType (list.car) != ListTab) {
    LISP$M_setError (lmi, LISP$_ERR$S_IVARGS, "IVARGS",
        "CAR\'s argument is not a list!");
    return (LISP$M_NULLREF);
  }

  return ((lmi->lstTab [LISP$M_getRefId (list.car)]).car);
}

LISP$Ref LISP$M_builtInCDR (LISP$MachIns * lmi, LISP$Ref args) {

  LISP$ListRecord list = lmi->lstTab [LISP$M_getRefId (args)];
  if (list.car == lmi->NIL) return lmi->NIL;
  if (LISP$M_getRefType (list.car) != ListTab) {
    LISP$M_setError (lmi, LISP$_ERR$S_IVARGS, "IVARGS",
        "CDR\'s argument is not a list!");
    return (LISP$M_NULLREF);
  }

  return ((lmi->lstTab [LISP$M_getRefId (list.car)]).cdr);
}

LISP$Ref LISP$M_builtInSETQ (LISP$MachIns * lmi, LISP$Ref args, int level) {

  LISP$Ref tmp  = (lmi->lstTab [LISP$M_getRefId (args)]).car;
  LISP$Ref tmpx = (lmi->lstTab [LISP$M_getRefId (args)]).cdr;

  /* Try to get a new value */
  tmpx = LISP$S_evalAction (lmi,
      (lmi->lstTab [LISP$M_getRefId (tmpx)]).car, level);
  if (tmpx == LISP$M_NULLREF) return (LISP$M_NULLREF);

  /* Discard old value if any */
  if ((lmi->atmTab [LISP$M_getRefId (tmp)]).type != Undefined)
    LISP$M_markMemNode (lmi, tmp, Temp, true);

  /* Just copy a value/type/bindList if they're both atoms */
  if (LISP$M_getRefType (tmpx) == AtomTab &&
      ((lmi->atmTab [LISP$M_getRefId (tmpx)]).type == UserDefFun      ||
       (lmi->atmTab [LISP$M_getRefId (tmpx)]).type == UnnamedFun      ||
       (lmi->atmTab [LISP$M_getRefId (tmpx)]).type == UserDefSpecForm ||
       (lmi->atmTab [LISP$M_getRefId (tmpx)]).type == UnnamedSpecForm)) {
    (lmi->atmTab [LISP$M_getRefId (tmp)]).value =
      (lmi->atmTab [LISP$M_getRefId (tmpx)]).value;
    (lmi->atmTab [LISP$M_getRefId (tmp)]).type =
      (lmi->atmTab [LISP$M_getRefId (tmpx)]).type;
    (lmi->atmTab [LISP$M_getRefId (tmp)]).bindList =
      (lmi->atmTab [LISP$M_getRefId (tmpx)]).bindList;
  }

  /* Only assign new value reference */
  else {
    (lmi->atmTab [LISP$M_getRefId (tmp)]).value = tmpx;
    (lmi->atmTab [LISP$M_getRefId (tmp)]).type  = Variable;
  }

  LISP$M_markMemNode (lmi, tmp, Used, false);

  if ((lmi->atmTab [LISP$M_getRefId (tmp)]).type == Variable)
    return ((lmi->atmTab [LISP$M_getRefId (tmp)]).value);
  else
    return tmp;
}

LISP$Ref LISP$M_builtInCONS (LISP$MachIns * lmi, LISP$Ref args) {

  LISP$Ref tmp = args;
  LISP$Ref tmpx = LISP$M_NULLREF;
  while ((lmi->lstTab [LISP$M_getRefId (tmp)]).cdr != lmi->NIL) {
    tmpx = tmp;
    tmp  = (lmi->lstTab [LISP$M_getRefId (args)]).cdr;
  }
  (lmi->lstTab [LISP$M_getRefId (tmpx)]).cdr =
    (lmi->lstTab [LISP$M_getRefId (tmp)]).car;

  return (args);
}

LISP$Ref LISP$M_builtInArithmetic (LISP$MachIns * lmi, LISP$Ref args,
                                   int argc, const char * oper) {

  double comp = 0;
  bool compInProgress = false;
  LISP$Ref tmp  = args;
  LISP$Ref tmpx = LISP$M_NULLREF;
  while (tmp != lmi->NIL) {
    tmpx = (lmi->lstTab [LISP$M_getRefId (tmp)]).car;
    if (LISP$M_getRefType (tmpx) != NumberTab) {
      LISP$M_setError (lmi, LISP$_ERR$S_IVARGS, "IVARGS",
          "%s works only with numeric arguments!", oper);
      return (LISP$M_NULLREF);
    }

    switch (oper [0]) {
      case '+':
        comp += lmi->numTab [LISP$M_getRefId (tmpx)];
        break;

      case '-':
        if (compInProgress || argc == 1)
          comp -= lmi->numTab [LISP$M_getRefId (tmpx)];
        else
          comp  = lmi->numTab [LISP$M_getRefId (tmpx)];
        break;

      case '*':
        if (!compInProgress) comp = 1;
          comp *= lmi->numTab [LISP$M_getRefId (tmpx)];
        break;

      case '/':
        if (compInProgress) {
          if (!lmi->numTab [LISP$M_getRefId (tmpx)]) {
            LISP$M_setError (lmi, LISP$_ERR$S_IVARGS, "IVARGS",
                "Zero division!");
            return (LISP$M_NULLREF);
          }
          comp /= lmi->numTab [LISP$M_getRefId (tmpx)];
        }
        else comp = lmi->numTab [LISP$M_getRefId (tmpx)];
        break;

      case '%':
        if (((double) ((int) lmi->numTab [LISP$M_getRefId (tmpx)]))
            != lmi->numTab [LISP$M_getRefId (tmpx)]) {
          LISP$M_setError (lmi, LISP$_ERR$S_IVARGS, "IVARGS",
              "Modulo gets only integer arguments!");
        }
        if (compInProgress)
          comp = ((int) comp) % ((int) lmi->numTab [LISP$M_getRefId (tmpx)]);
        else comp = lmi->numTab [LISP$M_getRefId (tmpx)];
        break;
    }

    /* Keep information that we've something computed yet */
    compInProgress = true;

    /* Move to next argument */
    tmp  = (lmi->lstTab [LISP$M_getRefId (tmp)]).cdr;
  }
  return (LISP$M_getCreateNumber (lmi, comp));
}

LISP$Ref LISP$M_builtInIS (LISP$MachIns * lmi, LISP$Ref args) {

  /* XXX: doesn't work for numbers, why?! */
  LISP$ListRecord list = lmi->lstTab [LISP$M_getRefId (args)];
  LISP$Ref tmp  = list.cdr;

  tmp = (lmi->lstTab [LISP$M_getRefId (tmp)]).car;

  return ((list.car == tmp)? lmi->T : lmi->NIL);
}

LISP$Ref LISP$M_builtInCOND (LISP$MachIns * lmi, LISP$Ref args, int level) {

  LISP$Ref tmp  = args;
  LISP$Ref tmpx = LISP$M_NULLREF;
  LISP$ListRecord list;

  while (tmp != lmi->NIL) {
    tmpx = (lmi->lstTab [LISP$M_getRefId (tmp)]).car;
    list = lmi->lstTab [LISP$M_getRefId (tmpx)];
    if (LISP$S_evalAction (lmi, list.car, level) != lmi->NIL) {
      tmp = LISP$S_evalAction (lmi,
          (lmi->lstTab [LISP$M_getRefId (list.cdr)]).car, level);
      return (tmp);
    }
    tmp = (lmi->lstTab [LISP$M_getRefId (tmp)]).cdr;
  }

  LISP$M_setError (lmi, LISP$_ERR$S_IVARGS, "IVARGS",
      "No COND's argument where to fall!");
  return (LISP$M_NULLREF);
}

LISP$Ref LISP$M_builtInLOAD (LISP$MachIns * lmi, LISP$Ref args, int level) {

  LISP$Ref tmp =
    LISP$S_evalAction (lmi, (lmi->lstTab [LISP$M_getRefId (args)]).car, level);

  if (LISP$M_getRefType (tmp) != AtomTab ||
      (lmi->atmTab [LISP$M_getRefId (tmp)]).name [0] != '"') {
    LISP$M_setError (lmi, LISP$_ERR$S_IVARGS, "IVARGS",
        "LOAD gets just one string argument!");
    return (LISP$M_NULLREF);
  }

  LISP$AtomRecord atom = lmi->atmTab [LISP$M_getRefId (tmp)];
  atom.name [strlen (atom.name) -1] = LISP$S_EOS;

  return (LISP$S_loadFile (lmi, atom.name + 1)? lmi->T : lmi->NIL);
}

LISP$Ref LISP$M_builtInSHOWMEM (LISP$MachIns * lmi, LISP$Ref args, int argc) {

  int len = 0;
  LISP$Ref tmp = LISP$M_NULLREF;
  LISP$AtomRecord atom;

  if (argc) {
    tmp = (lmi->lstTab [LISP$M_getRefId (args)]).car;
    atom = lmi->atmTab [LISP$M_getRefId (tmp)];
    len = strlen (atom.name);
    if (strncmp (atom.name, "FULL", (len > 4)? len : 4)) {
      LISP$M_setError (lmi, LISP$_ERR$S_IVARGS, "IVARGS",
          "SHOW-MEM gets only one optional argument: FULL");
      return (LISP$M_NULLREF);
    }
    else LISP$M_printMemoryDump (lmi, true);
  }
  else LISP$M_printMemoryDump (lmi, false);

  return (lmi->T);
}

LISP$Ref LISP$M_builtInGETLPI (LISP$MachIns * lmi, LISP$Ref args, int argc) {

  LISP$Ref res;
  LISP$Ref tmp = LISP$M_NULLREF;
  LISP$AtomRecord atom;

  if (argc) {
    tmp = (lmi->lstTab [LISP$M_getRefId (args)]).car;
    atom = lmi->atmTab [LISP$M_getRefId (tmp)];
    int i = 0, cnt = 0, len = strlen (atom.name);

    if (!strncmp (atom.name, "MODE", (len > 4)? len : 4)) {
      if (lmi->mode == Interactive)
        res = LISP$M_getCreateAtom (lmi, "INTERACTIVE");
      else
        res = LISP$M_getCreateAtom (lmi, "BATCH");
    }

    else if (!strncmp (atom.name, "LISTS_USED", (len > 10)? len : 10)) {
      for (i = 0, cnt = 0; i < LISP$MACH_LSTTABLEN; i++)
        if (lmi->lstIdx [i] != Free) cnt++;
      res = LISP$M_getCreateNumber (lmi, cnt);
    }

    else if (!strncmp (atom.name, "ATOMS_USED", (len > 10)? len : 10)) {
      for (i = 0, cnt = 0; i < LISP$MACH_ATMTABLEN; i++)
        if (lmi->atmIdx [i] != Free) cnt++;
      res = LISP$M_getCreateNumber (lmi, cnt);
    }

    else if (!strncmp (atom.name, "NUMBERS_USED", (len > 12)? len : 12)) {
      for (i = 0, cnt = 0; i < LISP$MACH_NUMTABLEN; i++)
        if (lmi->numIdx [i] != Free) cnt++;
      res = LISP$M_getCreateNumber (lmi, cnt);
    }

    else if (!strncmp (atom.name, "MAXLISTS", (len > 8)? len : 8))
      { res = LISP$M_getCreateNumber (lmi, LISP$MACH_LSTTABLEN); }

    else if (!strncmp (atom.name, "MAXATOMS", (len > 8)? len : 8))
      { res = LISP$M_getCreateNumber (lmi, LISP$MACH_ATMTABLEN); }

    else if (!strncmp (atom.name, "MAXNUMBERS", (len > 10)? len : 10))
      { res = LISP$M_getCreateNumber (lmi, LISP$MACH_NUMTABLEN); }

    else {
      LISP$M_setError (lmi, LISP$_ERR$S_IVARGS, "IVARGS",
          "Invalid argument!");
      res = lmi->NIL;
    }
  }
  else {
    LISP$M_setError (lmi, LISP$_ERR$S_IVARGS, "IVARGS",
        "GETLPI needs an argument!");
    res = lmi->NIL;
  }

  return (res);
}

LISP$Ref LISP$M_builtInLET (LISP$MachIns * lmi, LISP$Ref args, int level) {

  LISP$Ref tmp;
  LISP$Ref tmpx;
  LISP$Ref tmpy;
  LISP$Ref res = lmi->NIL;

  /* Process list of bindings */
  tmp = (lmi->lstTab [LISP$M_getRefId (args)]).car;
  if (LISP$M_getRefType (tmp) != ListTab) {
    LISP$M_setError (lmi, LISP$_ERR$S_IVARGS, "IVARGS",
        "LET's first argument must be a list bindings!");
    return (LISP$M_NULLREF);
  }
  while (tmp != lmi->NIL) {
    /* Get actual bind pair */
    tmpx = (lmi->lstTab [LISP$M_getRefId (tmp)]).car;

    if (LISP$M_getRefType (tmpx) != ListTab) {
      LISP$M_setError (lmi, LISP$_ERR$S_IVARGS, "IVARGS",
          "Invalid binding!");
      res = LISP$M_NULLREF; break;
    }

    /* Get the rest of list -- a value terminated by NIL */
    tmpy = (lmi->lstTab [LISP$M_getRefId (tmpx)]).cdr;

    /* Get atom to be bound to */
    tmpx = (lmi->lstTab [LISP$M_getRefId (tmpx)]).car;

    /* Get value without it's NIL termination */
    tmpy = LISP$S_evalAction (lmi,
        (lmi->lstTab [LISP$M_getRefId (tmpy)]).car, level);

    /* Bind them together */
    LISP$M_bind (lmi, tmpx, tmpy);

    /* Move to a next argument */
    tmp = (lmi->lstTab [LISP$M_getRefId (tmp)]).cdr;
  }

  /* Process body */
  if (res != LISP$M_NULLREF) {
    tmp = (lmi->lstTab [LISP$M_getRefId (args)]).cdr;
    while (tmp != lmi->NIL) {
      res = LISP$S_evalAction (lmi,
          (lmi->lstTab [LISP$M_getRefId (tmp)]).car, level);

      /* Move to a next argument */
      tmp = (lmi->lstTab [LISP$M_getRefId (tmp)]).cdr;
    }
  }

  /* Unbind values */
  tmp = (lmi->lstTab [LISP$M_getRefId (args)]).car;
  while (tmp != lmi->NIL) {
    /* Get actual bind pair */
    tmpx = (lmi->lstTab [LISP$M_getRefId (tmp)]).car;

    /* Here we probably got bind error, so *
     * there's nothing more to be unbound  */
    if (LISP$M_getRefType (tmpx) != ListTab) break;

    /* Get atom to be unbound */
    tmpx = (lmi->lstTab [LISP$M_getRefId (tmpx)]).car;

    /* Unbind it */
    LISP$M_unBind (lmi, tmpx);

    /* Move to a next argument */
    tmp = (lmi->lstTab [LISP$M_getRefId (tmp)]).cdr;
  }

  return (res);
}

LISP$Ref LISP$M_builtInISDEFINED (LISP$MachIns * lmi, LISP$Ref args) {

  LISP$Ref ref = (lmi->lstTab [LISP$M_getRefId (args)]).car;
  return ((lmi->atmTab [LISP$M_getRefId (ref)]).type != Undefined)?
    lmi->T : lmi->NIL;
}

LISP$Ref LISP$M_builtInISATOM (LISP$MachIns * lmi, LISP$Ref args) {

  LISP$Ref ref = (lmi->lstTab [LISP$M_getRefId (args)]).car;
  return (LISP$M_getRefType (ref) != ListTab)? lmi->T : lmi->NIL;
}

LISP$Ref LISP$M_builtInISLIST (LISP$MachIns * lmi, LISP$Ref args) {

  LISP$Ref ref = (lmi->lstTab [LISP$M_getRefId (args)]).car;
  return (LISP$M_getRefType (ref) == ListTab)? lmi->T : lmi->NIL;
}

LISP$Ref LISP$M_builtInISNUMBER (LISP$MachIns * lmi, LISP$Ref args) {

  LISP$Ref ref = (lmi->lstTab [LISP$M_getRefId (args)]).car;
  return (LISP$M_getRefType (ref) == NumberTab)? lmi->T : lmi->NIL;
}

LISP$Ref LISP$M_builtInBODY (LISP$MachIns * lmi, LISP$Ref args) {

  LISP$Ref ref = (lmi->lstTab [LISP$M_getRefId (args)]).car;

  if ((lmi->atmTab [LISP$M_getRefId (ref)]).type != UserDefFun      &&
      (lmi->atmTab [LISP$M_getRefId (ref)]).type != UserDefSpecForm &&
      (lmi->atmTab [LISP$M_getRefId (ref)]).type != UnnamedFun      &&
      (lmi->atmTab [LISP$M_getRefId (ref)]).type != UnnamedSpecForm) {
    LISP$M_setError (lmi, LISP$_ERR$S_IVARGS, "IVARGS",
        "Only user defined lambda/special is a valid argument for BODY!");
    return (LISP$M_NULLREF);
  }

  return ((lmi->atmTab [LISP$M_getRefId (ref)]).value);
}

LISP$Ref LISP$M_builtInLAMBDASPECIAL (LISP$MachIns * lmi, LISP$Ref args,
                                        bool isLambda) {

  LISP$Ref tmp  = (lmi->lstTab [LISP$M_getRefId (args)]).cdr;
  LISP$Ref tmpx = (lmi->lstTab [LISP$M_getRefId (args)]).car;
  LISP$AtomRecord atom;

  if (LISP$M_getRefType (tmpx) != ListTab) {
    LISP$M_setError (lmi, LISP$_ERR$S_IVARGS, "IVARGS",
        "%s's first argument must be a bindlist!",
        isLambda? "LAMBDA" : "SPECIAL");
    return (LISP$M_NULLREF);
  }

  /* XXX: this should be really UNNAMED!! */
  tmp = LISP$M_getCreateAtom (lmi, isLambda?
            "...UnNamedLambda..." : "...UnNamedSpecial...");
  atom = lmi->atmTab [LISP$M_getRefId (tmp)];

  atom.type     = isLambda? UserDefFun : UserDefSpecForm; /* UnnamedFun */
  atom.bindList = (lmi->lstTab [LISP$M_getRefId (args)]).car;
  tmpx          = (lmi->lstTab [LISP$M_getRefId (args)]).cdr;
  atom.value    = (lmi->lstTab [LISP$M_getRefId (tmpx)]).car;

  lmi->atmTab [LISP$M_getRefId (tmp)] = atom;

  return (tmp);
}

void LISP$M_listPush (LISP$MachIns * lmi, LISP$Ref * list, LISP$Ref item) {

  /* Create a new list */
  LISP$Ref res = LISP$M_createList (lmi);

  /* Push there a new value and the old list */
  (lmi->lstTab [LISP$M_getRefId (res)]).car = item;
  (lmi->lstTab [LISP$M_getRefId (res)]).cdr = *list;

  /* Refresh reference to a new list */
  *list = res;
}

LISP$Ref LISP$M_listPop (LISP$MachIns * lmi, LISP$Ref * list) {

  LISP$Ref res = *list;

  /* Get item from the list */
  res = (lmi->lstTab [LISP$M_getRefId (res)]).car;

  /* Drop item from a list */
  *list = ((lmi->lstTab [LISP$M_getRefId (*list)]).cdr);

  return (res);
}

void LISP$M_bind (LISP$MachIns * lmi, LISP$Ref atm, LISP$Ref val) {

  if (lmi->atmIdx [LISP$M_getRefId (atm)] == Prot) {
    LISP$M_setError (lmi, LISP$_ERR$S_IVARGS, "IVARGS",
        "Cannot bind to protected atoms, use another name than '%s'!",
        (lmi->atmTab [LISP$M_getRefId (atm)]).name);
    return;
  }

  /* Move its original value to the bindlist */
  LISP$M_listPush (lmi, &((lmi->atmTab [LISP$M_getRefId (atm)]).bindList),
      (lmi->atmTab [LISP$M_getRefId (atm)]).value);

  /* Set a new actual value */ 
  (lmi->atmTab [LISP$M_getRefId (atm)]).value = val;

  /* XXX: are we sure it's always variable? Why not to bind *
   * lambda/special?                                        */
  (lmi->atmTab [LISP$M_getRefId (atm)]).type = Variable;
}

void LISP$M_unBind (LISP$MachIns * lmi, LISP$Ref atm) {

  /* Restore old value */
  (lmi->atmTab [LISP$M_getRefId (atm)]).value =
    LISP$M_listPop (lmi, &((lmi->atmTab [LISP$M_getRefId (atm)]).bindList));

  /* XXX: check if there was an original value and then decide *
   * about its type!                                           */
  (lmi->atmTab [LISP$M_getRefId (atm)]).type =
    ((lmi->atmTab [LISP$M_getRefId (atm)]).value == lmi->NIL)?
      Undefined : Variable;
}


/******************************************************
 * S-Expression processing
 ******************************************************/

bool LISP$S_buildToken (LISP$MachIns * lmi, char ** token,
                        int i, char c, bool isCaseSensitive) {

  char * tok = *token;

  if (i >= LISP$S_TOKEN_SIZE) {
    LISP$M_setError (lmi, LISP$_ERR$S_LONGTOK,
        "LONGTOK", "Too long token!");
    return false;
  }

  tok [i] = isCaseSensitive? c : toupper (c);

  return true;
}

bool LISP$S_isValidAtomNameChar (char c, int i) {

  if (!i && isdigit (c)) return false;
  if (isalnum (c))       return true;

  static char set [] = {
    '!', '#', '$', '%', '&', '*', '+', ',', '-',
    '/', ':', '<', '=', '>', '?', '@', '^', '_',
    '{', '|', '}', '~', LISP$S_EOS
  };

  int j;
  for (j = 0; set [j] != LISP$S_EOS; j++)
    if (c == set [j]) return true;

  return false;
}

void LISP$S_makePrompt (LISP$MachIns * lmi, bool primary, bool is2ReadLine) {

  is2ReadLine = is2ReadLine;
  if (lmi->mode == Interactive) {
#ifdef _HAS_RDLN_
    if (lmi->readLineBuffer != NULL && (int) strlen (lmi->readLineBuffer) > lmi->inputReadPosition) {
      int remains = strlen (lmi->readLineBuffer) -lmi->inputReadPosition +1;
      char * tmp = (char *) malloc (remains * sizeof (char));
      strlcpy (tmp, lmi->readLineBuffer +lmi->inputReadPosition, remains);
      free (lmi->readLineBuffer);
      lmi->readLineBuffer = tmp;
      lmi->inputReadPosition = 0;
    }
    else if (is2ReadLine) {
      lmi->inputReadPosition = 0;
      if (lmi->readLineBuffer) { free (lmi->readLineBuffer); lmi->readLineBuffer = (char *) NULL; }
      lmi->readLineBuffer = (char *) readline (primary? LISP$PROMPT : LISP$PROMPT_ALT);
      if (lmi->readLineBuffer && *(lmi->readLineBuffer)) add_history (lmi->readLineBuffer);

      if (lmi->readLineBuffer != NULL) {
        int len = strlen (lmi->readLineBuffer) +1;
        char * tmp = (char *) malloc (len * sizeof (char));
        snprintf (tmp, len, "%s\n", lmi->readLineBuffer);
        free (lmi->readLineBuffer);
        lmi->readLineBuffer = tmp;
      }
    }
#else
    fprintf (lmi->outputStream, "\r%s",
        (primary? LISP$PROMPT : LISP$PROMPT_ALT));
#endif
  }
}

char LISP$S_getChar (LISP$MachIns * lmi) {

  char c;

#ifdef _HAS_RDLN_
  if (lmi->mode == Interactive)
    if (lmi->readLineBuffer == NULL) c = EOF;
    else if (lmi->readLineBuffer [lmi->inputReadPosition] == LISP$S_EOS) c = LISP$S_EOL;
    else
      c = ((int) strlen (lmi->readLineBuffer) < lmi->inputReadPosition)?
        LISP$S_EOL : lmi->readLineBuffer [lmi->inputReadPosition];
  else
#endif
    c = fgetc (lmi->inputStream);

  lmi->inputReadPosition++;

  bool isSpace = isspace (c);
  int  len = lmi->inputReadPosition; //strlen (lmi->inputBuffer);
  char tmp = isSpace? ' ' : c;
  if (!isSpace || (isSpace &&
      lmi->inputBuffer [len - 1] != ' '))
    snprintf (lmi->inputBuffer, sizeof (lmi->inputBuffer),
        "%s%c", lmi->inputBuffer, tmp);

  return c;
}

char LISP$S_getToken (LISP$MachIns * lmi, char ** token, bool * isNumeric) {

  int i = 0;
  char * tok = *token;
  char prevChar = ' ';
  char currentChar = ' ';
  bool hasFloatingPoint = false; *isNumeric = false;

  /* Ignore all white-chars */
  while (isspace (currentChar)) {
    if (currentChar == LISP$S_EOL) LISP$S_makePrompt (lmi, lmi->sexprEnd, true);
    currentChar = LISP$S_getChar (lmi);
  }

  /* Current token is a number */
  if (currentChar == '+' || currentChar == '-' || isdigit (currentChar)) {
    LISP$S_buildToken (lmi, &tok, i++, currentChar, false);
    if (isdigit (currentChar)) *isNumeric = true;
    currentChar = LISP$S_getChar (lmi);

    while (isdigit (currentChar) || currentChar == LISP$S_FLPDLM) {
      /* Mark token as a number right here, because '+' or '-'   *
       * could be a built-in functions!                          */
      *isNumeric = true;

      if (currentChar == LISP$S_FLPDLM) {
        if (!hasFloatingPoint) {
          /* Add the floating point to the current number token      *
           * NOTE: LISP$S_FLPDLM that could be ',' -> encoded as '.' */
          if (!LISP$S_buildToken (lmi, &tok, i++, '.', false)) break;
          hasFloatingPoint = true;
        }

        /* There is floating point already read, throw error! */
        else {
          LISP$M_setError (lmi, LISP$_ERR$S_BADSYN, "BADSYN",
              "Syntax error, second floating point?!");
          /* Set error position if there was no problem before */
          if (lmi->inputErrorPosition < 0)
            lmi->inputErrorPosition = lmi->inputReadPosition; //strlen (lmi->inputBuffer);
          /* No break -- we want to discard entire expression */
        }
      }

      /* Add more of digits */
      else if (!LISP$S_buildToken (lmi, &tok, i++, currentChar, false)) break;

      currentChar = LISP$S_getChar (lmi);
    }
  }

  /* Current token is an atom -- it's not neccessary to test if the first *
   * character of this atom's name is digit, because digits are collected *
   * in previous test                                                     */
  else if (LISP$S_isValidAtomNameChar (currentChar, i)) {
    LISP$S_buildToken (lmi, &tok, i++, currentChar, false);
    prevChar = currentChar;
    currentChar = LISP$S_getChar (lmi);
    while (LISP$S_isValidAtomNameChar (currentChar, i)) {
      if (prevChar == '%') /* Store % twice -- it's formatting char */
        LISP$S_buildToken (lmi, &tok, i++, prevChar, false);
      if (!LISP$S_buildToken (lmi, &tok, i++, currentChar, false)) break;
      prevChar = currentChar;
      currentChar = LISP$S_getChar (lmi);
    }
    if (prevChar == '%') /* Store % twice -- it's formatting char */
      LISP$S_buildToken (lmi, &tok, i++, prevChar, false);
  }

  /* Is's a string */
  else if (currentChar == '"') {
    LISP$S_buildToken (lmi, &tok, i++, currentChar, true);
    prevChar = currentChar;
    currentChar = LISP$S_getChar (lmi);

    /* Read entire string */
    prevChar = ' ';
    while (prevChar != '"') {
      LISP$S_buildToken (lmi, &tok, i++, currentChar, true);
      prevChar    = currentChar;
      currentChar = LISP$S_getChar (lmi);
    }
  }

  LISP$S_buildToken (lmi, &tok, i, LISP$S_EOS, false);
  **token = *tok;

  if (currentChar == LISP$S_EOL)
    LISP$S_makePrompt (lmi, lmi->sexprEnd, false);

  return currentChar;
}

LISP$Ref LISP$S_readAction (LISP$MachIns * lmi, int * level) {

  lmi->sexprEnd = (!*level);
  #ifdef DEBUG_PARSER
  LISP$M_throwMessage (lmi, 'D', "DEBUG", "#%d READ BEGIN", *level);
  #endif
  LISP$Ref res; LISP$Ref tmp; LISP$Ref swp;
  res = tmp = swp = LISP$M_NULLREF;
  bool isNumeric; char * token; int currentLevel = *level;
  token = (char *) malloc (LISP$S_TOKEN_SIZE * sizeof (char));
  char end = LISP$S_getToken (lmi, &token, &isNumeric);

  #ifdef DEBUG_PARSER
  LISP$M_throwMessage (lmi, 'D', "DEBUG",
      "level=%d; end=%c; token=%s;", *level, end, token);
  #endif

  /* Something is to be commented out -- read line up to its end */
  while (end == LISP$S_COMMENT) {
    while (end != LISP$S_EOL && end != LISP$S_EOS)
      end = LISP$S_getChar (lmi);

    end = LISP$S_getToken (lmi, &token, &isNumeric);
  }

  /* ^D read, so disallow any processing */
  if (end == EOF || !LISP$M_is2Process (lmi)) {
    LISP$M_setProcessed (lmi);
    free (token);
    return (LISP$M_NULLREF);
  }

  /* Something is to be quoted */
  if (end == '\'') {
    res =  LISP$M_createList (lmi);
    (lmi->lstTab [LISP$M_getRefId (res)]).car = lmi->QUOTE;
    (lmi->lstTab [LISP$M_getRefId (res)]).cdr = LISP$M_createList (lmi);
    tmp = (lmi->lstTab [LISP$M_getRefId (res)]).cdr;
    (lmi->lstTab [LISP$M_getRefId (tmp)]).car = LISP$S_readAction (lmi, level);
    (lmi->lstTab [LISP$M_getRefId (tmp)]).cdr = lmi->NIL;
  }

  /* Each list begins by left parenthesis */
  else if (end == LISP$S_PAR_OPEN) {
    (*level)++;

    #ifdef DEBUG_PARSER
    LISP$M_throwMessage (lmi, 'D', "DEBUG", "#%d OPEN", *level);
    #endif

    /* Create a new item in the list area which is to be filled and  *
     * then processed by LISP$S_eval () function                     */
    res = LISP$M_createList (lmi);
    (lmi->lstTab [LISP$M_getRefId (res)]).car = lmi->NIL;
    (lmi->lstTab [LISP$M_getRefId (res)]).cdr = lmi->NIL;
    tmp = res;

    /* Read the first object and store it to CAR */
    swp = LISP$S_readAction (lmi, level);

    /* For cases that ')' is read just after '(' (nil-lists).. */
    if (swp == LISP$M_NULLREF) swp = lmi->NIL;

    (lmi->lstTab [LISP$M_getRefId (tmp)]).car = swp;

    /* Collect all objects in this list */
    while (*level != currentLevel) {
      /* Read next object */
      swp = LISP$S_readAction (lmi, level);

      if (swp == LISP$M_NULLREF)
        (lmi->lstTab [LISP$M_getRefId (tmp)]).cdr = lmi->NIL;

      else {
        /* Create a new sublist where to save next values */
        (lmi->lstTab [LISP$M_getRefId (tmp)]).cdr = LISP$M_createList (lmi);

        /* Set the current list to a new one */
        tmp = (lmi->lstTab [LISP$M_getRefId (tmp)]).cdr;

        /* Fill it's CAR */
        (lmi->lstTab [LISP$M_getRefId (tmp)]).car = swp;
      }
    }
  }

  /* End of current list reached */
  else if (end == LISP$S_PAR_CLOSE) {
    #ifdef DEBUG_PARSER
    LISP$M_throwMessage (lmi, 'D', "DEBUG", "#%d CLOSE; %d",
        *level, currentLevel);
    #endif

    if (--(*level) < 0) {
      LISP$M_setError (lmi, LISP$_ERR$S_BADSYN,
          "BADSYN", "Bad syntax, unexpected parenthesis!");
      /* Set error position if there was no problem before */
      if (lmi->inputErrorPosition < 0)
        lmi->inputErrorPosition = lmi->inputReadPosition; //strlen (lmi->inputBuffer);
      res = LISP$M_NULLREF;
    }

    /* There's something before closing parenthesis */
    else if (!isspace (token [0]) && token [0] != LISP$S_EOS)
      res = isNumeric?
        LISP$M_getCreateNumber (lmi, strtod (token, NULL)) :
        LISP$M_getCreateAtom (lmi, token);

    /* NIL token, just only closing parenthesis.. */
    /* We return NULLREF, to recognize that this is a NIL enclosing the
     * list, not the NIL entered from user */
    else
      res = LISP$M_NULLREF;
  }

  /* Single atom */
  else {
    if (!isspace (end)) {
      LISP$M_setError (lmi, LISP$_ERR$S_BADSYN,
          "BADSYN", "Bad syntax!");
      /* Set error position if there was no problem before */
      if (lmi->inputErrorPosition < 0)
        lmi->inputErrorPosition = lmi->inputReadPosition; //strlen (lmi->inputBuffer);
      res = LISP$M_NULLREF;
    }
    else {
      res = isNumeric?
        LISP$M_getCreateNumber (lmi, strtod (token, NULL)) :
        LISP$M_getCreateAtom (lmi, token);
    }
  }

  free (token);
  return res;
}

LISP$Ref LISP$S_read (LISP$MachIns * lmi) {

  int level = 0;

  if (lmi->inputBuffer [strlen (lmi->inputBuffer)] == LISP$S_EOS)
    LISP$S_makePrompt (lmi, true, true);

  lmi->inputBuffer [0] = LISP$S_EOS;

  return (LISP$S_readAction (lmi, &level));
}

void LISP$S_trace (LISP$MachIns * lmi, LISP$Ref res, int level) {

  /* Is tracing enabled? */
  if (!LISP$M_isOptionEnabled (lmi, LISP$M_OPT_TRACE)) return;

  int i;
  fprintf (lmi->outputStream, "Trace ==");

  for (i = 0; i < level; i++)
    fprintf (lmi->outputStream, "==");

  fprintf (lmi->outputStream, "> ");

  if (res == LISP$M_NULLREF) fprintf (lmi->outputStream, "LISP$M_NULLREF");
  else             LISP$S_writeAction (lmi, res, false);

  fprintf (lmi->outputStream, "\n");
}

#define RETURN_TRACE(x) \
  { res = (x); LISP$S_trace (lmi, res, level); return (res); }

LISP$Ref LISP$S_evalAction (LISP$MachIns * lmi, LISP$Ref read,
                              int level) {

  level++;

  int             eargc  = 0;        /* Number of evaluated args    */
  LISP$Ref        action = lmi->NIL; /* Action                      */
  LISP$Ref        args   = lmi->NIL; /* Original  action arguments  */
  LISP$Ref        eargs  = lmi->NIL; /* Evaluated action arguments  */
  LISP$Ref        tmp    = lmi->NIL; /* Temporary reference         */
  LISP$Ref        tmpx   = lmi->NIL; /* Temporary reference         */
  LISP$Ref        res    = lmi->NIL; /* Result for RETURN macro     */
  LISP$AtomRecord atom;              /* Shared 'atom' helper var.   */
  LISP$ListRecord list;              /* Shared 'list' helper var.   */

  /* Error while reading S-Expression, nothing to evaluate */
  if (LISP$M_isError (lmi) || read == LISP$M_NULLREF)
    RETURN_TRACE (LISP$M_NULLREF);

  switch (LISP$M_getRefType (read)) {
    case AtomTab:
      atom = lmi->atmTab [LISP$M_getRefId (read)];
      if (atom.type != Variable &&
          atom.type != Undefined)
        RETURN_TRACE (read);

      if (atom.name [0] == '"') { /* It's a string */
        if (atom.name [1] == '"') { /* It's an empty string */
          RETURN_TRACE (lmi->NIL);
        }
        else {
          RETURN_TRACE (read);
        }
      }

      /* Ordinary atom */
      switch (atom.type) {
        case Undefined:
          LISP$M_setError (lmi, LISP$_ERR$S_UNDEF, "UNDEF",
              "Atom \'%s\' is not defined!", atom.name);
          RETURN_TRACE (LISP$M_NULLREF);

        default:
          RETURN_TRACE (atom.value);
      }

    case NumberTab:
      RETURN_TRACE (read);

    case ListTab:
      list = lmi->lstTab [LISP$M_getRefId (read)];

      /* NIL-list -- '()' */
      if (list.car == lmi->NIL && list.cdr == lmi->NIL)
        RETURN_TRACE (lmi->NIL);
  
      action = list.car;
      /* For unnamed lambdas and specials */
      if (LISP$M_getRefType (action) == ListTab) {
        action = LISP$S_evalAction (lmi, action, level);
        if (action == LISP$M_NULLREF) RETURN_TRACE (LISP$M_NULLREF);
      }

      if (LISP$M_getRefType (action) != AtomTab) {
        LISP$M_setError (lmi, LISP$_ERR$S_IVARGS, "IVARGS",
            "Specified lambda action is not an atom!");
        RETURN_TRACE (LISP$M_NULLREF);
      }
      atom = (lmi->atmTab [LISP$M_getRefId (action)]);

      #ifdef DEBUG_EVAL
      LISP$M_throwMessage (lmi, 'D', "DEBUG", "ACTION: %s", atom.name);
      #endif

      /* Special form gets arguments as they are -- only count them */
      if (LISP$M_isSpecialForm (atom)) {
        eargs = list.cdr;
        tmp   = eargs;
        while (tmp != lmi->NIL) {
          tmp = (lmi->lstTab [LISP$M_getRefId (tmp)]).cdr;
          eargc++;
        }
      }

      /* Function arguments pre-evaluation */
      else if (LISP$M_isFunction (atom)) {

        /* Save original arguments to be evaluated */
        args  = list.cdr;

        /* Create new evaluated args list */
        if (args != lmi->NIL) {
          eargs = LISP$M_createList (lmi);
          tmp   = eargs;
        }
        else eargs = lmi->NIL;

        while (args != lmi->NIL) {
          /* Evaluate CAR of the rest of arguments and store *
           * it to a list of evaluated arguments             */
          tmpx = LISP$S_evalAction (lmi,
              (lmi->lstTab [LISP$M_getRefId (args)]).car, level);
          if (tmpx == LISP$M_NULLREF) RETURN_TRACE (LISP$M_NULLREF);
          (lmi->lstTab [LISP$M_getRefId (tmp)]).car = tmpx;

          /* Increment evaluated argument counter */
          eargc++;

          /* Skip to the next argument to be evaluated */
          args = (lmi->lstTab [LISP$M_getRefId (args)]).cdr;

          if (args == lmi->NIL) break;

          /* Create a new list entry */
          (lmi->lstTab [LISP$M_getRefId (tmp)]).cdr = LISP$M_createList (lmi);
          tmp = (lmi->lstTab [LISP$M_getRefId (tmp)]).cdr;
        }
      }
      else {
        LISP$M_setError (lmi, LISP$_ERR$S_IVARGS, "IVARGS",
            "Lambda action must be a function or special form!");
        RETURN_TRACE (LISP$M_NULLREF);
      }

      list = lmi->lstTab [LISP$M_getRefId (eargs)];

      /* It's a built-in */
      if (atom.type == BuiltInFun || atom.type == BuiltInSpecForm) {

        if (LISP$M_checkBuiltIn (lmi, "CAR", atom, list, eargc, 1, 1))
          { RETURN_TRACE (LISP$M_builtInCAR (lmi, eargs)); }

        else if (LISP$M_checkBuiltIn (lmi, "CDR", atom, list, eargc, 1, 1))
          { RETURN_TRACE (LISP$M_builtInCDR (lmi, eargs)); }

        else if (LISP$M_checkBuiltIn (lmi, "LIST", atom,
                 list, eargc, 0, LISP$INF))
          { RETURN_TRACE (eargs); }

        else if (LISP$M_checkBuiltIn (lmi, "QUOTE", atom,
                   lmi->lstTab [LISP$M_getRefId (eargs)], eargc, 1, 1))
          { RETURN_TRACE ((lmi->lstTab [LISP$M_getRefId (eargs)]).car); }

        else if (LISP$M_checkBuiltIn (lmi, "SETQ", atom,
                   lmi->lstTab [LISP$M_getRefId (eargs)], eargc, 2, 2))
          { RETURN_TRACE (LISP$M_builtInSETQ (lmi, eargs, level)); }

        else if (LISP$M_checkBuiltIn (lmi, "CONS", atom,
                   lmi->lstTab [LISP$M_getRefId (eargs)], eargc, 2, 2))
          { RETURN_TRACE (LISP$M_builtInCONS (lmi, eargs)); }

        else if (LISP$M_checkBuiltIn (lmi, "+", atom,
                   lmi->lstTab [LISP$M_getRefId (eargs)], eargc, 1, LISP$INF) ||
                 LISP$M_checkBuiltIn (lmi, "-", atom,
                   lmi->lstTab [LISP$M_getRefId (eargs)], eargc, 1, LISP$INF) ||
                 LISP$M_checkBuiltIn (lmi, "*", atom,
                   lmi->lstTab [LISP$M_getRefId (eargs)], eargc, 1, LISP$INF) ||
                 LISP$M_checkBuiltIn (lmi, "/", atom,
                   lmi->lstTab [LISP$M_getRefId (eargs)], eargc, 2, LISP$INF) ||
                 LISP$M_checkBuiltIn (lmi, "%%", atom,
                   lmi->lstTab [LISP$M_getRefId (eargs)], eargc, 2, 2))
          { RETURN_TRACE (LISP$M_builtInArithmetic (lmi, eargs, eargc,
                                                    atom.name)); }

        else if (LISP$M_checkBuiltIn (lmi, "IS", atom,
                 lmi->lstTab [LISP$M_getRefId (eargs)], eargc, 2, 2))
          { RETURN_TRACE (LISP$M_builtInIS (lmi, eargs)); }

        #define COMPARE(op) \
          list = lmi->lstTab [LISP$M_getRefId (eargs)]; \
          tmp  = list.cdr; \
          tmp  = (lmi->lstTab [LISP$M_getRefId (tmp)]).car; \
          if (LISP$M_getRefType (list.car) != NumberTab || \
              LISP$M_getRefType (tmp) != NumberTab) { \
            LISP$M_setError (lmi, LISP$_ERR$S_IVARGS, "IVARGS", \
                "%s works only with numeric arguments!", atom.name); \
            RETURN_TRACE (LISP$M_NULLREF); \
          } \
          \
          RETURN_TRACE ((lmi->numTab [LISP$M_getRefId (list.car)] op\
                   lmi->numTab [LISP$M_getRefId (tmp)])? lmi->T : lmi->NIL);

        else if (LISP$M_checkBuiltIn (lmi, "=", atom,
                   lmi->lstTab [LISP$M_getRefId (eargs)],
                   eargc, 2, 2)) { COMPARE(==) }
        else if (LISP$M_checkBuiltIn (lmi, ">", atom,
                   lmi->lstTab [LISP$M_getRefId (eargs)],
                   eargc, 2, 2)) { COMPARE(>)  }
        else if (LISP$M_checkBuiltIn (lmi, ">=", atom,
                   lmi->lstTab [LISP$M_getRefId (eargs)],
                   eargc, 2, 2)) { COMPARE(>=) }
        else if (LISP$M_checkBuiltIn (lmi, "<", atom,
                   lmi->lstTab [LISP$M_getRefId (eargs)],
                   eargc, 2, 2)) { COMPARE(<)  }
        else if (LISP$M_checkBuiltIn (lmi, "<=", atom,
                   lmi->lstTab [LISP$M_getRefId (eargs)],
                   eargc, 2, 2)) { COMPARE(<=) }
        #undef COMPARE

        else if (LISP$M_checkBuiltIn (lmi, "EVAL", atom,
                 lmi->lstTab [LISP$M_getRefId (eargs)], eargc, 1, 1)) {
          tmp = LISP$S_evalAction (lmi,
              (lmi->lstTab [LISP$M_getRefId (eargs)]).car, level);
          RETURN_TRACE (LISP$S_evalAction (lmi, tmp, level));
        }

        else if (LISP$M_checkBuiltIn (lmi, "COND", atom,
                 lmi->lstTab [LISP$M_getRefId (eargs)], eargc, 1, LISP$INF))
          { RETURN_TRACE (LISP$M_builtInCOND (lmi, eargs, level)); }

        else if (LISP$M_checkBuiltIn (lmi, "LOAD", atom,
                lmi->lstTab [LISP$M_getRefId (eargs)], eargc, 1, 1))
          { RETURN_TRACE (LISP$M_builtInLOAD (lmi, eargs, level)); }

        else if (LISP$M_checkBuiltIn (lmi, "SHOW-MEM", atom,
                 lmi->lstTab [LISP$M_getRefId (eargs)], eargc, 0, 1))
          { RETURN_TRACE (LISP$M_builtInSHOWMEM (lmi, eargs, eargc)); }

        else if (LISP$M_checkBuiltIn (lmi, "GETLPI", atom,
                 lmi->lstTab [LISP$M_getRefId (eargs)], eargc, 0, 1))
          { RETURN_TRACE (LISP$M_builtInGETLPI (lmi, eargs, eargc)); }

        else if (LISP$M_checkBuiltIn (lmi, "LET", atom,
                 lmi->lstTab [LISP$M_getRefId (eargs)], eargc, 2, LISP$INF))
          { RETURN_TRACE (LISP$M_builtInLET (lmi, eargs, level)); }

        else if (LISP$M_checkBuiltIn (lmi, "LAMBDA", atom,
                 lmi->lstTab [LISP$M_getRefId (eargs)], eargc, 2, 2))
          { RETURN_TRACE (LISP$M_builtInLAMBDASPECIAL (lmi, eargs, true)); }

        else if (LISP$M_checkBuiltIn (lmi, "SPECIAL", atom,
                 lmi->lstTab [LISP$M_getRefId (eargs)], eargc, 2, 2))
          { RETURN_TRACE (LISP$M_builtInLAMBDASPECIAL (lmi, eargs, false)); }

        else if (LISP$M_checkBuiltIn (lmi, "ISDEFINED", atom,
                 lmi->lstTab [LISP$M_getRefId (eargs)], eargc, 1, 1))
          { RETURN_TRACE (LISP$M_builtInISDEFINED (lmi, eargs)); }

        else if (LISP$M_checkBuiltIn (lmi, "ISATOM", atom,
                 lmi->lstTab [LISP$M_getRefId (eargs)], eargc, 1, 1))
          { RETURN_TRACE (LISP$M_builtInISATOM (lmi, eargs)); }

        else if (LISP$M_checkBuiltIn (lmi, "ISNUMBER", atom,
                 lmi->lstTab [LISP$M_getRefId (eargs)], eargc, 1, 1))
          { RETURN_TRACE (LISP$M_builtInISNUMBER (lmi, eargs)); }

        else if (LISP$M_checkBuiltIn (lmi, "ISLIST", atom,
                 lmi->lstTab [LISP$M_getRefId (eargs)], eargc, 1, 1))
          { RETURN_TRACE (LISP$M_builtInISLIST (lmi, eargs)); }

        else if (LISP$M_checkBuiltIn (lmi, "BODY", atom,
                 lmi->lstTab [LISP$M_getRefId (eargs)], eargc, 1, 1))
          { RETURN_TRACE (LISP$M_builtInBODY (lmi, eargs)); }

        else {
          LISP$M_setError (lmi, LISP$_ERR$S_NOTIMP, "NOTIMP",
              "%s is not implemented built-in!", atom.name);
          RETURN_TRACE (LISP$M_NULLREF);
        }
      }

      /* It's user defined */
      else {
        /* Bind arguments */
        tmp  = atom.bindList;
        tmpx = eargs;
        while (tmp != lmi->NIL) {
          /* XXX: check it is always atom!! */
          /* Bind it */
          LISP$Ref beg = (lmi->lstTab [LISP$M_getRefId (tmp)]).car;
          if (beg == lmi->NIL) break;
          LISP$M_bind (lmi, beg,
              (lmi->lstTab [LISP$M_getRefId (tmpx)]).car);

          /* Move to the next atom/value to be bound together */
          tmp  = (lmi->lstTab [LISP$M_getRefId (tmp)]).cdr;
          tmpx = (lmi->lstTab [LISP$M_getRefId (tmpx)]).cdr;
          eargc--;
        }
        (lmi->atmTab [LISP$M_getRefId (action)]) = atom;

        /* Invalid argument count */
        if (eargc) {
          LISP$M_setError (lmi, LISP$_ERR$S_IVARGS, "IVARGS",
              "Too %s arguments!", (eargc > 0)? "much" : "few");
          res = LISP$M_NULLREF;
        }

        /* Evaluate */
        else res = LISP$S_evalAction (lmi, atom.value, level);

        /* UnBind aguments */
        tmp  = atom.bindList;
        while (tmp != lmi->NIL) {
          LISP$Ref beg = (lmi->lstTab [LISP$M_getRefId (tmp)]).car;
          if (beg == lmi->NIL) break;

          /* UnBind it */
          LISP$M_unBind (lmi, beg);

          /* Move to the next atom to be unbound */
          tmp = (lmi->lstTab [LISP$M_getRefId (tmp)]).cdr;
        }
        (lmi->atmTab [LISP$M_getRefId (action)]) = atom;

        RETURN_TRACE (res);
      }
  }

  /* Just for compiler, to be happy ;-) */
  RETURN_TRACE (LISP$M_NULLREF);
}
#undef RETURN_TRACE

LISP$Ref LISP$S_eval (LISP$MachIns * lmi, LISP$Ref read) {

  return (LISP$S_evalAction (lmi, read, -1));
}

char * LISP$S_getFormatAtomType (LISP$AtomType type) {

  switch (type) {
    case Undefined:
      return "UNDEFINED";
    case Variable:
      return "VARIABLE";
//    case Number:
//      return "NUMBER";
//    case DottedPair:
//      return "DOTTED-PAIR";
    case BuiltInFun:
      return "BUILT-IN-FUNCTION";
    case BuiltInSpecForm:
      return "BUILT-IN-SPECIAL-FORM";
    case UserDefFun:
      return "USER-DEFINED-FUNCTION";
    case UserDefSpecForm:
      return "USER-DEFINED-SPECIAL-FORM";
    case UnnamedFun:
      return "UNNAMED-FUNCTION";
    case UnnamedSpecForm:
      return "UNNAMED-SPECIAL-FORM";
    default:
      return "UNKNOWN";
  }
}

void LISP$S_writeAction (LISP$MachIns * lmi, LISP$Ref evaluated,
                         bool isTopLevel) {

  LISP$ListRecord list;
  LISP$ListRecord tmpList;
  LISP$AtomRecord atom;
  switch (LISP$M_getRefType (evaluated)) {
    case AtomTab:
      atom = lmi->atmTab [LISP$M_getRefId (evaluated)];
      if (isTopLevel && atom.type != Variable && atom.type != Undefined) {
        fprintf (lmi->outputStream,
            "{%s \'%s\', the ATM#" LISP$ADDRFMT "}",
            LISP$S_getFormatAtomType (atom.type),
            atom.name, LISP$M_getRefId (evaluated));
      }
      else {
        fprintf (lmi->outputStream, "%s",
            (lmi->atmTab [LISP$M_getRefId (evaluated)]).name);
      }
      break;
    case NumberTab:
      fprintf (lmi->outputStream, "%-g",
          lmi->numTab [LISP$M_getRefId (evaluated)]);
      break;
    case ListTab:
      list = lmi->lstTab [LISP$M_getRefId (evaluated)];

      fprintf (lmi->outputStream, "%c", LISP$S_PAR_OPEN);
      LISP$S_writeAction (lmi, list.car, false);

      tmpList = list;
      while (LISP$M_getRefType (tmpList.cdr) == ListTab) {
        fprintf (lmi->outputStream, " ");

        tmpList = lmi->lstTab [LISP$M_getRefId (tmpList.cdr)];
        LISP$S_writeAction (lmi, tmpList.car, false);
      }

      if (tmpList.cdr != lmi->NIL) {
        fprintf (lmi->outputStream, " . ");
        LISP$S_writeAction (lmi, tmpList.cdr, false);
      }
      fprintf (lmi->outputStream, "%c", LISP$S_PAR_CLOSE);
      break;
  }
}

void LISP$S_write (LISP$MachIns * lmi,
                   LISP$Ref evaluated) {

  /* Report error if there was something wrong  *
   * while parsing or evaluating                */
  if (LISP$M_isError (lmi)) {
    LISP$M_reportError (lmi);
  }

  /* We are going to quit, so LISP$M_NULLREF evaluator result is OK */
  else if (LISP$M_is2ShutDown (lmi)) {
    fprintf (lmi->outputStream, "\n");
    LISP$M_throwMessage (lmi, 'I', "QUIT", "Bye.");
  }

  /* Nothing evaluated */
  else if (evaluated == LISP$M_NULLREF) {
    #ifdef DEBUG_EVAL
    LISP$M_setError (lmi, LISP$_ERR$S_NOEVAL, "NOEVAL",
                     "No result from evaluator!");
    LISP$M_reportError (lmi);
    #endif
  }

  /* Print the result */
  else {
    fprintf (lmi->outputStream, "\r" LISP$PROMPT_RES);
    LISP$S_writeAction (lmi, evaluated, true);
    fprintf (lmi->outputStream, "\n");
  }

  /* Collect garbage */
  LISP$M_collectGarbage (lmi);
}

bool LISP$S_loadFile (LISP$MachIns * lmi, char * fileName) {

  if (fileName == NULL) return false;

  FILE * origInStream  = lmi->inputStream;
  FILE * origOutStream = lmi->outputStream;
  FILE * in; LISP$Ref res;
  LISP$Mode origMode = lmi->mode;

  in = fopen (fileName, "r");
  if (in == NULL) {
    LISP$M_setError (lmi, LISP$_ERR$S_NAOFIL, "NAOFIL",
        "unable to open file %s", fileName);
    LISP$M_reportError (lmi);
    return false;
  }

  /* Switch mode to BATCH */
  lmi->mode = Batch;

  /* Set current file as input stream */
  LISP$M_setStreams (lmi, in, origOutStream);

  /* Read code from there */
  res = LISP$S_read (lmi);
  res = LISP$S_eval (lmi, res);

  while (LISP$M_is2Process (lmi)) {
    LISP$S_write (lmi, res);
    res = LISP$S_read (lmi);
    res = LISP$S_eval (lmi, res);
  }
  fclose (in);

  /* Restore original I/O streams */
  LISP$M_setStreams (lmi, origInStream, origOutStream);

  /* Restore mode */
  lmi->mode = origMode;

  /* Report error if there was problem */
  if (LISP$M_isError (lmi)) {
    LISP$M_reportError (lmi);
    return false;
  }

  return true;
}

// vim: fdm=syntax:fdn=1:tw=74:ts=2:syn=c
