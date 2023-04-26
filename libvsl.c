// LIBVSL: a VSL backend library implementation

#define LIBVSL_BACKEND

#include "libvsl.h"

int main(void) {
  register int ret = 0;

  MAYBE_INIT(mm_init());
  MAYBE_INIT(lisp_symtab_init());

  if (frontend) {
    MAYBE_INIT(frontend());
  }

  MAYBE_INIT(lisp_prim_init());

  return lisp_parser(STDIN_FILENO);

  done_for(ret);
}
