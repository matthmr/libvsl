# vsl - very small lisp

A very small implementation of lisp, nothing too fancy.

## Building

Run `make libvsl` to compile the project. `make` variables are:

- `CC` - the C compiler
- `CFLAGS` - `CC` major flags
- `CFLAGSADD` - `CC` minor flags

## Usage in other projects

I started this as a sanity check to see if I was able to implement a full
functioning static lisp for `GPLD`, but I realized that this may be more
suitable for code inlining than my other project `GPLD`. So I decided to build a
library version of this program.

Run `make libvsl` to make the library, then create a `.pvsl` file with the
following syntax:

- header
```
(<c-file-name>)
```

- body
```
(<symbol-name> <input-form-sexp> <output-form-sexp> <c-function-name>)
```

Where:

- `<input-form-sexp>`: is a list containing either `t`s or `nil`s, where `t`s
  represent *sexp*s and `nil`s represent symbols
  - the same is true for `<output-form-sexp>`
- `<c-function-name>`: is the name of a *LISP*-defined C function (which you'll
  have to define beforehand

For example (this file is named `ex.pvsl`, and it has functions defined in a
file named `ex-func.c`):

```
(ex-func.c)
(greet (nil) (nil) lisp_function_greet)
```

Then run `prevsl < <input-pvsl-file> > <output-c-file>`.

For example:

``` shell
prevsl < ex.pvsl > ex.c
```

This will transpile to C, making as many optimizations as it can to the
resulting symbol table.

To create the executable that reads *LISP* lists, compile the resulting file
against `libvsl` and the function source and you'll have a *LISP* by the end.

For example:

``` shell
cc ex.c ex-func.c -L. -lvsl -o ex-lisp
```

Then run it:

``` shell
$ ex-lisp
(greet world)
C-d
hello world
$
```

# NOTE -- 20221226

If you saw this source tree under commit `123c482a` or lower, you may notice
that the repository had a stub around `main` on the `vslisp.c` file. That was
because the main version of `vslisp` was just for debugging (and also `vslisp`
was a compilable target), and `vlisp.c` could be made to forego `main` and
become `libvsl`, with another file taking the role to host `main`.

As of the date of this note, the project has become a pure template, with no
stub around `main`. However the names of the files stay the same to avoid
future merge conflicts (so sorry for the misleading names :))

## TL;DR

The project produces two binaries:

- `prevsl`
- `libvsl`

Use the former to link to the latter.

# License

This repository is licensed under the [MIT
License](https://opensource.org/licenses/MIT).
