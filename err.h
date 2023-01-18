#ifndef LOCK_ERR
#  define LOCK_ERR

#  include "utils.h"

#  define ERR_STRING(mod, msg) \
  STRING("[ !! ] " mod ": " msg)

#  define OR_ERR(x) \
  err(0)

enum ecode {
  EOK = 0,
  EIMBALANCED,
  EREAD,
  EFRONTEND,
  EARGTOOBIG,
  EISNOTFUNC,
  ENOHASHCHANGING,
};

int err(enum ecode code);

#endif
