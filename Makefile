all: libvsl.a

vslisp.o: vslisp.c vslisp.h pool.h symtab.h utils.h sexp.h debug.h
symtab.o: symtab.c symtab.h pool.h utils.h
prevsl.o: prevsl.c symtab.h sexp.h pool.h utils.h
stack.o: stack.c stack.h utils.h sexp.h symtab.h debug.h
OBJECTS:=vslisp.o symtab.o prevsl.o stack.o

libvsl.a: symtab.o vslisp.o stack.o
LIBRARIES:=libvsl.a

prevsl: prevsl.o libvsl.a
TARGETS:=

AR?=ar
CC?=clang
CFLAGS?=-Wall
CFLAGSADD?=

PRE?=
PREDEF_SRC:=lisp.c
PREDEF_BIN:=lisp

$(OBJECTS):
	@echo "CC " $@
	@$(CC) -c $(CFLAGS) $(CFLAGSADD) $< -o $@

$(TARGETS):
	@echo "CC" $@
	@$(CC) $(CFLAGS) $(CFLAGSADD) $< -o $@

$(LIBRARIES):
	@echo "AR" $@
	@$(AR) crUv $@ $?

prevsl:
	@echo "CC" $@
	@$(CC) $(CFLAGS) $(CFLAGSADD) -L. -lvsl $< -o $@

lisp: prevsl $(PRE)
	@echo "PREVSL" $(PRE)
	@./prevsl < $(PRE) > $(PREDEF_SRC)
	@echo "CC lisp"
	@$(CC) -c $(CFLAGS) $(CFLAGSADD) -o $(PREDEF_BIN) -L. -lvsl $(PREDEF_SRC)

tags:
	@echo "ETAGS TAGS"
	@find -type f -name '*.[ch]' | xargs etags -o TAGS

clean:
	@echo "RM TAGS" $(PREDEF_SRC) $(OBJECTS) $(TARGETS) $(LIBRARIES)
	@rm -rfv TAGS $(PREDEF_SRC) $(OBJECTS) $(TARGETS) $(LIBRARIES)

help:
	@echo -e "\
make: makes all targets \n\
make help: display this error message \n\
make lisp PRE=...: makes the lisp, with \`PRE' as the pvsl file \n\
make clean: clean targets"

.PHONY: tags clean help lisp $(TARGETS)
