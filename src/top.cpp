#include "top.hpp"
#include "xeval.hpp"
#include "compx.hpp"
#include "functions.hpp"
#include <iostream>

using namespace std;

namespace humble {

void top_included(Names & names, Macros & macros)
{
    string s = R"(
(ref (caar x) (car (car x)))
(ref (cadr x) (list-ref x 1))
(ref (cdar x) (cdr (car x)))
(ref (cddr x) (cdr (list-ref x 1)))
(ref (caaar x) (car (car (car x))))
(ref (caadr x) (car (list-ref x 1)))
(ref (cadar x) (list-ref (car x) 1))
(ref (caddr x) (list-ref x 2))
(ref (cdaar x) (cdr (car (car x))))
(ref (cdadr x) (cdr (list-ref x 1)))
(ref (cddar x) (cdr (cdr (car x))))
(ref (cdddr x) (cdr (cdr (cdr x))))  ; left-out: 4th level
(macro case-lambda args
`(lambda =>
   (apply (case (length =>)
       ,@(map (lambda (=>) `((,(length (car =>)))
                               (lambda ,@=>)))
         args)) =>)))
(macro ref@ args
  (ref n (length args))
  (ref n_1 (- n 1))
  (ref syms (take n_1 args))
  (ref s (list-ref args n_1))
  (ref ls (gensym))
  (ref defs (let loop ((i 0) (r '()))
     (if (equal? i n_1) r
       (loop (+ 1 i)
         (cons `(ref ,(list-ref syms i) (list-ref ,ls ,i)) r))
     )))
  `(seq (ref ,ls ,s) ,@defs)
)
(macro define-record-type (name constructor pred . accessors)
  (ref (getter x i)
    `(ref (,(cadr x) v) (record-get v ,i)))
  (ref getter-defs
    (let loop ((d accessors) (r '()) (i 0))
      (if (null? d) r
        (loop (cdr d) (cons (getter (car d) i) r) (+ i 1))
    )))
  (ref (setter x i)
    (if (eq? (length x) 2) #f
      `(ref (,(caddr x) v y) (record-set! v ,i y))))
  (ref setter-defs
    (let loop ((d accessors) (r '()) (i 0))
      (if (null? d) r
        (let ((e (setter (car d) i)))
          (loop (cdr d) (if e (cons e r) r) (+ i 1))
    ))))
  (ref c-args (cdr constructor))
  `(seq
    (ref ,constructor (make-record ',name @`,(list ,@c-args)))
    (ref (,pred v) (record? v ',name))
    ,@getter-defs
    ,@setter-defs
))
(macro class procs
  (ref (gen-case proc)
    `((',proc) ,proc))
  `(lambda (cmd . args)
     (apply
       (case cmd
         ,@(map gen-case procs)
         (else => error))
       args)))
(macro local args
  (cons 'seq (map (lambda (n) `(define ,n ,n)) args)))
)";
    auto & env = GlobalEnv::instance();
    static auto t = compx(s, names, macros, env.keys());
    // ^ keep lex tree for fun-ops refs
    for (auto & a : t.v) run(a, env);
}

GlobalEnv init_top(Macros & macros)
{
    macros_init(macros);
    auto & g = GlobalEnv::instance();
    return g.init();
}

} // ns

