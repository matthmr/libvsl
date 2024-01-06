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
#    include "symtab.h"
#  else

// it's a frontend implementation: give the programmer helpers to define their
// functions/variables. Former `clisp.h'/`clisp.c' files prior to and including
// commit `c45d56' (part of `master')

#  define OBJ(x) x##_obj

#  define CLISP_OBJ(__obj_typ,__obj_ass,...) \
  {._.__obj_ass, .typ = __obj_typ, .m_typ = __LISP_OBJ_C, .refs = 1, __VA_ARGS__}

#  define CLISP_OBJ_GEN(__obj_typ,__obj_dat,...) \
  {._.gen = {.typ = __obj_typ, .dat = __obj_dat}, .typ = __LISP_OBJ_FOREIGN, \
   .m_typ = __LISP_OBJ_C, .refs = 1, __VA_ARGS__}

#  define CLISP_DEFUN(__fun_name) \
  struct lisp_ret __fun_name \
  (struct lisp_sexp* argp, const uint argv, struct lisp_symtab* envp)

#  define CLISP_DEF(__sym_name,__sym_obj) \
  {                    \
    .str = __sym_name, \
    .obj = &__sym_obj,  \
  }

#  define CLISP_TAB(__ctab_name) \
  struct lisp_sym __ctab_name[] =

///

/** Inits a CLISP table @ctab of size @ctab_size within another table @with, or
    creates a new table if @with is NULL, returning the table after processing
*/
static struct lisp_symtab*
clisp_init(struct lisp_symtab* with, struct lisp_sym* ctab,
           uint ctab_size) {
  register int       ret = 0;
  struct lisp_sym* ret_t = NULL;

  if (!with) {
    with = mm_alloc(sizeof(*with));
    assert(with, OR_ERR());

    with->root = NULL;
    with->_    = (struct lisp_symtab_if) {0};
  }

  for (uint i = 0; i < ctab_size; i++) {
    struct lisp_sym csym = ctab[i];

    ret_t = lisp_symtab_set(to_string_ip((char*)csym.str), csym.obj, with, 0);
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

/** Top-level of LIBVSL, taking as input a SEXP rooted by @exp */
int lisp_toplevel_sexp(struct lisp_sexp* exp, struct lisp_symtab* envp);

/** Top-level of LIBVSL, taking as input a string @str */
int lisp_toplevel_str(char* str, struct lisp_symtab* envp);

#endif
