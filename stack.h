#ifndef LOCK_STACK
#  define LOCK_STACK

// we only want the *definition* of POOL_T from `sexp.h', not the implementation
// symtab.h <- sexp.h
#  define LOCK_POOL_DEF

#  include "sexp.h"

struct lisp_stack;

typedef void (*sexp_trans) (struct lisp_stack* stack);
typedef int (*lisp_fun)    (struct lisp_stack* stack);

enum lisp_stack_ev {
  __STACK_POP        = BIT(0),

  __STACK_PUSH_LEFT  = BIT(1),
  __STACK_PUSH_RIGHT = BIT(2),
  __STACK_PUSH_FUNC  = BIT(3),

  __STACK_PUSH_LIT   = BIT(4),
};

#  define STACK_QUOT(x) \
  ((x) & (__STACK_PUSH_LIT))

#  define STACK_POPPED(x) \
  ((x) & (__STACK_POP))

#  define STACK_PUSHED(x) \
  ((x) & (__STACK_PUSH_LEFT | __STACK_PUSH_RIGHT | __STACK_PUSH_FUNC))

#  define STACK_PUSHED_FUNC(x) \
  ((x) & (__STACK_PUSH_FUNC))

#  define STACK_PUSHED_VAR(x) \
  ((x) & (__STACK_PUSH_LEFT | __STACK_PUSH_RIGHT))

struct lisp_stack {
  struct lisp_sexp*  head;           /** @head: the current sexp head   */
  POOL_T*            mpp;            /** @mpp:  the current pool thread */
  lisp_fun           fun;            /** @fun:  the stack function      */
  sexp_trans         sexp_trans;     /** @sexp_trans:
                                         the sexp transverse algorithm  */
  enum lisp_stack_ev ev;             /** @ev:   the stack event         */
};

struct lisp_stack_symtab {
  struct lisp_symtab* reg;
  uint size;
  uint i;
};

struct lisp_frame {
  struct lisp_stack        stack;
  struct lisp_stack_symtab tab;
};

int lisp_stack_frame(struct lisp_stack* stack);

void lisp_stack_push(struct lisp_stack* stack,
                     POOL_T* mpp, struct lisp_sexp* head);
void lisp_stack_push_var(struct lisp_stack* stack, POOL_T* mpp,
                         struct lisp_sexp* head, enum lisp_stack_ev ev);

void lisp_stack_pop(struct lisp_stack* stack,
                    POOL_T* mpp, struct lisp_sexp* head);

/**
   there's no `lisp_stack_pop_var` because popping them doesn't
   directly affect their (N>1)-parents, only their (N=1)-parent.

   for instance:

   -- TOP    --
   [PARENT1] [p1VAR1] [p1VAR2]
   [PARENT2] [p2VAR1] [p2VAR2]
   -- BOTTOM --

   so internally we mask `push_var' for `alloc_var' or `set_var'
 */

#endif
