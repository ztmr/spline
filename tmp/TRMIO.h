/*
 * $Id: $
 *
 * Module:  TRMIO -- description
 * Created: 04-MAY-2008 14:14
 * Author:  tmr
 */

#ifndef _TRMIO_H_
#define _TRMIO_H_

/* Terminal I/O helper instance */
typedef struct {

  FILE  *     inputStream;
  FILE  *     outputStream;

} TRMIO$Instance;

TRMIO$Instance * TRMIO$P_init (FILE * input, FILE * output);

void TRMIO$P_destroy (TRMIO$Instance * trmio);

void TRMIO$P_setInput (TRMIO$Instance * trmio, FILE * inStream);

void TRMIO$P_setOutput (TRMIO$Instance * trmio, FILE * outStream);

int TRMIO$P_getc (TRMIO$Instance * trmio);

#endif

// vim: fdm=syntax:fdn=1:tw=74:ts=2:syn=c
