#ifndef LOCK_LIBVSL
#  define LOCK_LIBVSL

#  include "err.h"
#  include "utils.h"

typedef int (*Frontend) (void);

extern Frontend frontend;

#  define LIBVSL_FRONTEND(x)      Frontend frontend = &x
#  define LIBVSL_FRONTEND_STUB(x) Frontend frontend = (Frontend) NULL;

#  ifdef LIBVSL_BACKEND

#    include <stdlib.h>
#    include <unistd.h>

#    include "prim.h" // also includes `lisp.h'
#    include "lex.h"  // also includes `symtab.h', `stack.h'
#    include "mm.h"   // also includes `utils.h'

#  endif

#endif
