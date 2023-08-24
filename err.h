#ifndef LOCK_ERR
#  define LOCK_ERR

#  ifndef LOCK_ERR_UTILS
#    include "utils.h"
#  endif

#  define ERR_STRING(mod, msg) \
  STRING("[ !! ] " mod ": " msg)

#  define ERR_MSG(mod, msg) \
  MSG("[ !! ] " mod ": " msg "\n")

#  define OR_ERR(x) \
  eerr(NULL, 0)

#  define ECODE(...) enum ecode {EOK = 0, __VA_ARGS__}
#  define EMSG static const string_s emsg[] =
#  define MK_ERR static int err(enum ecode ecode) {return eerr(emsg, ecode);}

int eerr(const string_s* emsg, int code);

#endif
