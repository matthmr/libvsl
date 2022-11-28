#ifndef LOCK_VSLISP
#  define LOCK_VSLISP

#  ifndef false
#    define false 0x0
#  endif

#  ifndef true
#    define true  0x1
#  endif

#  ifndef neither
#    define neither 0x2
#  endif


#  define IOBLOCK (4096)
#  define SEXPPOOL (256)

#  define __LISP_WHITESPACE \
         0x00: \
    case 0x20: \
    case 0x09: \
    case 0x0a: \
    case 0x0b: \
    case 0x0c: \
    case 0x0d

#  define BIT(x) (1 << (x))

#  define __LISP_ALLOWED_IN_NAME(x) \
  (((x) > 0x20) || (x) != 0x7f)

#  define MEMPOOL_TMPL(t) __mempool_##t

#  define MEMPOOL(t,am)            \
  MEMPOOL_TMPL(t) {                \
    struct t mem[am];              \
    struct MEMPOOL_TMPL(t) * next; \
    uint used;                     \
    uint total;                    \
  }

typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned char bool;

struct pool_ret_t {
  void* mem;
  //int slave;
  bool  new; /**
                0 -> in the same pool
                1 -> on a different pool
              */
};

enum lisp_c {
  __LISP_PAREN_OPEN = '(',
  __LISP_PAREN_CLOSE = ')',
};

enum lisp_pev {
  __LISP_PAREN_OUT  = BIT(0),
  __LISP_SYMBOL_OUT = BIT(1),
};

enum lisp_pstat {
  __LISP_PAREN_IN  = BIT(0),
  __LISP_SYMBOL_IN = BIT(1),
};

struct __lisp_cps_master {
  enum lisp_pev ev;
  enum lisp_pstat stat;
};

struct lisp_cps {
  struct __lisp_cps_master master;
  int                      slave;
};

struct __hash_internal {
  uint i;
};

struct hash {
  struct __hash_internal internal;
  uint len;
  ulong hash;
};

struct lisp_sym {
  struct hash hash;
  bool        nil;
};

struct root_off_t {
  uint am;
  bool local; /**
                0 -> on another pool;
                   offsets become relative
                   to that pool's root
                1 -> in the same pool
              */
};

struct child_off_t {
  struct root_off_t t;
  struct lisp_sym sym;
  bool sexp;   /**
                 0 -> has sexp
                 1 -> has sym
                 2 -> has neither
              */
};

struct lisp_sexp {
  struct root_off_t  root;
  struct child_off_t left, right;
};

#endif
