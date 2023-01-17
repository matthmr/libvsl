// libvsl: a vsl implementation; not bootstrapped

// TODO: read from file as argument

#define LIBVSL_BACKEND

#include "libvsl.h"

int main(void) {
  int ret = 0;

  sexp_init();

  if (frontend) {
    assert(frontend() == 0, 1);
  }

  // TODO: verbose error messages
  assert_exec(
    parse_bytstream(STDIN_FILENO), 1,
    //err(ELIBVSLGEN)
    write(STDERR_FILENO,
          MSG("[ !! ] libvsl: error while parsing file\n")));

  done_for(ret);
}
