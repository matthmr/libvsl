#ifndef LOCK_DEBUG
#  define LOCK_DEBUG

#  ifdef DEBUG

#    include <stdio.h>
#    define DB_MSG(msg) \
       fputs(msg "\n", stderr)
#    define DB_FMT(msg, ...) \
       fprintf(stderr, msg "\n", __VA_ARGS__)

#  else

#    define DB_MSG(x)
#    define DB_FMT(x, ...)

#  endif

#endif
