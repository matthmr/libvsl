#include "debug.h"
#include "prim.h"  // also includes `symtab.h'
#include "err.h"   // also includes `utils.h'

#include <unistd.h>

static const string_s femsg[] = {
  [FENOARGP0] = ERR_STRING("libvsl", "argp[0] couldn't be gotten"),
};

struct clisp_sym libvsl_prim_funtab[] = {
  // turing completion
  CLISP_PRIM_DECLFUN("set", set,           /* (set 'x y) */
                     2, 2, 1, 1),
  CLISP_PRIM_DECLFUN("del", del,           /* (del 'x) */
                     1, 1, 1, 1),
  CLISP_PRIM_DECLFUN("ref", ref,           /* (ref 'x y) */
                     2, 2, 1, 1),
  CLISP_PRIM_DECLFUN("fun", fun,           /* (fun 'x (...) ...) */
                     2, INFINITY, 1, INFINITY),
  CLISP_PRIM_DECLFUN("lam", lam,           /* (lam (...) ...) */
                     1, INFINITY, 1, INFINITY),
  CLISP_PRIM_DECLFUN("if", if,             /* (if x y z?) */
                     2, 3, 2, 3),
  CLISP_PRIM_DECLFUN("eq", eq,             /* (eq x y) */
                     2, 2, 0, 0),
  CLISP_PRIM_DECLFUN("not", not,           /* (not x) */
                     1, 1, 0, 0),
  CLISP_PRIM_DECLFUN("block", block,       /* (block ...) */
                     0, INFINITY, 0, 0),
  CLISP_PRIM_DECLFUN("while", while,       /* (while x y?) */
                     1, 2, 0, 0),
  CLISP_PRIM_DECLFUN("break", break,       /* (break) */
                     0, 0, 0, 0),
  CLISP_PRIM_DECLFUN("continue", continue, /* (continue) */
                     0, 0, 0, 0),
  CLISP_PRIM_DECLFUN("return", return,     /* (return x?) */
                     0, 1, 0, 0),
  CLISP_PRIM_DECLFUN("goto", goto,         /* (goto 'x) */
                     1, 1, 1, 1),
  CLISP_PRIM_DECLFUN("label", label,       /* (label 'x) */
                     1, 1, 1, 1),
  CLISP_PRIM_DECLFUN("cond", cond,         /* (cond (x y)...) */
                     0, INFINITY, 0, INFINITY),

  // lisp-specific
  CLISP_PRIM_DECLFUN("left-child", left_child,
                     1, 1, 1, 1),                   /* (left-child 'x) */
  CLISP_PRIM_DECLFUN("right-child", right_child,
                     1, 1, 1, 1),                   /* (right-child 'x) */
  CLISP_PRIM_DECLFUN("parent", parent,
                     1, 1, 1, 1),                   /* (parent 'x) */
  CLISP_PRIM_DECLFUN("set-left-child", set_left_child,
                     2, 2, 1, 1),                   /* (set-left-child 'x y) */
  CLISP_PRIM_DECLFUN("set-right-child", set_right_child,
                     2, 2, 1, 1),                   /* (set-right-child 'x y) */
  CLISP_PRIM_DECLFUN("set-parent", set_parent,
                     2, 2, 1, 1),                   /* (set-parent 'x y) */
  CLISP_PRIM_DECLFUN("eval", eval,
                     1, 1, 1, 1),                   /* (eval 'x) */
  CLISP_PRIM_DECLFUN("quot", quot,
                     1, 1, 1, 1),                   /* (quot 'x) */
  CLISP_PRIM_DECLFUN("type", type,
                     1, 1, 1, 1),                   /* (type 'x) */
  CLISP_PRIM_DECLFUN("lexp", lexp,
                     1, 1, 1, 1),                   /* (lexp 'x) */

  // () is nil
  CLISP_PRIM_DECLFUN("", emptyisnil,
                     0, 0, 0, 0),                   /* () */

  // EOL
  CLISP_PRIM_DECLFUN_END(),
};

struct clisp_sym libvsl_prim_symtab[] = {
  // booleans
  [0] = CLISP_PRIM_DECLSYM("t", NULL),
  CLISP_PRIM_DECLSYM("nil", NULL),

  // types
  CLISP_PRIM_DECLSYM("@sym", NULL),
  CLISP_PRIM_DECLSYM("@sexp", NULL),
  CLISP_PRIM_DECLSYM("@lexp", NULL),

  // EOL
  CLISP_PRIM_DECLSYM_END(),
};

struct clisp_tab __libvsl_prim_itertab[] = {
  CLISP_PRIM_DECLTAB(libvsl_prim_funtab, __LISP_TYP_FUN),
  CLISP_PRIM_DECLTAB(libvsl_prim_symtab, __LISP_TYP_SYM),
  CLISP_PRIM_DECLTAB_END(),
};

static struct clisp_tab* libvsl_prim_itertab = __libvsl_prim_itertab;
static bool libvsl_keep_prim  = true;
static bool libvsl_init_other = false;

/**
   @local: iteration table to initialize
   @keep:  boolean to guard the initialization of LIBVSL's primitive table
      true:  initialize it along with the local one
      false: don't ¯\(ツ)/¯
 */
void lisp_prim_setlocal(struct clisp_tab* local_itertab, bool keep) {
  libvsl_prim_itertab = local_itertab;
  libvsl_keep_prim    = keep;
  libvsl_init_other   = true;
  lisp_prim_init();
}

int lisp_prim_init(void) {
  register int       ret = 0;
  struct lisp_sym* ret_t = NULL;

  struct clisp_tab* ctab = NULL;
  struct clisp_sym* csym = NULL;

  if (!libvsl_init_other) {
    assert(libvsl_keep_prim, 0);
  }

  // the user can also pass in `NULL' and expect us to not initialize
  assert(libvsl_prim_itertab, 0);

  for (ctab = libvsl_prim_itertab; ctab->tab; ++ctab) {
    for (csym = ctab->tab; csym->str; ++csym) {
      struct lisp_hash_ret sret = hash_str(csym->str);
      assert(sret.slave == 0, OR_ERR());

      csym->sym.hash = sret.master;
      csym->sym.typ  = ctab->typ;

      ret_t = lisp_symtab_set(csym->sym);
      assert(ret_t, OR_ERR());
    }
  }

  if (libvsl_init_other) {
    libvsl_prim_itertab = __libvsl_prim_itertab;
    libvsl_init_other   = false;
  }

  done_for(ret);
}

////////////////////////////////////////////////////////////////////////////////

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
