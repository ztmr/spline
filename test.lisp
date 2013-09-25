;
; $Id: $
;
; Module:  test -- description
; Created: 22-DEC-2007 16:55
; Author:  tmr

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

; vim: fdm=syntax:fdn=3:tw=74:ts=2:syn=lisp
