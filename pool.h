#ifndef LOCK_POOL
#  define LOCK_POOL

#  include "utils.h"
#  include <stdlib.h>

#  define MEMPOOL_TMPL(t) __mempool_##t
#  define MEMPOOL_RET_TMPL(t) __mempool_ret_##t

#  define MEMPOOL(t,name,am)          \
  MEMPOOL_TMPL(name) {                \
    t mem[am];                        \
    struct MEMPOOL_TMPL(name)* next;  \
    struct MEMPOOL_TMPL(name)* prev;  \
    uint idx;                         \
    uint total;                       \
    uint used;                        \
  }

/**
   MEMPOOL(t,am)
   -------------

   @mem:   memory pool
   @next:  next section
   @prev:  previous section
   @idx:   index of the current section within the pool chain
   @total: total number of entries
   @used:  total number of entries used
 */

#  define MEMPOOL_RET(t,name)         \
  MEMPOOL_RET_TMPL(name) {            \
    struct MEMPOOL_TMPL(name)* mem;   \
    struct MEMPOOL_TMPL(name)* base;  \
    t*        entry;                  \
    bool      same;                   \
  }

/**
   MEMPOOL_RET(t)
   --------------

   @mem:   current memory pool        (what we attach, child)
   @base:  base memory pool           (what we attach to, root)
   @entry: current memory pool entry
   @same:  inclusion boolean
 */

#endif
