# vsl - very small lisp

A very small implementation of lisp, nothing too fancy.

## Building

Run `./configure` then `make libvsl.a` to compile the project.
Run `./configure --flags` to see all the flags available.

Run either `./configure --help` or `make help` or both for more info.

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

# License

This repository is licensed under the [MIT License](https://opensource.org/licenses/MIT).
