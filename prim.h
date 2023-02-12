#ifndef LOCK_PRIM
#  define LOCK_PRIM

// the symbol table is defined by CGEN
#  include "symtab.h"

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

enum lisp_fun_mem_t {
  __LISP_FUN_VAR_GEN = 0,
  __LISP_FUN_VAR_SYMP,
  __LISP_FUN_VAR_SYM,
  __LISP_FUN_VAR_HASH,
  __LISP_FUN_VAR_SEXP,
};

union lisp_fun_mem_m {
  struct lisp_sym*  sym;  /** @sym:  a symbol pointer; for (ref) and alike */
  struct lisp_hash  hash; /** @hash: a hash; for most set/get quotes       */
  struct lisp_sexp* sexp; /** @sexp: a sexp; for most general quotes       */
  void*             gen;  /** @gen:  generic memory; casted by the caller  */
};

struct lisp_fun_mem {
  union lisp_fun_mem_m mem;
  enum lisp_fun_mem_t  typ;
};

struct lisp_fun_arg {
  struct lisp_fun_mem* argp;
  uint                 argv;
};

struct lisp_fun_ret {
  struct lisp_fun_arg master;
  enum lisp_fun_ret_t slave;
};

typedef struct lisp_fun_ret (*lisp_fun) (struct lisp_fun_arg arg,
                                         struct lisp_sym* sym);

#  ifdef EXTERN_PRIM_FUNC
struct lisp_fun_ret
lisp_prim_set(struct lisp_fun_arg arg, struct lisp_sym* sym);
struct lisp_fun_ret
lisp_prim_del(struct lisp_fun_arg arg, struct lisp_sym* sym);
struct lisp_fun_ret
lisp_prim_fun(struct lisp_fun_arg arg, struct lisp_sym* sym);
struct lisp_fun_ret
lisp_prim_lam(struct lisp_fun_arg arg, struct lisp_sym* sym);
struct lisp_fun_ret
lisp_prim_eval(struct lisp_fun_arg arg, struct lisp_sym* sym);
struct lisp_fun_ret
lisp_prim_quot(struct lisp_fun_arg arg, struct lisp_sym* sym);
struct lisp_fun_ret
lisp_prim_if(struct lisp_fun_arg arg, struct lisp_sym* sym);
struct lisp_fun_ret
lisp_prim_eq(struct lisp_fun_arg arg, struct lisp_sym* sym);
struct lisp_fun_ret
lisp_prim_not(struct lisp_fun_arg arg, struct lisp_sym* sym);
struct lisp_fun_ret
lisp_prim_block(struct lisp_fun_arg arg, struct lisp_sym* sym);
struct lisp_fun_ret
lisp_prim_while(struct lisp_fun_arg arg, struct lisp_sym* sym);
struct lisp_fun_ret
lisp_prim_break(struct lisp_fun_arg arg, struct lisp_sym* sym);
struct lisp_fun_ret
lisp_prim_continue(struct lisp_fun_arg arg, struct lisp_sym* sym);
struct lisp_fun_ret
lisp_prim_return(struct lisp_fun_arg arg, struct lisp_sym* sym);
struct lisp_fun_ret
lisp_prim_goto(struct lisp_fun_arg arg, struct lisp_sym* sym);
struct lisp_fun_ret
lisp_prim_label(struct lisp_fun_arg arg, struct lisp_sym* sym);
struct lisp_fun_ret
lisp_prim_cond(struct lisp_fun_arg arg, struct lisp_sym* sym);
struct lisp_fun_ret
lisp_prim_behead(struct lisp_fun_arg arg, struct lisp_sym* sym);
struct lisp_fun_ret
lisp_prim_head(struct lisp_fun_arg arg, struct lisp_sym* sym);
struct lisp_fun_ret
lisp_prim_list(struct lisp_fun_arg arg, struct lisp_sym* sym);
struct lisp_fun_ret
lisp_prim_parent(struct lisp_fun_arg arg, struct lisp_sym* sym);
struct lisp_fun_ret
lisp_prim_type(struct lisp_fun_arg arg, struct lisp_sym* sym);
struct lisp_fun_ret
lisp_prim_set_right_child(struct lisp_fun_arg arg, struct lisp_sym* sym);
struct lisp_fun_ret
lisp_prim_left_child(struct lisp_fun_arg arg, struct lisp_sym* sym);
struct lisp_fun_ret
lisp_prim_set_sibbling(struct lisp_fun_arg arg, struct lisp_sym* sym);
struct lisp_fun_ret
lisp_prim_set_parent(struct lisp_fun_arg arg, struct lisp_sym* sym);
struct lisp_fun_ret
lisp_prim_set_left_child(struct lisp_fun_arg arg, struct lisp_sym* sym);
struct lisp_fun_ret
lisp_prim_ref(struct lisp_fun_arg arg, struct lisp_sym* sym);
struct lisp_fun_ret
lisp_prim_right_child(struct lisp_fun_arg arg, struct lisp_sym* sym);
#  endif

#endif
