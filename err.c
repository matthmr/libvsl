#include <unistd.h>

// TODO: make this more verbose
// TODO: make this optional

#include "err.h"

static const string emsg[] = {
  [EIMBALANCED]     = ERR_STRING("lexer", "imbalanced parens"),
  [EREAD]           = ERR_STRING("libvsl", "error while reading file"),
  [EFRONTEND]       = ERR_STRING("libvsl", "frontend error"),
  [EARGTOOBIG]      =
    ERR_STRING("stack", "wrong number of arguments for function"),
  [EISNOTFUNC]      = ERR_STRING("libvsl", "symbol is not function"),
  [ENOHASHCHANGING] =
    ERR_STRING("libvsl", "hash-changing functions are not allowed"),
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
    write(STDERR_FILENO, emsg[ecode]._, emsg[ecode].size);
  }

  return (int) pecode;
}
