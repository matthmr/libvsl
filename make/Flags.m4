include(make/m4/Defaults.m4)dnl
define(makeflagdef, `$1:=M4FLAG_conf_$1')dnl
define(makeflagchange, `$1?=M4FLAG_conf_$1')dnl
define(incmakeflagdef, `$1:=M4FLAG_include_$1')dnl
define(incmakeflagchange, `$1?=M4FLAG_include_$1')dnl
dnl
makeflagdef(`CC')
makeflagdef(`CFLAGS')
makeflagdef(`ARFLAGS')
makeflagchange(`CFLAGSADD')
makeflagchange(`PRE')
dnl
undefine(`makeflag')dnl
undefine(`makeflagchange')dnl
undefine(`incmakeflag')dnl
undefine(`incmakeflagchange')dnl
