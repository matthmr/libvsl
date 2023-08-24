#include <unistd.h>

// TODO: make this more verbose
// TODO: make this optional

#include "debug.h"

#include "err.h"

static const string_s err_wtf =
  ERR_STRING("libvsl", "internal error to LIBVSL: did not issue error");

/**
   For now, this function is only called once and the entire
   program exits; there's no exception handling
 */
int eerr(const string_s* emsg, int ecode) {
  static bool did_msg = false;
  static int pecode = 0;

  if (!did_msg) {
    register string_s msg = emsg? emsg[ecode]: err_wtf;

    did_msg = true;
    pecode  = ecode;

    write(STDERR_FILENO, msg._, msg.size);
  }

  DB_FMT("[ err ] bubbling up error %d", pecode);

  return (int) pecode;
}
