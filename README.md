# vsl - Very Small/Simple Lisp

A very small implementation of LISP, nothing too fancy.

## Building

Run `./configure` then `make libvsl.a` to compile the project. Run `./configure
--flags` to see all the flags available.

Run either `./configure --help` or `make help` or both for more info.

## Usage in other projects

I started this as a sanity check to see if I was able to implement a fully
functioning static LISP for the stage 0 of bootstrapping of GPLD, but I realized
this may be useful as a standalone library. So I decided to build a library
version of this program.

Notice that this *is* less powerful than GPLD, as it is used only in the stage 0
of bootstrapping for GPLD. That means you still have to write C code for your
LISP functions/objects and you won't have numeric literals.

See the LIBVSL's [documentation](./docs/LIBVSL.org) to get a quick manual on how
to make a LISP with LIBVSL.

*NOTE*: while this library *is* able to implement a LISP with `PRIMVSL`, defined
        in `prim_defun.c`, it is best suited as a *base* to implement your own
        LISP on top of some primitives defined for turing completion (`set`,
        `if`, etc).

# License

This repository is licensed under the [MIT
License](https://opensource.org/licenses/MIT).
