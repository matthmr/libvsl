#include "symtab.h"
#include "debug.h"
#include "err.h"
#include "mm.h"

#include <strings.h>

// TODO: implement lexical scoping

SORT_FUNC_FOR(len);
SORT_FUNC_FOR(sum);
SORT_FUNC_FOR(wsum);
SORT_FUNC_FOR(rlov);

/**
   the sort fields are (by priority):

   1. @len
   2. @sum
   3. @wsum
   4. @rlov

   if any of the fields above are the same, the next one in line will sort the
   entries. these functions are called on top of one another through
   `lisp_symtab_sort`
*/
static struct sort_t* const sort_entry = &sort_len;
static struct lisp_sym_cell*    symtab = NULL;

////////////////////////////////////////////////////////////////////////////////

/**
   Boolean equality from hashes @hash_a and @hash_b
 */
static inline bool hash_eq(struct lisp_hash hash_a, struct lisp_hash hash_b) {
  return (hash_a.sum == hash_b.sum)  &&
    (hash_a.wsum     == hash_b.wsum) &&
    (hash_a.len      == hash_b.len)  &&
    (hash_a.rlov     == hash_b.rlov);
}

//// STATIC

/**
   Perform a single-character @c hash given the existing hasher @hash
 */
struct lisp_hash_ret hash_c(struct lisp_hash hash, char c) {
  register int ret = 0;

  struct lisp_hash_ret hash_ret;

  assert(hash.len < SYMTAB_MAX_SYM_LEN, err(EIDTOOBIG));

  ASCII_NORM(c);

  register uint _c   = ((hash.len+1)*c + hash.sum)*SYMTAB_PRIM;
  register uint _sum = hash.sum;

  hash.wsum += c*hash.w;
  hash.sum  += _c;

  if ((hash.sum & 0b111) > (_sum & 0b111)) {
    hash.rlov += _c*hash.w;
    hash.w    *= _c;
    hash.w   <<= 1;
    ++hash.w;
  }

  ++hash.len;

  hash_ret.master = hash;

  done_for_with(hash_ret, hash_ret.slave = ret);
}

/**
   Resets the value of the hasher @hash. Useful after finishing the
   single-character hash
 */
void hash_done(struct lisp_hash* hash) {
  hash->w    = 1;
  hash->sum  = 0;
  hash->wsum = 0;
  hash->len  = 0;
  hash->rlov = 0;
}

/**
   Hash string @str returning a hash to it
 */
struct lisp_hash_ret hash_str(const char* str) {
  register int ret = 0;

  struct lisp_hash_ret hash_ret = {
    .master = {
      .w = 1,
    },
    .slave  = 0,
  };

  char c = '\0';

  DB_FMT("[ symtab ] for string: %s", str);

  for (uint i = 0; (c = str[i]); ++i) {
    hash_ret = hash_c(hash_ret.master, c);

    DB_FMT("[ symtab ] character (%c) [%d]", c, HASH_IDX(hash_ret.master));

    assert(hash_ret.slave == 0, OR_ERR());
  }

  done_for_with(hash_ret, hash_ret.slave = ret);
}

////////////////////////////////////////////////////////////////////////////////

/**
   Applies the 'backprog' algorithm, similar to insertion sort.

   It swaps
 */
static struct lisp_sym*
lisp_symtab_sort_backprog(struct lisp_sym_cell* ccell, ushort goal_i,
                          struct lisp_sym sym) {
  register int       ret = 0;
  struct lisp_sym* ret_t = NULL;

  struct lisp_sym  reg_a, reg_b;

  struct lisp_sym* symarr;
  uint             c_size = 0;

  symarr = ccell->entry.sym;
  ret_t  = (ccell->entry.sym + goal_i);

  reg_a  = ccell->entry.sym[goal_i];
  ccell->entry.sym[goal_i] = sym;

  do {
    c_size = ccell->entry.size;
    symarr = ccell->entry.sym;

    for (uint i = (goal_i + 1); i < c_size; ++i) {
      reg_b     = symarr[i];
      symarr[i] = reg_a;
      reg_a     = reg_b;
    }
  } while(ccell->next && (ccell = ccell->next));

  // on the edge
  if (c_size == SYMTAB_ENTRIES) {
    ccell->next = mm_alloc(sizeof(struct lisp_sym_cell));
    assert(ccell->next, OR_ERR());

    ccell = ccell->next;
    ccell->entry.size = 0;
    symarr      = ccell->entry.sym;
    symarr[0]   = reg_a;
  }

  else {
    symarr[c_size] = reg_a;
  }

  done_for((ret_t = ret? NULL: ret_t));
}

/**
   Find the 'last equal' to a symbol @sym element given an symbol array pointed
   to by the cell @entry, and a field sorted by @sort
 */
static struct lisp_sym_cell*
lisp_symtab_sort_lsteq(struct lisp_sym_cell* ccell, uint lteq_i,
                       struct lisp_sym sym, struct sort_t* sort) {
  uint       idx = 0;
  uint hsh_yield = sort->hsh_yield(sym.hash);

  uint i = lteq_i;

  do {
    idx = ccell->entry.size;

    for (; i < idx; ++i) {
      if (hsh_yield > sort->sym_yield(ccell->entry.sym + i)) {
        break;
      }
    }

    i = 0;
  } while (ccell->next && (ccell = ccell->next));

  return ccell;
}

/**
   Tries to insert symbol @sym close to the entry cell @entry

   It will try to see if the symbol can be inserted in the current cell. This
   function is called primarily by `lisp_symtab_set_sorted', which means the
   current cell is *always* inclusive to the symbol
 */
static struct lisp_sym*
lisp_symtab_sort_insert(struct lisp_sym_cell* ccell, struct lisp_sym sym,
                        struct sort_t* sort) {
  register int       ret = 0;
  struct lisp_sym* ret_t = NULL;

  uint                 c_size = 0;
  uint                   lteq = 0;
  uint              hsh_yield = 0;

insert:
  c_size    = ccell->entry.size;
  lteq      = 0;
  hsh_yield = sort->hsh_yield(sym.hash);

  // find the last element that's smaller than or equal to us
  for (uint i = 0; i < c_size; ++i) {
    register uint sym_yield = sort->sym_yield(ccell->entry.sym + i);

    if (sym_yield <= hsh_yield) {
      lteq = i;
    }
  }

  // repeats: find the last one that does, mask the one previous, as well as
  // ourselves, with field that repeated
  if (sort->hsh_yield(sym.hash) == sort->sym_yield(ccell->entry.sym + lteq)) {
repeats:
    // the one that's equal to us, as well as ourselves, gets marked
    (ccell->entry.sym + lteq)->rep |= sort->field;
    sym.rep |= sort->field;

    ccell    = lisp_symtab_sort_lsteq(ccell, lteq, sym, sort);
    sort     = sort->next;
    assert(sort, err(EFUCKINGHASH));

    goto insert;
  }

  // there are elements to the right of us: backprog
  else if (IDX_MH(lteq) < c_size) {
    ret_t = lisp_symtab_sort_backprog(ccell, IDX_MH(lteq), sym);
  }

  // we're the last one, with empty elements to the right, and not on edge
  else if (IDX_MH(lteq) == c_size && c_size < SYMTAB_ENTRIES) {
    ret_t = (ccell->entry.sym + IDX_MH(lteq));
  }

  // we're the *literal* last one: see if the right has anything
  else if (IDX_MH(lteq) == SYMTAB_ENTRIES) {
    // there's a right: see if the first element repeats, if so we're unlucky,
    // if not do allocation
    if (ccell->next && sort->hsh_yield(sym.hash) ==
                       sort->sym_yield(ccell->next->entry.sym)) {
      ccell = ccell->next;
      goto repeats;
    }

    // there's nothing on the right: allocate the new right, put us there
    ccell->next = mm_alloc(sizeof(struct lisp_sym_cell));
    assert(ccell->next, OR_ERR());

    ccell = ccell->next;

    ccell->entry.size = 0;
    ret_t = ccell->entry.sym;
  }

  done_for((ret_t = ret? NULL: ret_t));
}

/**
   Tries to set symbol @sym at index @idx on the symbol table
 */
static struct lisp_sym*
lisp_symtab_set_sorted(struct lisp_sym_cell* ccell, struct lisp_sym sym,
                       const struct sort_t* sort) {
  register int       ret = 0;
  struct lisp_sym* ret_t = NULL;

  uint               idx = ccell->entry.size;

  // the good ending
  if (!idx) {
    ret_t = ccell->entry.sym;
    goto assign;
  }

  // the left edge is bigger than us: backprog us to the left edge
  if (sort->hsh_yield(sym.hash) < sort->sym_yield(ccell->entry.sym)) {
    ret_t = lisp_symtab_sort_backprog(ccell, 0, sym);
    goto assign;
  }

  // find the cell we're in between
  do {
    idx = ccell->entry.size;

    if (sort->in_between(ccell->entry.sym, sym.hash, 0, IDX_HM(idx))) {
      ret_t = lisp_symtab_sort_insert(ccell, sym, (struct sort_t*) sort);
      goto assign;
    }
  } while (ccell->next && (ccell = ccell->next));

  // we're bigger than the right edge: add us as the new right edge, or the new
  // symbol array
  if (idx < SYMTAB_ENTRIES) {
    ret_t = (ccell->entry.sym + idx);
  }
  else {
    ccell->next = mm_alloc(sizeof(struct lisp_sym_cell));
    assert(ccell->next, OR_ERR());

    ccell = ccell->next;

    ccell->entry.size = 0;
    ret_t = ccell->entry.sym;
  }

assign:
  ++ccell->entry.size;
  *ret_t = sym;

  done_for((ret_t = ret? NULL: ret_t));
}

/**
   Find symbol in the symbol table from hash @hash given an assorted state
 */
static struct lisp_sym*
lisp_symtab_get_sorted(struct lisp_sym_cell* ccell, struct lisp_hash hash,
                       const struct sort_t* sort) {
  register int       ret = 0;
  struct lisp_sym* ret_t = NULL;

  uint               idx = 0;

  uint         hsh_yield = sort->hsh_yield(hash);
  uint         sym_yield = sort->sym_yield(ccell->entry.sym);

  // the first entry: if the hash is smaller than the base, we know there's
  // nothing for us
  assert(sym_yield <= hsh_yield, err(ENOTFOUND));

  do {
    idx = ccell->entry.size;

    // in between the current cell
    if (sort->in_between(ccell->entry.sym, hash, 0, (idx - 1))) {
      for (uint i = 0; i < idx && sym_yield <= hsh_yield; ++i) {
        sym_yield = sort->sym_yield(ccell->entry.sym + i);

        // this repeats on the given field; find it on the next field
        if (sort->repeats(ccell->entry.sym + i)) {
          goto next;
        }

        if (sym_yield == hsh_yield) {
          ret_t = (ccell->entry.sym + i);
        }
      }
    }
  } while (ccell->next && (ccell = ccell->next));

  assert(ret_t, err(ENOTFOUND));
  defer();

next:
  if (sort->next) {
    ret_t = lisp_symtab_get_sorted(ccell, hash, sort->next);
    assert(ret_t, OR_ERR());
  }

  assert(ret_t, err(EFUCKINGHASH));

  done_for((ret_t = ret? NULL: ret_t));
}

//// STATIC

/**
   Sets @sym as a symbol on the symbol table, returning its memory
 */
struct lisp_sym* lisp_symtab_set(struct lisp_sym sym) {
  register int       ret = 0;
  struct lisp_sym* ret_t = NULL;

  const uint         idx = HASH_IDX(sym.hash);

  DB_FMT("[ symtab ] adding at index %d\n---", idx);

  ret_t = lisp_symtab_set_sorted((symtab + idx), sym, sort_entry);
  assert(ret_t, OR_ERR());

  done_for((ret_t = ret? NULL: ret_t));
}

/**
   Gets a symbol of hash @hash from the symbol table
 */
struct lisp_sym* lisp_symtab_get(struct lisp_hash hash) {
  register int       ret = 0;
  struct lisp_sym* ret_t = NULL;

  const uint         idx = HASH_IDX(hash);

  DB_FMT("[ symtab ] getting index %d", idx);

  struct lisp_sym_cell* ccell = (symtab + idx);

  assert(ccell->entry.size > 0, err(ENOTFOUND));

  ret_t = lisp_symtab_get_sorted(ccell, hash, sort_entry);
  assert(ret_t && hash_eq(ret_t->hash, hash), OR_ERR());

  done_for((ret_t = ret? NULL: ret_t));
}

int lisp_symtab_init(void) {
  register int ret = 0;

  symtab = mm_alloc(SYMTAB_CELLS*sizeof(struct lisp_sym_cell));
  assert(symtab, OR_ERR());

  // `mm_alloc' doesn't initialize the memory for us
  bzero(symtab, SYMTAB_CELLS*sizeof(struct lisp_sym_cell));

  sort_len.next  = &sort_sum;
  sort_sum.next  = &sort_wsum;
  sort_wsum.next = &sort_rlov;

  done_for(ret);
}
