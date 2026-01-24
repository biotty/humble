(let ((f (open-output-file "foo"))) (write-string "hi there\n" f))
(let ((f (open-input-file "foo"))) (read-line f))
(open-output-command '("wc") (lambda (p) (write-string "foo bar\n" p)))
(open-input-command '("rm" "-v" "foo") (lambda (p) (display (read-line p))))
