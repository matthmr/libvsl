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

<!-- TODO: does `set' copy of reference memory? -->

Those symbols are identical for all implementation of LIBVSL.

- function: `set`
  - form: `(set <NAME> <EXPR>)`
  - sets `<NAME>` to the value of `<EXPR>`. `<NAME>` is a literal symbol.
  - example:
    ```lisp
    (set true t)
    (set false nil)
    (set booleans (quot (true false)))
    ```
- function: `del`
  - form: `(del <NAME>)`
  - unbounds `<NAME>`. Unbounding means the symbol that had the hash of `<NAME>`
  is taken off the symbol table. `<NAME>` is a symbol.
  - example:
    ```lisp
    (set x nil)
    (del x)

    (fun something ())
    (del something)
    ```

- function: `fun`
  - form: `(fun <NAME> (<ARGS>) <BODY>...)`
  - sets `<NAME>` to be the head of the tree of `<BODY>`, substituting the
    symbols by the values positionally matched by the symbols in `<ARGS>`.
    `<NAME>` is a literal symbol, `<ARGS>` is a list of symbols and `<BODY>` is
    multiple expressions.
  - example:
    ```lisp
    (fun boolean? (x) (return (if (eq x t) t (eq x nil))))
    (fun is-empty-sexp-nil? (expr) (return (eq () nil)))
    (fun type-of-head (e) (return (type (head e))))
    ```

- function: `lam`
  - form: `(lam (<ARGS>) <EXPR>)`
  - returns a lambda function that returns `<EXPR>`. `<ARGS>` is a list of
    symbols.
  - example:
    ```lisp
    (filter list (lam (e) (is-even e)))
    (set append-prepend-nil (lam (e) (quot nil e nil)))
    (set append-prepend-t (lam (e) (quot t e t)))
    ```

- function: `eval`
  - form: `(eval <EXPR>)`
  - evals `<EXPR>`. If the first element of `<EXPR>` is `()`, then eval will
    recurse, creating a new tree with elements of the same recusrion level as
    sibblings in the tree (similar to ``(a ,b)` in *elisp*). `<EXPR>` is quoted.
  - example:
    ```lisp
    (eval (quot nil)) -> nil
    (eval nil) -> nil
    (eval (() (quot a) nil)) -> (a nil)
    ```

- function: `quot`
  - form: `(quot <EXPR>)`
  - "quotes" `<EXPR>`. Here, a quote just means take the SEXP tree and store it
    as data, instead of evaluating.
  - example:
    ```lisp
    (quot x) -> x
    (quot (quot x)) -> (quot x)
    (quot ()) -> ()
    ```

- function: `lexp`
  - form: `(lexp <TREE>)`
  - makes `<EXPR>` be an LEXP, in-place.
  - example:
    ```lisp
    (set tree (quot (a b))) -> (a b)

    (fun list-append (tree elem)
       (set old-tree-right (right-child tree))
       (lexp old-tree-right)
       (set-left-child (right-child tree) (left-child old-tree-right))
       (set-right-child (right-child tree) (quot elem))
       (return tree))

    (list-append tree c) -> (a b c)
    (list-append (list-append tree c) d) -> (a b c d)
    ```

- function: `if`
  - form: `(if <COND> <TRUE-EXPR> <FALSE-EXPR>?)`
  - evals `<COND>`, if non-nil, then evals `<TRUE-EXPR>` then exit. If nil, then
    evals `<FALSE-EXPR>` then exit. `<FALSE-EXPR>` is optional. If nil and no
    `<FALSE-EXPR>` exists, then returns `nil`.
  - example:
    ```lisp
    (if (not (eq nil ())) @sym) -> ni
    (if (eq nil (head (quot (nil)))) (list nil) nil) -> (nil)
    (if (quot t) nil t) -> nil
    ```

- function: `eq`
  - form `(eq <EXPR1> <EXPR2>)`
  - returns `t` if `<EXPR1>` is "equal" to `<EXPR2>`. Equality here depends on
    the type of the return value of the expressions. As LIBVSL, the base for
    PREVSL, is a strictly symbolic LISP, equality either means:
    1. the symbols are the same; have the same hash; point to the same address
    2. the SEXP trees are the same; they have the same size and each element of
       it respects `1.` `eq` will return `nil` if the expressions have different
       types.
  - example:
    ```lisp
    (eq () nil) -> t
    (eq (quot ()) nil) -> nil ; different types: (eq SEXP SYM)
    (eq eq eq) -> t ; same symbol
    ```

- function: `not`
  - form: `(not <EXPR>)`
  - negates the boolean value of `<EXPR>`. The boolean value of `<EXPR>` will
    follow that anything that's not `nil` (or bound to `nil`) is `t`.
  - example:
    ```lisp
    (not (quot nil)) -> nil
    (not (quot (some-tree))) -> nil
    (not nil) -> t
    ```

- function: `block`
  - form: `(block <EXPR>...)`
  - evals as many `<EXPR>`s in it as it can. This is used to "bypass" the
    one-expr limit of functions like `if`.
  - example:
    ```lisp
    (if (eq @sexp (type ()))
     (block
       (do-something)
       (do-something-else)))
    ```

- function: `while`
  - form: `(while <COND> <EXPR>...)`
  - evaluates all expressions in `<EXPR>` while `<COND>` is boolean `t`.
  - example:
    ```lisp
    (while t (do-something-forever))
    (while (not thing) (do-thing-for thing) (do-another-thing-for thing))
    (while nil (never-execute))
    ```

- function: `break`
  - form: `(break)`
  - breaks out of the closest `while` scope.
  - example:
    ```lisp
       (while t (break))
       (while (some-function)
         (if some-condition
           (break)))
       (while t
         (if (not (head some-list))
             (break)))
    ```

- function: `continue`
  - form: `(continue)`
  - goes to the next iteration in a `while` block.
  - example:
    ```lisp
    (while t (continue))
    (while some-condition
      (if (some-other-condition)
        (continue)))
    (while t
      (if (not (head some-list))
        (continue)
        (break)))
    ```

- function: `return`
  - form: `(return <EXPR>)`
  - returns `<EXPR>` as the return value of a function.
  - example:
    ```lisp
    (fun foo ()
      (return (quot bar)))
    (fun hello ()
      (return (quot world)))
    (fun sym-eq (a b)
      (return (eq
               (if (eq (type a) @sexp) nil a)
               (if (eq (type b) @sexp) nil b))))
    ```

- function: `goto`/`label`
  - form:
    + `(goto <LABEL>)`
      + goes to `<LABEL>`. If it doesn't exist, then do nothing. `<LABEL>`
        *must* be declared **before** the `goto` function. Other implementations
        of LIBVSL, such as bootstrapped ones, may not enforce this. However,
        'raw' LIBVSL does.
    + `(label <NAME>)`
      + sets `<NAME>` as a label.
  - example
    ```lisp
    (while (foo)
      (do-something-before-bar)
      (label bar)
      (do-something)
      (if (some-condition)
          (goto bar)
          (if (some-breaking-condition) (break))))
    ```
- function: `cond`
  - form: `(cond (<COND> <EXPR>)...)`
  - evaluates the expressions of the form `(<COND> <EXPR>)` where `<EXPR>` is a
    single expression. If the return value of `<COND>` is boolean `t`, then
    execute `<EXPR>` and exit the `cond` function.
  - example:
    ```lisp
      (cond
        ((eq @sym ()) (quot (foo bar)))
        ((foo) bar)
    ```

- function: `left-child`/`right-child`
  - form: `(left-child <EXPR>)`/`(right-child <EXPR>)`
  - gets the left/right child of `<EXPR>` as a reference. Returns nil if it
    doesn't exist.
  - example:
    ```lisp
      (left-child (quot (a b c))) -> a
      (right-child (quot (a b c))) -> b

      (left-child (quot ((a) ((b) c) d))) -> (a)
      (right-child (quot ((a) ((b) c) d))) -> ((b) c)

      (left-child nil) -> nil
      (right-cihld nil) -> nil
    ```

- function: `parent`
  - form: `(parent <EXPR>)`
  - gets the paren of `<EXPR>` as a reference. Returns nil if it doesn't exist.
  - example:
    ```lisp
      (set tree (quot (a b c)))
      (set tree (right-child tree)) -> b

      (paren tree) -> a
    ```

- function: `set-left-child`/`set-right-child`
  - form: `(set-left-child <TREE> <EXPR>)`/`(set-right-child <TREE> <EXPR>)`
  - sets the left/right child of `<TREE>` as `<EXPR>`. Will silently fail if
    `<TREE>` is not a SEXP tree.
  - example:
    ```lisp
    (set tree (quot ())) -> ()
    (set-left-child tree (quot a)) -> (a)
    (set-right-child tree (quot (b))) -> (a (b))
    (set-left-child tree nil) -> tree -> (nil (b))
    ```

- function: `set-parent`
  - form: `(set-parent <TREE> <EXPR>)`
  - sets the parent of `<TREE>` as `<EXPR>`. Will silently fail if `<TREE>` is
    not a SEXP tree.
  - example:
    ```lisp
    (set tree (quot (b))) -> (b)

    (set tree2 (set-parent tree a)) -> (a b)

    (set-parent tree2 c) -> (c (a b))
    ```

- function: `type`
  - form: `(type <EXPR>)`
  - returns the type of the expression `<EXPR>` as one of these types:
    1. `@sym`
    2. `@sexp`
    3. `@lexp`
  - example:
    ```lisp
    (type ()) -> @sym
    (type nil) -> @sym
    (type (quot ())) -> @sexp
    ```

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

For example, the function `fun` has LR set to 3, because:

```
(fun <sym> <sym-list> <code...>)
 1    2     3          4
```

The third element, if being a list, is to be taken literally.

### Static argument range

The range of elements of a function is static.
