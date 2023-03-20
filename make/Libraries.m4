include(make/m4/Makefile.m4)

library(`libvsl.a')
library_obj(
`libvsl.o', `symtab.o', `stack.o', `sexp.o',
`lex.o', `err.o')
library_gen

library(`libvslprim.a')
library_obj(
`prim.o', `primtab.o')
library_gen

library(`libcgen.a')
library_obj(
`libcgen.o')
library_gen
