(let ((f (output-file "foo"))) (write-string "hi there\n" f))
(let ((f (input-file "foo"))) (read-line f))
(output-command '("wc") (lambda (p) (write-string "foo bar\n" p)))
(input-command '("ls") (lambda (p) (display (read-line p))))
