include(make/m4/Makefile.m4)

library(`libvsl.a')
library_obj(
`libvsl.o', `symtab.o', `stack.o', `sexp.o', `prim.o', `prim_defun.o',
`mm.o', `lex.o', `err.o')
library_gen
