// TODO: OOM should be panic, not error

#include "debug.h"
#include "mm.h"

#include <string.h>

//// ERRORS

ECODE(EOOM);
EMSG {[EOOM] = ERR_STRING("libvsl", "out of memory")};

////////////////////////////////////////////////////////////////////////////////

/* TODO: implementation with mmap */
#ifdef MM_MMAP
#  error "[ !! ] `mmap' implementation is not done"
#else
#  include <unistd.h>
#  define ALLOC(x) sbrk(MM_PAGESIZE*(x))
#  define CBRK()   sbrk(0)
#endif

#define MM_NEXT(x) (((byte*) (__mm_payload_of(m_header))) + \
                    m_header->m_alloc + m_header->s_delta)
#define FROM_EMPTY(x) ((x) - sizeof(struct mm_header))

/** @m_if: the memory interface for LIBVSL */
static struct mm_if m_if = {0};

////////////////////////////////////////////////////////////////////////////////

/** Allocates @m_pages whole pages, returning a pointer to it */
static void* __mm_alloc_page(uint m_pages) {
  DB_FMT("  -> alloc: allocating %d new pages", m_pages);

  void* ret_t = NULL;
  int     ret = 0;

  ret_t = ALLOC(m_pages);

  assert(ret_t != (void*) -1, err(EOOM));

  done_for_with(ret_t, ret_t = (ret? NULL: ret_t));
}

/** Returns a pointer to the memory header given some managed memory */
static inline struct mm_header* __mm_header_of(void* mem) {
  return ((struct mm_header*) (mem)) - 1;
}

/** Returns a pointer to the managed memory given some memory header */
static inline void* __mm_payload_of(struct mm_header* m_header) {
  return m_header + 1;
}

/** Generic 'current break' function */
static inline void* __mm_cbrk(void) {
  return CBRK();
}

/** Optional return of next header */
static inline struct mm_header* __mm_header_next(struct mm_header* m_header) {
  register struct mm_header* ret = (struct mm_header*) MM_NEXT(m_header);

  if (ret >= m_if.m_upper) {
    ret = NULL;
  }

  return ret;
}

/** Returns a bound header given some edge header @m_header and some payload
    size @m_size. This header will always be virtual at first */
static inline struct mm_header*
__mm_header_next_from(struct mm_header* m_header, uint m_size) {
  return (struct mm_header*)
    ((byte*)(__mm_payload_of((struct mm_header*) MM_NEXT(m_header)))
     + m_size);
}

////////////////////////////////////////////////////////////////////////////////

/** 'First-time' variant of `mm_alloc'. Returns the header for the tail of
    `mm_alloc' */
static struct mm_header* mm_ftalloc(const uint m_size) {
  register int     ret = 0;
  register void* ret_t = NULL;

  struct mm_header* c_header = m_if.m_mem;
  struct mm_header* n_header = NULL;
  struct mm_header* u_header =
    __mm_header_next_from(c_header, FROM_EMPTY(m_size));

  register uint t_size = m_size + sizeof(*c_header);

  m_if.m_size += t_size;

  if (m_if.m_size > m_if.m_cap) {
    const register void* c_brk = __mm_cbrk();
    register uint      m_pages = 0;

    // it could be the case that some other program used `brk' to allocate some
    // memory: we have to handle it
    if (c_brk != ((byte*)m_if.m_mem + m_if.m_cap)) {
      DB_MSG("  -> alloc: allocation clash!");

      register const uint vm_alloc = m_if.m_cap - sizeof(*c_header);

      // the memory of the first page becomes completly virtual until the
      // boundary
      c_header->s_delta = ((byte*)c_brk - (byte*)(c_header + vm_alloc));
      c_header->m_alloc = vm_alloc;

      m_if.m_size -= t_size;
      m_if.m_size += t_size + (m_if.m_cap - m_if.m_size);

      u_header = __mm_header_next_from(c_header, m_size);
      m_pages  = PAGES_FOR(t_size);
    }
    else {
      m_pages  = PAGES_FOR(m_if.m_size - m_if.m_cap);
    }

    ret_t = __mm_alloc_page(m_pages);
    assert(ret_t, OR_ERR());

    m_if.m_cap += m_pages*MM_PAGESIZE;

    if (c_header->s_delta) {
      n_header = __mm_header_next(c_header);

      n_header->h_prev  = c_header;
      n_header->s_alloc = __MM_ALLOC_FREE;
      n_header->s_delta = 0;

      c_header = n_header;
    }
  }

  c_header->m_alloc = m_size;
  m_if.m_upper = u_header;

  done_for((ret? NULL: c_header));
}

////////////////////////////////////////////////////////////////////////////////

void* mm_alloc(const uint m_size) {
  register int     ret = 0;
  register void* ret_t = NULL;

  DB_FMT("[ mm ] alloc: size = %d", m_size);

  struct mm_header* c_header = m_if.m_header;
  struct mm_header* p_header = NULL;
  struct mm_header* n_header = NULL;

  register uint t_size = m_size + sizeof(*c_header);
  enum mm_alloc_stat  c_stat = __MM_ALLOC_FREE;

  if (!c_header) {
    c_header = mm_ftalloc(m_size);
    assert(c_header, OR_ERR());
  }

  ////

  for (;;) {
    c_stat = c_header->s_alloc;

    // this header is virtual, and has enough space: alloc on it
    if (!ALLOC_SELF(c_stat) && c_header->m_alloc >= m_size) {
      DB_MSG("  -> alloc: on virtual memory");
      break;
    }

    // the next header is empty: alloc on it
    else if (!ALLOC_NEXT(c_stat)) {
      DB_MSG("  -> alloc: on edge");

      c_header->s_alloc |= __MM_ALLOC_NEXT;

      // premptively shift the header to after the payload
      p_header = c_header;
      c_header = __mm_header_next_from(c_header, m_size);

      break;
    }

    p_header = c_header;
    c_header = __mm_header_next(c_header); // this cannot fail
  }

  ////

  if (m_if.m_upper < c_header) {
    DB_FMT("  -> alloc: new upper: %p", c_header);
    m_if.m_upper = c_header;
    m_if.m_size += t_size;
  }

  if (m_if.m_size > m_if.m_cap) {
    const register void* c_brk = __mm_cbrk();
    register uint      m_pages = 0;

    // it could be the case that some other program used `brk' to allocate some
    // memory: we have to handle it
    if (c_brk != ((byte*)m_if.m_mem + m_if.m_cap)) {
      DB_MSG("  -> alloc: allocation clash!");

      p_header->s_delta = ((byte*)c_brk - (byte*)__mm_header_next(p_header));
      m_pages = PAGES_FOR(t_size);

      m_if.m_size -= t_size;
      m_if.m_size += t_size + (m_if.m_cap - m_if.m_size);
    }
    else {
      m_pages = PAGES_FOR(m_if.m_size - m_if.m_cap);
    }

    ret_t = __mm_alloc_page(m_pages);
    assert(ret_t, OR_ERR());

    *(struct mm_header*)ret_t = (struct mm_header) {
      .m_alloc = 0,
      .s_delta = 0,
      .s_alloc = __MM_ALLOC_FREE,
      .h_prev  = NULL,
    };

    m_if.m_upper =
      __mm_header_next_from((struct mm_header*)ret_t, FROM_EMPTY(m_size));

    m_if.m_cap += m_pages*MM_PAGESIZE;
  }

  ////

  // edge allocation
  if (ALLOC_SELF(c_stat)) {
    c_header = __mm_header_next(p_header); // this cannot fail
    c_header->s_alloc |= __MM_ALLOC_PREV;
    c_header->h_prev   = p_header;
  }

  // virtual allocation
  else {
    register uint vm_alloc = c_header->m_alloc;

    if ((p_header = c_header->h_prev)) {
      p_header->s_alloc |= __MM_ALLOC_NEXT;
    }

    p_header = c_header;
    n_header = __mm_header_next(c_header);

    // manage remaining virtual memory
    if (c_header->m_alloc != m_size) {
      c_header->m_alloc = m_size;
      c_header = __mm_header_next(c_header); // this always exists

      if (p_header->s_delta) {
        c_header->s_delta = p_header->s_delta;
        p_header->s_delta = 0;
      }

      c_header->m_alloc = (vm_alloc - m_size);
      c_header->s_alloc = __MM_ALLOC_FREE | __MM_ALLOC_PREV;
      c_header->h_prev  = p_header;

      if (n_header) {
        // n_header->s_alloc &= ~__MM_ALLOC_PREV;
        n_header->h_prev   = c_header;
        c_header->s_alloc |= __MM_ALLOC_NEXT;
      }
    }

    else if (n_header) {
      n_header->s_alloc |= __MM_ALLOC_PREV;
      n_header->h_prev   = c_header;
    }

    c_header = p_header;
  }

  ////

  c_header->m_alloc  = m_size;
  c_header->s_alloc |= __MM_ALLOC_SELF;
  c_header->s_delta  = 0;

  m_if.m_header = c_header;
  ret_t = __mm_payload_of(c_header);

  done_for((ret_t = ret? NULL: ret_t));
}

void mm_free(void* m_mem) {
  if (!m_mem) {
    return;
  }

  struct mm_header* c_header = __mm_header_of(m_mem);
  struct mm_header* p_header = NULL;
  struct mm_header* n_header = NULL;
  enum mm_alloc_stat  c_stat = __MM_ALLOC_FREE;

  DB_FMT("[ mm ] free (%p): size = %d", m_mem, c_header->m_alloc);

  c_header->s_alloc &= ~__MM_ALLOC_SELF;
  c_stat = c_header->s_alloc;

  ////

  if ((p_header = c_header->h_prev)) {
    if (ALLOC_PREV(c_stat)) {
      DB_MSG("  -> free: prev alloc");

      p_header->s_alloc &= ~__MM_ALLOC_NEXT;
    }
    else {
      DB_MSG("  -> free: prev virtual");

      p_header->m_alloc += c_header->m_alloc;
      c_header = p_header;
    }
  }

  ////

  if ((n_header = __mm_header_next(c_header))) {
    n_header->h_prev = c_header;

    if (ALLOC_NEXT(c_stat)) {
      DB_MSG("  -> free: next alloc");

      n_header->s_alloc &= ~__MM_ALLOC_PREV;
    }
    else {
      DB_MSG("  -> free: next virtual");

      c_header->m_alloc += n_header->m_alloc;

      if ((n_header = __mm_header_next(n_header))) {
        n_header->s_alloc &= ~__MM_ALLOC_PREV;
      }
    }
  }

  m_if.m_header = c_header;
}

void* mm_ndup(void* m_src, uint n) {
  register int ret = 0;
  void*      ret_t = NULL;

  ret_t = mm_alloc(n);
  assert(ret_t, OR_ERR());

  memcpy(ret_t, m_src, n);

  done_for((ret_t = ret? NULL: ret_t));
}

// TODO: maybe we could host a low-memory structure for easy references to
// 'known' memory gaps?
int mm_init(void) {
  register int ret = 0;
  register int pages_init = 1;

  m_if.m_size = 0;
  m_if.m_mem  = __mm_alloc_page(pages_init);

  assert(m_if.m_mem, OR_ERR());

  m_if.m_header = NULL;
  m_if.m_upper  = NULL;
  m_if.m_cap    = pages_init*MM_PAGESIZE;

  *(struct mm_header*) m_if.m_mem = (struct mm_header) {
    .m_alloc = 0,
    .s_delta = 0,
    .s_alloc = __MM_ALLOC_FREE,
    .h_prev  = NULL,
  };

  done_for(ret);
}
