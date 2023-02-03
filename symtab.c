#include "symtab.h"
#include "debug.h"
#include "err.h"

// TODO: ensure that the hash algorithm doesn't trigger false positives
// TODO: implement lexical scoping

static uint hash_i = 1;

POOL_T* symtab_pp[SYMTAB_CELL] = {0};

// TODO: stub
static inline void pool_clean(POOL_T* pp) {
  return;
}

static inline int s(int t) {
  return t*(t+1)/2;
}

static inline int mod_norm(int val, int len) {
  return val - s(len);
}

////////////////////////////////////////////////////////////////////////////////

struct lisp_hash_ret inc_hash(struct lisp_hash hash, char c) {
  struct lisp_hash_ret hash_ret = {
    .master = hash,
    .slave  = 0,
  };

  if (hash.len > SYMTAB_MAX_SYM) {
    defer_for_as(hash_ret.slave, err(EIDTOOBIG));
  }

  ASCII_NORM(c);

  uchar prt_pre = hash.psum % SYMTAB_CELL;
  hash.sum  += c*hash_i;
  hash.psum += c;
  uchar prt_post = c;

  hash_i *= SYMTAB_PRIM;
  hash_i %= SYMTAB_CELL;

  ++hash.len;

  // TODO: another field, `::com_mod', that contains similar information to
  // `::com_part', but uses the `SYMTAB_CELL` module

  // cache if there was a roll-over
  if ((prt_pre + prt_post) > SYMTAB_MAX_SYM) {
    hash.com_part += c*hash_i;
  }

  hash_ret.master = hash;

  done_for(hash_ret);
}

void inc_hash_done(struct lisp_hash* hash) {
  hash->sum = mod_norm(hash->sum, hash->len);

  hash_i = 1;
}

void hash_done(struct lisp_hash* hash) {
  hash->sum      = 0;
  hash->psum     = 0;
  hash->len      = 0;
  hash->com_part = 0;
}

struct lisp_hash_ret str_hash(const char* str) {
  struct lisp_hash_ret hash_ret = {
    .master = {0},
    .slave  = 0,
  };

  char c = '\0';

  DB_FMT("[ == ] symtab: for string: %s", str);

  for (uint i = 0;; ++i) {
    if (!(c = str[i])) {
      break;
    }

    hash_ret = inc_hash(hash_ret.master, c);
    DB_FMT("[ == ] symtab: character (%c) (%d)", c, hash_ret.master.sum);

    assert_for(hash_ret.slave == 0, OR_ERR(), hash_ret.slave);
  }

  inc_hash_done(&hash_ret.master);
  done_for(hash_ret);
}

int lisp_symtab_set(struct lisp_sym sym) {
  int ret = 0;

  const uint idx = HASH_IDX(sym.hash);

  POOL_T*    mpp = symtab_pp[idx];
  POOL_RET_T pr  = pool_add_node(mpp);
  assert(pr.stat == 0, OR_ERR());

  if (pr.base != pr.mem) {
    mpp = symtab_pp[idx] = pr.mem;
  }

  DB_FMT("[ == ] symtab: adding at index %d\n---", idx);
  mpp->mem[IDX_HM(mpp->idx)] = sym;

  done_for(ret);
}

struct lisp_sym_ret lisp_symtab_get(struct lisp_hash hash) {
  struct lisp_sym_ret ret = {0};
  uint idx = HASH_IDX(hash);

  DB_FMT("[ == ] symtab: trying to get index %d", idx);

  POOL_T* pp = symtab_pp[idx];
  struct lisp_sym* tab = pp->mem;

  // TODO: stub; this is wrong in general

  ret.master = &pp->mem[0];

  done_for(ret);
}

int symtab_init(void) {
  for (uint i = 0; i < SYMTAB_CELL; ++i) {
    symtab_pp[i] = (symtab + i);
  }

  return 0;
}
