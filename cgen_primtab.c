#include <unistd.h>

#define PROVIDE_SYMTAB_TABDEF

#include "debug.h"
#include "prim.h"  // also include `symtab.h'
#include "cgen.h"
#include "err.h"

static const char* __cgen_sym_typ[] = {
  [__LISP_CLISP_FUN] = "__LISP_FUN",
  [__LISP_CLISP_SYM] = "__LISP_SYM",
};

static void __cgen_preamble(void) {
  cgen_notice();
  cgen_string(
    LINE("#define PROVIDE_PRIM_SYMTAB") \
    LINE("#include \"prim.h\"")         \
    LINE(""));
}

static int __cgen_compile_prim(struct clisp_sym* tab) {
  // this interface is ugly but it is what it is
  struct {
    struct lisp_sym      master;
    struct lisp_hash_ret hash_sb;
    int slave;
  } ret = {0};

  for (; tab->str; ++tab) {
    ret.hash_sb = str_hash(tab->str);

    ret.slave = ret.hash_sb.slave;
    assert_for(ret.slave == 0, OR_ERR(), ret.slave);

    ret.master = tab->sym;
    ret.master.hash = ret.hash_sb.master;

    ret.slave = lisp_symtab_set(ret.master);
    assert_for(ret.slave == 0, OR_ERR(), ret.slave);
  }

  done_for(ret.slave);
}

static void __cgen_transpile_sym_field(char* field, uint idx, uint pidx) {
  char itoabuf[20] = {0};

  string_is stris = cgen_stris_from(itoabuf, SIZEOF(itoabuf));
  string_is* strisp = &stris;

  cgen_string_for("&_", strisp);
  cgen_itoa_for(idx, strisp);
  cgen_string_for("_", strisp);
  cgen_itoa_for(pidx, strisp);

  cgen_field(field,  CGEN_STRING, stris.inc.string);
}

static void __cgen_transpile_sym_future(char* field, char* future, uint idx) {
  char itoabuf[40] = {0};
  string_is stris = cgen_stris_from(itoabuf, SIZEOF(itoabuf));
  string_is* strisp = &stris;

  cgen_string_for("&", strisp);
  cgen_string_for(future, strisp);
  cgen_index_for(idx, strisp);

  cgen_field(field,  CGEN_STRING, stris.inc.string);
}

static void __cgen_transpile_sym_data(POOL_T* pp, uint idx, uint pidx,
                                      bool in_array) {
  struct lisp_sym* sym = pp->mem;
  uint am = pp->idx;

  cgen_string("\n    ");

  if (pp->prev) {
    if (pidx == 1) {
      __cgen_transpile_sym_future("prev", "symtab", idx);
    }
    else {
      __cgen_transpile_sym_field("prev", idx, (pidx - 1));
    }
  }
  else {
    cgen_field("prev",  CGEN_STRING, "NULL");
  }

  if (pp->next) {
    __cgen_transpile_sym_field("next", idx, (pidx + 1));
  }
  else {
    cgen_field("next",  CGEN_STRING,  "NULL");
  }

  cgen_field("idx",   CGEN_INT,     &pp->idx);
  cgen_field("mem",   CGEN_RECURSE, NULL);

  FOR_EACH_TABENT(i, am) {
    cgen_string("\n      {");
    cgen_field("hash",     CGEN_RECURSE, NULL);
    cgen_field("sum",      CGEN_INT,     &sym[i].hash.sum);
    cgen_field("psum",     CGEN_INT,     &sym[i].hash.psum);
    cgen_field("len",      CGEN_SHORT,   &sym[i].hash.len);
    cgen_field("com_part", CGEN_SHORT,   &sym[i].hash.com_part);
    cgen_close_field();
    cgen_field("dat",      CGEN_STRING,  sym[i].dat);
    cgen_field("typ",      CGEN_STRING,  __cgen_sym_typ[sym[i].typ]);
    if (sym[i].typ == __LISP_CLISP_FUN) {
      cgen_field_array("size", CGEN_INT, sym[i].size, SIZEOF(sym[i].size));
      cgen_field_array("litr", CGEN_INT, sym[i].litr, SIZEOF(sym[i].litr));
    }
    cgen_close_field();
    cgen_string("\n");
  }

  cgen_string("    ");

  if (in_array) {
    cgen_close_field();
    cgen_string("\n  ");
  }
  cgen_close_field();
  cgen_string("\n");
}

static inline void __cgen_transpile_sym_entry(POOL_T* pp, uint pidx,
                                              bool in_array) {
  uint idx = HASH_IDX(pp->mem->hash);

  cgen_string("  ");
  cgen_index(idx);
  cgen_string(" = {");

  __cgen_transpile_sym_data(pp, idx, pidx, in_array);
}

static int __cgen_transpile_sym(struct lisp_symtab_pp* stab_pp) {
  int ret = 0;

  POOL_T* pp = NULL, * hpp = NULL;

  FOR_EACH_TABENT(i, SYMTAB_CELL) {
    pp     = stab_pp[i].mem;
    uint n = 0;

    while (pp->prev /* || pp->next */) {
      DB_FMT(LINE("[ == ] cgen_primtab: 0x%p memory"), pp->prev);
      pp = pp->prev;
      ++n;
    }

    if (n) {
      DB_FMT(LINE("[ == ] cgen_primtab: stab_pp[%d] has %d extra copies"), i, n);

      uint idx = HASH_IDX(pp->mem->hash);
      uint in  = 1;
      hpp      = pp;
      pp       = pp->next;

      // TODO: make this optional
      // only needed when a lot of collisions happen
      while (in <= n) {
        cgen_string("extern POOL_T _");
        cgen_itoa_string(idx);
        cgen_string("_");
        cgen_itoa_string(in);
        cgen_string(LINE(";"));
        ++in;
      }

      in = 1;
      cgen_string(LINE(""));

      while (in <= n) {
        cgen_string("POOL_T _");
        cgen_itoa_string(idx);
        cgen_string("_");
        cgen_itoa_string(in);
        cgen_string(" = {");
        __cgen_transpile_sym_data(pp, idx, in, false);
        cgen_string(LINE("};"));
        cgen_string(LINE(""));
        pp = pp->next;
        ++in;
      }

      stab_pp[i].mem = hpp;
    }
  }

  cgen_string(LINE("POOL_T symtab[SYMTAB_CELL] = {"));

  FOR_EACH_TABENT(i, SYMTAB_CELL) {
    POOL_T* pp = stab_pp[i].mem;

    if (pp->idx) {
      __cgen_transpile_sym_entry(pp, 0, true);
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
