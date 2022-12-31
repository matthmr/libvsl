#include "pool.h"
#include "symtab.h"

static uint chash_i = 0;

struct MEMPOOL(ulong, ulong, SYMPOOL);
struct MEMPOOL_RET(ulong, ulong);

struct t {
  struct ulong mem[10];
  struct t* next;
  struct t* prev;
  uint idx;
  uint total;
  uint used;
};

static struct MEMPOOL_TMPL(ulong) symtab[SYMTAB];

static struct MEMPOOL_TMPL(lisp_symtab) symmp = {
  .mem   = {0},
  .next  = NULL,
  .prev  = NULL,
  .idx   = 0,
  .total = SYMPOOL,
  .used  = 1,
};
static struct MEMPOOL_TMPL(lisp_symtab)* symmpp = &symmp;

static inline void pool_clean(struct MEMPOOL_TMPL(lisp_symtab)* pp) {
  return;
}
static inline struct MEMPOOL_RET_TMPL(lisp_symtab)
pool_add_node(struct MEMPOOL_TMPL(lisp_symtab)* mpp) {
  struct MEMPOOL_RET_TMPL(lisp_symtab) ret = {
    .mem   = NULL,
    .base  = mpp,
    .entry = NULL,
    .same  = true,
  };

  if (mpp->used == mpp->total) {
    if (!mpp->next) {
      mpp->next        = malloc(sizeof(struct MEMPOOL_TMPL(lisp_symtab)));

      mpp->next->idx   = (mpp->idx + 1);
      mpp->next->prev  = mpp;
      mpp->next->next  = NULL;
    }

    ret.same          = false;
    mpp->next->total  = mpp->total;
    mpp->next->used   = 0;
    mpp               = mpp->next;
  }

  ret.mem   = mpp;
  ret.entry = (mpp->mem + mpp->used);
  ++mpp->used;

  return ret;
}
static inline struct MEMPOOL_RET_TMPL(lisp_symtab)
pool_from_idx(struct MEMPOOL_TMPL(lisp_symtab)* mpp,
              uint idx) {
  struct MEMPOOL_RET_TMPL(lisp_symtab) ret = {0};
  struct MEMPOOL_TMPL(lisp_symtab)* pp     = mpp;
  int diff = (idx - mpp->idx);

  if (diff > 0) {
    for (; diff; --diff) {
      pp = pp->next;
    }
  }
  else if (diff < 0) {
    for (diff = -diff; diff; --diff) {
      pp = pp->prev;
    }
  }

  ret.entry  =
    (ret.mem = pp)->mem;
  ret.base   = mpp;
  ret.same   = (bool) (pp == mpp);

  return ret;
}

ulong do_chash(ulong chash, char c) {
  return chash*(chash_i += 1);
}

ulong done_chash(ulong chash) {
  ulong ret = chash % chash_i;
  hash_i    = 0;

  return ret;
}
