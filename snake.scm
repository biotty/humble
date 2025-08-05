#!/usr/bin/env mics.py

(macro inc! (v) `(set! ,v (+ ,v 1)))
(macro dec! (v) `(set! ,v (- ,v 1)))

; ideas: * purple frog gives hazard direction after 3 turns
;        * speedy toggle by space
;        * show points and have ~/.games/snake.hiscore
; bugs: * using trace color for erase, notice that first
;         step has butt more than one initial head.  why.
(define JPFR 100)
(define JGOVER 800)
(define GROWPF 6)
(define SP "  ")
(define SQ (list->string '(0x2588 0x2588)))
(define scr (nc-initscr))
(define@ grid-height grid-width
         (let ((yx (nc-getmaxyx scr)))
           (list (- (list-ref yx 0) 1)
                 (/ (- (list-ref yx 1) 2) 2))))
(define (grid-set-str y x s c) (nc-addstr scr y (* x 2) s c))
(define (grid-unset y x) (grid-set-str y x SP 7))
(define (grid-sq y x c) (grid-set-str y x SQ c))
(define (grid-wall . yx) (grid-sq @yx 2))
(define (grid-snake . yx) (grid-sq @yx 3))
(define (grid-frog . yx) (grid-sq @yx 6))
(define (grid-bad . yx) (grid-sq @yx 1))
(let loop ((x (- grid-width 1)))
  (when (>= x 1)
    (grid-wall 0 x) (grid-wall grid-height x) (loop (- x 1))))
(let loop ((y (- grid-height 1)))
  (when (>= y 1)
    (grid-wall y 0) (grid-wall y grid-width) (loop (- y 1))))
(define (rnd-frog body)
  (let loop ()
    (let ((y (random 1 (- grid-height 1)))
          (x (random 1 (- grid-width 1))))
      (if (member (nonlist y x) body)
        (loop)  ; yes we get tco here
        (begin
          (define r (list y x))
          (apply grid-frog r)
          r)))))

(define (make-game maxy maxx)
  (let ((y (random 1 (- maxy 1)))
        (x (random 1 (/ maxx 2))))
    (define body-length 5)
    (define butt (list (+ y) (+ x)))
    (define (butt-dup n)
      (let loop ((b '()) (i n))
        (if (zero? i) b
          (loop (cons (nonlist (car butt) (cadr butt))
                      b) (- i 1)))))
    (define body (butt-dup body-length))
    (define (get-butt) butt)
    (define dir 'right)
    (inc! x)
    (define (get-head) (list y x))
    (define frog (rnd-frog '()))
    (define (set-dir! v) (set! dir v))
    (define (step!)
      (set! body (cons (nonlist (+ y) (+ x)) body))
      (define d (list-tail body (- body-length 1)))
      (set! butt (list (caar d) (cdar d)))
      (set-cdr! d '())
      (case dir
        (('up) (dec! y))
        (('down) (inc! y))
        (('left) (dec! x))
        (('right) (inc! x))
        (else => error))
      (when (equal? (list y x) frog)
        (set! frog (rnd-frog body))
        (set! body (append body (butt-dup GROWPF)))
        (set! body-length (+ GROWPF body-length))))
    (define (get-frog) frog)
    (define (is-over)
      (or (>= y maxy) (<= y 0)
            (>= x maxx) (<= x 0) (member (nonlist y x) body)))
    (class
      get-head
      get-butt
      set-dir!
      step!
      is-over
      get-frog)))

(define keys (alist->dict
               '((#\a . up)
                 (#\z . down)
                 (#\o . left)
                 (#\p . right))))

(define (quit) (nc-endwin) (exit 0))
(define game (make-game grid-height grid-width))
(let loop ((t (current-jiffy)))
    (define ch (nc-getch scr))
    (cond ((eq? ch #\q)
           (quit))
          ((game 'is-over)
           (apply grid-bad (game 'get-head))
           (nc-getch scr)
           (pause JGOVER)
           (quit))
          (else
            (dict-if-get keys ch 'ign
                         (lambda (d)
                           (game 'set-dir! d)))
            (game 'step!)
            (apply grid-snake (game 'get-head))
            (apply grid-unset (game 'get-butt))
            (define nt (current-jiffy))
            (pause (- JPFR (- nt t)))
            (loop (+ t JPFR)))))

