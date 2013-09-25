/*
 * $Id: LISP_Main.c 35 2008-01-08 21:37:42Z tmr $
 *
 * Module:  LISP_Main -- LISP Interpreter's main interaction loop
 * Created: 30-NOV-2007 20:55
 * Author:  tmr
 */

#include <stdio.h>

#include "LISP_Core.h"

#ifdef _VMS_
#include <ssdef.h>
#else
/* Normal completition */
#define SS$_NORMAL        0
#define SS$_INSFARG      10
#define SS$_ACCVIO       20
#endif

#define LISP$INPUT    stdin
#define LISP$OUTPUT   stdout


int main (int argc, char * argv []) {

  int i;
  LISP$MachIns * lmi;
  LISP$Ref       ref;

  /* Initialize LISP Machine */
  lmi = LISP$M_init (argv [0], LISP$INPUT, LISP$OUTPUT);

  /* Batch mode -- load startup files */
  for (i = 0; i < argc; i++)
    LISP$S_loadFile (lmi, (!i)? "startup.lisp" : argv [i]);

  while (LISP$M_is2Process (lmi)) {
    ref = LISP$S_read (lmi);
    ref = LISP$S_eval (lmi, ref);

    LISP$S_write (lmi, ref);
  }

  LISP$M_destroy (lmi);

  return SS$_NORMAL;
}

// vim: fdm=syntax:fdn=1:tw=74:ts=2:syn=c
