;
; $Id: $
;
; Module:  mode -- description
; Created: 13-SEP-2009 15:43
; Author:  tmr

(SETQ PRINT-MODE (SPECIAL ()
  (COND ( (IS (GETLPI MODE) 'INTERACTIVE) 'We-Are-Interactive )
        ( T                               'We-Are-Batch ) )))

(PRINT-MODE)

; vim: fdm=syntax:fdn=3:tw=74:ts=2:syn=lisp
