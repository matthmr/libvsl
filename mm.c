#include "debug.h"
#include "err.h"
#include "mm.h"

/**
   We just allocate a whole memory page. it's very rare we'll need more than
   one page, but if we do, we have functions defined for that

   TODO: implementation with mmap
 */
#ifdef MM_MMAP
#  error "TODO: `mmap' implementation is not finished"
// #  include <sys/mman.h>
#else
#  include <unistd.h>
#endif

/** @m_if: the memory interface for LIBVSL */
static struct mm_if m_if = {0};

////////////////////////////////////////////////////////////////////////////////

/**
   Allocates the whole page, returning a pointer to it
 */
static void* __alloc_page(uint pages) {
  register void* ret_t = NULL;
  register int     ret = 0;

  ret_t = sbrk(MM_PAGESIZE*pages);

  assert(ret_t != (void*) -1, err(EOOM));

  done_for_with(ret_t, ret_t = (ret? NULL: ret_t));
}

static inline struct mm_header* __mm_header_next(struct mm_header* m_header) {
  return (struct mm_header*) ((byte*) m_header + m_header->m_alloc);
}

static inline struct mm_header* __mm_header_prev(struct mm_header* m_header) {
  return (struct mm_header*) ((byte*) m_header - m_header->m_dist);
}

////////////////////////////////////////////////////////////////////////////////

/**
   Allocates @m_size bytes, returning a pointer to the memory
 */
void* mm_alloc(const uint m_size) {
  register int     ret = 0;
  register void* ret_t = NULL;

  DB_FMT("[ mm ] alloc: size = %d", m_size);

  struct mm_header* c_header = m_if.m_header;
  struct mm_header* p_header = m_if.m_header;
  struct mm_header* n_header = NULL;
  enum mm_alloc_stat  c_stat = __MM_ALLOC_FREE;

  //// FIND THE CURRENT AVAILABLE CHUNK

  // the header is virtual
  if (!c_header) {
    p_header = c_header = (struct mm_header*) m_if.m_mem;
    goto alloc;
  }

  for (;;) {
    c_stat = c_header->s_alloc;

    // this is *always* virtual
    if (!ALLOC_SELF(c_stat) && c_header->m_alloc >= m_size) {
      // NOTE: 'edge' memory is considered 'virtual' at least once
      DB_MSG("  -> alloc: on virtual memory");
      break;
    }

    // this is *never* virtual
    else if (!ALLOC_NEXT(c_stat)) {
      DB_MSG("  -> alloc: on edge");

      c_stat            |= __MM_ALLOC_NEXT;
      c_header->s_alloc |= c_stat;

      p_header = c_header;
      c_header = __mm_header_next(c_header);

      c_header->s_alloc |= (__MM_ALLOC_PREV | __MM_ALLOC_SELF);

      break;
    }

    p_header = c_header;
    c_header = __mm_header_next(c_header);
  }

  //// ALLOCATE NEW MEMORY IF NEEDED

alloc:

  if (m_if.m_upper < c_header) {
    m_if.m_upper  = c_header;
    m_if.m_size  += (m_size + sizeof(struct mm_header));
  }

  if (m_if.m_size > m_if.m_cap) {
    const uint m_pages = ((m_if.m_size - m_if.m_cap) / MM_PAGESIZE) + 1;
    ret_t = __alloc_page(m_pages);
    assert(ret_t, OR_ERR());

    m_if.m_cap += m_pages*MM_PAGESIZE;
  }

  //// FIX UP THE CHUNK CHAIN

  /* virtualize the 'empty space', having a header there that points to
     the `next' of the current available space.

     visualization:

     [....|...........|.] ...
      ^~~~ ^~~~~~~~~~~ ^~~~~~
      PH   VH          NH
      ^    ^ vm_header ^ n_header
      c_header

      `PH' used to occupy the whole space between itself and `NH', but now
      this new 'empty space' will be given to `VH', unless the very specific
      case of the new `PH' being the same as the old `PH' happens, in which
      case we just return `PH' back to normal
  */
  if (!ALLOC_SELF(c_header->s_alloc) && ALLOC_NEXT(c_stat)) {
    n_header = __mm_header_next(c_header);

    if (c_header->m_alloc > m_size) {
      DB_MSG("  -> alloc: new virtual memory");
      c_header->m_alloc  = m_size;

      register struct mm_header* vm_header = __mm_header_next(c_header);

      n_header->m_dist   = (uint) ((byte*) n_header - (byte*) vm_header);
      n_header->s_alloc |= __MM_ALLOC_PREV;

      vm_header->m_alloc = n_header->m_dist;
      vm_header->m_dist  = (m_size + sizeof(struct mm_header));
      vm_header->s_alloc = (__MM_ALLOC_PREV | __MM_ALLOC_NEXT);
    }
    else {
      DB_MSG("  -> alloc: reusing virtual memory");
      n_header->s_alloc |= __MM_ALLOC_PREV;
    }
  }

  //// SETUP THE RETURNING MEMORY

  c_header->m_alloc  = (m_size + sizeof(struct mm_header));
  c_header->s_alloc |= __MM_ALLOC_SELF;
  c_header->m_dist   = (uint) ((byte*) c_header - (byte*) p_header);

  m_if.m_header = c_header;
  ret_t         = (c_header + 1);

  done_for((ret_t = ret? NULL: ret_t));
}

/**
   Deallocates the memory of @m_mem
 */
void mm_free(void* m_mem) {
  if (!m_mem) {
    return;
  }

  struct mm_header* c_header = ((struct mm_header*) m_mem - 1);
  struct mm_header* p_header = NULL;
  struct mm_header* n_header = NULL;
  enum mm_alloc_stat  c_stat = __MM_ALLOC_FREE;
  enum mm_alloc_stat   _stat = __MM_ALLOC_FREE;

#ifdef DEBUG
  register const uint m_size = (uint)
    (c_header->m_alloc - sizeof(struct mm_header));
#endif

  DB_FMT("[ mm ] free: size = %d", m_size);

  c_header->s_alloc &= ~__MM_ALLOC_SELF;
  c_stat             = c_header->s_alloc;

  //// FIND THE CURRENT AVAILABLE CHUNK

  if (ALLOC_PREV(c_stat)) {
    DB_MSG("  -> free: handle prev memory");

    p_header = __mm_header_prev(c_header);
    _stat    = p_header->s_alloc;

    // prev is virtual: join it with us
    if (!ALLOC_SELF(_stat)) {
      DB_MSG("  -> free: prev is virtual");

      p_header->m_alloc += c_header->m_alloc;
      c_header           = p_header;

      // this is never virtual
      if (!ALLOC_PREV(_stat)) {
        goto alloc_next;
      }
      else {
        p_header = __mm_header_prev(p_header);
      }
    }

    p_header->s_alloc &= ~__MM_ALLOC_NEXT;
  }

alloc_next:

  if (ALLOC_NEXT(c_stat)) {
    DB_MSG("  -> free: handle next memory");

    n_header = __mm_header_next(c_header);
    _stat    = n_header->s_alloc;

    // next is virtual: join us with it
    if (!ALLOC_SELF(_stat)) {
      DB_MSG("  -> free: next is virtual");

      c_header->m_alloc += n_header->m_alloc;

      // this is never virtual
      if (!ALLOC_NEXT(_stat)) {
        goto done;
      }
      else {
        n_header         = __mm_header_next(n_header);
        n_header->m_dist = c_header->m_alloc;
      }
    }

    n_header->s_alloc &= ~__MM_ALLOC_PREV;
  }

  n_header = __mm_header_next(c_header);

  if (m_if.m_upper >= n_header) {
    c_stat = n_header->s_alloc;

    if (ALLOC_PREV(c_stat)) {
      DB_MSG("  -> free: fix next memory");
      n_header->s_alloc &= ~__MM_ALLOC_PREV;
    }
  }

done:
  m_if.m_header = c_header;
}

int mm_init(void) {
  register int ret = 0;

  register const uint m_pages = 7;

  m_if.m_size = 0;
  m_if.m_mem  = __alloc_page(m_pages);

  assert(m_if.m_mem, OR_ERR());

  m_if.m_cap    = m_pages*MM_PAGESIZE;
  m_if.m_header = NULL;
  m_if.m_upper  = NULL;

  // the header starts out virtual
  struct mm_header* vm_header = (struct mm_header*) m_if.m_mem;

  vm_header->m_dist  = vm_header->m_alloc = 0;
  vm_header->s_alloc = __MM_ALLOC_FREE;

  done_for(ret);
}
