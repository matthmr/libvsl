# LIBVSL

This documents the primitives built-in for the turing complete implementation of
LIBVSL as well as the applications of LIBVSL frontend implementations.

## LIBVSL primitives

This repository includes a built-in, optional frontend called PRIMVSL (primitive
VSL). PRIMVSL is a frontend implementation of LIBVSL that defines functions and
symbols to be used by default. PRIMVSL can be excluded from LIBVSL if you want
to build your own implementation. See the '_Implementating LIBVSL_' chapter for
more info.

<!-- TODO: does `set' *copy* or *reference* memory? -->

### PRIMVSL functions

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
### PRIMVSL symbols

- symbol: `t`
  - primitive true boolean

- symbol: `nil`
  - primitive false boolean

## Implementating LIBVSL

To create a LISP using LIBVSL, you'll need *at least one* C source file,
including the `libvsl.h` header with either of the macros `LIBVSL_FRONTEND` or
`LIBVSL_FRONTEND_STUB` used at least once, and link it to `libvsl.a` with the
`-lvsl` or equivalent compiler option set on your compiler of choice.

If using a stub implementation of LIBVSL, you can use the
`LIBVSL_FRONTEND_STUB` macro with no arguments, then compile it, as the `main`
function is located in `libvsl.c`. You don't define the `main` entry point, just
the `frontend` function that gets run in `main` before passing control to the
lexer.

### Stub implementation

The stub implementation of LIBVSL is turing complete, so *in theory* you could
just create a stub file and use it as a LISP. However it's too cumbersome and
hard to use, as the only lexical elements defined in the lexer are '`(`' and
'`)`' as expression delimiters, with no useful constructs like '`'`' for
quoting. Nevertheless, you can pull the `dev` branch and run the following
command:

```shell
$ git show dev:dev/sh/dev-on-branch.sh | sh -
```

This should create a `dev/` directory and inside of it, a `dev/sh/gen-lisp.sh`
shell script. After configuring with `./configure`, you can run this script and
it will use a file in `dev/c/lisp.c` as a LIBVSL stub to generate an executable,
`./lisp`.

### Frontend implementation

However, if you want to build your own implementation either on top of PRIMVSL
or substituting PRIMVSL, along with the instructions above, you'll have to:

1. define one or more arrays of type `struct clisp_sym` using **only one** of
   this macros:
   1. `CLISP_PRIM_DECLFUN(name, function, lower_arg, upper_arg, lower_lit,
      upper_lit)`: declare a LISP function
     - `name`: name of the function as used in LISP code
     - `function`: name of the function as used in C code
     - `lower_arg`: lower bound of arguments for this function
     - `upper_arg`: upper bound of arguments for this function
     - `lower_lit`: lower bound of literal arguments for this function
     - `upper_lit`: upper bound of literal arguments for this function
     - end the definition with `CLISP_PRIM_DECLFUN_END()`
   2. `CLISP_PRIM_DECLSYM(name, value)`: declare a LISP symbol
     - `name`: name of the function as used in LISP code
     - `value`: generic `void` pointer to the data of the symbol
     - end the definition with `CLISP_PRIM_DECLSYM_END()`
2. define your functions declared to be used in the `function` argument on
   `CLISP_PRIM_DECLFUN` with the macro `CLISP_PRIM_DEFUN`. These functions are
   of the type `struct lisp_ret (*) (struct mm_if argp, uint argv)`.
3. define an *array iterator* with the type `struct clisp_tab` containing all
   your arrays defined prior, with `CLISP_PRIM_DECLTAB_END` as the terminating
   element.
   - each array element should be wrapped arround the `CLISP_PRIM_DECLTAB`
     macro, with the name of the element as the first argument, and the type of
     the each element of the element array as the second argument.
4. call the function `lisp_prim_setlocal` with a pointer to your array iterator
   as the first argument, and a boolean as the second argument
   - this boolean will decide if PRIMVSL should be included as well. Set it to
     true if you want to implement your functions on top of PRIMVSL. Set if to
     false if you want that your functions replace PRIMVSL.
5. wrap the call to `lisp_prim_setlocal`, along with whatever else you want to
   initialize, into a function with the type `int (*) (void)`, then pass the
   name of this function to the macro `LIBVSL_FRONTEND`. Use this `int` return
   value to signal that something inside your function has errored; as a error
   code. It should be set to zero if everything has succeeded, non-zero
   otherwise.

**NOTE**: each array can only contain **one** type of these macros. So you can
only have an array full of functions, or full of symbols, without mixing the
two.

#### LIBVSL functions

All of LIBVSL's functions are of the type `struct lisp_ret (*) (struct lisp_arg*
argp, uint argv)`. Functions are declared and defined with the
`CLISP_PRIM_DEFUN` CPP macro. Like `libc`'s `main`, the name of the function is
passed as the first argument of the function, `argp[0]`. If you want to access
its arguments, they start at `argp[1]`.

<!-- TODO: lazy evaluation -->

A standard LIBVSL function might look like:

```c
CLISP_PRIM_DEFUN(foo) {
    struct lisp_ret ret = {0};

    for (uint i = 1; i < argv; ++i) {
       // do something
    }

    return ret;
}
```

#### Implementation example

Here's an implementation that defines a LISP with a single function called
`(hello-world)` that just prints "hello world".

Source code (`myvsl.c`):

```c
#include "libvsl.h"

#include <stdio.h>

CLISP_PRIM_DEFUN(myvsl_hello_world) {
    printf("hello world");
    return (struct lisp_ret) {0};
}

static struct clisp_sym myvsl_prim_funtab[] = {
    CLISP_PRIM_DECLFUN("hello-world", myvsl_hello_world, /* (hello-world) */
                       0, 0, 0, 0),
    CLISP_PRIM_DECLFUN_END(),
};

static struct clisp_tab iterarray[] = {
    CLISP_PRIM_DECLTAB(myvsl_prim_funtab, __LISP_TYP_FUN),

    CLISP_PRIM_DECLTAB_END(),
};

int myvsl_frontend(void) {
    lisp_prim_setlocal(iterarray, false);
    return 0;
}

LIBVSL_FRONTEND(myvsl_frontend);
```

Building:

```sh
$ make libvsl.a
$ clang -I. -L. -lvsl myvsl.c -o myvsl
$ ./myvsl
(hello-world)
hello world
(something-thats-not-defined)
[ !! ] libvsl: symtab: symbol was not found
$
```

## LIBVSL limitations

By design, any frontend implementation of LIBVSL will have some limitations.

### Static literal range

The range of elements of a function that are to be automatically literals (i.e.
lists that won't resolute) is static.

For example, the function `fun` has LR set to `[3, ∞)`, because:

```
(fun <sym> <sym-list> <code...>)
 1    2     3          4   ...
```

From the third element forward, the expressions are taken literally.

### No string/integer literal

This is a **purely symbolic** LISP. That means the only elements of the language
you can define are symbols and expressions. As mentioned in the '_README_', this
was made with the intent of being the stage 0 of bootstrapping of another LISP,
so it had to be _as small as we can make it_.

You can still _use_ string/integer literals, but you can't _define_ them in a
program written with LIBVSL yourself.

For this reason, no mathematical operation made it into PRIMVSL.
