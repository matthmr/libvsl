#ifndef LOCK_POOL
#  define LOCK_POOL

#  define MEMPOOL_TMPL(t) __mempool_##t
#  define MEMPOOL_RET_TMPL(t) __mempool_ret_##t

#  define MEMPOOL(t,am)            \
  MEMPOOL_TMPL(t) {                \
    struct t mem[am];              \
    struct MEMPOOL_TMPL(t)* next;  \
    struct MEMPOOL_TMPL(t)* prev;  \
    uint idx;                      \
    uint total;                    \
    uint used;                     \
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

#  define MEMPOOL_RET(t)           \
  MEMPOOL_RET_TMPL(t) {            \
    struct MEMPOOL_TMPL(t)* mem;   \
    struct MEMPOOL_TMPL(t)* base;  \
    struct t* entry;               \
    bool      same;                \
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
