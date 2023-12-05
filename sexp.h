/** SEXP functionality */

#ifndef LOCK_SEXP
#  define LOCK_SEXP

#  include "lisp.h"

////////////////////////////////////////////////////////////////////////////////

/** SEXP tree yield state */
enum lisp_yield_t {
  /* generic success */
  __YIELD_OK = 0,

   /* for stack, always SEXP */
  __YIELD_LEFT_EXPR,

  __YIELD_RIGHT_EXPR,
  __YIELD_LEFT_OBJ,
  __YIELD_RIGHT_OBJ,
  __YIELD_END_EXPR,
  __YIELD_DONE,
};

/** SEXP tree yield suppressors */
enum lisp_yield_ignore_t {
  __YIELD_LIT = 0,

  __YIELD_IGNORE_OBJ     = BIT(0),
  __YIELD_IGNORE_LEXP    = BIT(1),
  __YIELD_IGNORE_REBOUND = BIT(2),
};

#  define IGNORE_OBJ(x) ((x) & __YIELD_IGNORE_OBJ)
#  define IGNORE_LEXP(x) ((x) & __YIELD_IGNORE_LEXP)
#  define IGNORE_REBOUND(x) ((x) & __YIELD_IGNORE_REBOUND)

/** Yield status */
struct lisp_yield {
  /* current 'yielded' expression */
  struct lisp_sexp*  exp;

  /* current stat. NOTE: do *not* clear this after handling */
  enum lisp_yield_t stat;
};

////////////////////////////////////////////////////////////////////////////////

/** Yield expressions from the SEXP tree */
struct lisp_yield lisp_sexp_yield_exp(struct lisp_yield trans);

/** Adds @obj to the ends of the expression rooted by @expr_head, returning the
    new sub-expression */
struct lisp_sexp*
lisp_sexp_obj(struct lisp_obj obj, struct lisp_sexp* expr_head);

/** Applies the 'end' algorithm to @expr_head

    This algorithm will try to get the parent of @expr_head once. If the current
    expression head is a LEXP, then it will recurse until the parent is either
    an SEXP, then it will try again to get the parent unless it has hit root

    NOTE: it's the job of the language engine to stop the calls to the SEXP
    module once the last paren has been issued, otherwise this *will* have a bug
    where expressions that are supposed to be separate are counted as child of
    the root @expr_head. It's a design concession: this prevents the caller from
    always checking NULL against a copy of the expression head */
struct lisp_sexp* lisp_sexp_end(struct lisp_sexp* expr_head);

// DEBUG
#if 0
/** Copies the SEXP tree rooted by @expr_head. Returns a pointer to the root of
    the new tree */
struct lisp_sexp* lisp_sexp_copy(struct lisp_sexp* expr_head);
#endif

/** Clears the SEXP tree rooted by @expr_head. @expr_head is not returned back,
    so the caller has to be aware that the memory it had before is probably
    garbage now */
void lisp_sexp_clear(struct lisp_sexp* expr_head);

#endif
