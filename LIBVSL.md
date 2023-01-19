# LIBVSL / PREVSL

This documents the primitives built-in for the PREVSL implementation of LIBVSL
as well as the applications of LIBVSL frontend implementations. PREVSL is the
program you'll need to use to generate the grammar and symbol table for your
LISP.

## PREVSL primitives

In the lists below, any `function:` is a symbol which when put as the first
element of a list, will have a grammar form for its sibblings elements,
returning a value. Any `symbol:` is a LISP symbol or a LISP primitive.

PREVSL implements the following primitives:

### Generic

Those symbols are identical for all implementation of LIBVSL.

- function: `set`
  - form: `set ::= <SYM*> <SEXP> -> <SYM*>`
  - sets `<SYM>` to the value of `<SEXP>`

- function: `func`
  - form: `func ::= <SYM*> <SYM-LIST> <EXP-LIST> -> <SYM*>`
    + where:
      + `SYM-LIST ::= <SYM> <SYM-LIST> | <EMPTY-LIST>`
      + `EXP-LIST ::=  <EXP> <EXP-LIST> | nil`
  - sets `<SYM>` to be the head of the tree of `<EXP-LIST>`, substituting the
    symbols in `<EXP-LIST>` by the values positionally matched by the symbols in
    `<SYM-LIST>`

- function: `eval`
  - form: `eval ::= <SEXP> -> ??`
  - evals the first argument.

- function: `quot`
  - form: `quot ::= <SEXP> -> <SEXP>`
  - "quotes" the first argument. Here, a quote just means take the SEXP tree and
    store it as data, instead of evaluating, e.g.:
    ```lisp
    (+ 1 2) -> 3
    (quot (+ 1 2)) -> (+ 1 2)
    ```
- function: `if`
  - form: `if ::= <SEXPC> <SEXPT> <SEXPF> -> nil`
  - evals `<SEXPC>`, if non-nil, then eval `<SEXPT>` then exit, if false, then eval
    `<SEXPF>` then exit. `<SEXPF>` is optional.

- function: `eq`
  - form `eq ::= <SEXP1> <SEXP2> -> ( t | nil )`
  - returns `t` if `<SEXP1>` is equal to `<SEXP2>`. As this is a strictly
    symbolic LISP (i.e. no literals), equality means two SEXPs have the same
    resulting tree. e.g:
    ```lisp
    (set thing (quot (a-tree)))

    (eq thing (quot (a-tree))) -> t
    (eq thing (quot a-tree)) -> nil
    ```

- function: `not`
  - form: `not ::= <SEXP> -> (t | nil)`
  - negates the sexp, reducing it to a boolean. e.g:
  ```lisp
  (not t) -> nil
  (not (quot (some-tree))) -> nil
  (not nil) -> t
  ```

- function: `block`
  - form: `block ::= <SEXP> block | nil`
  - evals as many `<SEXP>`s in it as it can. e.g:
  ```lisp
  (if (eq answer-to-everything fourty-two)
   (block
    (do-something)
    (do-something-else)))
  ```

- function: `while`
  - form: `while ::= <SEXPC> <EXP-LIST> -> nil`
    + where:
      + `EXP-LIST ::= <SEXP> <EXP-LIST> | nil`
  - keeps evaluating from the top of the tree of `<EXP-LIST>` until `<SEXPC>` is
    false

- function: `break`
  - form: `break ::= nil -> nil`
  - breaks out of the closest `while` scope. e.g:

- function: `continue`
  - form: `continue ::= nil -> nil`
  - put the parsing head back to the first `<EXP>` below the closest `while`
    scope. e.g:

- function: `return`
  - form: `return ::= <SEXP> | nil -> nil`
  - assigns the return value for a function defined with `func`

- function: `goto`/`label`
  - form(s):
    + goto: `goto ::= <SYM> -> nil`
      + go to a label
    + label: `label ::= <SYM> -> nil`
      + marks an sexp as a label, e.g.:
      ```lisp
      (label do-thing)
      (thing)
      (goto do-thing)

      ; equivalent to
      (while t
        (thing))
      ```
- function: `cond`
  - form: `cond ::= <SEXP> cond | nil -> <SEXP>`
  - evaluates the first element of its SEXPs children, if true, evaluates the
    the second element, if false, go to the next, e.g.:
  ```lisp
  (set cond-1 nil)
  (set cond-2 t)
  (set cond-3 nil)
  (cond
   (cond-1 (do-thing1))
   (cond-2 (do-thing2))
   (cond-3 (do-thing3))) ; evals (do-thing2) only
  ```

- function: `head`
  - form: `head ::= <SEXP> | nil -> <SYM> | nil`
  - gets the first element of `<SEXP>`

- function: `behead`
  - form: `behead ::= <SEXP> | nil -> <SYM> | nil`
  - gets all but the first element of `<SEXP>`

- function: `list`
  - form: `list ::= <SYM> list | nil -> <SEXP>`
  - forms an SEXP out of the elements

- symbol: `t`
  - primitive true boolean

- symbol: `nil`
  - primitive false boolean

### Grammar-defining

Those symbols are specific for defining a grammar with LIBVSL.

- symbol: `start`
  - externed as nil by default. This is a grammar for the first SEXP of a file
    parsed by the language being defined

- symbol: `end`
  - externed as nil by default. This is a grammar for the last SEXP of a file
    parsed by the language being defined

## Implementation syntax

To create a LISP using PREVSL, you'll need *at least one* C source file.

Then include the statement:

```c
LIBVSL_FRONTEND(<your-pvsl-frontend>);
```

The `frontend` function will execute in `main` before `parse_bytstream` is
called. If no such function exist, include the statement:

```c
LIBVSL_FRONTEND_STUB();
```

### Header implementation

You don't need to explictly include the `libvsl.h` file in your source
implementation, as PREVSL will do that for you.

### PREVSL implementation

You'll have to declare the name of the source file as the first symbolic
argument to the `source` function. Even though VSL has no string literals,
variable names are pretty laxed. As long as your source doesn't have whitespace
or non-ASCII characters in it, it should be fine. Fore xample:

```lisp
(source my-vsl-implementation.c)

(code)
(more-code)
...
```

## LIBVSL limitations

By design, any frontend implementation of LIBVSL will have some limitations.

### Static literal range

The range of elements of a function that are to be automatically literals (i.e.
lists that won't resolute) is static.

For example, the function `func` has LR set to 3, because:

```
(func <sym> <sym-list> <code...>)
 1    2     3          4
```

The third element, if being a list, is to be taken literally.

### Static argument range

The range of elements of a function is static.
