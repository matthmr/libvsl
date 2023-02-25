#define EXTERN_PRIM_FUNC

#include <unistd.h>

#include "debug.h"
#include "prim.h"  // also includes `symtab.h'
#include "err.h"   // also includes `utils.h'

static const string_s femsg[] = {
  [FENOARGP0] = ERR_STRING("libvsl", "argp[0] couldn't be gotten"),
};

const struct clisp_sym vsl_primtab[] = {
  // () is nil
  CLISP_PRIM_FUN("", 0, 0, 0, 0), // ()

  // turing completion
  CLISP_PRIM_FUN("set", 2, 2, 1, 1), // (set 'x y)
  CLISP_PRIM_FUN("del", 1, 1, 1, 1), // (del 'x)
  CLISP_PRIM_FUN("ref", 1, 2, 1, 2), // (ref 'x 'y?)
  CLISP_PRIM_FUN("fun", 2, INFINITY, 1, INFINITY), // (fun 'x (...) ...)
  CLISP_PRIM_FUN("lam", 1, INFINITY, 1, INFINITY), // (lam (...) ...)
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
  CLISP_PRIM_FUN("set-parent", 2, 2, 1, 1), // (set-parent 'x y)
  CLISP_PRIM_FUN("eval", 1, 1, 1, 1), // (eval 'x)
  CLISP_PRIM_FUN("quot", 1, 1, 1, 1), // (quot 'x)
  CLISP_PRIM_FUN("type", 1, 1, 1, 1), // (type 'x)

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

int ferr(enum fecode fecode) {
  static bool did_msg = false;
  static enum fecode pfecode = 0;

  if (!did_msg) {
    did_msg = true;
    pfecode = fecode;

    if (fecode >= FECODE_BEGIN && fecode <= FECODE_END) {
      write(STDERR_FILENO, femsg[fecode]._, femsg[fecode].size);
    }
  }

  return (int) pfecode;
}

////////////////////////////////////////////////////////////////////////////////

CLISP_PRIM() {
  struct lisp_fun_ret ret = {0};

  DB_MSG(" -> ()");

  ret.master.typ = __LISP_VAR_SYM;
  ret.master.mem.sym = argp[0].mem.sym;

  done_for(ret);
}

CLISP_PRIM(set) {
  struct lisp_fun_ret ret = {0};

  DB_MSG(" -> lisp_prim_set()");

  return ret;
}

CLISP_PRIM(fun) {
  struct lisp_fun_ret ret = {0};

  DB_MSG(" -> lisp_prim_fun()");

  return ret;
}

CLISP_PRIM(lam) {
  struct lisp_fun_ret ret = {0};

  DB_MSG(" -> lisp_prim_lam()");

  return ret;
}

CLISP_PRIM(eval) {
  struct lisp_fun_ret ret = {0};

  DB_MSG(" -> lisp_prim_eval()");

  return ret;
}

CLISP_PRIM(quot) {
  struct lisp_fun_ret ret = {0};

  DB_MSG(" -> lisp_prim_quot()");

  return ret;
}

CLISP_PRIM(if) {
  struct lisp_fun_ret ret = {0};

  DB_MSG(" -> lisp_prim_if()");

  return ret;
}

CLISP_PRIM(eq) {
  struct lisp_fun_ret ret = {0};

  DB_MSG(" -> lisp_prim_eq()");

  return ret;
}

CLISP_PRIM(not) {
  struct lisp_fun_ret ret = {0};

  DB_MSG(" -> lisp_prim_not()");

  return ret;
}

CLISP_PRIM(block) {
  struct lisp_fun_ret ret = {0};

  DB_MSG(" -> lisp_prim_block()");

  return ret;
}

CLISP_PRIM(while) {
  struct lisp_fun_ret ret = {0};

  DB_MSG(" -> lisp_prim_while()");

  return ret;
}

CLISP_PRIM(break) {
  struct lisp_fun_ret ret = {0};

  DB_MSG(" -> lisp_prim_break()");

  return ret;
}

CLISP_PRIM(continue) {
  struct lisp_fun_ret ret = {0};

  DB_MSG(" -> lisp_prim_continue()");

  return ret;
}

CLISP_PRIM(return) {
  struct lisp_fun_ret ret = {0};

  DB_MSG(" -> lisp_prim_return()");

  return ret;
}

CLISP_PRIM(goto) {
  struct lisp_fun_ret ret = {0};

  DB_MSG(" -> lisp_prim_goto()");

  return ret;
}

CLISP_PRIM(label) {
  struct lisp_fun_ret ret = {0};

  DB_MSG(" -> lisp_prim_label()");

  return ret;
}

CLISP_PRIM(cond) {
  struct lisp_fun_ret ret = {0};

  DB_MSG(" -> lisp_prim_cond()");

  return ret;
}

CLISP_PRIM(parent) {
  struct lisp_fun_ret ret = {0};

  DB_MSG(" -> lisp_prim_parent()");

  return ret;
}

CLISP_PRIM(type) {
  struct lisp_fun_ret ret = {0};

  DB_MSG(" -> lisp_prim_type()");

  return ret;
}

CLISP_PRIM(set_right_child) {
  struct lisp_fun_ret ret = {0};

  DB_MSG(" -> lisp_prim_set_right_child()");

  return ret;
}

CLISP_PRIM(left_child) {
  struct lisp_fun_ret ret = {0};

  DB_MSG(" -> lisp_prim_left_child()");

  return ret;
}

CLISP_PRIM(set_parent) {
  struct lisp_fun_ret ret = {0};

  DB_MSG(" -> lisp_prim_set_parent()");

  return ret;
}

CLISP_PRIM(set_left_child) {
  struct lisp_fun_ret ret = {0};

  DB_MSG(" -> lisp_prim_set_left_child()");

  return ret;
}

CLISP_PRIM(ref) {
  struct lisp_fun_ret ret = {0};

  DB_MSG(" -> lisp_prim_ref()");

  return ret;
}

CLISP_PRIM(right_child) {
  struct lisp_fun_ret ret = {0};

  DB_MSG(" -> lisp_prim_right_child()");

  return ret;
}

CLISP_PRIM(del) {
  struct lisp_fun_ret ret = {0};

  DB_MSG(" -> lisp_prim_del()");

  return ret;
}
