#define LIBVSL_BACKEND

#include "libvsl.h"
#include "debug.h"

int lisp_toplevel_lex(struct lisp_symtab* envp) {
  register int ret = 0;
  struct lisp_ret sret = {0};

  ret = lisp_lex_feed();
  assert(ret == 0, OR_ERR());

  for (;;) {
    ret = lisp_lex_yield();

    // no error, no input: exit
    assert(ret != __LEX_NO_INPUT, 0);
    assert(ret == __LEX_OK, OR_ERR());

    // push symbol
    if (LEX_SYMBOL_OUT(lex.ev)) {
      lex.ev &= ~__LEX_EV_SYMBOL_OUT;
      DB_MSG("[ libvsl ] top-level yield symbol");

      lex.symbuf.idx = 0;
    }

    // push function `('
    else if (LEX_PAREN_IN(lex.ev)) {
      sret = lisp_stack_frame_lex(envp);
      assert(sret.slave == __STACK_OK, OR_ERR());
    }
  }

  done_for(ret);
}

int libvsl_init(void) {
  register int ret = 0;

  MAYBE_INIT(mm_init());

  done_for(ret);
}
