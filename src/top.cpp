#include "top.hpp"
#include "xeval.hpp"
#include "compx.hpp"
#include "functions.hpp"
#include <iostream>

using namespace humble;
using namespace std;

namespace {

void inc_macros(Names & names, Macros & macros)
{
    string s = R"(
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

} // ans

namespace humble {

GlobalEnv init_top(Names & names, Macros & macros, SrcOpener & opener)
{
    init_env(names);
    auto & g = GlobalEnv::instance();
    // TODO:  add extra from arg

    init_macros(macros, names, opener);
    // TODO:  add more macros if desired
    inc_macros(names, macros);
    macros_init(macros);
    return g.init();
}

} // ns

