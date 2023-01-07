// vslisp: a very simple lisp implementation;
//         not bootstrapped

// TODO: untangle this include mess

#define LIBVSL_BACKEND

#include "libvsl.h"

// unlock the internals of `sexp.h' to get the memory pool
#ifdef LOCK_SEXP_INTERNALS
#  undef LOCK_SEXP_INTERNALS
#endif

#include "sexp.h"

int main(void) {
  int ret = 0;

  root    = POOLP->mem;
  root->t = (__SEXP_SELF_ROOT | __SEXP_LEFT_EMPTY | __SEXP_RIGHT_EMPTY);

  defer_assert(frontend && (frontend() == 0), 1);

  ret = parse_bytstream(STDIN_FILENO);

  if (ret) {
    write(STDERR_FILENO,
          MSG("[ !! ] libvsl: error while parsing file\n"));
    defer(1);
  }

  defer_ret(ret);
}
