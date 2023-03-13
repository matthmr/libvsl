define(`__target_link__', `')dnl
dnl
define(`librec', `ifelse($1, `', , `$1.a librec(shift($@))')')dnl
define(`rec', `ifelse($1, `', ,`$1 rec(shift($@))')')dnl
define(`ifrec', `ifelse($1, `', ,`ifelse($2, `1', `$1 ifrec(shift(shift($@)))', )')')dnl
dnl
define(`target', `define(`__target__', `$1')')dnl
define(`target_obj', `define(`__target_objects__', `rec($@)')')dnl
define(`target_link_local', `define(`__target_link__', `librec($@)')')dnl
define(`target_gen', `dnl
ifelse(`__target_link__', `',, `LINK_LOCAL:' __target_link__
)dnl
__target__: __target_objects__ dnl
define(`__target_link__', `')')dnl
dnl
define(`library', `define(`__library__', `$1')')dnl
define(`library_obj', `define(`__library_objects__', `rec($@)')')dnl
define(`library_gen', `dnl
__library__: __library_objects__')dnl
dnl
define(`cgen_target', dnl
`define(`__cgen_src__', $1)'dnl
`define(`__cgen_prog__', $2)')dnl
define(`cgen_gen', `dnl
__cgen_src__: __cgen_prog__
	@echo "CGEN __cgen_src__"
	@./__cgen_prog__ > ./__cgen_src__ || exit 1')dnl
