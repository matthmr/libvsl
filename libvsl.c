// libvsl: a vsl implementation; not bootstrapped

// TODO: read from file as argument
// TODO: a branch without `cgen' or `prim' (or with being them empty)

#define LIBVSL_BACKEND

#include "libvsl.h"

int main(void) {
  int ret = 0;

  sexp_init();
  symtab_init();

  if (frontend) {
    assert(frontend() == 0, err(EFRONTEND));
  }

  assert(parse_bytstream(STDIN_FILENO) == 0, OR_ERR());

  done_for(ret);
}
