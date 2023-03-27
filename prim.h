#ifndef LOCK_PRIM
#  define LOCK_PRIM

#  define LOCK_POOL_DEF

#  include "lisp.h" // also includes `symtab.h', `sexp.h'

#  define CLISP_PRIM_FUN(__fun,_s0,_s1,_l0,_l1) \
  {                                             \
    .str = __fun,                               \
    .sym = {                                    \
      .typ  = __LISP_CLISP_FUN,                 \
      .dat  = "lisp_prim_" __fun,               \
      .size = {_s0, _s1},                       \
      .litr = {_l0, _l1},                       \
    },                                          \
  }

#  define CLISP_PRIM_SYM(__sym,__val) \
  {                                   \
    .str = __sym,                     \
    .sym = {                          \
      .typ = __LISP_CLISP_SYM,        \
      .dat = __val,                   \
    },                                \
  }

extern const struct clisp_sym vsl_primtab[];

enum fecode {
  FEOK = 0,

  FENOARGP0,

  FEOK_END,
};

#define FECODE_BEGIN (FEOK)
#define FECODE_END   (FEOK_END)

#  define CLISP_PRIM(name) \
  struct lisp_ret lisp_prim_##name(struct lisp_arg* argp, uint argv)

#  ifdef EXTERN_PRIM_FUNC
CLISP_PRIM();
CLISP_PRIM(set);
CLISP_PRIM(ref);
CLISP_PRIM(del);
CLISP_PRIM(fun);
CLISP_PRIM(lam);
CLISP_PRIM(eval);
CLISP_PRIM(quot);
CLISP_PRIM(if);
CLISP_PRIM(eq);
CLISP_PRIM(not);
CLISP_PRIM(block);
CLISP_PRIM(while);
CLISP_PRIM(break);
CLISP_PRIM(continue);
CLISP_PRIM(return);
CLISP_PRIM(goto);
CLISP_PRIM(label);
CLISP_PRIM(cond);
CLISP_PRIM(parent);
CLISP_PRIM(type);
CLISP_PRIM(set_right_child);
CLISP_PRIM(left_child);
CLISP_PRIM(set_parent);
CLISP_PRIM(set_left_child);
CLISP_PRIM(right_child);
#  endif

#endif
