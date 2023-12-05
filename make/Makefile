all: libvsl.a

-include make/Flags.mk
-include make/Objects.mk
-include make/Libraries.mk
-include make/Targets.mk

MAKEFILES:=         \
	make/Flags.mk     \
	make/Objects.mk   \
	make/Libraries.mk \
	make/Targets.mk   \
	make/Deps.mk

clean-obj:
	@echo "RM " $(OBJECTS)
	@rm -rfv $(OBJECTS)

clean-targets:
	@echo "RM " $(TARGETS) $(LIBRARIES)
	@rm -rfv $(TARGETS) $(LIBRARIES)

clean-make:
	@echo "RM " $(MAKEFILES)
	@rm -rfv $(MAKEFILES)

clean-tags:
	@echo "RM TAGS"
	@rm -rfv TAGS

clean-dev: clean-obj clean-targets
clean: clean-obj clean-tags clean-targets clean-make

tags:
	@echo "ETAGS TAGS"
	@find -type f -name '*.[ch]' | xargs etags -o TAGS

help:
	@echo -e "\
make: makes the current dependency of \`all' (libvsl.a)\n\
make clean-obj: cleans objects files\n\
make clean-tags: cleans tags\n\
make clean-targets: cleans targets\n\
make clean-make: cleans makefiles\n\
make clean: cleans all"

.PHONY: clean clean-tags clean-obj clean-targets clean-make \
        tags help lisp
