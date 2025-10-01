#!/usr/bin/env mics.py

; humble scheme demonstration, where ncurses functions
; (nc-) has been brought in (example of extension
; mechanism onto interpreter) so to provide a
; classic snake game at the terminal.  we here exemplify
; pass-by-reference, the lone "@", a class, use of
; a dictionary, and the "ref@", the "class" macro,
; alternative parens, unicode, and the many r7rs forms.
; demo of io: the high-score is recorded in a file.

(ref (inc! v) (setv! v (+ v 1)))
(ref (dec! v) (setv! v (- v 1)))
; demonstrates that we pass by reference

(ref random (prng (clock)))
; initialize a pseudo random sequence

(ref JPFR 100)
(ref JGOVER 800)
(ref GROWPF 6)  ; <-- snake growth per frog
(ref SP "  ")
(ref SQ (list->string '(0x2588 0x2588)))
; will operate on grid of double characters, so the
; filled "square" are defined by this unicode string
; and we may erase with two spaces.

(ref scr (nc-initscr 1))
; halfdelay takes tenths, and jiffies-per-second is
; fixed to thousand by humble scheme, so we give
; JPFR (jiffies-per-frame) 100x the nc-initscr arg.

(ref@ grid-height grid-width
         (let ((yx (nc-getmaxyx scr)))
           (list (- (list-ref yx 0) 1)
                 (/ (- (list-ref yx 1) 2) 2))))
(ref (grid-set-str y x s c) (nc-addstr scr y (* x 2) s c))
(ref (grid-unset y x) (grid-set-str y x SP 7))
(ref (grid-sq y x c) (grid-set-str y x SQ c))
(ref (grid-wall . yx) (grid-sq @yx 2))
(ref (grid-snake . yx) (grid-sq @yx 3))
(ref (grid-frog . yx) (grid-sq @yx 4))
(ref (grid-bad . yx) (grid-sq @yx 1))

; frame the screen except for the very corners
(let loop ((x (- grid-width 1)))
  (when (>= x 1)
    (grid-wall 0 x) (grid-wall grid-height x) (loop (- x 1))))
(let loop ((y (- grid-height 1)))
  (when (>= y 1)
    (grid-wall y 0) (grid-wall y grid-width) (loop (- y 1))))

; arbitrarily choose non-occupied place for frog
(ref (rnd-frog body)
  (let loop ()
    (let ((y (random 1 (- grid-height 1)))
          (x (random 1 (- grid-width 1))))
      (if (member (nonlist y x) body)
        (loop)  ; yes we get tco here
        (begin
          (ref r (list y x))
          (apply grid-frog r)
          r)))))

; game-state, unaware of two-char cells for display
(ref (make-game maxy maxx)
  (let ((y (random 1 (- maxy 1)))
        (x (random 1 (/ maxx 2))))
    (ref INIT-LENGTH 15)
    (def body-length INIT-LENGTH)
    (ref butt (list (+ y) (+ x)))
    (ref (butt-dup n)
      (let loop ((b '()) (i n))
        (if (zero? i) b
          (loop (cons (nonlist (car butt) (cadr butt))
                      b) (- i 1)))))
    (ref body (butt-dup body-length))
    (ref (get-butt) butt)
    (ref dir 'right)
    (inc! x)
    (ref (get-head) (list y x))
    (ref frog (rnd-frog '()))
    (ref (set-dir! v) (setv! dir v))
    (ref (step! score-fun)
      (setv! body (cons (nonlist (+ y) (+ x)) body))
      (ref d (list-tail body (- body-length 1)))
      (setv! butt (list (caar d) (cdar d)))
      (set-cdr! d '())
      (case dir
        (('up) (dec! y))
        (('down) (inc! y))
        (('left) (dec! x))
        (('right) (inc! x))
        (else => error))
      (when (equal? (list y x) frog)
        (setv! frog (rnd-frog body))
        (setv! body (append body (butt-dup GROWPF)))
        (setv! body-length (+ GROWPF body-length))
        (score-fun (/ (- body-length INIT-LENGTH) GROWPF))))
    (ref (get-frog) frog)
    (ref (is-over)
      (or (>= y maxy) (<= y 0)
            (>= x maxx) (<= x 0) (member (nonlist y x) body)))
    (class
      get-head
      get-butt
      set-dir!
      step!
      is-over
      get-frog)))

(ref keys (alist->dict
               '((#\a . up)
                 (#\z . down)
                 (#\o . left)
                 (#\p . right))))

(ref (show-score x n)
     (nc-addstr scr 0 x (number->string n) 2))

(ref score-path ".snake_score")
(ref hiscore (case (input-file score-path)
               ((#f) 0)
               (else => (lambda (f)
                 (ref s (read-line f))
                 (if (eof-object? s) 0
                   (string->number s))))))

(ref score 0)
(ref has-record #f)
(ref (up-score n)
     (setv! score n)
     (show-score 8 n)
     (when (> score hiscore)
       (setv! hiscore score)
       (setv! has-record #t)
       (show-score 14 score)))
(show-score 14 hiscore)
(up-score 0)

(ref (quit) (nc-endwin)
     (exit (if has-record
       {case (output-file score-path)
         ((#f) 1)
         (else => [lambda (f)
           (write-string (number->string score) f)
           0])} 0)))

(ref game (make-game grid-height grid-width))
(grid-snake @(game 'get-head))
(let loop ((t (current-jiffy)))
    (ref ch (nc-getch scr))
    (cond [(eq? ch #\q)
           (quit)]
          [(game 'is-over)
           (apply grid-bad (game 'get-head))
           (nc-getch scr)
           (pause JGOVER)
           (quit)]
          [else
            (dict-if-get keys ch 'ign
                         (lambda (d)
                           (game 'set-dir! d)))
            (game 'step! up-score)
            (apply grid-snake (game 'get-head))
            (apply grid-unset (game 'get-butt))
            (ref nt (current-jiffy))
            (pause (- JPFR (- nt t)))
            (loop (+ t JPFR))]))

