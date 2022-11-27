#ifndef LOCK_VSLISP
#  define LOCK_VSLISP

#  ifndef true
#    define true  0x1
#  endif

#  ifndef false
#    define false 0x0
#  endif

#  define IOBLOCK (4096)

#  define __LISP_WHITESPACE \
         0x00: \
    case 0x20: \
    case 0x09: \
    case 0x0a: \
    case 0x0b: \
    case 0x0c: \
    case 0x0d

#  define BIT(x) (1 << (x))

typedef unsigned int uint;
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
  long hash;
};

#endif
