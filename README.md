# vsl - very small lisp

A very small implementation of lisp, nothing too fancy.

## Building

Run `./configure` then `make libvsl` to compile the project.

`./configure` variables are:
- `[+/-]debug`: generate verbose binaries (`-`)

`make` variables are:

- `CC` - the C compiler (`cc`)
- `CFLAGS` - `CC` major flags (`-Wall`)
- [1] `CFLAGSADD` - `CC` minor flags ()
- `AR` - the ELF archiver (`ar`)
- `ARFLAGS` - `AR` major flags
- `PRE` - the *prevsl* source file that will make your lisp (read below)

`make` variables are also `./configure` variables. Pass them with double dashes
and lowercase, for example:

```shell
# make variable
make CC=tcc CFLAGS=-O2 thing

# ./configure variable
./configure --cc=tcc --cflags=-O2 && make thing
```

See `./configure --help`.

[1]: major flags are always passed to all targets at ./configure time, minor
     flags are only passed at make time. This is a way to pass flags at make
     time that don't intefere with ./configure time

## Usage in other projects

I started this as a sanity check to see if I was able to implement a fully
functioning static LISP for GPLD, but I realized this may be useful as a more
lightweight version of GPLD. So I decided to build a library version of this
program.

Notice that this *is* less powerful than GPLD, as it is used only in
the stage 0 of bootstrapping for GPLD. That means you still have to write some C
code for your LISP functions/objects and you won't have numeric literals. See
the [documentation](./LIBVSL.md) on LIBVSL's PREVSL.

Create a `.pvsl` file then run `./configure --pre=<your-pvsl-file> && make
lisp`, or `make PRE=<your-pvsl-file> lisp` if you've already run `./configure`
before.

If you've followed PREVSL's guidelines, you should have a fully
functioning LISP by the end.

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

This repository is licensed under the [MIT License](https://opensource.org/licenses/MIT).
