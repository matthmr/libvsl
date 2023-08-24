/**
   Main interface for LIBVSL. Used for both frontend and backend
*/

#ifndef LOCK_LIBVSL
#  define LOCK_LIBVSL

#  include "err.h"
#  include "utils.h"

typedef int (*Frontend) (void);

extern Frontend frontend;

#  define LIBVSL_DECL_FRONTEND(x) int x (void)
#  define LIBVSL_USE_FRONTEND(x) Frontend frontend = &x
#  define LIBVSL_USE_STUB(x) Frontend frontend = NULL
#  define LIBVSL_USE_TAB(x) clisp_init(tab)

#  ifdef LIBVSL_BACKEND
#    include <stdlib.h>
#    include <unistd.h>

#    include "lex.h"  // also includes `symtab.h', `stack.h'
#    include "mm.h"   // also includes `utils.h'
#  else
#    include "clisp.h"
#  endif

#endif
