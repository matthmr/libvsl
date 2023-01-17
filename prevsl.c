// prevsl: symbolic implementation of `libvsl';
//         can be bootstrapped

//#include "libvsl.h" // <- TODO: stub
//#include "prim.h"

#define CLISP(x) \
  static int clisp_##x

static struct clisp_symtab clisp_symtab[] = {
  // primitive functions
  {
    .str  = "set", .hash = {.typ = __LISP_FUNC, .dat.func = NULL}, // = clisp_set,
  },
  {
    .str = "func", .hash = {.typ = __LISP_FUNC, .dat.func = NULL}, // = clisp_func,
  },
  {
    .str = "eval", .hash = {.typ = __LISP_FUNC, .dat.func = NULL}, // = clisp_eval,
  },
  {
    .str = "quot", .hash = {.typ = __LISP_FUNC, .dat.func = NULL}, // = clisp_quot,
  },
  {
    .str = "if", .hash = {.typ = __LISP_FUNC, .dat.func = NULL}, // = clisp_if,
  },
  {
    .str = "while", .hash = {.typ = __LISP_FUNC, .dat.func = NULL}, // = clisp_while,
  },
  {
    .str = "break", .hash = {.typ = __LISP_FUNC, .dat.func = NULL}, // = clisp_br,
  },
  {
    .str = "continue", .hash = {.typ = __LISP_FUNC, .dat.func = NULL}, // = clisp_cont,
  },

  // list-like functions
  {
    .str = "no-head", .hash = {.typ = __LISP_FUNC, .dat.func = NULL}, // = clisp_nhead,
  },
  {
    .str = "head", .hash = {.typ = __LISP_FUNC, .dat.func = NULL}, // = clisp_head,
  },
  {
    .str = "cat", .hash = {.typ = __LISP_FUNC, .dat.func = NULL}, // = clisp_cat,
  },

  // primitive symbols
  {
    .str = "t", .hash = {0}, // = clisp_t,
  },
  {
    .str = "nil", .hash = {0}, // = clisp_nil,
  },

  // EOL
  {0},
};

// TODO: this
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
        str_hash(stab), 1,
        write(STDERR_FILENO,
              MSG("[ !! ] prevsl: compiler error while parsing source\n")));

      // add the entry to the symbol table
      assert_exec(
        lisp_symtab_add(stab->hash, stab->grammar), 1,
        write(STDERR_FILENO,
              MSG("[ !! ] prevsl: compiler error while parsing source\n"
                  "  - could not enter item to the symbol table\n")));
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

  assert(compile_grammar() == 0, 1);
  assert(compile_prim() == 0, 1);

  done_for(ret);
}

VSL_FRONTEND(prevsl);
