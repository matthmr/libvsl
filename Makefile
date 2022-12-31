all: libvsl.a

vslisp.o: vslisp.c vslisp.h pool.h symtab.h utils.h
symtab.o: symtab.c symtab.h utils.h
prevsl.o: prevsl.c
OBJECTS:=vslisp.o symtab.o prevsl.o

libvsl.a: symtab.o vslisp.o
LIBRARIES:=libvsl.a

prevsl: prevsl.o libvsl.a
TARGETS:=

AR?=ar
CC?=clang
CFLAGS?=-Wall
CFLAGSADD?=

PRE?=

$(OBJECTS):
	@echo "CC " $@
	@$(CC) -c $(CFLAGS) $(CFLAGSADD) $< -o $@

$(TARGETS):
	@echo "CC" $@
	@$(CC) $(CFLAGS) $(CFLAGSADD) $< -o $@

$(LIBRARIES):
	@echo "AR" $@
	@$(AR) crv $@ $?

prevsl:
	@echo "CC" $@
	@(CC) -c $(CFLAGS) $(CFLAGSADD) prevsl.c -o $@ -L. -lvsl

lisp: libvsl.a prevsl $(PRE)
	@echo "PREVSL" $(PRE)
	@./prevsl < $(PRE) > lisp.c
	@echo "CC lisp"
	@$(CC) -c $(CFLAGS) $(CFLAGSADD) lisp.c -o lisp -L. -lvsl

clean:
	@echo "RM lisp.c " $(OBJECTS) $(TARGETS) $(LIBRARIES)
	@rm -rfv lisp.c $(OBJECTS) $(TARGETS) $(LIBRARIES)

help:
	@echo -e "\
make: makes all targets \n\
make help: display this error message \n\
make lisp PRE=...: makes the lisp, with \`PRE' as the pvsl file \n\
make clean: clean targets"

.PHONY: clean help lisp $(TARGETS)
