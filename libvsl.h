#ifndef LOCK_LIBVSL
#  define LOCK_LIBVSL

#  include "err.h"
#  include "utils.h"

typedef int (*Frontend)(void);
extern Frontend frontend;

#  define LIBVSL_FRONTEND(x) \
  Frontend frontend = &x

#  define LIBVSL_FRONTEND_STUB(x)     \
  Frontend frontend = (Frontend) NULL;

// TODO: we probably need this for the frontend but i'm way to tired to fix it
//#  include "symtab.h"

#  ifdef LIBVSL_BACKEND

#    include <stdlib.h>
#    include <unistd.h>

#    define LOCK_POOL_DEF

#    include "sexp.h" // also includes `symtab.h'
#    include "lex.h"  // also includes `symtab.h', `stack.h'

#  endif

#endif
