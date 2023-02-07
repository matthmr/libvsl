#include <unistd.h>

// TODO: make this more verbose
// TODO: make this optional

#include "utils.h"
#include "err.h"

static const string_s emsg[] = {
  [EIMBALANCED]     = ERR_STRING("lexer", "imbalanced parens"),
  [EREAD]           = ERR_STRING("libvsl", "error while reading file"),
  [EFRONTEND]       = ERR_STRING("libvsl", "frontend error"),
  [EARGTOOBIG]      =
    ERR_STRING("stack", "wrong number of arguments for function"),
  [EISNOTFUNC]      = ERR_STRING("libvsl", "symbol is not function"),
  [ENOHASHCHANGING] =
    ERR_STRING("libvsl", "hash-changing functions are not allowed"),
  [EIDTOOBIG]       = ERR_STRING("libvsl", "identifier is too big"),
  [EOOM]            = ERR_STRING("libvsl", "out-of-memory malloc"),
  [ENOTFOUND]       = ERR_STRING("symtab", "symbol was not found"),
  [EHASHERR]        =
    ERR_STRING("symtab", "symbol cannot be determined because of a hash error"),
};

/** for now, this function is only called once and the entire
    program exits; there's no exception handling
 */
int err(enum ecode ecode) {
  static bool did_msg = false;
  static enum ecode pecode = 0;

  if (!did_msg) {
    did_msg = true;
    pecode  = ecode;

    /** we can still use the `err' module to backwards-comply with
        frontend implementations that don't; they just have to error
        with the `ERROR' macro and `err' will work normally */
    if (ecode <= ECODE_LEN) {
      write(STDERR_FILENO, emsg[ecode]._, emsg[ecode].size);
    }
  }

  return (int) pecode;
}
