#include "debug.h"

#include "err.h"   // also includes `utils.h'
#include "clisp.h" // also includes `symtab.h'

#include <unistd.h>

static const string_s femsg[] = {
  [FENOARGP0] = ERR_STRING("libvsl", "argp[0] couldn't be gotten"),
};

/**
   Inits a CLISP table @tab in the global symtab
*/
int clisp_init(struct clisp_sym* ctab) {
  register int       ret = 0;
  struct lisp_sym* ret_t = NULL;

  for (struct clisp_sym* csym = ctab; csym->str; ++csym) {
    struct lisp_hash_ret sret = hash_str(csym->str);
    assert(sret.slave == 0, OR_ERR());

    csym->sym.hash = sret.master;

    ret_t = lisp_symtab_set(csym->sym);
    assert(ret_t, OR_ERR());
  }

  done_for(ret);
}

////////////////////////////////////////////////////////////////////////////////

int ferr(enum fecode fecode) {
  static bool did_msg = false;
  static enum fecode pfecode = 0;

  if (!did_msg) {
    did_msg = true;
    pfecode = fecode;

    if (fecode >= FECODE_BEGIN && fecode <= FECODE_END) {
      write(STDERR_FILENO, femsg[fecode]._, femsg[fecode].size);
    }
  }

  return (int) pfecode;
}
