#ifndef LOCK_DEBUG
#  define LOCK_DEBUG

#  ifdef DEBUG

#    include <stdio.h>
#    include <unistd.h>

#    define DB_MSG(msg) \
       fputs(msg "\n", stderr)
#    define DB_BYT(byt) \
       write(STDERR_FILENO, byt, sizeof(byt)/sizeof(*byt))
#    define DB_NBYT(byt, n) \
       write(STDERR_FILENO, byt, n)
#    define DB_FMT(msg, ...) \
       fprintf(stderr, msg "\n", __VA_ARGS__)

#  else

#    define DB_MSG(x)
#    define DB_BYT(x)
#    define DB_NBYT(x)
#    define DB_FMT(x, ...)

#  endif

#endif
