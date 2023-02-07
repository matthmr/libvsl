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
  EISNOTFUNC,
  ENOHASHCHANGING,
  EIDTOOBIG,
  EOOM,
  ENOTFOUND,
  EHASHERR,
};

#  define ECODE_LEN (EHASHERR)
#  define ERROR (-1u)

#  if ECODE_LEN == ERROR
#    error "[ !! ] libvsl: Too many errors (how did this even happen?)"
#  endif

int err(enum ecode code);

#endif
