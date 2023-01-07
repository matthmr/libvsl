#ifndef LOCK_LIBVSL
#  define LOCK_LIBVSL

typedef int (*Frontend)(void);
extern Frontend frontend;

#  define VSL_FRONTEND(x) \
  Frontend frontend = &x

#  ifdef LIBVSL_BACKEND

#    include <unistd.h>

#    include "lex.h"

#  endif

#endif
