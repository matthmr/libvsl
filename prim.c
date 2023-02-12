#define EXTERN_PRIM_FUNC

#include "debug.h"
#include "prim.h"

const struct clisp_sym vsl_primtab[] = {
  // turing completion
  CLISP_PRIM_FUN("set", 2, 2, 1, 1), // (set 'x y)
  CLISP_PRIM_FUN("del", 1, 1, 1, 1), // (del 'x)
  CLISP_PRIM_FUN("ref", 2, 2, 2, 2), // (ref 'x 'y)
  CLISP_PRIM_FUN("fun", 2, INFINITY, 1, 2), // (fun 'x (...) ...)
  CLISP_PRIM_FUN("lam", 1, INFINITY, 1, 1), // (lam (...) ...)

  CLISP_PRIM_FUN("if", 2, 3, 0, 0), // (if x y z?)
  CLISP_PRIM_FUN("eq", 2, 2, 0, 0), // (eq x y)
  CLISP_PRIM_FUN("not", 1, 1, 0, 0), // (not x)
  CLISP_PRIM_FUN("block", 0, INFINITY, 0, 0), // (block ...)
  CLISP_PRIM_FUN("while", 1, 2, 0, 0), // (while x y?)
  CLISP_PRIM_FUN("break", 0, 0, 0, 0), // (break)
  CLISP_PRIM_FUN("continue", 0, 0, 0, 0), // (continue)
  CLISP_PRIM_FUN("return", 0, 1, 0, 0), // (return x?)
  CLISP_PRIM_FUN("goto", 1, 1, 1, 1), // (goto 'x)
  CLISP_PRIM_FUN("label", 1, 1, 1, 1), // (label 'x)
  CLISP_PRIM_FUN("cond", 1, INFINITY, 1, INFINITY), // (cond (x y)...)

  // lisp-specific
  CLISP_PRIM_FUN("left-child", 1, 1, 1, 1), // (left-child 'x)
  CLISP_PRIM_FUN("right-child", 1, 1, 1, 1), // (right-child 'x)
  CLISP_PRIM_FUN("parent", 1, 1, 1, 1), // (parent 'x)
  CLISP_PRIM_FUN("set-left-child", 2, 2, 1, 1), // (set-left-child 'x y)
  CLISP_PRIM_FUN("set-right-child", 2, 2, 1, 1), // (set-right-child 'x y)
  CLISP_PRIM_FUN("set-sibbling", 2, 2, 1, 1), // (set-sibbling 'x y)
  CLISP_PRIM_FUN("set-parent", 2, 2, 1, 1), // (set-parent 'x y)
  CLISP_PRIM_FUN("behead", 1, 1, 0, 0), // (behead x)
  CLISP_PRIM_FUN("head", 1, 1, 0, 0), // (head x)
  CLISP_PRIM_FUN("eval", 1, 1, 1, 1), // (eval 'x)
  CLISP_PRIM_FUN("quot", 1, 1, 1, 1), // (quot 'x)
  CLISP_PRIM_FUN("type", 1, 1, 1, 1), // (type 'x)
  CLISP_PRIM_FUN("list", 0, INFINITY, 0, INFINITY), // (list ...)

  // booleans
  CLISP_PRIM_SYM("t", "NULL"),
  CLISP_PRIM_SYM("nil", "NULL"),

  // types
  CLISP_PRIM_SYM("@sym", "NULL"),
  CLISP_PRIM_SYM("@sexp", "NULL"),
  CLISP_PRIM_SYM("@lexp", "NULL"),

  // EOL
  {.str = NULL},
};

////////////////////////////////////////////////////////////////////////////////

struct lisp_fun_ret
lisp_prim_set(struct lisp_fun_arg args, struct lisp_sym* sym) {
  struct lisp_fun_ret ret = {0};

  DB_MSG("-> lisp_prim_set");

  return ret;
}

struct lisp_fun_ret
lisp_prim_fun(struct lisp_fun_arg args, struct lisp_sym* sym) {
  struct lisp_fun_ret ret = {0};

  DB_MSG("-> lisp_prim_fun");

  return ret;
}

struct lisp_fun_ret
lisp_prim_lam(struct lisp_fun_arg args, struct lisp_sym* sym) {
  struct lisp_fun_ret ret = {0};

  DB_MSG("-> lisp_prim_lam");

  return ret;
}

struct lisp_fun_ret
lisp_prim_eval(struct lisp_fun_arg args, struct lisp_sym* sym) {
  struct lisp_fun_ret ret = {0};

  DB_MSG("-> lisp_prim_eval");

  return ret;
}

struct lisp_fun_ret
lisp_prim_quot(struct lisp_fun_arg args, struct lisp_sym* sym) {
  struct lisp_fun_ret ret = {0};

  DB_MSG("-> lisp_prim_quot");

  ret.master.argv = sym->size[0];
  ret.master.argp->mem.sym = sym;
  ret.master.argp->typ     = __LISP_FUN_VAR_SYM;

  return ret;
}

struct lisp_fun_ret
lisp_prim_if(struct lisp_fun_arg args, struct lisp_sym* sym) {
  struct lisp_fun_ret ret = {0};

  DB_MSG("-> lisp_prim_if");

  return ret;
}

struct lisp_fun_ret
lisp_prim_eq(struct lisp_fun_arg args, struct lisp_sym* sym) {
  struct lisp_fun_ret ret = {0};

  DB_MSG("-> lisp_prim_eq");

  return ret;
}

struct lisp_fun_ret
lisp_prim_not(struct lisp_fun_arg args, struct lisp_sym* sym) {
  struct lisp_fun_ret ret = {0};

  DB_MSG("-> lisp_prim_not");

  return ret;
}

struct lisp_fun_ret
lisp_prim_block(struct lisp_fun_arg args, struct lisp_sym* sym) {
  struct lisp_fun_ret ret = {0};

  DB_MSG("-> lisp_prim_block");

  return ret;
}

struct lisp_fun_ret
lisp_prim_while(struct lisp_fun_arg args, struct lisp_sym* sym) {
  struct lisp_fun_ret ret = {0};

  DB_MSG("-> lisp_prim_while");

  return ret;
}

struct lisp_fun_ret
lisp_prim_break(struct lisp_fun_arg args, struct lisp_sym* sym) {
  struct lisp_fun_ret ret = {0};

  DB_MSG("-> lisp_prim_break");

  return ret;
}

struct lisp_fun_ret
lisp_prim_continue(struct lisp_fun_arg args, struct lisp_sym* sym) {
  struct lisp_fun_ret ret = {0};

  DB_MSG("-> lisp_prim_continue");

  return ret;
}

struct lisp_fun_ret
lisp_prim_return(struct lisp_fun_arg args, struct lisp_sym* sym) {
  struct lisp_fun_ret ret = {0};

  DB_MSG("-> lisp_prim_return");

  return ret;
}

struct lisp_fun_ret
lisp_prim_goto(struct lisp_fun_arg args, struct lisp_sym* sym) {
  struct lisp_fun_ret ret = {0};

  DB_MSG("-> lisp_prim_goto");

  return ret;
}

struct lisp_fun_ret
lisp_prim_label(struct lisp_fun_arg args, struct lisp_sym* sym) {
  struct lisp_fun_ret ret = {0};

  DB_MSG("-> lisp_prim_label");

  return ret;
}

struct lisp_fun_ret
lisp_prim_cond(struct lisp_fun_arg args, struct lisp_sym* sym) {
  struct lisp_fun_ret ret = {0};

  DB_MSG("-> lisp_prim_cond");

  return ret;
}

struct lisp_fun_ret
lisp_prim_behead(struct lisp_fun_arg args, struct lisp_sym* sym) {
  struct lisp_fun_ret ret = {0};

  DB_MSG("-> lisp_prim_behead");

  return ret;
}

struct lisp_fun_ret
lisp_prim_head(struct lisp_fun_arg args, struct lisp_sym* sym) {
  struct lisp_fun_ret ret = {0};

  DB_MSG("-> lisp_prim_head");

  return ret;
}

struct lisp_fun_ret
lisp_prim_list(struct lisp_fun_arg args, struct lisp_sym* sym) {
  struct lisp_fun_ret ret = {0};

  DB_MSG("-> lisp_prim_list");

  return ret;
}

struct lisp_fun_ret
lisp_prim_parent(struct lisp_fun_arg args, struct lisp_sym* sym) {
  struct lisp_fun_ret ret = {0};

  DB_MSG("-> lisp_prim_parent");

  return ret;
}

struct lisp_fun_ret
lisp_prim_type(struct lisp_fun_arg args, struct lisp_sym* sym) {
  struct lisp_fun_ret ret = {0};

  DB_MSG("-> lisp_prim_type");

  return ret;
}

struct lisp_fun_ret
lisp_prim_set_right_child(struct lisp_fun_arg args, struct lisp_sym* sym) {
  struct lisp_fun_ret ret = {0};

  DB_MSG("-> lisp_prim_set_right_child");

  return ret;
}

struct lisp_fun_ret
lisp_prim_left_child(struct lisp_fun_arg args, struct lisp_sym* sym) {
  struct lisp_fun_ret ret = {0};

  DB_MSG("-> lisp_prim_left_child");

  return ret;
}

struct lisp_fun_ret
lisp_prim_set_sibbling(struct lisp_fun_arg args, struct lisp_sym* sym) {
  struct lisp_fun_ret ret = {0};

  DB_MSG("-> lisp_prim_set_sibbling");

  return ret;
}

struct lisp_fun_ret
lisp_prim_set_parent(struct lisp_fun_arg args, struct lisp_sym* sym) {
  struct lisp_fun_ret ret = {0};

  DB_MSG("-> lisp_prim_set_parent");

  return ret;
}

struct lisp_fun_ret
lisp_prim_set_left_child(struct lisp_fun_arg args, struct lisp_sym* sym) {
  struct lisp_fun_ret ret = {0};

  DB_MSG("-> lisp_prim_set_left_child");

  return ret;
}

struct lisp_fun_ret
lisp_prim_ref(struct lisp_fun_arg args, struct lisp_sym* sym) {
  struct lisp_fun_ret ret = {0};

  DB_MSG("-> lisp_prim_ref");

  return ret;
}

struct lisp_fun_ret
lisp_prim_right_child(struct lisp_fun_arg args, struct lisp_sym* sym) {
  struct lisp_fun_ret ret = {0};

  DB_MSG("-> lisp_prim_right_child");

  return ret;
}

struct lisp_fun_ret
lisp_prim_del(struct lisp_fun_arg args, struct lisp_sym* sym) {
  struct lisp_fun_ret ret = {0};

  DB_MSG("-> lisp_prim_del");

  return ret;
}
