(let ((f (open-output-file "foo"))) (write-string "hi there\n" f))
(let ((f (open-input-file "foo"))) (read-line f))
(with-output-pipe (lambda () (exec-command "wc")) (lambda (p) (write-string "foo bar\n" p)))
(with-input-pipe (lambda () (exec-command "rm" "-v" "foo")) (lambda (p) (display (read-line p))))
