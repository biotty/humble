#!/usr/bin/env mics.py

(define nop (lambda a (begin)));
(define (stty . args)
  (define@ s t (input-from-pipe
                 (cons "stty" args) nop))
  (when (> s 0) (exit s)))

(stty "raw")
(define in (input-file "/dev/tty"));

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

