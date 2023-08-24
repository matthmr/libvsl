# vsl - Very Small/Simple Lisp

A very small LISP implementation framework: a library you can program alongside
to produce a LISP.

## Building

Run `./configure` then `make libvsl.a` to compile the project. Run `./configure
--flags` to see all the flags available.

Run either `./configure --help` or `make help` or both for more info.

## API

See the LIBVSL's [documentation](./docs/LIBVSL.org) to get a quick manual on
using the API to make a LISP with LIBVSL.

## Motive

This is the standalone version of the step 0 of the bootstrapping process of
GPLD. As such, the LISPs you can make with this library are pretty limited in
scope and syntax, as we want something really simple to parse (the lexer doesn't
even use regexes!)

# License

This repository is licensed under the [MIT
License](https://opensource.org/licenses/MIT).
