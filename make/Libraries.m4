include(make/m4/Makefile.m4)

library(`libvsl.a')
library_obj(
`libvsl.o', `symtab.o', `stack.o', `sexp.o', `lex.o')
library_gen
