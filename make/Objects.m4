include(make/m4/Objects.m4)
include(make/Include.m4)
override_object(`prim',dnl
@echo "CC primtab"
	@$(CC) M4FLAG_include_prim $(CFLAGS) $(CFLAGSADD) $< -o primtab
	@echo "CGEN primtab.c"
	@./primtab > primtab.c
	@echo "CC primtab.c"
	@$(CC) M4FLAG_include_primtab $(CFLAGS) $(CFLAGSADD) primtab.c -o prim.o)
dnl include(make/Objects-override.in)
