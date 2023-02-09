#ifndef LOCK_PRIM
#  define LOCK_PRIM

// the symbol table is defined by CGEN
#  include "symtab.h"

#  define CLISP_PRIM_FUN(__fun,_s0,_s1,_l0,_l1) \
  {                                           \
    .str = __fun,                             \
    .sym = {                                  \
      .typ  = __LISP_CLISP_FUN,               \
      .dat  = "lisp_prim_" __fun,             \
      .size = {_s0, _s1},                     \
      .litr = {_l0, _l1},                     \
    },                                        \
  }

#  define CLISP_PRIM_SYM(__sym,__val) \
  {                                 \
    .str = __sym,                   \
    .sym = {                        \
      .typ = __LISP_CLISP_SYM,      \
      .dat = __val,                 \
    },                              \
  }

extern const struct clisp_sym vsl_primtab[];

struct lisp_fun_ret {
  void* master; // TODO
  int   slave;
};

struct lisp_fun_arg {
  uint size[2];
  uint litr[2];
};

typedef struct lisp_fun_ret (*lisp_fun) (struct lisp_fun_arg args);

#  ifdef EXTERN_PRIM_FUNC
struct lisp_fun_ret lisp_prim_set(struct lisp_fun_arg args);
struct lisp_fun_ret lisp_prim_fun(struct lisp_fun_arg args);
struct lisp_fun_ret lisp_prim_eval(struct lisp_fun_arg args);
struct lisp_fun_ret lisp_prim_quot(struct lisp_fun_arg args);
struct lisp_fun_ret lisp_prim_if(struct lisp_fun_arg args);
struct lisp_fun_ret lisp_prim_eq(struct lisp_fun_arg args);
struct lisp_fun_ret lisp_prim_not(struct lisp_fun_arg args);
struct lisp_fun_ret lisp_prim_block(struct lisp_fun_arg args);
struct lisp_fun_ret lisp_prim_while(struct lisp_fun_arg args);
struct lisp_fun_ret lisp_prim_break(struct lisp_fun_arg args);
struct lisp_fun_ret lisp_prim_continue(struct lisp_fun_arg args);
struct lisp_fun_ret lisp_prim_return(struct lisp_fun_arg args);
struct lisp_fun_ret lisp_prim_goto(struct lisp_fun_arg args);
struct lisp_fun_ret lisp_prim_label(struct lisp_fun_arg args);
struct lisp_fun_ret lisp_prim_cond(struct lisp_fun_arg args);
struct lisp_fun_ret lisp_prim_behead(struct lisp_fun_arg args);
struct lisp_fun_ret lisp_prim_head(struct lisp_fun_arg args);
struct lisp_fun_ret lisp_prim_list(struct lisp_fun_arg args);
struct lisp_fun_ret lisp_prim_parent(struct lisp_fun_arg args);
struct lisp_fun_ret lisp_prim_type(struct lisp_fun_arg args);
struct lisp_fun_ret lisp_prim_set_right_child(struct lisp_fun_arg args);
struct lisp_fun_ret lisp_prim_left_child(struct lisp_fun_arg args);
struct lisp_fun_ret lisp_prim_set_sibbling(struct lisp_fun_arg args);
struct lisp_fun_ret lisp_prim_set_parent(struct lisp_fun_arg args);
struct lisp_fun_ret lisp_prim_set_left_child(struct lisp_fun_arg args);
struct lisp_fun_ret lisp_prim_ref(struct lisp_fun_arg args);
struct lisp_fun_ret lisp_prim_right_child(struct lisp_fun_arg args);
#  endif

#endif
