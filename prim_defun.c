#include "debug.h"
#include "prim.h"  // also includes `symtab.h'
#include "err.h"   // also includes `utils.h'

CLISP_PRIM_DEFUN(emptyisnil) {
  struct lisp_ret ret = {0};

  DB_MSG("  -> CALLING: lisp_prim_emptyisnil()");

  ret.master.mem.sym = &libvsl_prim_symtab[__CLISP_PRIM_SYM_NIL].sym;
  ret.master.typ     = __LISP_TYP_SYM;

  return ret;
}

CLISP_PRIM_DEFUN(set) {
  struct lisp_ret ret = {0};

  DB_MSG("  -> CALLING: lisp_prim_set()");

  return ret;
}

CLISP_PRIM_DEFUN(fun) {
  struct lisp_ret ret = {0};

  DB_MSG("  -> CALLING: lisp_prim_fun()");

  return ret;
}

CLISP_PRIM_DEFUN(lam) {
  struct lisp_ret ret = {0};

  DB_MSG("  -> CALLING: lisp_prim_lam()");

  return ret;
}

CLISP_PRIM_DEFUN(eval) {
  struct lisp_ret ret = {0};

  DB_MSG("  -> CALLING: lisp_prim_eval()");

  return ret;
}

CLISP_PRIM_DEFUN(quot) {
  struct lisp_ret ret = {0};

  DB_MSG("  -> CALLING: lisp_prim_quot()");

  return ret;
}

CLISP_PRIM_DEFUN(if) {
  struct lisp_ret ret = {0};

  DB_MSG("  -> CALLING: lisp_prim_if()");

  return ret;
}

CLISP_PRIM_DEFUN(eq) {
  struct lisp_ret ret = {0};

  DB_MSG("  -> CALLING: lisp_prim_eq()");

  return ret;
}

CLISP_PRIM_DEFUN(not) {
  struct lisp_ret ret = {0};

  DB_MSG("  -> CALLING: lisp_prim_not()");

  return ret;
}

CLISP_PRIM_DEFUN(block) {
  struct lisp_ret ret = {0};

  DB_MSG("  -> CALLING: lisp_prim_block()");

  return ret;
}

CLISP_PRIM_DEFUN(while) {
  struct lisp_ret ret = {0};

  DB_MSG("  -> CALLING: lisp_prim_while()");

  return ret;
}

CLISP_PRIM_DEFUN(break) {
  struct lisp_ret ret = {0};

  DB_MSG("  -> CALLING: lisp_prim_break()");

  return ret;
}

CLISP_PRIM_DEFUN(continue) {
  struct lisp_ret ret = {0};

  DB_MSG("  -> CALLING: lisp_prim_continue()");

  return ret;
}

CLISP_PRIM_DEFUN(return) {
  struct lisp_ret ret = {0};

  DB_MSG("  -> CALLING: lisp_prim_return()");

  return ret;
}

CLISP_PRIM_DEFUN(goto) {
  struct lisp_ret ret = {0};

  DB_MSG("  -> CALLING: lisp_prim_goto()");

  return ret;
}

CLISP_PRIM_DEFUN(label) {
  struct lisp_ret ret = {0};

  DB_MSG("  -> CALLING: lisp_prim_label()");

  return ret;
}

CLISP_PRIM_DEFUN(cond) {
  struct lisp_ret ret = {0};

  DB_MSG("  -> CALLING: lisp_prim_cond()");

  return ret;
}

CLISP_PRIM_DEFUN(parent) {
  struct lisp_ret ret = {0};

  DB_MSG("  -> CALLING: lisp_prim_parent()");

  return ret;
}

CLISP_PRIM_DEFUN(type) {
  struct lisp_ret ret = {0};

  DB_MSG("  -> CALLING: lisp_prim_type()");

  return ret;
}

CLISP_PRIM_DEFUN(set_right_child) {
  struct lisp_ret ret = {0};

  DB_MSG("  -> CALLING: lisp_prim_set_right_child()");

  return ret;
}

CLISP_PRIM_DEFUN(left_child) {
  struct lisp_ret ret = {0};

  DB_MSG("  -> CALLING: lisp_prim_left_child()");

  return ret;
}

CLISP_PRIM_DEFUN(set_parent) {
  struct lisp_ret ret = {0};

  DB_MSG("  -> CALLING: lisp_prim_set_parent()");

  return ret;
}

CLISP_PRIM_DEFUN(set_left_child) {
  struct lisp_ret ret = {0};

  DB_MSG("  -> CALLING: lisp_prim_set_left_child()");

  return ret;
}

CLISP_PRIM_DEFUN(ref) {
  struct lisp_ret ret = {0};

  DB_MSG("  -> CALLING: lisp_prim_ref()");

  return ret;
}

CLISP_PRIM_DEFUN(right_child) {
  struct lisp_ret ret = {0};

  DB_MSG("  -> CALLING: lisp_prim_right_child()");

  return ret;
}

CLISP_PRIM_DEFUN(del) {
  struct lisp_ret ret = {0};

  DB_MSG("  -> CALLING: lisp_prim_del()");

  return ret;
}

CLISP_PRIM_DEFUN(lexp) {
  struct lisp_ret ret = {0};

  DB_MSG("  -> CALLING: lisp_prim_lexp()");

  return ret;
}
