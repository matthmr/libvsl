/**
   Generic interface for LISP functions and for evaluation
 */

#ifndef LOCK_LISP
#  define LOCK_LISP

#  include "sexp.h"

enum lisp_ret_t {
  __LISP_ERR  = -1, /** generic error */
  __LISP_OK   =  0,
};

enum lisp_arg_t {
  __LISP_ARG_OK          = 0,
  __LISP_ARG_KEEP_LOCAL  = BIT(0),
  __LISP_ARG_KEEP_GLOBAL = BIT(1),
};

union lisp_u {
  struct lisp_sym*   sym; /** @sym:  a symbol pointer; for (ref) and alike */
  struct lisp_sym   ssym; /** @ssym: a (static) symbol; for the stack      */
  struct lisp_hash  hash; /** @hash: a hash; for most set/get quotes       */
  struct lisp_sexp* sexp; /** @sexp: a sexp; for most general quotes       */
  void*              gen; /** @gen:  generic memory; casted by the caller  */
};

struct lisp_arg {
  union lisp_u      mem; /** @mem:  the proper memory of the argument */
  enum lisp_sym_typ typ; /** @typ:  the argument type                 */
  struct lisp_arg* next; /** @next: the optional `next' pointer, used for
                             non-right-bounding functions             */
  enum lisp_arg_t m_typ; /** @m_typ: the argument type, as seen by the stack;
                             (memory-type)                            */
};

// f(x) = y -> master := y, slave := err
struct lisp_ret {
  struct lisp_arg master;
  enum lisp_ret_t  slave;
};

// TODO: like LIBC, take `ENV' as the environment (scope) for the function.
// Maybe the environment is the local lexical scope?
typedef struct lisp_ret
(*lisp_fun) (struct lisp_arg* argp, uint argv /*, ??? envp*/);

#endif
