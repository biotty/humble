#! /usr/bin/env humble

(define args (system-command-line))
(when (> 2 (length args))
  (display "usage: N\n-- "
           "will output primes below N\n")
  (exit 1))

(define n (string->number (list-ref args 1)))
(when (> 3 n)
  (display "error: N below 3\n")
  (exit 1))

(define m (/ n 2))
(define a (make-list n #t))
(do ((i 2 (+ i 1)))
  ((>= i m))
  (let loop ((j i))
    (define k (+ j i))
    (when (< k n)
      (list-set! a k #f)
      (loop k))))

(do ((i 2 (+ i 1)))
  ((= i n))
  (when (list-ref a i)
    (display " " i)))
(display "\n")

