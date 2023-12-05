#ifndef LOCK_UTILS
#  define LOCK_UTILS

//// GENERIC

#  ifndef false
#    define false 0x0
#  endif

#  ifndef true
#    define true  0x1
#  endif

#  ifndef NULL
#    define NULL ((void*)0)
#  endif

#  define BIT(x) (1 << (x))

#  define TOP_BIT(x,y) ((x) >> (sizeof(x)*8 - ((y)-1)))

#  define MSG(x) \
  x, ((sizeof(x)/sizeof(char)) - 1)

#  define SIZEOF(x) \
  (sizeof((x))/sizeof(*(x)))

#  define LINE(x) \
  x "\n"

#  define STRING(x) \
  {MSG(LINE(x))}

// machine-human
#  define IDX_MH(x) \
  ((x) + 1)

// human-machine
#  define IDX_HM(x) \
  ((x) - 1)

#  define MAYBE_INIT(x) \
  assert((x) == 0, OR_ERR());

typedef unsigned char  bool, byte, uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;
typedef unsigned long  ulong;

//// STRING UTILS

/** Sized string */
struct string_s {
  const char* _;
  const uint  size;
};

typedef struct string_s string_s;

#  define INC_RESET(x) ((x).idx = 0)

/** Incremental string */
struct string_ip {
  char* str;
  uint idx;
};

typedef struct string_ip string_ip;

string_ip to_string_ip(char* str);
char* from_string_ip(string_ip strip);

#  define ITOA(x) ((x) + 0x30)

//// ERRCNTL

/** ERROR CONTROL MACROS

    These macros are used for error control. There are three main categories:

    - done macros
    - defer macros
    - assert macros

    You have to define a done macro in the end of your function. Unconditionally
    jump to them using defer macros, or conditionally with assert macros. If using
    the default defer or assert family of functions, make sure to define a stack
    variable named `ret'

    Every macro has modifiers which are separated by underscores. The positional
    argument is the modifier + 1. E.g:

    defer_as_with(x,y)
      - defer
        - as   y
        - with y */

///

// generic done guard, returning for `x'
#  define done_for(x) \
  done:              \
    return (x)

// done guard, returning for `x', executing `y'
#  define done_for_with(x, y) \
  done:                       \
    y;                        \
    return (x)

///

// generic defer statement
#  define defer(x) \
  goto done

// defer statement, with the value of `x'
#  define defer_as(x) \
  ret = (x);          \
  defer()

// defer statement, with the value of `y' for `x'
#  define defer_for_as(x,y) \
  (x) = (y);                \
  defer()

// defer statement, with the value of `x', executing `y'
#  define defer_as_exec(x,y) \
  ret = (x);                 \
  (y);                       \
  defer()

// defer statement, with the value of `y', for `x', executing `z'
#  define defer_for_as_exec(x,y,z) \
  (x) = (y);                       \
  (z);                             \
  defer()

///

// conditional defer statement
#  define defer_if(x) \
  if (x) {            \
    defer();          \
  }

// conditional defer statement, with return set to `y'
#  define defer_if_as(x,y) \
  if (x) {                 \
    defer_as(y);           \
  }

// conditional defer statement, executing `y'
#  define defer_if_exec(x,y) \
  if (x) {                   \
    y;                       \
    defer();                 \
  }

// conditional defer statement, with return set to `y', executing `z'
#  define defer_if_as_exec(x,y,z) \
  if (x) {                        \
    defer_as_exec((y), (z));      \
  }

// conditional defer statement, with the value of `z' set to `y'
#  define defer_if_for_as(x,y,z)  \
  if (x) {                        \
    defer_for_as((z), (y));       \
  }

// conditional defer statement, with the value of `z' set to `y', executing `w'
#  define defer_if_for_as_exec(x,y,z,w) \
  if (x) {                              \
    defer_for_as_exec((z), (y), (w));   \
  }

///

// generic assert statement
#  define assert(x,y)      \
  defer_if_as(!(x), (y))

// assert statement with the value of `z'
#  define assert_for(x,y,z) \
  defer_if_for_as(!(x), (y), (z))

// assert statement executing `z'
#  define assert_exec(x,y,z) \
  defer_if_as_exec(!(x), (y), (z))

// assert statement with the value of `z', executing `w'
#  define assert_for_exec(x,y,z,w) \
  defer_if_for_as_exec(!(x), (y), (z), (w))

///

#  define ERR_STRING(mod, msg) \
  STRING("[ !! ] " mod ": " msg)

#  define ERR_MSG(mod, msg) \
  MSG("[ !! ] " mod ": " msg "\n")

#  define OR_ERR(x) \
  eerr(NULL, 0)

#  define ECODE(...) enum ecode {EOK = 0, __VA_ARGS__}
#  define EMSG static const string_s emsg[] =
#  define err(ecode) eerr(emsg, ecode)

/** Top-level error function */
int eerr(const string_s* emsg, int code);

#endif
