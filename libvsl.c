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

  // `frontend' is not NULL, and `frontend()' exits with 0
  assert(frontend && !frontend(), 1);

  // TODO: verbose error messages
  assert_exec(
    parse_bytstream(STDIN_FILENO), 1,
    //err(ELIBVSLGEN)
    write(STDERR_FILENO,
          MSG("[ !! ] libvsl: error while parsing file\n")));

  done_for(ret);
}
