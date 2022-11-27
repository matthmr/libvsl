#ifndef LOCK_VSLISP
#  define LOCK_VSLISP

#  ifndef true
#    define true  0x1
#  endif

#  ifndef false
#    define false 0x0
#  endif

#  define IOBLOCK (4096)
#  define SEXPPOOL (1024)

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
    uint free;                     \
    uint total;                    \
  }

typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned char bool;

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
};

struct lisp_sexp {
  struct lisp_sym sym;
  struct sexp* left,
             * right;
  bool   is_sym; /**
                    0 -> entry is a symbol, use the hash table
                    1 -> entry is a node, set append state
                 */
};

#endif
