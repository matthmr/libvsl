all: vslisp

vslisp.o: vslisp.c vslisp.h
vslisp: vslisp.o

OBJECTS:=vslisp.o
TARGETS:=vslisp

CC?=clang
CFLAGS?=-Wall

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
