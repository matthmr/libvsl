#include <unistd.h>

#define PROVIDE_SYMTAB_TABDEF
#include "symtab.h"
#include "debug.h"
#include "prim.h"
#include "cgen.h"
#include "err.h"

static const char* __cgen_sym_typ[] = {
  [__LISP_CLISP_FUN] = "__LISP_FUN",
  [__LISP_CLISP_SYM] = "__LISP_SYM",
};

#define CLISP_PRIM_FUN(__fun)    \
  {                              \
    .str = __fun,                \
    .sym = {                     \
      .typ = __LISP_CLISP_FUN,   \
      .dat = "lisp_prim_" __fun, \
    },                           \
  }

#define CLISP_PRIM_SYM(__sym, __val) \
  {                                  \
    .str = __sym,                    \
    .sym = {                         \
      .typ = __LISP_CLISP_SYM,       \
      .dat = __val,                  \
    },                               \
  }

static struct clisp_sym vsl_primtab[] = {
  // turing completion + code-data switching
  CLISP_PRIM_FUN("set"),
  CLISP_PRIM_FUN("func"),
  CLISP_PRIM_FUN("eval"),
  CLISP_PRIM_FUN("quot"),
  CLISP_PRIM_FUN("if"),
  CLISP_PRIM_FUN("eq"),
  CLISP_PRIM_FUN("not"),
  CLISP_PRIM_FUN("block"),
  CLISP_PRIM_FUN("while"),
  CLISP_PRIM_FUN("break"),
  CLISP_PRIM_FUN("continue"),
  CLISP_PRIM_FUN("return"),
  CLISP_PRIM_FUN("goto"),
  CLISP_PRIM_FUN("label"),
  CLISP_PRIM_FUN("cond"),

  // lisp-specific
  CLISP_PRIM_FUN("behead"),
  CLISP_PRIM_FUN("head"),
  CLISP_PRIM_FUN("list"),

  // booleans
  CLISP_PRIM_SYM("t", "NULL"),
  CLISP_PRIM_SYM("nil", "NULL"),

  // EOL
  {.str = NULL},
};

static void __cgen_preamble(void) {
  cgen_notice();
  cgen_string(
    LINE("#define PROVIDE_PRIM_SYMTAB") \
    LINE("#include \"prim.h\"")         \
    LINE(""));
}

static int __cgen_compile_prim(struct clisp_sym* tab) {
  struct lisp_hash_ret hash_ret = {
    .master = {0},
    .slave  = 0,
  };

  for (; tab->str; ++tab) {
    hash_ret = str_hash(tab->str);
    assert_for(hash_ret.slave == 0, OR_ERR(), hash_ret.slave);

    tab->sym.hash = hash_ret.master;
    assert_for(lisp_symtab_set(tab->sym) == 0, OR_ERR(), hash_ret.slave);
  }

  done_for(hash_ret.slave);
}

static int __cgen_transpile_sym_entry(uint xtent, POOL_T* pp) {
  cgen_string("  ");
  cgen_index(HASH_IDX(pp->mem->hash));
  cgen_string(" = {");
  cgen_string("\n    ");
  cgen_field("prev", CGEN_STRING, "NULL");

  if (xtent) {
    // TODO
  }
  else {
    cgen_field("next", CGEN_STRING, "NULL");
  }

  cgen_field("idx",   CGEN_INT,     &pp->idx);
  cgen_field("total", CGEN_INT,     &pp->total);
  cgen_field("used" , CGEN_INT,     &pp->used);
  cgen_field("mem",   CGEN_RECURSE, NULL);

  struct lisp_sym* sym = pp->mem;
  uint am = pp->used;

  FOR_EACH_TABENT(i, am) {
    cgen_string("\n      {");
    cgen_field("hash",     CGEN_RECURSE, NULL);
    cgen_field("sum",      CGEN_INT,     &pp->mem[i].hash.sum);
    cgen_field("psum",     CGEN_INT,     &pp->mem[i].hash.psum);
    cgen_field("len",      CGEN_SHORT,   &pp->mem[i].hash.len);
    cgen_field("com_part", CGEN_CHAR,    &pp->mem[i].hash.com_part);
    cgen_field("com_pos",  CGEN_CHAR,    &pp->mem[i].hash.com_pos);
    cgen_close_field();
    cgen_field("dat",      CGEN_STRING,  pp->mem[i].dat);
    cgen_field("typ",      CGEN_STRING,  __cgen_sym_typ[pp->mem[i].typ]);
    // TODO: `::size' and `::litr'
    cgen_close_field();
    cgen_string("\n");
  }

  cgen_string("    ");
  cgen_close_field();
  cgen_string("\n  ");
  cgen_close_field();
  cgen_string("\n");

  return 0;
}

static int __cgen_transpile_sym(POOL_T** stab_pp) {
  int ret = 0;

  char itoabuf[3];
  POOL_T* pp = NULL;

  uint pid[SYMTAB_PRIM] = {0};
  uint xtent = 0;

  FOR_EACH_TABENT(i, SYMTAB_PRIM) {
    pp = stab_pp[i];

    while (pp->prev) {
      pp = pp->prev;
      ++pid[i];
    }

    xtent = pid[i];

    if (xtent) {
      FOR_EACH_TABENT(j, xtent) {
        // TODO
      }
    }
    else {
      continue;
    }
  }

  cgen_string(LINE("POOL_T symtab[SYMTAB_PRIM] = {"));

  FOR_EACH_TABENT(i, SYMTAB_PRIM) {
    pp    = stab_pp[i];
    xtent = pid[i];

    if (pp->used) {
      __cgen_transpile_sym_entry(xtent, pp);
    }
  }

  cgen_string("};\n");

  return ret;
}

int main(void) {
  int ret = 0;

  __cgen_preamble();
  symtab_init();

  ret = __cgen_compile_prim(vsl_primtab);
  assert(ret == 0, OR_ERR());

  ret = __cgen_transpile_sym(symtab_pp);
  assert(ret == 0, OR_ERR());

  cgen_flush();

  done_for(ret);
}
