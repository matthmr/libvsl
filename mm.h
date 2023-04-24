#ifndef LOCK_MM
#  define LOCK_MM

#  include "utils.h"

#  ifndef MM_PAGESIZE
// i'm not aware if you can get the page size at compile time so I hope you *do*
// have 4KiB pages ¯\(ツ)/¯
#    define MM_PAGESIZE (4096)
#  endif

/**
   These interfaces are used for absolute/relative memory indexing and for
   freeing memory

   NOTE: this interface is *unique*, meaning that you should really have one of
   it per mem-op, e.g one for each alloc
 */

/** UNUSED */
struct mm_chunk {
  /** @t_next, @t_prev: thread pointers                                 */
  struct mm_chunk* t_next, * t_prev;

  byte* p_base; /** @p_base: base chunk pointer                         */

  uint   e_idx; /** @e_idx: chunk's current element offset [0,MM_CHUNK) */
  uint   t_idx; /** @t_idx: chunk's current thread offset               */
};

////////////////////////////////////////////////////////////////////////////////

enum mm_alloc_stat {
  __MM_ALLOC_FREE = 0,

  __MM_ALLOC_SELF = BIT(0),
  __MM_ALLOC_NEXT = BIT(1),
  __MM_ALLOC_PREV = BIT(2),
};

#  define ALLOC_SELF(x) ((x) & __MM_ALLOC_SELF)
#  define ALLOC_NEXT(x) ((x) & __MM_ALLOC_NEXT)
#  define ALLOC_PREV(x) ((x) & __MM_ALLOC_PREV)

struct mm_header {
  uint m_alloc; /** @m_alloc: size of the current payload */
  uint  m_dist; /** @m_dist:  the distance from the previous
                    header, 0 for no previous header      */

  /** @s_alloc: allocation status for neighbouring chunks */
  enum mm_alloc_stat s_alloc;
};

struct mm_if {
  ulong m_size; /** @m_size: the current allocated size  */
  ulong  m_cap; /** @m_cap:  the current memory capacity */
  byte*  m_mem; /** @m_mem:  the current memory base     */

  /** @m_header: the current header, NULL means this
      is a virtual header                                */
  struct mm_header* m_header;
  /** @m_upper   the current header upper bound; does not
      have to be allocated                               */
  struct mm_header* m_upper;
};

////////////////////////////////////////////////////////////////////////////////

int   mm_init(void);
void* mm_alloc(const uint m_size);
void  mm_free(void* m_mem);

#endif
