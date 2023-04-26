#ifndef LOCK_SEXP
#  define LOCK_SEXP

#  include "symtab.h"
#  include "lisp.h"

/**
   For SEXPs and LEXPs, right shift pushes from left to right, left shift pushes
   from from right to left

   e.g. __SEXP_LEFT_SEXP >> 1 = __SEXP_RIGHT_SEXP
 */
enum lisp_sexp_t {
  __SEXP_SELF_ROOT  = BIT(0),

  __SEXP_SELF_SEXP  = BIT(1),
  __SEXP_LEFT_SEXP  = BIT(2),
  __SEXP_RIGHT_SEXP = BIT(3),

  __SEXP_SELF_LEXP  = BIT(4),
  __SEXP_LEFT_LEXP  = BIT(5),
  __SEXP_RIGHT_LEXP = BIT(6),

  __SEXP_LEFT_SYM   = BIT(7),
  __SEXP_RIGHT_SYM  = BIT(8),
};

#  define __RIGHT_EXPR (__SEXP_RIGHT_SEXP | __SEXP_RIGHT_LEXP)
#  define __LEFT_EXPR  (__SEXP_LEFT_SEXP  | __SEXP_LEFT_LEXP)
#  define __RIGHT      (__SEXP_RIGHT_SYM  | __RIGHT_EXPR)
#  define __LEFT       (__SEXP_LEFT_SYM   | __LEFT_EXPR)
#  define __ROOT       (__SEXP_SELF_ROOT)

#  define RIGHT_SYM(x)  ((x) & __SEXP_RIGHT_SYM)
#  define LEFT_SYM(x)   ((x) & __SEXP_LEFT_SYM)
#  define RIGHT_EXPR(x) ((x) & __RIGHT_EXPR)
#  define LEFT_EXPR(x)  ((x) & __LEFT_EXPR)
#  define IS_ROOT(x)    ((x) & __ROOT)
#  define IS_SEXP(x)    ((x) & __SEXP_SELF_SEXP)
#  define IS_LEXP(x)    ((x) & __SEXP_SELF_LEXP)
#  define IS_EXPR(x)    (IS_SEXP(x) || IS_LEXP(x))

#  define RIGHT(x) ((x) & __RIGHT) // symbol or expr
#  define LEFT(x)  ((x) & __LEFT)  // symbol or expr

#  define SWAP_RL(x) (((x) & __RIGHT_EXPR) >> 1)

struct lisp_sexp;

union lisp_node_t {
  struct lisp_hash  sym;
  struct lisp_sexp* exp;
};

struct lisp_sexp {
  struct lisp_sexp* root;
  union lisp_node_t left, right;
  enum lisp_sexp_t  t;
};

// NOTE: this assumes a stack exists
enum lisp_trans_t {
  __TRANS_ERR           = -1,     /** generic error   */
  __TRANS_OK            = 0,      /** generic success */

  __TRANS_LEFT_EXPR     = BIT(0), /** always SEXP */
  __TRANS_RIGHT_EXPR    = BIT(1),
  __TRANS_LEXP          = BIT(2), /** always on the right */
  __TRANS_LEFT_SYM      = BIT(3),
  __TRANS_RIGHT_SYM     = BIT(4),
  __TRANS_REBOUND_LEFT  = BIT(5),
  __TRANS_REBOUND_RIGHT = BIT(6),
  __TRANS_DONE          = BIT(7),
};

struct lisp_trans {
  struct lisp_sexp*  exp;
  enum lisp_trans_t stat;
};

////////////////////////////////////////////////////////////////////////////////

struct lisp_trans lisp_sexp_yield(struct lisp_trans trans);
struct lisp_sexp* lisp_sexp_node(struct lisp_sexp*  expr_head);
struct lisp_sexp* lisp_sexp_sym(struct lisp_sexp*   expr_head,
                                struct lisp_hash    sym_hash);
struct lisp_sexp* lisp_sexp_end(struct lisp_sexp*   expr_head);
void              lisp_sexp_clear(struct lisp_sexp* expr_head);

#endif
