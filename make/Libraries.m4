include(make/m4/Makefile.m4)

library(`libvsl.a')
library_obj(
`libvsl.o', `symtab.o', `stack.o', `sexp.o', `clisp.o', `mm.o', `lex.o',
`err.o')
library_gen
