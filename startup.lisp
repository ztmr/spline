;
; $Id: $
;
; Module:  startup -- The LISP startup script
; Created: 22-DEC-2007 16:55
; Author:  tmr
;
;  _     _____ ___________  ___  ___           _     _
; | |   |_   _/  ___| ___ \ |  \/  |          | |   (_)
; | |     | | \ `--.| |_/ / | .  . | __ _  ___| |__  _ _ __   ___
; | |     | |  `--. \  __/  | |\/| |/ _` |/ __| '_ \| | '_ \ / _ \
; | |_____| |_/\__/ / |     | |  | | (_| | (__| | | | | | | |  __/
; \_____/\___/\____/\_|     \_|  |_/\__,_|\___|_| |_|_|_| |_|\___|
;
;                          +-+-+-+-+ +-+ +-+-+-+-+-+-+-+ +-+-+-+-+
;                          |J|U|S|T| |A| |C|O|M|M|E|N|T| |T|E|S|T|
;                          +-+-+-+-+ +-+ +-+-+-+-+-+-+-+ +-+-+-+-+

; Enable garbage collector, evaluator tracing and logging
(setq LISP$GC    T) ; this is default
(setq LISP$LOG   NIL)
(setq LISP$TRACE NIL)

(setq report-error (lambda (code msg) (list '%ERROR-REPORT: code '=> msg) ))

; LISP Startup file
;(setq a 1)  ; set A=1
;(setq b 2)  ; set B=2
;(setq c 3)  ; set C=3

;(setq d (list a b c))
;(setq e (list 'a a 'b b 'c c 'd d))

;(setq f
;  (list
;    (list 'a '=> a)
;    (list 'b '=> b)
;    (list 'c '=> c)
;    (list 'd '=> d)
;    (list 'e '=> e)
;  )
;)

;(setq g
;  (list
;    (cons 'list123 (list 1 2 3))
;    (cons 'list456 (list 4 5 6))
;    (cons 'list789 (list 7 8 9))
;  )
;)

;(setq cond4a '(cond ((= a inf) 'A=INF) (t 'A<INF)))
;(setq cond4all '(cond ((<= a inf) (eval cond4a))
;                      ((<= b inf) 'B<=INF)
;                      (t          'DEFAULT)))
;(eval cond4all)   ; let's evaluate it

;(load "test.lisp")

; ---------------------- BegOfTest

; Simple arithmetic test helper
;(setq test$run ; Without lambda which is not implemented yet
;  '(list 'TEST$RUN (list test$args '= test$result) '=>
;         (cond ((= (eval test$args) test$result) 'SUCCESS)
;               (t                                'FAILURE))))

; Lambda version
(setq test$run
  (special (test result) (list test '= result '=>
    (cond ((= (eval test) (eval result)) 'SUCCESS)
          (t                             'FAILURE)))))

; AND, OR, READ, READLN, WRITE, WRITELN

; Basic arithmetic test
;(setq test$args '(+ 1 2 3 4 5)) (setq test$result 15) (eval test$run)
(test$run (+ 1 2 3 4 5) 15)

; +/-Inf and NaN test
;(setq test$args '(+ inf (- inf))) (setq test$result NaN) (eval test$run)
(test$run (+ inf (- inf)) NaN)

; Factorial, a typical recursion example
; Note that there is no lambda nor special built-in implemented,
; so we have to do it a little tricky way
;(setq fact$run '(eval
;  '(cond ((> fact$x 1) (* (+ (setq fact$x (- fact$x 1)) 1) (eval fact$run)))
;         ((show-mem) 1))))

; Using lambda definition
;(setq factorial (lambda (n)
;  (cond ((<= n 1) 1)
;        (t (* n (factorial (- n 1)))))))

; Iterative factorial definition
;(setq fact-iter (lambda (product counter n)
;  (cond ((> counter n) product)
;        (t (fact-iter (* counter product) (+ counter 1) n)))))
;
;(setq factorial (lambda (n) (fact-iter 1 1 n)))

; With memory listing in the deepest scope
(setq factorial (lambda (n)
  (cond ((> n 1) (* n (factorial (- n 1))))
        ((show-mem) 1))))

;(setq test$args '(eval fact$run))
;(setq fact$x   5) (setq test$result 120) (eval test$run)
;(setq fact$x 256) (setq test$result Inf) (eval test$run)

(test$run (factorial   5) 120)
(test$run (factorial 256) Inf)

(setq ! (lambda (x)
  (cond ((isnumber x) (factorial x))
        (T            'NOT-A-NUMBER))))

; ---------------------- EndOfTest

;(setq mycons (lambda (a b) (cons a b)))

(setq not (lambda (x)
  (cond ((is x nil)   t)
        (t          nil))))

(setq abs (lambda (x)
  (cond ((not (isnumber x)) 'NOT-A-NUMBER)
        ((>= x 0) x)
        ((<  x 0) (- x)))))

(setq good-enough (lambda (guess x)
  (< (abs (- (* guess guess) x)) 0.001)))

(setq improve (lambda (guess x)
  (/ (+ guess (/ x guess)) 2.0)))

(setq sqrt-iter (lambda (guess x)
  (cond ((good-enough guess x) guess)
        (t (sqrt-iter (improve guess x) x)))))

(setq sqrt (lambda (x)
  (sqrt-iter (/ x 2.0) x)))

(setq set (special (x y)
  (eval (cons 'setq (cons (eval x) (cons y nil))))))

(setq append (lambda (x y)
  (cond ((is x nil) y)
        ((isatom x) (cons x y))
        (t (cons (car x) (append (cdr x) y))))))

(setq reverse (lambda (x)
  (cond ((isatom x) x)
        (t (append (reverse (cdr x)) (cons (car x) nil))))))

(setq apply (special (f x) (eval (cons f x))))

(setq merge (lambda (v l)
  (cond ((is nil l)    (cons v l))
        ((<= v (car l)) (cons v l))
        (t (cons (car l) (merge v (cdr l)))))))

(setq sort (lambda (x) 
   (cond ((is nil x) x)
         (t (merge (car x) (sort (cdr x)))))))

(setq member (lambda (a s)
  (cond ((is s nil)     nil)
        ((is a (car s)) t)
        (t (member a (cdr s))))))

(setq exp (lambda (a x)
            (cond
              ( (> x 0) (* a (exp a (- x 1))) )
              ( (< x 1) 1 ) )))

(setq bool2bin (lambda (x)
  (cond ( (is x t) 1 )
        ( (is x nil) 0 )
        ( t (report-error 'BOOL2BIN-E-IVARGS "Only T/NIL arguments are accepted!") ) )))

(setq bin2bool (lambda (x)
  (cond ( (= x 1) t )
        ( (= x 0) nil)
        ( t (report-error 'BIN2BOOL-E-IVARGS "Only 0/1 arguments are accepted!") ) )))

; -----------------------------------------------------
; Logical operators -- implemented in a LISP itself,
; since their built-in friends are not implemented yet..
; Note: they are only binary (arity of two) since
; we don't support a variable lenght argument list for
; lambdas...

; Binary AND operation
(setq && (lambda (x y)
  (bin2bool (* (bool2bin x) (bool2bin y))) ))

; Binary OR operation
(setq || (lambda (x y)
  (let ( (res (+ (bool2bin x) (bool2bin y))) )
       (cond ((> res 0) t) (t nil)) )))

; A bit of set operations
(setq is-in-set (lambda (key set)
  (cond ( (not (isatom key))   (report-error 'IS-IN-SET-E-IVARGS "First argument is not an atom!") )
        ( (is set nil)         nil )
        ( (not (islist   set)) (report-error 'IS-IN-SET-E-IVARGS "Second argument is not a list!") )
        ( (is key (car set))   t )
        ( t                    (is-in-set key (cdr set)) ) )))

(setq append-unique (lambda (x y)
  (cond ( (not (isatom x)) (report-error 'APPEND-UNIQUE-E-IVARGS "First argument is not an atom!") )
        ( (is y nil)       (list x) )
        ( (not (islist y)) (report-error 'APPEND-UNIQUE-E-IVARGS "Second argument is not a list!") )
        ( (not (is-in-set x y)) (append x y) )
        ( t y ) )))

(setq conjunct (lambda (x y)
  (cond ( (is x nil) y )
        ( (is y nil) x )
        ( t (append-unique (car x) (conjunct (cdr x) y)) ) )))

(setq remove (lambda (x y)
  (cond ( (not (isatom x)) (report-error 'REMOVE-E-IVARGS "First argument is not an atom!") )
        ( (is y nil)       nil )
        ( (not (islist y)) (report-error 'REMOVE-E-IVARGS "Second argument is not a list!") )
        ( (is x (car y))   (cdr y) )
        ( t                (cons (car y) (remove x (cdr y))) ) )))

(setq disjunct (lambda (x y)
  (cond ( (is x nil) y )
        ( (is y nil) x )
        ( (not (is-in-set (car x) y)) (cons (car x) (disjunct (cdr x) y)) )
        ( t (disjunct (cdr x) (remove (car x) y)) ) )))

(setq intersect (lambda (x y)
  (cond ( (is x nil) nil )
        ( (is y nil) nil )
        ( (is-in-set (car x) y) (cons (car x) (intersect (cdr x) y)) )
        ( t (intersect (cdr x) y) ) )))

(setq conjunct-sorted
      (lambda (x y)
        (sort (conjunct x y)) ))

(setq disjunct-sorted
      (lambda (x y)
        (sort (disjunct x y)) ))

(setq intersect-sorted
      (lambda (x y)
        (sort (intersect x y)) ))

(load "mode.lisp")

; End of startup
; vim: fdm=syntax:fdn=3:tw=74:ts=2:syn=lisp
