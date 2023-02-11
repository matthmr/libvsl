#ifndef LOCK_ERR
#  define LOCK_ERR

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
  ENOHASHCHANGING,
  EIDTOOBIG,
  EOOM,
  ENOTFOUND,
  EHASHERR,
  ENOTALLOWED,
};

#  define ECODE_BEGIN (EOK)
#  define ECODE_END   (ENOTALLOWED)
#  define ERROR       (-1u)

#  if ECODE_LEN == ERROR
#    error "[ !! ] libvsl: Too many errors (how did this even happen?)"
#  endif

int err(enum ecode code);

#endif
