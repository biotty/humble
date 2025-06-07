#!/usr/bin/env python3

from mics import init_top, parse, run_top, set_verbose

import sys
def test(s):
    sys.stdout.write("test\n")
    names, env, macros = init_top()
    filename = "test-snip"
    tree = parse(s, names, macros, env.keys())
    run_top(tree, env, names)
    sys.stdout.write("*\n")

# for debug:
# set_verbose(True)

test("")
test("1")
test("(+)")
test("'(#f #t)")
test("(define i '()) i")
test("(define (f) 1) `(,(f))")
test("""[ define y 1 ] ; a comment
(list y (list y y))
{+ 1 2}
y
(append '(1) 2)
(set! y (+ 1 y))
(display (set! y (+ 1 y)))
(cond (#f 1) (1 y))""")
test("(macro M () 123) (define z (M))")
test("""
(macro incr! (yy) `(set! ,yy (+ 1 ,yy)))
(define y 1)
(incr! y)
(unless (equal? y 2) (error))
(display "alright") (newline)
(define obj [lambda () (define i 0) (lambda () (incr! i) i)])
(define a (obj))
(define b (obj))
(unless
  (equal?
    (list (+ (a)) (+ (a)) (+ (b)) (+ (a)))
    '(1 2 1 3))
  (error))
(display "hello world!")
""")
test(""" [ define y 1 ]
(let ((s 2))
  (let ((s 3) (t s)) (+ s t)))
(let* ((A 300) (B (+ A 10))) (+ A B 9))
(define foo
[lambda (x)
              {
                (lambda (a) (define i 1) (+ a x i)) y
              }
])
(foo 3)
(letrec ((a 1)
         (b (list (lambda () a))))
    ((list-ref b 0)))
(letrec ((a 1)
         (b (list (lambda () a)))
         (c (lambda (x) (+ ((list-ref b 0)) a 1 x))))
    (c 9))
(letrec ((A (lambda (n i) (cond ((eqv? n 0) i) (1 (B n i)))))
         (B (lambda (n i) (A (+ n -1) (+ i 2)))))
  (A 4 1)) ; another comment
#f
#t
(list-ref (list 1 2 3) 2)
`(a (a b ,(+ 1 1)))
(car (car (cdr '(1 (8 2) 3 () 5))))
(list (quote ()))
(list (list 1 2))
'((list 1 2))
(letrec* ((foo (lambda () 123)) (goo foo)) (goo))
`(a ,@(list (+ 1 2)))
(cdr (cdr (cdr '(1 2 3 4))))
(cons 0 '(1 2))
(append '(1 2) '(3 4) '(5 6))
(car '(1 . 2))
(cdr '(1 . 2))
((lambda (a b . c) c) 1 2)
((lambda (a b . c) c) 1 2 3 4)
(define (f x) (+ x 1))
(f 1)
(define (g . x) x)
(g 1 2 3)
(define (((h . x) y) z) (+ (car x) y z))
(((h 1) 2) 3)
(letrec ((even?
          (lambda (n)
            (if (zero? n)
                #t
                (odd? (- n 1)))))
         (odd?
          (lambda (n)
            (if (zero? n)
                #f
                (even? (- n 1))))))
  (even? 123))
(let loop ((x 0) (y 0)) (if (eqv? x 9) `(,x ,y) (loop (+ x 1) (- y 3))))
(define li_a '(1 2 3))
(define li_b '(4 5 6))
(list li_b)
(append li_a li_b)
(list li_a)
(list li_b)
(eqv? li_a li_b)
(equal? li_a li_b)
(equal? li_a '(1 2 3))
(eq? li_a '(1 2 3))
(boolean? #f)
(number? 1)
(procedure? h)
(pair? '(1 2))
(symbol? 'a)
(null? '())
(list? '(1 2))
(pair? '(1 . 2))
(null? 0)
(list? '(1 . 2))
(pair? '())
(not #f)
(not 12)
(and)
(and 1)
(and 0 1)
(or)
(or 0)
(or #f 1 #f)
(= 5 5 5)
(= 5 5 4)
(> 5 4 3)
(< 5 4)
(<=)
(>= 9 9)
(begin 1 2 3)
(let* () 0 1 #f)
(if (let* () 0 1 #f) 2 3)
(cond (#f not) (else 1 2 3))
(cond (#f 0) (#t => not) (1 2))
(cond ((cdr '(1 2 3)) => car))
(cond ((or #f #t) => (lambda (x) (or x))))
(and #t 55)
(and (or (eqv? 1 2) (eqv? 1 1)) '(4 5))
(or (and (or (eqv? 1 1) (eqv? 2 1)) '(4 5)))
(car (or (and (or (eqv? 1 2) (eqv? 2 1)) '(4 5))
         (and (or (eqv? 1 2) (eqv? 1 1)) '(9 7))
      ))
(eq? '() '())
(case 1 ((2 (+ 0 1)) => +) ((0) 123) ((0) 55) (else 99 101))
(case 1 ((2 (+ 0 1)) => (lambda (x) (+ x 1))) ((0) 123) ((0) 55) (else 99 101))
(case 3 ((3) 111))
`(foo ,(list 1 2))
`(foo ,@(list 1 2))
(define li '(1 2 3))
`(doo ,li)
`(doo ,@li)
((lambda x x) 1 2 3)
(when 1 2 3 4 5)
(unless #f #f 2)
(unless #t)
(unless #f)
(display 123)
(newline)
(let loop ((a 9)) (cond (#f 0) (1 (if (eqv? a 0) `(,a) (loop (- a 1))))))
(do ((a 0 (+ a 1)) (b 1)) ((eqv? a 9) 1 `(,a ,b)) (display a) (newline))
(let ((a 1)) `,a)
`(a ,li)
`,(list 'a)
(define e 1)
`(,@(list e))
(quasiquote (1 2 (unquote (+ 3 4))))
(quote (unquote 1))
`(a `(b ,(+ 1 2) ,(foo ,(+ 1 3) d) e) f)
`(list ,(+ 1 2) 4)
(let ((name1 'x)
  (name2 'y))
  `(a `(b ,,name1 ,',name2 d) e))
(let ((name (quote a))) (quasiquote (list (unquote name) (quote (unquote name)))))
(let ((name 'a)) `(list ,name ',name))
`(a `(b ,(+ 1 2) ,(foo ,(+ 1 3) d) e) f)
(let ((name1 'x)
(name2 'y))
`(a `(b ,,name1 ,',name2 d) e))
(let ((a 3)) `((1 2) ,a ,4 ,'five 6))
(let ((a 3)) (list (list 1 2) a 4 'five 6))
(cdr '(1 2 3))
(define x '(1 2 3))
`(list ,@(cdr x))
(let ((x '(1 2 399))) `(list ,@(cdr x)))
(macro fu x `(list ,@(cdr x)))
(fu 1 22 3)
(macro fi (x . y) `(list ,@y))
(fi 4 5 6)
(define ix 'a)
`(foo ((,ix 123)) ,ix)
`(let ((,ix 123)) ,ix)
``(a ,,(+ 1 2))
`(( foo ,(- 10 3)) ,@(cdr '(c)) . ,(car '(cons)))
((case-lambda ((x) x) ((x y) (+ x y))) 123 1)
(define qw (lambda (x) (if (zero? x) 0 (qw (- x 1)))))
(qw 9)
(define range
(case-lambda
((e) (range 0 e))
((b e) (do ((r '() (cons e r))
(e (- e 1) (- e 1)))
((< e b) r)))))
(range 3)
(range 3 5)
(apply + (list 3 4))
(map + '(1 3) '(2 9 0))
(gensym)
`(9 `((unquote (unquote e))))
`(9 `((unquote e)))
`(9 `(,e))
`(a ,(+ 1 2) ,(map abs '(4 -5 6)) b)
``,'e
``,(quote e)
(let ((foo '(foo bar)) (@baz 'baz))
  `(list ,@foo , @baz))
(macro xx (yy) (let ((zz (gensym))) `(list ',zz ,yy)))
(xx 78)
(cdr '(1 . 2))
(string->list "hello world")
(symbol->string 'foo)
(letrec ((y (lambda (x) (if (zero? x) 0 (y (- x 1)))))) (y 99))
(letrec ((y (lambda (x) (if (not (zero? x)) (y (- x 1)) 0)))) (y 99))
(list-ref (cons 1 '(2)) 1)
(list-ref (cons 1 '(2 . 3)) 1)
(define z (list 1 2))
(define w '(3 3 . 4))
(append (cdr z) (cdr z))
(append (cdr z) (cdr w))
(list-copy (cons 1 '(2 3)))
(define range
(case-lambda
((e) (range 0 e))
((b e) (do ((r '() (cons e r))
(e (- e 1) (- e 1)))
((< e b) r)))))
(range 3)
(range 3 5)
(define (aa s) 2)
'(#\\tab #\\@ #\\# #\\")
(macro zx (a) a)
(zx (aa ''e))
(* 1 2 3)
(/ 99 3 2)
(div 11 3)
(define a '(2 4 6))
(list-tail a 1)
(list-set! a 1 5)a
(let ((x 1) (y 2)) x y)
(macro chko (x y) ''DISABLED-CHK)
(macro chk (x y)
  (let ((a (gensym)) (b (gensym)))
  `(let ((,a ,x) (,b ,y))
    (unless (equal? ,a ,b) (error (list ,a ,b))))))
(chk 0 0)
(chk a '(2 5 6))
(unless (equal? a '(2 5 6)) (error))
(member 9 '())
(member 9 (cdr '(0)))
(member 9 '(3 9 1))
(member 9 '(3 9 . 1))
(member (lambda (x) (eqv? x 0)) '(3 0 5 4))
(chk
  (assoc 5 '((3 . 1) (0) (5 6 7)))
  (assoc (lambda (x) (eqv? x 5)) '((3 . 1) (0) (5 6 7))))
(substring "foo bar" 4)
(substring(string-append "foo" "bar" "gee") 1 8)
(if (string=? "a" "a")
    (unless (string<? "a" "b") (error 'juv))
    (error 'esh)); #| multi-
;    line comment |# 123
(define-record-type <nm> (drt i j) tt? (i iget) (j jget jset))
(define v (drt 1 2))
(tt? v)
(iget v)
(jset v 9)
(jget v)
'(1 . (2 . ())); ==> (1 2)
'(1 . (2 . 5)); ==> (1 2 . 5)
(define d (alist->dict '[(1 . 2)]))
(chk (dict? d) #t)
(dict-get-default! d 1 9)
(dict-get-default! d 2 9)
(assoc 2 (dict->alist d))
(chk (assoc 2 (dict->alist d)) '(2 . 9))
(dict-set! d 7 8)
(chk 8 (dict-if-get d 7 0 (lambda (x) x)))
'((6 1 3) (-5 -2))
(chk '((6 1 3) (-5 -2))
  (let loop ((numbers '(3 -2 1 6 -5))
      (nonneg '())
      (neg '()))
  (cond ((null? numbers) (list nonneg neg))
    ((>= (car numbers) 0)
     (loop (cdr numbers)
        (cons (car numbers) nonneg)
        neg))
    ((< (car numbers) 0)
     (loop (cdr numbers)
        nonneg
        (cons (car numbers) neg)))))
)
(define fact
(lambda (n)
(if (= n 0) 1 (* n (fact (- n 1))))))
(fact 19)
; above is not tail-recursive (as defined)
; following will hang to output repr
; this is a known behavior (as decided)
; because we do not cope with data loops
; neither in input (label-syntax) nor output
; (let ((x (list 'a 'b 'c)))
; (set-cdr! (cddr x) x) x)
(define add4
     (let ((x 4))
     (lambda (y) (+ x y))))
(chk (add4 6) 10)
(chk ((lambda (x y . z) z)
    3 4 5 6)
       '(5 6))
; note:  quoted names needed in case, such as the following
; -- this deviates from r7rs specification (spec.typo?)
(define c
  (case (car '(c d))
     (('a 'e 'i 'o 'u) 'vowel)
     (('w 'y) 'semivowel)
     (else => (lambda (x) x))))
(chk c 'c)
; ^ anomaly: when doing c inline -- as chk macro arg,
;            then chk report (~fun~ c), not equal
(scope (export b+) (define a+ 11) (define b+ a+))
(chk b+ 11)
(define@ c+ d+ (list 22 33))
(chk d+ 33)
(chk '(2 3 4) (cdr '(1 2 3 4)))
(chk (cdr '(1 2 3 4)) (cdr '(1 2 3 4)))
(let tco ((a 789)) (if (zero? a) 0 (apply tco (list (- a 1)))))
""")
#
# --- the following deferred for c++ impl (python too encoding-nosy)
#
#     a string is of bytes, but io works with that, while it can be
#     operated on as an ustring as follows.
# ( ) ustring-ref that gives unicode-value doing UTF-8
# ( ) list->ustring and ustring->list (UTF-8 <-> unicode values)
#
# ( ) file-exists? delete-file rename-file file-stat exit
# ( ) with-input-from-file with-output-to-file
# ( ) write-byte read-byte eof-object?
# ( ) peek-byte read-line write-string
# ( ) with-input-from-pipe with-output-to-pipe
#
# ( ) builtin posix getopts w help very vanilla in the code
#     written in c++ with shared_ptr all the things.
#
# ( ) regex match-to-groups (ontop c++regex)
#
# --- features:
#
# checked ()[]{}
# contigous (non)lists until cdr-usage on which list variable
# transforms to a cons chain.
#     -- list-copy may be done to establish contigous one
# the dict type (an alist with efficient representation)
# macro (exactly as lisp defmacro) -- and gensym
# define@ -- define with list values (no multi-value)
# seq -- lexical block without own scope
# scope -- an import with contents (not loading file)
#       -- export certain names
#       -- just like "here document" import
# import of file (that needs "export" as first expression)
# record type -- not provided for user -- but;
# define-record-type provided as a macro
# cadr combinations
# nonlist function (like list)
# most functions and special forms as specified in r7rs
# but-
#
# --- excluded ofcourse:
#
# multi-value
# eval
# call/cc, values, dyn-param, force-delay, exception
# str->sym (parse-time intern of all names)
# implicit quoting in case
# other commenting than ; and #| .. |#
# label-syntax for data-loops, or output-representation
# float, exact, complex.. ; only integer (even char is)
# not #true #false and no alternative representation
# of #\ except the (unicode) character itself, or
# the ascii specials as specified in r7rs
# |n a m e s|
# port; all with with-
# define-syntax; back to power of unhygienic lisp macro
# make-list prone to error on i-e all to one int and the set!
# no char, but ascii-only strings, where str->list is INT values,
# there is IO for string and byte, no UTF except ustring-functions,
# this is pms for systems programing.  we can process strings
# of non-ascii, as these are the raw bytes, but language
# has no encoding knowledge such as UTF-8, except when parsing
# one #\ in order to get the whole utf value (a mere LEX_NUM)

