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
PREDEF:=lisp.c

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
	@./prevsl < $(PRE) > $(PREDEF)
	@echo "CC lisp"
	@$(CC) -c $(CFLAGS) $(CFLAGSADD) -o lisp -L. -lvsl $(PREDEF)

tags:
	@echo "ETAGS TAGS"
	@find -type f -name '*.[ch]' | xargs etags -o TAGS

clean:
	@echo "RM TAGS" $(PREDEF) $(OBJECTS) $(TARGETS) $(LIBRARIES)
	@rm -rfv TAGS $(PREDEF) $(OBJECTS) $(TARGETS) $(LIBRARIES)

help:
	@echo -e "\
make: makes all targets \n\
make help: display this error message \n\
make lisp PRE=...: makes the lisp, with \`PRE' as the pvsl file \n\
make clean: clean targets"

.PHONY: tags clean help lisp $(TARGETS)
