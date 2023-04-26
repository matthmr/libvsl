/**
   Generic interface for LISP functions and for evaluation
 */

// TODO: do we even need this? couldn't we just use symtab?

#ifndef LOCK_LISP
#  define LOCK_LISP

#  include "symtab.h" // also includes `mm.h', `utils.h', `err.h'
#  include "sexp.h"

enum lisp_ret_t {
  __LISP_ERR  = -1, /** generic error */
  __LISP_OK   =  0,
};

union lisp_u {
  struct lisp_sym*   sym; /** @sym:  a symbol pointer; for (ref) and alike */
  struct lisp_sym   ssym; /** @ssym: a (static) symbol; for the stack      */
  struct lisp_hash  hash; /** @hash: a hash; for most set/get quotes       */
  struct lisp_sexp* sexp; /** @sexp: a sexp; for most general quotes       */
  void*              gen; /** @gen:  generic memory; casted by the caller  */
};

struct lisp_arg {
  union lisp_u      mem;
  enum lisp_sym_typ typ;
};

// f(x) = y -> master := y, slave := err
struct lisp_ret {
  struct lisp_arg master;
  enum lisp_ret_t  slave;
};

// TODO: like LIBC, take `ENV' as the environment (scope) for the function
typedef struct lisp_ret
(*lisp_fun) (struct lisp_arg* argp, uint argv /*, ??? envp*/);

#endif
