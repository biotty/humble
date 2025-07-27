#!/usr/bin/env mics.py

(macro inc! (v d) `(set! ,v (+ ,v ,d)))
(macro dec! (v d) `(set! ,v (- ,v ,d)))

(define row 8)
(define col 8)
(define dir 'right)

(define scr (nc-initscr))
(let loop ((t (current-jiffy)))
    (define ch (nc-getch scr))
    (if (eq? ch #\q)
      'bye
      (begin
        (case ch
          ((#\a) (set! dir 'up))
          ((#\z) (set! dir 'down))
          ((#\o) (set! dir 'left))
          ((#\p) (set! dir 'right))
          (else 'nop))
        (case dir
          (('up) (dec! row 1))
          (('down) (inc! row 1))
          (('left) (dec! col 2))
          (('right) (inc! col 2))
          (else 'nop))
        (nc-addstr scr row col
                   (list->string '(0x2588 0x2588)))
        (define nt (current-jiffy))
        (pause (- 100 (- nt t)))
        (loop nt))))

(nc-endwin)

