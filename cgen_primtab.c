#include <unistd.h>

#define LOCK_SYMTAB_INTERNALS
#include "symtab.h"
#include "prim.h"
#include "cgen.h"
#include "err.h"

static const char* cgen_sym_typ[] = {
  [__LISP_FUN] = "__LISP_FUN",
  [__LISP_SYM] = "__LISP_SYM",
};

static const struct clisp_sym primtab[] = {
  // turing completion + code-data switching
  {
    .str  = "set", .fun = "lisp_prim_set", .sym = {.typ = __LISP_FUN, .dat = NULL},
  },

  {
    .str = "func", .fun = "lisp_prim_func", .sym = {.typ = __LISP_FUN, .dat = NULL},
  },

  {
    .str = "eval", .fun = "lisp_prim_eval", .sym = {.typ = __LISP_FUN, .dat = NULL},
  },

  {
    .str = "quot", .fun = "lisp_prim_quot", .sym = {.typ = __LISP_FUN, .dat = NULL},
  },

  {
    .str = "if", .fun = "lisp_prim_if", .sym = {.typ = __LISP_FUN, .dat = NULL},
  },

  {
    .str = "eq", .fun = "lisp_prim_eq", .sym = {.typ = __LISP_FUN, .dat = NULL},
  },

  {
    .str = "not", .fun = "lisp_prim_not", .sym = {.typ = __LISP_FUN, .dat = NULL},
  },

  {
    .str = "block", .fun = "lisp_prim_block", .sym = {.typ = __LISP_FUN, .dat = NULL},
  },

  {
    .str = "while", .fun = "lisp_prim_while", .sym = {.typ = __LISP_FUN, .dat = NULL},
  },

  {
    .str = "break", .fun = "lisp_prim_break", .sym = {.typ = __LISP_FUN, .dat = NULL},
  },

  {
    .str = "continue", .fun = "lisp_prim_continue", .sym = {.typ = __LISP_FUN, .dat = NULL},
  },

  {
    .str = "return", .fun = "lisp_prim_return", .sym = {.typ = __LISP_FUN, .dat = NULL},
  },

  {
    .str = "goto", .fun = "lisp_prim_goto", .sym = {.typ = __LISP_FUN, .dat = NULL},
  },

  {
    .str = "label", .fun = "lisp_prim_label", .sym = {.typ = __LISP_FUN, .dat = NULL},
  },

  {
    .str = "cond", .fun = "lisp_prim_cond", .sym = {.typ = __LISP_FUN, .dat = NULL},
  },

  // booleans
  {
    .str = "t", .fun = "NULL", .sym = {.typ = __LISP_SYM, .dat = NULL},
  },

  {
    .str = "nil", .fun = "NULL", .sym = {.typ = __LISP_SYM, .dat = NULL},
  },

  // lisp-specific
  {
    .str = "behead", .fun = "lisp_prim_behead", .sym = {.typ = __LISP_FUN, .dat = NULL},
  },

  {
    .str = "head", .fun = "lisp_prim_head", .sym = {.typ = __LISP_FUN, .dat = NULL},
  },

  {
    .str = "list", .fun = "lisp_prim_list", .sym = {.typ = __LISP_FUN, .dat = NULL},
  },

  // EOL
  {.str = NULL},
};

static int transpile_tab(const struct clisp_sym* tab) {
  struct lisp_hash_ret hash_ret = {
    .master = {0},
    .slave  = 0,
  };

  for (;;) {
    if (tab->str == NULL) {
      break;
    }

    hash_ret = str_hash(tab->str);

    assert_for_exec(hash_ret.slave == 0, ERROR, hash_ret.slave,
                     write(STDERR_FILENO, ERR_MSG("prim",
                     "libvsl errored while generating the primitives")));

    cgen_string("  ");
    cgen_index(hash_ret.master.sum % SYMTAB_PRIM);
    cgen_string(" = { ");
    cgen_field("hash",     CGEN_RECURSE, NULL);
    cgen_field("sum",      CGEN_INT,     &hash_ret.master.sum);
    cgen_field("psum",     CGEN_INT,     &hash_ret.master.psum);
    cgen_field("len",      CGEN_INT,     &hash_ret.master.len);
    cgen_field("com_part", CGEN_INT,     &hash_ret.master.com_part);
    cgen_field("com_pos",  CGEN_INT,     &hash_ret.master.com_pos);
    cgen_close_field();
    cgen_field("dat",      CGEN_STRING,  tab->fun);
    cgen_field("typ",      CGEN_STRING,  cgen_sym_typ[tab->sym.typ]);
    cgen_close_field();
    cgen_string("\n");

    ++tab;
  }

  done_for(hash_ret.slave);
}

int main(void) {
  int ret = 0;

  cgen_notice();

  cgen_string(
    LINE("#include \"symtab.h\"\n") \
    LINE("POOL_T symtab[SYMTAB_PRIM] = {"));
  assert(transpile_tab(primtab) == 0, OR_ERR());
  cgen_string("};\n");

  cgen_flush();

  done_for(ret);
}
