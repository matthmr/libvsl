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
    uint idx;               \
    t mem[am];              \
  }

/**
   MEMPOOL(t,am)
   -------------

   @mem:   memory pool
   @next:  next section
   @prev:  previous section
   @idx:   index of the current section within the pool chain; also used as
           length. use the `IDX_HM' macro from `utils.h' to use it as an array
           index
 */

#  define MEMPOOL_RET(t)    \
  POOL_RET_T {              \
    struct __mempool* mem;  \
    struct __mempool* base; \
    t*   entry;             \
    int  stat;              \
  }

/**
   MEMPOOL_RET(t)
   --------------

   @mem:   current memory pool        (what we attach, child)
   @base:  base memory pool           (what we attach to, root)
   @entry: current memory pool entry
   @stat:  exit status
 */

#  ifndef POOL_ENTRY_T
#    error [ !! ] No POOL_ENTRY_T macro defined in the includer. Cannot compile
#  endif

#  ifndef POOL_AM
#    error [ !! ] No POOL_AM macro defined in the includer. Cannot compile
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
  .idx   = 0,
};
#  define POOL __mempool_t

// the thread pointer
static POOL_T* __mempool_tp = &__mempool_t;
#  define POOLP __mempool_tp

#endif

#ifndef LOCK_POOL_DEF
#  define LOCK_POOL_DEF

/**
   Adds a node to a memory pool, returning a structure with the memory for the
   node and information about if the node is on another pool thread

   @mpp: the current pool thread
 */
static POOL_RET_T pool_add_node(POOL_T* mpp) {
  POOL_RET_T ret = {
    .mem   = NULL,
    .base  = mpp,
    .entry = NULL,
    .stat  = 0,
  };

  if (mpp->idx == POOL_AM) {
    if (!mpp->next) {
      // TODO: even though this has no way to get leaked,
      //       free it when exiting `main'; also
      mpp->next = malloc(sizeof(POOL_T));

      // OOM (somehow)
      if (mpp->next == NULL) {
        defer_for_as(ret.stat, err(EOOM));
      }

      mpp->next->prev = mpp;
      mpp->next->next = NULL;
    }

    mpp->next->idx = 0;
    mpp            = mpp->next;
  }

  ret.mem   = mpp;
  ret.entry = (mpp->mem + mpp->idx);
  ++mpp->idx;

  done_for(ret);
}

/**
   Gets a memory pool thread from a given index

   @mpp: the current pool thread
   @idx: the given index
 */
static POOL_RET_T pool_from_idx(POOL_T* mpp, uint idx) {
  POOL_RET_T ret = {0};
  POOL_T* pp     = mpp;
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

  return ret;
}

#endif
