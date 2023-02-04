#include "symtab.h"
#include "debug.h"
#include "err.h"

// TODO: implement lexical scoping

static uint hash_i = 1;

struct lisp_symtab_pp symtab_pp[SYMTAB_CELL] = {0};

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

static void lisp_symtab_sort_backprog(POOL_T* base_pp, uint base_idx,
                                      POOL_T* pp, uint pp_idx) {
  struct lisp_sym tmp = {0}, * tmpp = NULL;
  struct lisp_sym* pt = NULL;
  POOL_T* p = pp;
  uint iter = 0;

  do {
    pt = pp->mem;

    if (iter > 0) {
      *tmpp = pt[0];
      pt[0] = tmp;
    }

    DB_FMT("[ == ] lisp_symtab_sort_backprog: i = %d", ((pp == p)? pp_idx: 0));
    DB_FMT("[ == ] lisp_symtab_sort_backprog: lim = %d",
           ((pp == base_pp)? base_idx: SYMPOOL));

    for (uint i   = ((pp == p)? pp_idx: 0),
              lim = ((pp == base_pp)? base_idx: SYMPOOL);
         i < lim; ++i) {
      tmp     = pt[i+1];
      pt[i+1] = pt[i];
      pt[i]   = tmp;
    }

    tmpp = &pt[pp == base_pp? base_idx: SYMPOOL];

    if (base_pp == pp) {
      return;
    }
  } while (++iter, pp->next);
}

static void lisp_symtab_sort_com_part(POOL_T* pp, struct lisp_sym* ppm,
                                      struct lisp_hash hash, uint idx) {}


static void lisp_symtab_sort_psum(POOL_T* pp, struct lisp_sym* ppm,
                                  struct lisp_hash hash, uint idx) {}

static void lisp_symtab_sort_sum(POOL_T* pp, struct lisp_sym* ppm,
                                 struct lisp_hash hash, uint idx) {
}

static void lisp_symtab_sort_len(POOL_T* pp, struct lisp_sym* ppm,
                                 struct lisp_hash hash, uint idx) {
  POOL_T* base  = pp;
  uint base_idx = idx;
  uint pp_idx   = 1;

  if (idx > 0) {
    // bigger than the one before: the good ending
    if (hash.len > ppm[idx-1].hash.len) {
      return;
    }

    // inclusively in between its base and its neighbour
    if (hash.len >= ppm[0].hash.len && hash.len <= ppm[idx-1].hash.len) {
      lisp_symtab_sort_sum(pp, ppm, hash, idx);
      return;
    }
  }

  // smaller than its current thread
  while (pp->prev) {
    pp  = pp->prev;
    ppm = pp->mem;

    if (hash.len >= ppm[0].hash.len && hash.len <= ppm[IDX_HM(SYMPOOL)].hash.len) {
      if (ppm[0].hash.len == ppm[IDX_HM(SYMPOOL)].hash.len) {
        goto next;
      }

      for (; pp_idx < SYMPOOL; ++pp_idx) {
        // exclusively bigger: we have a gap!
        // back-prog with the one we're one, then bail
        if (ppm[pp_idx].hash.len   > hash.len &&
            ppm[pp_idx-1].hash.len < hash.len) {
          lisp_symtab_sort_backprog(base, base_idx, pp, pp_idx);
          return;
        }
      }
    }
  }

  uint cpp_idx = pp->idx;

  for (pp_idx = 1; pp_idx < cpp_idx; ++pp_idx) {
    if (ppm[pp_idx-1].hash.len < hash.len) {
      lisp_symtab_sort_backprog(base, base_idx, pp, pp_idx);
      return;
    }
  }

  lisp_symtab_sort_backprog(base, base_idx, pp, 0);
  return;

next:
  lisp_symtab_sort_sum(pp, ppm, hash, idx);
}

/** the sort fields are (by priority):

    1. ::len
    2. ::sum
    3. ::psum
    4. ::com_part

    if any of the fields above are the same, the next one in line will sort the
    entries. these functions are called on top of one another
*/
static void lisp_symtab_sort(POOL_T* pp, struct lisp_hash hash, uint idx) {
  struct lisp_sym* ppm = pp->mem;

  // at the absolute beginning: do nothing
  if (!pp->prev && !idx) {
    return;
  }

  lisp_symtab_sort_len(pp, ppm, hash, idx);
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

// TODO: we could probably return `struct lisp_sym_ret' like `get' does
int lisp_symtab_set(struct lisp_sym sym) {
  int ret = 0;

  const uint idx = HASH_IDX(sym.hash);
  uint ppent_idx = 0;

  POOL_T*    mpp = symtab_pp[idx].mem;
  ppent_idx      = mpp->idx;
  POOL_RET_T pr  = pool_add_node(mpp);
  assert(pr.stat == 0, OR_ERR());

  if (pr.base != pr.mem) {
    mpp       = symtab_pp[idx].mem = pr.mem;
    ppent_idx = 0;
  }

  DB_FMT("[ == ] symtab: adding at index %d\n---", idx);
  mpp->mem[IDX_HM(mpp->idx)] = sym;

  lisp_symtab_sort(mpp, sym.hash, ppent_idx);

  done_for(ret);
}

struct lisp_sym_ret lisp_symtab_get(struct lisp_hash hash) {
  struct lisp_sym_ret ret = {0};
  uint idx = HASH_IDX(hash);

  DB_FMT("[ == ] symtab: trying to get index %d", idx);

  POOL_T* pp = symtab_pp[idx].mem;
  struct lisp_sym* tab = pp->mem;

  // TODO: stub; this is wrong in general

  ret.master = &pp->mem[0];

  done_for(ret);
}

int symtab_init(void) {
  for (uint i = 0; i < SYMTAB_CELL; ++i) {
    symtab_pp[i].mem = symtab_pp[i].base = (symtab + i);
  }

  return 0;
}
