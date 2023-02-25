#ifndef LOCK_DEBUG
#  define LOCK_DEBUG

#  ifdef DEBUG

#    include <stdio.h>
#    include <unistd.h>

#    define DB_MSG(msg) \
       fputs(msg "\n", stderr)
#    define DB_BYT(byt) \
       write(STDERR_FILENO, byt, sizeof(*byt))
#    define DB_MSG_SAFE(msg) \
       write(STDERR_FILENO, msg "\n", sizeof(msg))
#    define DB_FMT(msg, ...) \
       fprintf(stderr, msg "\n", __VA_ARGS__)

#  else

#    define DB_MSG(x)
#    define DB_BYT(x)
#    define DB_MSG_SAFE(msg)
#    define DB_FMT(x, ...)

#  endif

#endif
