#ifndef LOCK_PRIM
#  define LOCK_PRIM

// the symbol table is defined by CGEN
#  include "symtab.h" // also includes `utils.h`, `err.h'

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

enum lisp_fun_ret_t {
  __LISP_FUN_MASK = -1, /** stack the call to the function, waiting for more
                            arguments */
  __LISP_FUN_OK   = 0,
};

union lisp_fun_u {
  struct lisp_sym*  sym;  /** @sym:  a symbol pointer; for (ref) and alike */
  struct lisp_hash  hash; /** @hash: a hash; for most set/get quotes       */
  struct lisp_sexp* sexp; /** @sexp: a sexp; for most general quotes       */
  void*             gen;  /** @gen:  generic memory; casted by the caller  */
};

struct lisp_fun_arg {
  union lisp_fun_u  mem;
  enum lisp_sym_typ typ;
};

// struct lisp_fun_args {
//   struct lisp_fun_argp* argp;
//   uint                  argv;
// };

// f(x) = y -> master := y, slave := err
struct lisp_fun_ret {
  struct lisp_fun_arg master;
  enum lisp_fun_ret_t slave;
};

typedef struct lisp_fun_ret
(*lisp_fun) (struct lisp_fun_arg* argp, uint argv, struct lisp_sym* sym);

#  define CLISP_PRIM(name) \
  struct lisp_fun_ret lisp_prim_##name(struct lisp_fun_arg* argp, \
                                       uint argv, struct lisp_sym* sym)

#  ifdef EXTERN_PRIM_FUNC
CLISP_PRIM();
CLISP_PRIM(set);
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
CLISP_PRIM(behead);
CLISP_PRIM(head);
CLISP_PRIM(list);
CLISP_PRIM(parent);
CLISP_PRIM(type);
CLISP_PRIM(set_right_child);
CLISP_PRIM(left_child);
CLISP_PRIM(set_sibbling);
CLISP_PRIM(set_parent);
CLISP_PRIM(set_left_child);
CLISP_PRIM(ref);
CLISP_PRIM(right_child);
#  endif

#endif
