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
  err(0)

enum ecode {
  EOK = 0,

  EIMBALANCED,
  EREAD,
  EFRONTEND,
  EARGTOOBIG,
  EARGTOOSMALL,
  EISNOTFUNC,
  EIDTOOBIG,
  EOOM,
  ENOTFOUND,
  EHASHERR,
  ENOTALLOWED,
  EMMUNDER,
  EMMOVER,
  EFUCKINGHASH,

  EOK_END,
};

#  define ECODE_BEGIN (EOK)
#  define ECODE_END   (EOK_END)
#  define ERROR       (-1u)

#  if ECODE_LEN == ERROR
#    error "[ !! ] libvsl: Too many errors (how did this even happen?)"
#  endif

int err(enum ecode code);

#endif
