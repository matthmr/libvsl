#include "vslisp.h"
#include "symtab.h"

//static struct lisp_symtab_ret lisp_set(struct lisp_symtab_in in) {}

const struct lisp_symtab_raw lisp_symtab_prim[] = {
  // primitive functions
  {"set",      0, __LISP_FUN, {lisp_set}},
  {"func",     0, __LISP_FUN, {lisp_func}},
  {"eval",     0, __LISP_FUN, {lisp_eval}},
  {"quot",     0, __LISP_FUN, {lisp_quot}},
  {"if",       0, __LISP_FUN, {lisp_if}},
  {"whil",     0, __LISP_FUN, {lisp_while}},
  {"break",    0, __LISP_FUN, {lisp_br}},
  {"continue", 0, __LISP_FUN, {lisp_cont}},

  // list-like functions
  {"no-head",  0, __LISP_FUN, {lisp_nhead}},
  {"head",     0, __LISP_FUN, {lisp_head}},
  {"cat",      0, __LISP_FUN, {lisp_cat}},

  // primitive symbols
  {"t",        0, __LISP_SYM, {lisp_t}},
  {"nil",      0, __LISP_SYM, {lisp_nil}},

  // EOL
  {NULL,       0, {NULL}},
};

// TODO: make the symbol table integrate with `stack.h'
// static

static void compile_prim(void) {
  struct lisp_symtab_raw* stab = NULL;
  ulong hash = 0;

  prim_symtab = symtab_new();

  for (uint i = 0; ; ++i) {
    stab = lisp_symtab_prim[i];

    if (stab->str) {
      hash = hash_str(stab->str);

      symtab_add()
      continue;
    }

    break;
  }
}

void (*frontend)(void) = compile_prim;
