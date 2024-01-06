#include "debug.h"

#include "lisp.h"
#include "symtab.h"
#include "mm.h"

#include <string.h>

//// ERRORS

ECODE(ENOTFOUND);

EMSG {
  [ENOTFOUND] = ERR_STRING("libvsl: symtab", "symbol was not found"),
};

////////////////////////////////////////////////////////////////////////////////

/** Wrapped type for hash */
struct lisp_hash {
  /* hash index (non offset) */
  hash_t idx;

  /* hash weighted sum */
  uint sum;
};

/** Recomputes hash with spread offset */
static inline struct lisp_hash __hash_with(struct lisp_hash hash, uint off) {
  struct lisp_hash ret = hash;

  ret.sum += off*PRIME;
  ret.idx  = (ret.sum % HASH_SIZE) + 1;

  return ret;
}

/** Boolean return of whether the position @at is set in the bitfield @elems */
static inline bool __idx_exists(char* elems, uint at) {
  return (bool) elems[(at >> 3)] & (1 << (at & 0x07));
}

/** Set index of bitfield @elems on */
static inline void __idx_set(char* elems, uint at) {
  elems[(at >> 3)] |= 0xff & (1 << (at & 0x07));
}

/** Set index of bitfield @elems off */
static inline void __idx_unset(char* elems, uint at) {
  elems[(at >> 3)] &= ~(0xff & (1 << (at & 0x07)));
}

// TODO: a bit inefficient
/** Compute the spread offset to have @hash be a vacant value. -1u means none
    was found */
static inline uint __hash_soff(struct lisp_hash hash, char* elems) {
  for (uint i = 0; i < HASH_SIZE; i++) {
    struct lisp_hash new_hash = __hash_with(hash, i);

    if (__idx_exists(elems, new_hash.idx)) {
      return i;
    }
  }

  return -1u;
}

////////////////////////////////////////////////////////////////////////////////

/** Return status. Tagged for caller handling */
enum chk_ret_stat {
  __CHK_MATCHES = BIT(0),
  __CHK_NEW_IF  = BIT(1),
};

/** Wrapped return for `lisp_symtab_get_chk' */
struct chk_ret_t {
  /* close chunk */
  struct lisp_symtab_chk* chk;

  /* close table interface */
  struct lisp_symtab_if* tab_if;

  /* close node (+1) */
  uint node;

  /* spread offset needed. mostly for `set' */
  uint soff;

  /* return status */
  enum chk_ret_stat stat;
};

/** Type of call to `lisp_symtab_get_chk' */
enum chk_t {
  __CHK_FOR_SET,
  __CHK_FOR_GET,
};

/** Ordely insert symbol in chunk */
static struct lisp_sym*
lisp_symtab_ins(struct lisp_symtab_chk* chk, uint at, struct lisp_hash sym_hash,
                struct lisp_sym sym) {
  const register uint chk_size = chk->chk_size;
  const register uint hash_idx = sym_hash.idx;
  struct lisp_symtab_node* nodes = chk->_;

  struct lisp_symtab_node this = {
    .idx  = hash_idx,
    .soff = 0,
    .sym  = sym,
  };

  struct lisp_sym* ret = NULL;

  if (!chk_size) {
    chk->_[at] = this;

    ret = &chk->_[at].sym;

    goto done;
  }

  for (uint i = 0; i < chk_size; i++) {
    if (nodes[i].idx > hash_idx) {
      // shift the remaining ones to the left, insert ourselves at this position

      struct lisp_symtab_node a = nodes[i], b;

      nodes[i] = this;

      for (uint j = i; j < chk_size; j++) {
        b = nodes[j + 1];
        nodes[j + 1] = a;
        a = b;
      }

      ret = &nodes[i].sym;

      goto done;
    }
  }

  nodes[chk_size] = this;
  ret = &nodes[chk_size].sym;

done:
  chk->chk_size++;
  return ret;
}

/** Gets the chunk of table @sym_tab given hash @sym_hash and string
    representation @sym_str, respecting type @typ */
static struct chk_ret_t
lisp_symtab_get_chk(struct lisp_hash sym_hash, string_ip sym_str,
                    struct lisp_symtab* sym_tab,
                    const enum lisp_symtab_t typ, const enum chk_t chk_typ) {
  struct chk_ret_t ret = {0};

  struct lisp_hash hash = sym_hash;

  struct lisp_symtab*        tab = sym_tab;
  struct lisp_symtab_if*  tab_if = NULL;
  struct lisp_symtab_chk* chk_if = NULL;

  struct lisp_symtab_node node = {0};
  struct lisp_symtab_node* chk = NULL;

  tab_if = &tab->_;

  // loop through the hierarchy of tables
  do {
with_chk:
    chk_if = &tab_if->chk;
    chk = chk_if->_;

    ret.chk = chk_if;
    ret.tab_if = tab_if;

    // loop through the table chunks
    do {
      const register uint chk_size = chk_if->chk_size;

with_hash:
      if (chk_typ == __CHK_FOR_SET && chk_size < HASH_SIZE) {
        ret.node = (chk_size + 1);

        if (!__idx_exists(tab_if->elems, hash.idx)) {
          goto done;
        }

        // the index is already occupied: compute the spread offset

        uint soff = __hash_soff(hash, tab_if->elems);

        if (soff != -1u) {
          ret.soff = soff;

          goto done;
        }

        // no index available: tag a new interface

        ret.stat |= __CHK_NEW_IF;

        goto done;
      }

      if (chk_typ == __CHK_FOR_GET && !__idx_exists(tab_if->elems, hash.idx)) {
        break;
      }

      // out-of-bounds
      if (chk_size &&
          (hash.idx > chk[chk_size - 1].idx || hash.idx < chk[0].idx)) {
        continue;
      }

      for (uint i = 0; i < chk_size; i++) {
        ret.node = (i + 1);
        node = chk[i];

        if (hash.idx == node.idx) {
          // found. by the way the symbols work, unless gotten from a byte-lexer
          // (STDIN, string, ...), we can check to see if the string pointers
          // are the same, instead of iterating with `strncmp'
          if (sym_str.str == node.sym.str ||
              strncmp(sym_str.str, node.sym.str, sym_str.idx) == 0) {
            ret.stat |= __CHK_MATCHES;

            goto done;
          }

          // with offset: reiterate the chunk with offset to hash
          if (node.soff != 0) {
            hash = __hash_with(hash, node.soff);

            goto with_hash;
          }
        }
      }

    } while ((chk_if = chk_if->next));

    // not found: reiterate the chunks with the next table node
    if ((tab_if = tab_if->next)) {
      goto with_chk;
    }

  } while (!(typ & __LISP_SYMTAB_SCOPE) && (tab = tab->root));

done:
  return ret;
}

////////////////////////////////////////////////////////////////////////////////

struct lisp_hash hash(string_ip sym_str) {
  struct lisp_hash ret = {0};

  const char* str = sym_str.str;
  const uint  len = sym_str.idx;

  for (uint i = 0; i < len; i++) {
    ret.sum += ASCII_NORM(str[i])*PRIME;
  }

  ret.idx = (ret.sum % HASH_SIZE);

  return ret;
}

// TODO: this
struct lisp_sym*
lisp_symtab_set(string_ip sym_str, struct lisp_obj* sym_obj,
                struct lisp_symtab* sym_tab, const enum lisp_symtab_t typ) {
  register int       ret = 0;
  struct lisp_sym* ret_t = NULL;

  struct lisp_hash sym_hash = hash(sym_str);

  struct chk_ret_t gc =
    lisp_symtab_get_chk(sym_hash, sym_str, sym_tab, typ, __CHK_FOR_SET);

  gc.node--;

  // overwrite existing symbol: only if `__LISP_SYMTAB_SAFE' is not set
  if (gc.stat & __CHK_MATCHES) {
    ret_t = &(gc.chk->_ + gc.node)->sym;

    if (typ & __LISP_SYMTAB_SAFE) {
      defer();
    }

    else {
      lisp_free_obj(ret_t->obj);

      ret_t->obj = sym_obj;
    }

    defer();
  }

  ////

  // insert at new interface
  if (gc.stat & __CHK_NEW_IF) {
    struct lisp_symtab_if* tab_if = gc.tab_if;

    tab_if->next = mm_alloc(sizeof(*tab_if->next));
    assert(tab_if->next, OR_ERR());

    gc.tab_if = tab_if;
    gc.chk    = &tab_if->chk;
    gc.node   = 0;

    goto insert;
  }

  ////

  // insert with spread offset
  if (gc.soff) {
    gc.chk->_[gc.node].soff = gc.soff;

    sym_hash = __hash_with(sym_hash, gc.soff);
  }

  ////

  if (gc.node != HASH_CHKSIZE) {
    struct lisp_sym sym;

insert: // classic C
    sym = (struct lisp_sym) {
      .obj = sym_obj,
      .str = sym_str.str, // TODO: this should share the string
    };

    ret_t = lisp_symtab_ins(gc.chk, gc.node, sym_hash, sym);

    __idx_set(gc.tab_if->elems, sym_hash.idx);

    defer();
  }

  // insert at next chunk
  else {
    gc.chk->next = mm_alloc(sizeof(*gc.chk->next));
    assert(gc.chk->next, OR_ERR());

    gc.chk = gc.chk->next;

    goto insert;
  }

  done_for((ret_t = ret? NULL: ret_t));
}

void
lisp_symtab_del(string_ip sym_str, struct lisp_symtab* sym_tab,
                const enum lisp_symtab_t typ) {
  if (!sym_tab) {
    return;
  }

  struct lisp_hash sym_hash = hash(sym_str);

  struct chk_ret_t gc =
    lisp_symtab_get_chk(sym_hash, sym_str, sym_tab, typ, __CHK_FOR_GET);

  gc.node--;

  if (gc.stat & __CHK_MATCHES) {
    // TODO: this
  }
}

struct lisp_sym*
lisp_symtab_get(string_ip sym_str, struct lisp_symtab* sym_tab,
                const enum lisp_symtab_t typ) {
  register int ret = 0;
  struct lisp_sym* ret_t = NULL;

  assert(sym_tab, err(ENOTFOUND));

  struct lisp_hash sym_hash = hash(sym_str);

  DB_BYT("[ symtab ] hash of symbol (");
  DB_NBYT(sym_str.str, sym_str.idx);
  DB_FMT(") is (%d, %d)", sym_hash.idx, sym_hash.sum);

  struct chk_ret_t gc =
    lisp_symtab_get_chk(sym_hash, sym_str, sym_tab, typ, __CHK_FOR_GET);

  gc.node--;

  if (gc.stat & __CHK_MATCHES) {
    ret_t = &(gc.chk->_ + gc.node)->sym;
  }
  else {
    defer_as(err(ENOTFOUND));
  }

  done_for((ret_t = ret? NULL: ret_t));
}
