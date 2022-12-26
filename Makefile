all: libvsl

OBJECTS:=vslisp.o symtab.o
TARGETS:=libvsl prevsl

vslisp.o: vslisp.c vslisp.h pool.h symtab.h
symtab.o: symtab.c symtab.h

prevsl.o: prevsl.c

prevsl: symtab.o prevsl.o
libvsl: symtab.o vslisp.o prevsl

CC?=clang
CFLAGS?=-Wall

$(TARGETS):
	@echo "CC" $@
	@$(CC) -c $(CFLAGS) $(CFLAGSADD) $< -o $@

$(OBJECTS):
	@echo "CC" $@
	@$(CC) -c $(CFLAGS) $(CFLAGSADD) $< -o $@

$(TARGETS):
	@echo "CC" $@
	@$(CC) $(CFLAGS) $(CFLAGSADD) $^ -o $@

clean:
	@echo "RM " $(OBJECTS) $(TARGETS)
	@rm -rfv $(OBJECTS) $(TARGETS)

help:
	@echo "\
make: makes all targets \n\
make help: display this error message \n\
make clean: clean targets"

.PHONY: help vslisp clean
