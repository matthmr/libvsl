#include "debug.h"

#include "lisp.h"
#include "symtab.h"
#include "mm.h"

#include <strings.h>
#include <string.h>

//// ERRORS

ECODE(ENOTFOUND);

EMSG {
  [ENOTFOUND] = ERR_STRING("libvsl: symtab", "symbol was not found"),
};

////////////////////////////////////////////////////////////////////////////////

/** Return a node in the symbol table @t_node closest to hash @sym_hash */
static struct lisp_symtab_node*
lisp_symtab_get_node(hash_t sym_hash, struct lisp_symtab_node t_node) {
  return NULL;
}

////////////////////////////////////////////////////////////////////////////////

hash_t hash(string_ip sym) {
  register int     ret = 0;
  register hash_t ret_t = 0;

  const char* str = sym.str;
  const uint  len = sym.idx;

  for (uint i = 0; i < len; i++) {
    ret_t += ASCII_NORM(str[i])*PRIME;
  }

  ret_t = ret_t % HASH_SIZE;

  done_for(ret_t);
};

struct lisp_sym*
lisp_symtab_set(string_ip sym_str, struct lisp_obj sym_obj,
                struct lisp_symtab* sym_tab) {
  register int       ret = 0;
  struct lisp_sym* ret_t = NULL;

  struct lisp_obj* obj = NULL;
  char*            str = NULL;

  hash_t sym_hash = hash(sym_str);

  struct lisp_symtab_node* t_node =
    lisp_symtab_get_node(sym_hash, sym_tab->tab);

  if (!t_node) {
    struct lisp_sym* sym = &sym_tab->tab.self;

    str = mm_alloc(sizeof(*str));
    assert(str, OR_ERR());
    memcpy(str, sym_str.str, sym_str.idx);

    obj = mm_alloc(sizeof(*obj));
    assert(obj, OR_ERR());
    memcpy(obj, &sym_obj, sizeof(*obj));

    sym->dis   = __LISP_SYM_NODE;
    sym->_.dat = (struct lisp_sym_dat) {
      .str = str,
      .obj = *obj,
    };

    sym_tab->tab = (struct lisp_symtab_node) {
      .hash_idx = sym_hash,
      .self     = *sym,
      .left     = NULL,
      .right    = NULL,
    };
  }

  done_for((ret_t = ret? NULL: ret_t));
}

struct lisp_sym*
lisp_symtab_sets(string_ip sym_str, struct lisp_obj sym_obj,
                    struct lisp_symtab* sym_tab) {
  register int       ret = 0;
  struct lisp_sym* ret_t = NULL;

  hash_t sym_hash = hash(sym_str);

  done_for((ret_t = ret? NULL: ret_t));
}

struct lisp_sym*
lisp_symtab_get(string_ip sym_str, struct lisp_symtab* sym_tab) {
  register int ret = 0;
  struct lisp_sym* ret_t = NULL;

  hash_t sym_hash = hash(sym_str);

  done_for((ret_t = ret? NULL: ret_t));
}
