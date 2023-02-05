#include "symtab.h"
#include "debug.h"
#include "err.h"

// TODO: implement lexical scoping

#define SORT_FUNC_FOR(x) \
  static inline bool good_ending__##x(struct lisp_sym* ppm,            \
                                      struct lisp_hash hash,           \
                                      uint idx) {                      \
    return hash.x > ppm[idx].hash.x;                                   \
  } \
  static inline bool in_between__##x(struct lisp_sym* ppm,             \
                                     struct lisp_hash hash,            \
                                      uint lower, uint upper) {        \
    return hash.x >= ppm[lower].hash.x && hash.x <= ppm[upper].hash.x; \
  } \
  static inline bool ex_between__##x(struct lisp_sym* ppm,             \
                                     struct lisp_hash hash,            \
                                     uint lower, uint upper) {         \
    return hash.x > ppm[lower].hash.x && hash.x < ppm[upper].hash.x;   \
  } \
  static inline bool eq__##x(uint n, struct lisp_hash hash) {          \
    return n == hash.x;                                                \
  } \
  static inline bool lt__##x(uint n, struct lisp_hash hash) {          \
    return n < hash.x;                                                 \
  } \
  static inline uint yield__##x(struct lisp_sym* ppm, uint i) {        \
    return (uint) ppm[i].hash.x;                                       \
  } \
  struct sort_t sort_##x = {         \
    .good_ending = good_ending__##x, \
    .in_between  = in_between__##x,  \
    .ex_between  = ex_between__##x,  \
    .eq          = eq__##x,          \
    .lt          = lt__##x,          \
    .yield       = yield__##x,       \
                                     \
    .next        = NULL,             \
  }

// what lack of classes does to a mfr...

SORT_FUNC_FOR(len);
SORT_FUNC_FOR(sum);
SORT_FUNC_FOR(psum);
SORT_FUNC_FOR(com_part);

struct sort_t* sort_entry;

////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////

enum lisp_symtab_sort {
  SORT_RETURN = -1,
  SORT_OK     = 0,
  SORT_NEXT   = 1,
};

static inline int lisp_symtab_sort_small(uint* pp_idx, const uint bpp_idx,
                                         struct lisp_sym* ppm,
                                         struct lisp_hash hash,
                                         struct sort_t* sort) {
  struct {
    uint idx;
    uint am;
  } sm_idx = {
    .idx = -1u,
    .am  = 0,
  };

  for (uint i = 0, lower = 0; i < bpp_idx; ++i) {
    uint len = sort->yield(ppm, i);

    if (sort->lt(len, hash)) {
      if (len > lower) {
        *pp_idx = (i+1);
      }
    }

    else if (sort->eq(len, hash)) {
      if (sm_idx.idx == -1u) {
        sm_idx.idx = i;
      }
      ++sm_idx.am;
    }
  }

  if (sm_idx.am) {
    return SORT_OK;
  }

  return (*pp_idx == bpp_idx)? SORT_RETURN: SORT_OK;
}

/**             pp        base_pp
   [ . . . . | . . . . | . . . . ]
                 ^pp_idx       ^base_idx
*/
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

////////////////////////////////////////////////////////////////////////////////

static void lisp_symtab_sort_base(POOL_T* pp, struct lisp_sym* ppm,
                                  struct lisp_hash hash, uint idx,
                                  struct sort_t* sort) {
  POOL_T* base  = pp;
  uint base_idx = idx;
  uint pp_idx   = 0;

  if (idx > 0) {
    if (sort->good_ending(ppm, hash, (idx-1))) {
      return;
    }

    // inclusively in between its base and its neighbour
    if (sort->in_between(ppm, hash, 0, (idx-1))) {
      int sstat = lisp_symtab_sort_small(&pp_idx, pp->idx, ppm, hash, sort);

      if (sstat == SORT_NEXT) {
        goto next;
      }

      lisp_symtab_sort_backprog(base, base_idx, pp, pp_idx);
      return;
    }
  }

  // smaller than its current thread
  while (pp->prev) {
    pp  = pp->prev;
    ppm = pp->mem;

    // in this pool thread
    if (sort->in_between(ppm, hash, 0, IDX_HM(SYMPOOL))) {
      for (; pp_idx < SYMPOOL; ++pp_idx) {
        // exclusively bigger: we have a gap; back-prog with the one we're on
        if (sort->ex_between(ppm, hash, pp_idx, (pp_idx-1))) {
          lisp_symtab_sort_backprog(base, base_idx, pp, (pp_idx - 1));
          return;
        }
      }
    }
  }

  pp_idx    = 1;
  int sstat = lisp_symtab_sort_small(&pp_idx, pp->idx, ppm, hash, sort);

  switch (sstat) {
  case SORT_RETURN:
    return;
  case SORT_NEXT:
    goto next;
  }

  // we tried to find something smaller us, if we hit this function we're
  // probably the smallest in the thread, so we swap with the base.
  // this is the *worst* case, and is O(n) time-wise
  lisp_symtab_sort_backprog(base, base_idx, pp, 0);
  return;

next:
  if (sort->next) {
    lisp_symtab_sort_base(pp, ppm, hash, idx, sort->next);
  }
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

  lisp_symtab_sort_base(pp, ppm, hash, idx, sort_entry);
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

  sort_len.next  = &sort_sum;
  sort_sum.next  = &sort_psum;
  sort_psum.next = &sort_com_part;

  sort_entry = &sort_len;

  return 0;
}
