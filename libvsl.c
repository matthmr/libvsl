// libvsl: a vsl implementation; not bootstrapped

// TODO: read from file as argument
// TODO: a branch without `cgen' or `prim' (or with being them empty)

#define LIBVSL_BACKEND

#include "libvsl.h"

int main(void) {
  sexp_init();
  symtab_init(false);

  if (frontend && frontend() != 0) {
    return err(EFRONTEND);
  }

  return parse_bytstream(STDIN_FILENO);
}
