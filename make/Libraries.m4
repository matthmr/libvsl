include(make/m4/Makefile.m4)

library(`libvsl.a')
library_obj(
`libvsl.o', `lisp.o', `symtab.o', `stack.o', `sexp.o', `mm.o', `lex.o',
`utils.o')
library_gen
