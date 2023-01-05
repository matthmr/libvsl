#ifndef LOCK_UTILS
#  define LOCK_UTILS

#  ifndef false
#    define false 0x0
#  endif

#  ifndef true
#    define true  0x1
#  endif

#  define defer_func(x) \
  goto done

#  define defer(x) \
  ret = (x);       \
  defer_func()

#  define defer_var(x, y) \
  (x) = (y);              \
  defer_func()

#  define maybe(x) \
  ret = (x);       \
  if (ret) {       \
    defer_func();  \
  }

#  define maybe_var(x) \
  (x) = (y);           \
  if (x) {             \
    defer_func();      \
  }

#  define BIT(x) (1 << (x))
#  define MSG(x) \
  x, ((sizeof(x)/sizeof(char)) - 1)

typedef unsigned char  bool, uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;
typedef unsigned long  ulong;

struct __string {
  const char* _;
  const uint  size;
};

typedef struct __string string;

#endif
