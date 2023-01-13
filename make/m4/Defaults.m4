define(`default', `ifdef(`M4FLAG_conf_$1',, `define(`M4FLAG_conf_$1', `$2')')')dnl
define(`incdefault', `ifdef(`M4FLAG_include_$1',, `define(`M4FLAG_include_$1', `$2')')')dnl
dnl
default(`CC', `cc')dnl
default(`CFLAGS', `-Wall')dnl
default(`CFLAGSADD', `')dnl
default(`ARFLAGS', `')dnl
default(`PRE', `')dnl
dnl
undefine(`default')dnl
undefine(`incdefault')dnl
