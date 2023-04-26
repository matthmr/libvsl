// PRIMVSL: a primitive implementation of LIBVSL

#include "debug.h"
#include "prim.h"  // also includes `symtab.h'
#include "err.h"   // also includes `utils.h'

CLISP_PRIM_DEFUN(emptyisnil) {
  struct lisp_ret ret_t = {0};

  DB_MSG("  -> primvsl: lisp_prim_emptyisnil()");

  ret_t.master.mem.sym = &libvsl_prim_symtab[__CLISP_PRIM_SYM_NIL].sym;
  ret_t.master.typ     = __LISP_TYP_SYM;

  return ret_t;
}

CLISP_PRIM_DEFUN(set) {
  struct lisp_ret ret_t = {0};

  DB_MSG("  -> primvsl: lisp_prim_set()");

  return ret_t;
}

CLISP_PRIM_DEFUN(fun) {
  struct lisp_ret ret_t = {0};

  DB_MSG("  -> primvsl: lisp_prim_fun()");

  return ret_t;
}

CLISP_PRIM_DEFUN(lam) {
  struct lisp_ret ret_t = {0};

  DB_MSG("  -> primvsl: lisp_prim_lam()");

  return ret_t;
}

CLISP_PRIM_DEFUN(eval) {
  struct lisp_ret ret_t = {0};

  DB_MSG("  -> primvsl: lisp_prim_eval()");

  return ret_t;
}

CLISP_PRIM_DEFUN(quot) {
  register int ret      = __LISP_OK;
  struct lisp_ret ret_t = {0};

  DB_MSG("  -> primvsl: lisp_prim_quot()");

  ret_t.master = argp[1];

  return ret_t.slave = ret, ret_t;
}

CLISP_PRIM_DEFUN(if) {
  struct lisp_ret ret_t = {0};

  DB_MSG("  -> primvsl: lisp_prim_if()");

  return ret_t;
}

CLISP_PRIM_DEFUN(eq) {
  struct lisp_ret ret_t = {0};

  DB_MSG("  -> primvsl: lisp_prim_eq()");

  return ret_t;
}

CLISP_PRIM_DEFUN(not) {
  struct lisp_ret ret_t = {0};

  DB_MSG("  -> primvsl: lisp_prim_not()");

  return ret_t;
}

CLISP_PRIM_DEFUN(block) {
  struct lisp_ret ret_t = {0};

  DB_MSG("  -> primvsl: lisp_prim_block()");

  return ret_t;
}

CLISP_PRIM_DEFUN(while) {
  struct lisp_ret ret_t = {0};

  DB_MSG("  -> primvsl: lisp_prim_while()");

  return ret_t;
}

CLISP_PRIM_DEFUN(break) {
  struct lisp_ret ret_t = {0};

  DB_MSG("  -> primvsl: lisp_prim_break()");

  return ret_t;
}

CLISP_PRIM_DEFUN(continue) {
  struct lisp_ret ret_t = {0};

  DB_MSG("  -> primvsl: lisp_prim_continue()");

  return ret_t;
}

CLISP_PRIM_DEFUN(return) {
  struct lisp_ret ret_t = {0};

  DB_MSG("  -> primvsl: lisp_prim_return()");

  return ret_t;
}

CLISP_PRIM_DEFUN(goto) {
  struct lisp_ret ret_t = {0};

  DB_MSG("  -> primvsl: lisp_prim_goto()");

  return ret_t;
}

CLISP_PRIM_DEFUN(label) {
  struct lisp_ret ret_t = {0};

  DB_MSG("  -> primvsl: lisp_prim_label()");

  return ret_t;
}

CLISP_PRIM_DEFUN(cond) {
  struct lisp_ret ret_t = {0};

  DB_MSG("  -> primvsl: lisp_prim_cond()");

  return ret_t;
}

CLISP_PRIM_DEFUN(parent) {
  struct lisp_ret ret_t = {0};

  DB_MSG("  -> primvsl: lisp_prim_parent()");

  return ret_t;
}

CLISP_PRIM_DEFUN(type) {
  struct lisp_ret ret_t = {0};

  DB_MSG("  -> primvsl: lisp_prim_type()");

  return ret_t;
}

CLISP_PRIM_DEFUN(set_right_child) {
  struct lisp_ret ret_t = {0};

  DB_MSG("  -> primvsl: lisp_prim_set_right_child()");

  return ret_t;
}

CLISP_PRIM_DEFUN(left_child) {
  struct lisp_ret ret_t = {0};

  DB_MSG("  -> primvsl: lisp_prim_left_child()");

  return ret_t;
}

CLISP_PRIM_DEFUN(set_parent) {
  struct lisp_ret ret_t = {0};

  DB_MSG("  -> primvsl: lisp_prim_set_parent()");

  return ret_t;
}

CLISP_PRIM_DEFUN(set_left_child) {
  struct lisp_ret ret_t = {0};

  DB_MSG("  -> primvsl: lisp_prim_set_left_child()");

  return ret_t;
}

CLISP_PRIM_DEFUN(ref) {
  struct lisp_ret ret_t = {0};

  DB_MSG("  -> primvsl: lisp_prim_ref()");

  return ret_t;
}

CLISP_PRIM_DEFUN(right_child) {
  struct lisp_ret ret_t = {0};

  DB_MSG("  -> primvsl: lisp_prim_right_child()");

  return ret_t;
}

CLISP_PRIM_DEFUN(del) {
  struct lisp_ret ret_t = {0};

  DB_MSG("  -> primvsl: lisp_prim_del()");

  return ret_t;
}

CLISP_PRIM_DEFUN(lexp) {
  struct lisp_ret ret_t = {0};

  DB_MSG("  -> primvsl: lisp_prim_lexp()");

  return ret_t;
}
