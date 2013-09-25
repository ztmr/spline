/*
 * $Id: $
 *
 * Module:  key -- description
 * Created: 04-MAY-2008 00:08
 * Author:  tmr
 */

#include <stdio.h>
#include <termios.h>

#include "TRMIO.h"

TRMIO$Instance * TRMIO$P_init (FILE * input, FILE * output) {

  /* Create 'instance' */
  TRMIO$Instance * trmio = (TRMIO$Instance *) malloc (sizeof (TRMIO$Instance));

  /* Set I/O streams */
  TRMIO$P_setInput (trmio, input);
  TRMIO$P_setOutput (trmio, output);

  return (trmio);
}

void TRMIO$P_destroy (TRMIO$Instance * trmio) { free (trmio); }

void TRMIO$P_setInput (TRMIO$Instance * trmio, FILE * inStream) {

  /* Flush old stream */
  if (trmio->inputStream != NULL) fflush (trmio->inputStream);

  /* Set a new stream */
  trmio->inputStream = inStream;
}

void TRMIO$P_setOutput (TRMIO$Instance * trmio, FILE * outStream) {

  /* Flush old stream */
  if (trmio->outputStream != NULL) fflush (trmio->outputStream);

  /* Set a new stream */
  trmio->outputStream = outStream;
}

int TRMIO$P_getc (TRMIO$Instance * trmio) {

  struct termios oldt,
                 newt;
  int            c, fd;

  fd = fileno (trmio->inputStream);

  tcgetattr (fd, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);

  tcsetattr (fd, TCSANOW, &newt);
  c = fgetc (trmio->inputStream);

  tcsetattr (fd, TCSANOW, &oldt);

  return (c);
}

int main () {

  int c;
  TRMIO$Instance * trmio;

  trmio = TRMIO$P_init (stdin, stdout);

  while (c != EOF) {
    c = TRMIO$P_getc (trmio);
    printf (">>>%c<<< >>>%d<<< >>>%X<<<\n", c, c, c);
  }

  TRMIO$P_destroy (trmio);

  return (0);
}

// vim: fdm=syntax:fdn=1:tw=74:ts=2:syn=c
