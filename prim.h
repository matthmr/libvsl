#ifndef LOCK_PRIM
#  define LOCK_PRIM

// TODO: a #define header that lets the user define their lisp without primvsl

#  include "lisp.h" // also includes `symtab.h', `sexp.h'

#  define CLISP_PRIM_DECLFUN(__fun,__funname,_s0,_s1,_l0,_l1) \
  {                                  \
    .str = __fun,                    \
    .sym = {                         \
      .typ  = __LISP_TYP_FUN,        \
      .dat  = lisp_prim_##__funname, \
      .size = {_s0, _s1},            \
      .litr = {_l0, _l1},            \
    },                               \
  }

#  define CLISP_PRIM_DECLSYM(__sym,__val) \
  {                                       \
    .str = __sym,                         \
    .sym = {                              \
      .typ = __LISP_TYP_SYM,              \
      .dat = __val,                       \
    },                                    \
  }

#  define CLISP_PRIM_DECLTAB(__tab, __typ) \
  {                                        \
    .tab = __tab,                          \
    .typ = __typ,                          \
  }                                        \

#  define CLISP_PRIM_DECL_END()    {.str = NULL}

#  define CLISP_PRIM_DECLSYM_END() CLISP_PRIM_DECL_END()
#  define CLISP_PRIM_DECLFUN_END() CLISP_PRIM_DECL_END()
#  define CLISP_PRIM_DECLTAB_END() {.tab = NULL}

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

struct clisp_tab {
  struct clisp_sym* tab;
  enum lisp_sym_typ typ;
};

extern struct clisp_sym libvsl_prim_symtab[];
extern struct clisp_sym libvsl_prim_funtab[];
extern struct clisp_tab __libvsl_prim_itertab[];

int lisp_prim_init(void);
void lisp_prim_setlocal(struct clisp_tab* local, bool keep);

enum fecode {
  FEOK = 0,

  FENOARGP0,

  FEOK_END,
};

#define FECODE_BEGIN (FEOK)
#define FECODE_END   (FEOK_END)

#  define CLISP_PRIM_DEFUN(name) \
  struct lisp_ret lisp_prim_##name(struct lisp_arg* argp, uint argv)

CLISP_PRIM_DEFUN(emptyisnil);
CLISP_PRIM_DEFUN(set);
CLISP_PRIM_DEFUN(ref);
CLISP_PRIM_DEFUN(del);
CLISP_PRIM_DEFUN(fun);
CLISP_PRIM_DEFUN(lam);
CLISP_PRIM_DEFUN(eval);
CLISP_PRIM_DEFUN(quot);
CLISP_PRIM_DEFUN(if);
CLISP_PRIM_DEFUN(eq);
CLISP_PRIM_DEFUN(not);
CLISP_PRIM_DEFUN(block);
CLISP_PRIM_DEFUN(while);
CLISP_PRIM_DEFUN(break);
CLISP_PRIM_DEFUN(continue);
CLISP_PRIM_DEFUN(return);
CLISP_PRIM_DEFUN(goto);
CLISP_PRIM_DEFUN(label);
CLISP_PRIM_DEFUN(cond);
CLISP_PRIM_DEFUN(parent);
CLISP_PRIM_DEFUN(type);
CLISP_PRIM_DEFUN(lexp);
CLISP_PRIM_DEFUN(set_right_child);
CLISP_PRIM_DEFUN(left_child);
CLISP_PRIM_DEFUN(set_parent);
CLISP_PRIM_DEFUN(set_left_child);
CLISP_PRIM_DEFUN(right_child);

#endif
