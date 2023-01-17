#ifndef LOCK_LIBVSL
#  define LOCK_LIBVSL

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

#    define  LOCK_SEXP_INTERNALS
#    include "sexp.h"
#    include "lex.h"

#  endif

#endif
