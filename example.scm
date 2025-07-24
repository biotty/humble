#!/usr/bin/env mics.py

(define nop (lambda a (begin)));
(define (stty . args)
  (define@ s t (input-from-pipe
                 (cons "stty" args) nop))
  (when (> s 0) (exit s)))

(stty "raw")
(define in (input-file "/dev/tty"));

; surprised i don't have number->string,
; well let's implement it in my scheme
(define (number->string i)
  (let loop ((r '()) (k i))
    (if (eq? k 0) (list->string r)
      (let ((d (div k 10)))
        (loop (cons (+ #\0 (cdr d)) r)
              (car d))))))

(define (pos row col)
  (let ((s (out-string)))
    (write-string "\033[" s)
    (write-string (number->string row) s)
    (write-string ";" s)
    (write-string (number->string col) s)
    (write-string "H" s)
  (out-string-get s)))

(define clr "\033[2J")
(display clr)
(display (pos 12 34))
(display "hello\n")
; in need of interval-clock
(read-byte in)
(stty "-raw")
(newline)

