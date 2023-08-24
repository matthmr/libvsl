/**
	 An interface to define LIBVSL objects through C
 */

#ifndef LOCK_CLISP
#  define LOCK_CLISP

#  include "symtab.h" // also includes `utils.h'

#  define CLISP_DECLFUN(__funstr,__funptr,_s0,_s1,_l0,_l1) \
  {                          	 \
    .str = __funstr,         	 \
    .sym = {                 	 \
      .typ  = __CLISP_FUN_TYP, \
      .dat  = __funptr,      	 \
      .size = {_s0, _s1},    	 \
      .litr = {_l0, _l1},    	 \
    },                       	 \
  }

#  define CLISP_DECLSYM(__symstr,__symdata) \
  {                           \
    .str = __symstr,          \
    .sym = {                  \
      .typ = __CLISP_SYM_TYP, \
      .dat = __symdata,       \
    },                        \
  }

#  define CLISP_DECLTAB(__tab, __typ) \
  {               \
    .tab = __tab, \
    .typ = __typ, \
  }               \

#  define CLISP_DECL_END()    {.str = NULL}

#  define CLISP_DECLSYM_END() LISP_DECL_END()
#  define CLISP_DECLFUN_END() LISP_DECL_END()
#  define CLISP_DECLTAB_END() {.tab = NULL}

#  define CLISP_DEFUN(name) \
  struct lisp_ret name (struct lisp_arg* argp, uint argv)

/* as the entry on the original `clisp' tables are also modified, we can easily
   address a symbol on the symbol table with just the index to the `clisp'
   table, instead of the on-the-fly symbol from the hash
*/
enum clisp_libvsl_sym_t {
  __CLISP_PRIM_SYM_T    = 0,
  __CLISP_PRIM_SYM_NIL,
  __CLISP_PRIM_SYM_SYM,
  __CLISP_PRIM_SYM_SEXP,
  __CLISP_PRIM_SYM_LEXP,
};

struct clisp_sym {
  const char*     str; /** @str: the C-string representation of the symbol */
  struct lisp_sym sym; /** @sym: the symbol template for the symtab        */
};

int clisp_init(struct clisp_sym* ctab);

enum fecode {
  FEOK = 0,

  FENOARGP0,

  FEOK_END,
};

#define FECODE_BEGIN (FEOK)
#define FECODE_END   (FEOK_END)

#endif
