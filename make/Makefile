all: libvsl.a

-include make/Flags.mk
-include make/Objects.mk
-include make/Libraries.mk

MAKEFILES:=					\
	make/Flags.mk			\
	make/Objects.mk		\
	make/Targets.mk		\
	make/Libraries.mk \
	make/Autodep.mk		\
	make/Deps.mk			\
	make/Objects.in

PRE_SRC:=lisp.c
PRE_BIN:=lisp

TARGETS:=prevsl $(PRE_BIN)

$(LIBRARIES):
	@echo "AR" $@
	@$(AR) crUv$(ARFLAGS) $@ $?

prevsl:
	@echo "CC" $@
	@$(CC) $(CFLAGS) $(CFLAGSADD) -L. -lvsl $< -o $@

$(PRE_BIN): prevsl $(PRE)
ifndef $(PRE)
	@echo "[ !! ] Missing \`PRE'. Did you set it in \`./configure'?"
	@exit 1
endif
	@echo "PREVSL" $(PRE)
	@./prevsl < $(PRE) > $(PRE_SRC) || exit 1
	@echo "CC lisp"
	@$(CC) -c $(CFLAGS) $(CFLAGSADD) -o $(PRE_BIN) -L. -lvsl $(PRE_SRC)

clean-obj:
	@echo "RM " $(OBJECTS)
	@rm -rfv $(OBJECTS)

clean-targets:
	@echo "RM " $(TARGETS) $(LIBRARIES) $(PRE_SRC)
	@rm -rfv $(TARGETS) $(LIBRARIES) $(PRE_SRC)

clean-make:
	@echo "RM " $(MAKEFILES)
	@rm -rfv $(MAKEFILES)

clean-tags:
	@echo "RM TAGS"
	@rm -rfv TAGS

clean: clean-obj clean-tags clean-targets clean-make

tags:
	@echo "ETAGS TAGS"
	@find -type f -name '*.[ch]' | xargs etags -o TAGS

help:
	@echo -e "\
make: makes the current dependency of \`all'\n\
make lisp PRE=<vsl-file> makes the lisp, with \`PRE' as the pvsl file.\n\
                         \`PRE' can also be defined at \`./configure' time\n\
make clean-obj: cleans objects files\n\
make clean-tags: cleans tags\n\
make clean-make: cleans targets\n\
make clean: cleans all"

.PHONY: clean clean-tags clean-obj clean-targets clean-make \
        tags help lisp $(PRE)