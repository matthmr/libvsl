#ifndef LOCK_MM
#  define LOCK_MM

#  include "utils.h"

#  ifndef MM_PAGESIZE
// i'm not aware if you can get the page size at compile time so I hope you *do*
// have 4KiB pages ¯\(ツ)/¯
#    define MM_PAGESIZE (4096)
#  endif

#  define PAGES_FOR(size) \
  (((size) / MM_PAGESIZE) + (int) (((size) % MM_PAGESIZE) != 0))

/** Header status (bitfield) */
enum mm_alloc_stat {
  __MM_ALLOC_FREE = 0,

  __MM_ALLOC_SELF = BIT(0),
  __MM_ALLOC_NEXT = BIT(1),
  __MM_ALLOC_PREV = BIT(2),
};

#  define ALLOC_SELF(x) ((x) & __MM_ALLOC_SELF)
#  define ALLOC_NEXT(x) ((x) & __MM_ALLOC_NEXT)
#  define ALLOC_PREV(x) ((x) & __MM_ALLOC_PREV)

/** Memory chunk header */
struct mm_header {
  /* size of the current payload */
  uint m_alloc;

  /* the previous header */
  struct mm_header* h_prev;

  /* the delta to another managed section, from our allocation */
  uint s_delta;

  /* allocation status for self and neighbouring chunks */
  enum mm_alloc_stat s_alloc;
};

/** Memory allocator interface. The layout of memory is that the first
    `sizeof(struct mm_header)' bytes are for the header in the chunk, and the
    rest of bytes discriminated in the header are for the payload

    There is only one allocator per thread */
struct mm_if {
  /* the current allocated size, along with headers */
  ulong m_size;

  /* the current memory capacity */
  ulong m_cap;

  /* the current memory base */
  struct mm_header* m_mem;

  /* the current header upper bound. This header should always be virtual */
  struct mm_header* m_upper;

  /* the current header */
  struct mm_header* m_header;
};

////////////////////////////////////////////////////////////////////////////////

/** Inits the global allocator */
int mm_init(void);

/** Allocates @m_size bytes, returning a pointer to the memory */
void* mm_alloc(const uint m_size);

/** Deallocates the memory of @m_mem */
void mm_free(void* m_mem);

///

/** Duplicate @n bytes of @m_src */
void* mm_ndup(void* m_src, uint n);

#endif
