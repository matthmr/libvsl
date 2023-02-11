#define EXTERN_PRIM_FUNC
#include "prim.h"

const struct clisp_sym vsl_primtab[] = {
  // turing completion
  CLISP_PRIM_FUN("set", 2, 2, 1, 1), // (set 'x y)
  CLISP_PRIM_FUN("del", 1, 1, 1, 1), // (del 'x)
  CLISP_PRIM_FUN("ref", 2, 2, 2, 2), // (ref 'x 'y)

  // NOTE: (fun (...) ...) is a lambda
  CLISP_PRIM_FUN("fun", 1, INFINITY, 1, 2), // (fun 'x? (...) ...)

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
  CLISP_PRIM_SYM("boolt", "NULL"),
  CLISP_PRIM_SYM("symt", "NULL"),
  CLISP_PRIM_SYM("listt", "NULL"),
  CLISP_PRIM_SYM("treet", "NULL"),

  // EOL
  {.str = NULL},
};

////////////////////////////////////////////////////////////////////////////////

struct lisp_fun_ret lisp_prim_set(struct lisp_fun_arg args) {
  struct lisp_fun_ret ret = {0};
  return ret;
}

struct lisp_fun_ret lisp_prim_fun(struct lisp_fun_arg args) {
  struct lisp_fun_ret ret = {0};
  return ret;
}

struct lisp_fun_ret lisp_prim_eval(struct lisp_fun_arg args) {
  struct lisp_fun_ret ret = {0};
  return ret;
}

struct lisp_fun_ret lisp_prim_quot(struct lisp_fun_arg args) {
  struct lisp_fun_ret ret = {0};
  return ret;
}

struct lisp_fun_ret lisp_prim_if(struct lisp_fun_arg args) {
  struct lisp_fun_ret ret = {0};
  return ret;
}

struct lisp_fun_ret lisp_prim_eq(struct lisp_fun_arg args) {
  struct lisp_fun_ret ret = {0};
  return ret;
}

struct lisp_fun_ret lisp_prim_not(struct lisp_fun_arg args) {
  struct lisp_fun_ret ret = {0};
  return ret;
}

struct lisp_fun_ret lisp_prim_block(struct lisp_fun_arg args) {
  struct lisp_fun_ret ret = {0};
  return ret;
}

struct lisp_fun_ret lisp_prim_while(struct lisp_fun_arg args) {
  struct lisp_fun_ret ret = {0};
  return ret;
}

struct lisp_fun_ret lisp_prim_break(struct lisp_fun_arg args) {
  struct lisp_fun_ret ret = {0};
  return ret;
}

struct lisp_fun_ret lisp_prim_continue(struct lisp_fun_arg args) {
  struct lisp_fun_ret ret = {0};
  return ret;
}

struct lisp_fun_ret lisp_prim_return(struct lisp_fun_arg args) {
  struct lisp_fun_ret ret = {0};
  return ret;
}

struct lisp_fun_ret lisp_prim_goto(struct lisp_fun_arg args) {
  struct lisp_fun_ret ret = {0};
  return ret;
}

struct lisp_fun_ret lisp_prim_label(struct lisp_fun_arg args) {
  struct lisp_fun_ret ret = {0};
  return ret;
}

struct lisp_fun_ret lisp_prim_cond(struct lisp_fun_arg args) {
  struct lisp_fun_ret ret = {0};
  return ret;
}

struct lisp_fun_ret lisp_prim_behead(struct lisp_fun_arg args) {
  struct lisp_fun_ret ret = {0};
  return ret;
}

struct lisp_fun_ret lisp_prim_head(struct lisp_fun_arg args) {
  struct lisp_fun_ret ret = {0};
  return ret;
}

struct lisp_fun_ret lisp_prim_list(struct lisp_fun_arg args) {
  struct lisp_fun_ret ret = {0};
  return ret;
}

struct lisp_fun_ret lisp_prim_parent(struct lisp_fun_arg args) {
  struct lisp_fun_ret ret = {0};
  return ret;
}

struct lisp_fun_ret lisp_prim_type(struct lisp_fun_arg args) {
  struct lisp_fun_ret ret = {0};
  return ret;
}

struct lisp_fun_ret lisp_prim_set_right_child(struct lisp_fun_arg args) {
  struct lisp_fun_ret ret = {0};
  return ret;
}

struct lisp_fun_ret lisp_prim_left_child(struct lisp_fun_arg args) {
  struct lisp_fun_ret ret = {0};
  return ret;
}

struct lisp_fun_ret lisp_prim_set_sibbling(struct lisp_fun_arg args) {
  struct lisp_fun_ret ret = {0};
  return ret;
}

struct lisp_fun_ret lisp_prim_set_parent(struct lisp_fun_arg args) {
  struct lisp_fun_ret ret = {0};
  return ret;
};

struct lisp_fun_ret lisp_prim_set_left_child(struct lisp_fun_arg args) {
  struct lisp_fun_ret ret = {0};
  return ret;
}

struct lisp_fun_ret lisp_prim_ref(struct lisp_fun_arg args) {
  struct lisp_fun_ret ret = {0};
  return ret;
}

struct lisp_fun_ret lisp_prim_right_child(struct lisp_fun_arg args) {
  struct lisp_fun_ret ret = {0};
  return ret;
}

struct lisp_fun_ret lisp_prim_del(struct lisp_fun_arg args) {
  struct lisp_fun_ret ret = {0};
  return ret;
}
