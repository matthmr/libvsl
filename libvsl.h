/** Main interface for LIBVSL. Used for both frontend and backend */

#ifndef LOCK_LIBVSL
#  define LOCK_LIBVSL

#  include "stack.h"
#  include "mm.h"
#include "utils.h"

#  ifdef LIBVSL_BACKEND
#    include <stdlib.h>
#    include <unistd.h>
#    include "lex.h"
#  else

// it's a frontend implementation: give the programmer helpers to define their
// functions/variables. Former `clisp.h'/`clisp.c' files prior to and including
// commit `c45d56' (part of `master')

#  define OBJ(x) x##_obj

#  define CLISP_OBJ(__obj_typ,__obj_ass) \
  {._.__obj_ass, .typ = __obj_typ}

#  define CLISP_OBJ_GEN(__obj_typ,__obj_dat) \
  {._.gen = {.typ = __obj_typ, .dat = __obj_dat}, .typ = __LISP_OBJ_FOREIGN}

#  define CLISP_DEFUN(__fun_name) \
  struct lisp_ret __fun_name \
  (struct lisp_sexp* argp, const uint argv, struct lisp_symtab* envp)

#  define CLISP_DEF(__sym_name,__sym_obj) \
  {                    \
    .str = __sym_name, \
    .obj = __sym_obj,  \
  }

#  define CLISP_TAB(__ctab_name) \
  struct lisp_sym_dat __ctab_name[] =

///

/** Inits a CLISP table @ctab of size @ctab_size within another table @with, or
    creates a new table if @with is NULL, returning the table after processing
*/
static struct lisp_symtab*
clisp_init(struct lisp_symtab* with, struct lisp_sym_dat* ctab,
           uint ctab_size) {
  register int       ret = 0;
  struct lisp_sym* ret_t = NULL;

  if (!with) {
    with = mm_alloc(sizeof(*with));
    assert(with, OR_ERR());

    with->root = NULL;
    with->tab  = (struct lisp_symtab_node) {
      .hash_idx = 0,
      .self     = {0},
      .left     = NULL,
      .right    = NULL,
    };
  }

  for (uint i = 0; i < ctab_size; i++) {
    struct lisp_sym_dat csym = ctab[i];

    ret_t = lisp_symtab_set(to_string_ip((char*)csym.str), csym.obj, with);
    assert(ret_t, OR_ERR());
  }

  done_for((ret? NULL: with));
}

#  endif

/** Init LIBVSL internals */
int libvsl_init(void);

/** Top-level of LIBVSL, taking as input the bytestream from STDIN. Defers lex
    handling to the stack after the first paren is opened. Returns when the
    last paren of toplevel is closed. Ignores return values for top-level
    expressions, including symbols */
int lisp_toplevel_lex(struct lisp_symtab* envp);

#endif
