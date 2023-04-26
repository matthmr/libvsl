#include <unistd.h>

// TODO: make this more verbose
// TODO: make this optional

#include "debug.h"
#include "err.h"

static const string_s emsg[] = {
  [EOK]          = ERR_STRING("libvsl", "EOK was sent"),
  [EIMBALANCED]  = ERR_STRING("libvsl: lexer", "imbalanced parens"),
  [EREAD]        = ERR_STRING("libvsl", "error while reading file"),
  [EFRONTEND]    = ERR_STRING("libvsl", "frontend error"),
  [EARGTOOBIG]   = ERR_STRING("libvsl: stack",
                              "too many arguments for function"),
  [EARGTOOSMALL] = ERR_STRING("libvsl: stack",
                              "missing arguments for function"),
  [EISNOTFUNC]   = ERR_STRING("libvsl: stack", "symbol is not a function"),
  [EIDTOOBIG]    = ERR_STRING("libvsl: lexer", "identifier is too big"),
  [EOOM]         = ERR_STRING("libvsl", "out-of-memory memory allocation"),
  [ENOTFOUND]    = ERR_STRING("libvsl: symtab", "symbol was not found"),
  [EHASHERR]     =
    ERR_STRING("libvsl: symtab",
               "symbol cannot be determined because of a hash error"),
  [EMMUNDER]     =
    ERR_STRING("libvsl: mm",
               "underflow prevetion caused by a faulty call of `mm_free'"),
  [EMMOVER]      =
    ERR_STRING("libvsl: mm",
               "overflow prevetion caused by a faulty call of `mm_mem' or"
               "`mm_malloc`"),
  [EFUCKINGHASH] = ERR_STRING("libvsl: symtab",
                              "internal error with the hash function"),
  [ENOTALLOWED]  = ERR_STRING("libvsl: lexer",
                              "character is not allowed in symbol"),
};

/**
   For now, this function is only called once and the entire
   program exits; there's no exception handling
 */
int err(enum ecode ecode) {
  static bool did_msg = false;
  static enum ecode pecode = 0;

  if (!did_msg) {
    did_msg = true;
    pecode  = ecode;

    /* we can still use the `err' module to backwards-comply with
       frontend implementations that don't; they just have to error
       with the `ERROR' macro and `err' will work normally
    */
    if (ecode >= ECODE_BEGIN && ecode <= ECODE_END) {
      write(STDERR_FILENO, emsg[ecode]._, emsg[ecode].size);
    }
  }

  DB_FMT("[ err ] bubbling up error %d", pecode);

  return (int) pecode;
}
