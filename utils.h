#ifndef LOCK_UTILS
#  define LOCK_UTILS

#  ifndef false
#    define false 0x0
#  endif

#  ifndef true
#    define true  0x1
#  endif

#  define defer(x) \
  ret = x;         \
  goto done

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
