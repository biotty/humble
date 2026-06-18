(let ((f (open-output-file "foo"))) (write-string "hi there\n" f))
(let ((f (open-input-file "foo"))) (read-line f))

(with-output-pipe
  (lambda ()
    (pipe-system-output
      (lambda ()
        (pipe-system-output
          (lambda ()
            (exec-command "dd" "of=foo" "status=none")))
        (exec-command "wc")))
    (exec-command "tr" "-d" "o"))
  (lambda (p) (write-string "foo bar\n" p)))

(with-input-pipe
  (lambda ()
    (pipe-system-input
      (lambda ()
        (exec-command "cat" "foo")))
    (exec-command "tail" "-n" "1"))
  (lambda (p) (display (read-line p))))

(with-output-pipe (lambda () (exec-command "wc")) (lambda (p) (write-string "foo bar\n" p)))
(with-input-pipe (lambda () (exec-command "rm" "-v" "foo")) (lambda (p) (display (read-line p))))

