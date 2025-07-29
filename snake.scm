#!/usr/bin/env mics.py

(macro inc! (v) `(set! ,v (+ ,v 1)))
(macro dec! (v) `(set! ,v (- ,v 1)))

(define sq (list->string '(0x2588 0x2588)))
(define keys (alist->dict
               '((#\a . up)
                 (#\z . down)
                 (#\o . left)
                 (#\p . right))))

(define (make-game row col)
    (define dir 'right)
    (define (get-pos) (list row col))
    (define (set-dir! v) (set! dir v))
    (define (step!)
      (case dir
        (('up) (dec! row))
        (('down) (inc! row))
        (('left) (dec! col))
        (('right) (inc! col))
        (else => error)))
    (class get-pos set-dir! step!))

(define game (make-game 8 8))

(define scr (nc-initscr))
(let loop ((t (current-jiffy)))
    (define ch (nc-getch scr))
    (if (eq? ch #\q)
      (begin
        (nc-endwin)
        (exit 0))
      (begin
        (dict-if-get keys ch 'ign
                     (lambda (d)
                       (game 'set-dir! d)))
        (game 'step!)
        (define@ row col (game 'get-pos))
        (nc-addstr scr row (* col 2) sq)
        (define nt (current-jiffy))
        (pause (- 100 (- nt t)))
        (loop nt))))

