define(`makeflagdef',
`ifdef(`M4FLAG_conf_$1',, `define(`M4FLAG_conf_$1', `$2')')'
`$1:=M4FLAG_conf_$1')dnl

define(`makeflagchange',
`ifdef(`M4FLAG_conf_$1',, `define(`M4FLAG_conf_$1', `$2')')'
`$1?=M4FLAG_conf_$1')dnl

define(`incmakeflagdef',
`ifdef(`M4FLAG_include_$1',, `define(`M4FLAG_include_$1', `$2')')'
`$1:=M4FLAG_include_$1')dnl

define(`incmakeflagchange',
`ifdef(`M4FLAG_include_$1',, `define(`M4FLAG_include_$1', `$2')')'
`$1?=M4FLAG_include_$1')dnl

makeflagdef(`CC', `cc')
makeflagdef(`CFLAGS', `-Wall')
makeflagchange(`CFLAGSADD', `')
makeflagdef(`AR', `ar')
makeflagdef(`ARFLAGS', `')
makeflagchange(`PRE_SRC', `')
makeflagchange(`PRE_PVSL', `')

undefine(`makeflag')
undefine(`makeflagchange')
undefine(`incmakeflag')
undefine(`incmakeflagchange')
