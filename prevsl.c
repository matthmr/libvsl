// prevsl: symbolic implementation of `libvsl';
//         can be bootstrapped

#include "libvsl.h"
//#include "prim.h"

#define CLISP(x) \
  static int clisp_##x

static struct clisp_symtab clisp_symtab[] = {};

// TODO: these
static int compile_grammar(void) {
  return 0;
}

static int compile_prim(void) {
  int ret = 0;

  struct clisp_symtab* stab;

  for (uint i = 0;; ++i) {
    stab = &clisp_symtab[i];

    if (stab->str) {
      // compile the hash for the table entry
      assert_exec(
        str_hash(stab), ERROR,
        write(STDERR_FILENO,
              ERR_STRING("prevsl", "compiler error while parsing source")));

      // add the entry to the symbol table
      assert_exec(
        lisp_symtab_add(stab->hash, stab->grammar), ERROR,
        write(STDERR_FILENO,
              ERR_STRING(\
                "prevsl", "compiler error while parsing source\n" \
                          "  - could not enter item to the symbol table")));
      }

      continue;
    }
    else {
      break;
    }
  }

  done_for(ret);
}

static int prevsl(void) {
  int ret = 0;

  ret = compile_grammar();
  assert(ret == 0, OR_ERR());

  ret = compile_prim();
  assert(ret == 0, OR_ERR());

  done_for(ret);
}

LIBVSL_FRONTEND(prevsl);
