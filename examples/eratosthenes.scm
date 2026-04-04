#! /usr/bin/env humble

(define args (system-command-line))
(when (> 2 (length args))
  (display "usage: N\n-- "
           "will output primes below N\n")
  (exit 1))

(define n (string->number (list-ref args 1)))
(when (> 3 n)
  (exit 0))

(define m (/ n 2))
(define odd-primes (make-list m #t))
(define (p-ref t) (list-ref odd-primes (/ t 2)))
(define (p-unset! t) (list-set! odd-primes (/ t 2) #f))
(do ((i 3 (+ i 2)))
  ((>= i m))
  (when (p-ref i)
    (let loop ((j i))
      (define k (+ j i i))
      (when (< k n)
        (p-unset! k)
        (loop k)))))

(display "2")
(do ((i 3 (+ i 2)))
  ((>= i n))
  (when (p-ref i)
    (display " " i)))
(display "\n")

