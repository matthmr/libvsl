#ifndef LOCK_POOL
#  define LOCK_POOL

#  define MEMPOOL_TMPL(t) __mempool_##t

#  define MEMPOOL(t,am)            \
  MEMPOOL_TMPL(t) {                \
    struct t mem[am];              \
    struct MEMPOOL_TMPL(t)* next;  \
    struct MEMPOOL_TMPL(t)* prev;  \
    struct MEMPOOL_TMPL(t)* base;  \
    uint idx;                      \
    uint used;                     \
    uint total;                    \
  }


#endif
