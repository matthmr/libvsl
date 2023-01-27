include(make/m4/Makefile.m4)

target(`prevsl')
target_obj(`')
target_link_local(`vsl')
target_gen

target(`primtab')
target_obj(`cgen_primtab.o', `cgen.o',
`symtab.o', `prim.o', `err.o')
target_gen
