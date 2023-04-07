// libvsl: a vsl implementation; not bootstrapped

#define LIBVSL_BACKEND

#include "libvsl.h"

int main(void) {
  register int ret = 0;

  sexp_init();
  MAYBE_INIT(symtab_init(false));

  if (frontend) {
    ret = frontend();
    assert(ret == 0, err(EFRONTEND));
  }

  MAYBE_INIT(lisp_prim_init());

  ret = parse_bytstream(STDIN_FILENO);

  done_for(ret);
}
