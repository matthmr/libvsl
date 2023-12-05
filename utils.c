#include "utils.h"
#include "mm.h"
#include "debug.h"

#include <unistd.h>

static inline uint size(char* str) {
  register uint ret = 0;

  while (*str && (ret++, str++));

  return ret;
}

static inline void* mcpy(void* from, uint fe_size, uint fm_size, void* to) {
  return NULL;
}

////////////////////////////////////////////////////////////////////////////////

char* from_string_ip(string_ip strip) {
  register int ret = 0;

  void* mem = mm_alloc(strip.idx*sizeof(*strip.str));
  assert(mem, OR_ERR());

  mem = mcpy(strip.str, sizeof(*strip.str), strip.idx, mem);

  done_for((mem = ret? NULL: mem));
}

string_ip to_string_ip(char* str) {
  return (string_ip) {
    .str = str,
    .idx = size(str)
  };
}

////////////////////////////////////////////////////////////////////////////////

static const string_s err_wtf =
  ERR_STRING("libvsl", "internal error to LIBVSL: did not issue error");

int eerr(const string_s* emsg, int ecode) {
  static bool did_msg = false;
  static int pecode = 0;

  if (!did_msg) {
    register string_s msg = emsg? emsg[ecode]: err_wtf;

    did_msg = true;
    pecode  = -1;

    write(STDERR_FILENO, msg._, msg.size);
  }

  DB_FMT("[ err ] bubbling up error %d", pecode);

  return (int) pecode;
}
