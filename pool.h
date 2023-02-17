#ifndef LOCK_POOL
#  define LOCK_POOL

#  include <stdlib.h>

#  include "utils.h"
#  include "err.h"

#  define POOL_T     struct __mempool
#  define POOL_RET_T struct __mempool_ret

#  define MEMPOOL(t,am)     \
   POOL_T {                 \
    struct __mempool* next; \
    struct __mempool* prev; \
    uint c_idx;             \
    uint p_idx;             \
    t mem[am];              \
  }

/**
   MEMPOOL(t,am)
   -------------

   @mem:   memory pool
   @next:  next section
   @prev:  previous section
   @c_idx: index of the current section within the pool chain
   @p_idx: index of the current element within the pool thread
 */

#  define MEMPOOL_RET(t)    \
  POOL_RET_T {              \
    struct __mempool* new;  \
    struct __mempool* base; \
    t*   entry;             \
    int  stat;              \
  }

/**
   MEMPOOL_RET(t)
   --------------

   @new:   current memory pool        (what we attach, child)
   @base:  base memory pool           (what we attach to, root)
   @entry: current memory pool entry
   @stat:  exit status
 */

#  ifndef POOL_ENTRY_T
#    error "[ !! ] libvsl(cc): No POOL_ENTRY_T macro defined in the includer. Cannot compile"
#  endif

#  ifndef POOL_AM
#    error "[ !! ] libvsl(cc): No POOL_AM macro defined in the includer. Cannot compile"
#  endif

MEMPOOL(POOL_ENTRY_T, POOL_AM);
MEMPOOL_RET(POOL_ENTRY_T);
#endif

#ifndef LOCK_POOL_THREAD
#  define LOCK_POOL_THREAD

// the first thread entry
static POOL_T __mempool_t = {
  .mem   = {0},
  .next  = NULL,
  .prev  = NULL,
  .c_idx = 0,
  .p_idx = 0,
};
#  define POOL __mempool_t

static POOL_T* __mempool_p = &__mempool_t;
#  define POOL_P __mempool_p

#endif

#ifndef LOCK_POOL_DEF
#  define LOCK_POOL_DEF

/**
   Adds a node to a memory pool, returning a structure with the memory for the
   node and information about if the node is on another pool thread

   @pp: the current pool thread
 */
static POOL_RET_T pool_add_node(POOL_T* pp) {
  POOL_RET_T ret = {
    .new   = NULL,
    .base  = pp,
    .entry = NULL,
    .stat  = 0,
  };

  if (pp->p_idx == POOL_AM) {
    if (!pp->next) {
      // TODO: even though this has no way to get leaked,
      //       free it when exiting `main'; also
      pp->next = malloc(sizeof(POOL_T));

      // OOM (somehow)
      if (pp->next == NULL) {
        defer_for_as(ret.stat, err(EOOM));
      }

      pp->next->prev  = pp;
      pp->next->next  = NULL;
      pp->next->c_idx = pp->c_idx + 1;
    }

    pp->next->p_idx = 0;
    pp              = pp->next;
  }

  ret.new   = pp;
  ret.entry = (pp->mem + pp->p_idx);
  ++pp->p_idx;

  done_for(ret);
}

/**
   Gets a memory pool thread from a given index. The difference is computed
   relatively, so we don't *always* have O(n) time search

   @pp:   the current pool thread
   @c_idx: the given index
 */
static POOL_RET_T pool_from_idx(POOL_T* pp, uint c_idx) {
  POOL_RET_T ret = {0};
  int diff       = (c_idx - pp->c_idx);
  ret.base       = pp;

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

  ret.new   = pp;
  ret.entry = pp->mem;

  return ret;
}

#endif
