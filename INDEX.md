# Language Index

This is the index of the language-macros.
After the name of each macro I indicate the required arguments.
The may be (q)Name, String, Form or any kind of token, denoted by X.
qName here means a possibly quoted name, while QName means a quoted name.
"\*", "+" and "?" has the common meanings for repetition or optionality.
"-" is used when no argument shall be provided.
The names "else" and "=>" are used as syntactical keywords.
Form:export means a form with the name "export", as used in "scope".
Note that such a form must also begin a file parsed for "import".

| Name | Args |
| --- | --- |
| and | X\* |
| begin | X\* |
| case | X Form+ |
| case | X Form\* else X\* |
| cond | Form+ |
| cond | Form\* else X\* |
| define | Form X |
| define | Name X |
| do | Form Form X\* |
| export | qName\* |
| gensym | - |
| if | X X X? |
| import | String Name? QName? |
| lambda | Form X |
| lambda | Name X |
| let | Form X\* |
| let | Name Form X\* |
| letrec | Form X\* |
| letrecx | Form X\* |
| letx | Form X\* |
| macro | Form X |
| macro | Name X |
| or | X\* |
| quasiquote | X |
| quote | X |
| ref | Name X |
| scope | Form:export X\* |
| seq | X\* |
| unless | X X\* |
| unquote | X |
| when | X X\* |

# Macro Index

Here is the user-macros provided with Humble.
In a Humble program you can create such user-macros as well.
The "define-record-type" and "make-prng" builds ontop
functions that are otherwise not intended for direct use.
In the belo function index, I have therefore taken then out
resulting in the last index; the internal one.

| Name | Args |
| --- | --- |
| case-lambda | Form+ |
| class | Name\* |
| define-record-type | Name Form Name Form\* |
| local | Name\* |
| make-prng | X |
| ref@ | X+ Form |

# Function Index

After the name of each function I indicate the argument order and types.
Cons is used here to mean either a NonList or a List.  Any means any type.
InPort stands for Ext:input-string,input-file,input-pipe or system-input,
and OutPort similarly but for output.
For possibly repeated types, "\*" is used for zero-or-more, while "+"
means one-or-more.  "?" means optional.
"-" is used when no argument shall be provided.

| Name | Args |
| --- | --- |
| \* | Number\* |
| + | Number\* |
| - | Number+ |
| / | Number+ |
| < | Number\* |
| <= | Number\* |
| = | Number\* |
| > | Number\* |
| >= | Number\* |
| abs | Number |
| alias? | Any Any |
| append | Cons\* List |
| apply | Proc List |
| assoc | Any List |
| boolean? | Any |
| car | Cons |
| cdr | Cons |
| clock | - |
| cons | Any Any |
| cont?? | Any |
| current-jiffy | - |
| display | Any\* |
| div | Number+ |
| dup | Any |
| eof-object? | Any |
| eq? | Any Any |
| equal? | Any Any |
| eqv? | Any Any |
| error | Any\* |
| even? | Number |
| exit | Number |
| length | List |
| list | Any\* |
| list? | Any |
| list-copy | List |
| list-ref | List Number |
| list-set! | List Number Any |
| list-\>string | List |
| list-tail | List |
| make-list | Number Any? |
| map | Proc List |
| max | Number+ |
| member | Any List |
| min | Number+ |
| negative? | Number |
| nonlist | Any Any+ |
| not | Any |
| null?  | Any |
| number? | Any |
| number-\>string | Number |
| odd? | Number |
| open-input-file | String |
| open-input-string | String |
| open-input-string-bytes | Number\* |
| open-output-file | String |
| open-output-string | - |
| output-string-get | Ext:output-string |
| output-string-get-bytes | Ext:output-string |
| pair? | Any |
| pause | Number |
| port? | Any |
| positive? | Number |
| procedure? | Any |
| read | String |
| read-byte | InPort |
| read-line | InPort |
| read-to-eof | InPort |
| reverse | List |
| set! | Any Any |
| set!! | Any Any |
| set-car! | Cons Any |
| set-cdr! | Cons Any |
| splice | List |
| string<? | String String |
| string=? | String String |
| string>? | String String |
| string-append | String\* |
| string-length | String |
| string-\>list | String |
| string-\>number | String |
| string-ref | String Number |
| substring | String Number Number? |
| substring-index | String String |
| symbol? | Any |
| symbol-\>string | Name |
| system-command-line | - |
| system-error-file | - |
| system-input-file | - |
| system-output-file | - |
| take | Number Cons |
| void? | Any |
| with-input-pipe | Cons |
| with-output-pipe | Cons |
| write | Any |
| write-byte | Number OutPort |
| write-string | String OutPort |
| zero? | Number |

# Hidden Index

The following are functions that are not meant for
direct use.  They are not in any way hidden,
as could have been this index.

| Name | Args |
| --- | --- |
| make-prng-state | Number |
| make-record | Name Any\* |
| prng-get | Ext:prng-state |
| record? | Any |
| record-get | Record Number |
| record-set! | Record Number Any |
