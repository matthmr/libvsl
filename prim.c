#include "prim.h"

const struct clisp_sym vsl_primtab[] = {
  // turing completion + code-data switching
  CLISP_PRIM_FUN("set", 2, 0, 1, 0), // (set 'x y)
  CLISP_PRIM_FUN("fun", 2, INFINITY, 1, 2), // (fun 'x (...) ...)
  CLISP_PRIM_FUN("eval", 1, 0, 1, 0), // (eval ...)
  CLISP_PRIM_FUN("quot", 1, 0, 1, 0), // (quot ...)
  CLISP_PRIM_FUN("if", 2, 3, 0, 0), // (if x y z?)
  CLISP_PRIM_FUN("eq", 2, 0, 0, 0), // (eq x y)
  CLISP_PRIM_FUN("not", 1, 0, 0, 0), // (not x)
  CLISP_PRIM_FUN("block", 0, INFINITY, 0, 0), // (block ...)
  CLISP_PRIM_FUN("while", 1, 2, 0, 0), // (while x y?)
  CLISP_PRIM_FUN("break", 0, 0, 0, 0), // (break)
  CLISP_PRIM_FUN("continue", 0, 0, 0, 0), // (continue)
  CLISP_PRIM_FUN("return", 0, 1, 0, 0), // (return x?)
  CLISP_PRIM_FUN("goto", 1, 0, 1, 0), // (goto 'x)
  CLISP_PRIM_FUN("label", 1, 0, 1, 0), // (label 'x)
  CLISP_PRIM_FUN("cond", 1, INFINITY, 1, INFINITY), // (cond (x y)...)

  // lisp-specific
  CLISP_PRIM_FUN("behead", 1, 0, 0, 0), // (behead x)
  CLISP_PRIM_FUN("head", 1, 0, 0, 0), // (head x)
  CLISP_PRIM_FUN("list", 0, INFINITY, 0, INFINITY), // (list ...)

  // booleans
  CLISP_PRIM_SYM("t", "NULL"),
  CLISP_PRIM_SYM("nil", "NULL"),

  // EOL
  {.str = NULL},
};

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
