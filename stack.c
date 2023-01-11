// TODO: if any bug reports appear regarding stack overflow,
//       create a pool for the stack. as far as I've tested,
//       as long as you don't try to compute something
//       ridiculously recursive you should be fine

#include "stack.h"

void
lisp_stack_push(struct lisp_stack* stack,
                POOL_T* mpp, struct lisp_sexp* head) {
  stack->ev   |= __STACK_PUSH_FUNC;
  stack->head  = head;
  stack->mpp   = mpp;
  stack->fun   = lisp_symtab_get(head->left.sym);
}

void
lisp_stack_push_var(struct lisp_stack* stack, POOL_T* mpp,
                    struct lisp_sexp* head, enum lisp_stack_ev ev) {
  stack->ev   |= ev;
  stack->head  = head;
  stack->mpp   = mpp;
  stack->fun   = lisp_symtab_get(
    (ev & __STACK_PUSH_LEFT)? head->left.sym: head->right.sym);
}

// TODO: this function
void
lisp_stack_pop(struct lisp_stack* stack, POOL_T* mpp, struct lisp_sexp* head) {
  stack->ev   |= __STACK_POP;
  stack->head  = head;
  stack->mpp   = mpp;
}

int
lisp_stack_push_to_frame(struct lisp_stack* stack,
                         struct lisp_frame* frame) {
  int ret = 0;

  if (frame->tab.i >= frame->tab.size) {
    defer(1);
  }

  frame->tab.reg[frame->tab.i] = (frame->stack.ev & __STACK_PUSH_LEFT)?
    frame->stack.head->left.sym: frame->stack.head->right.sym;

  ++frame->tab.i;

done:
  return ret;
}

// TODO: these

int lisp_stack_frame(struct lisp_stack* stack) {
  int ret = 0;

  struct lisp_frame frame = {
    .stack    = *stack,
    .tab.size = lisp_symtab_getsize(stack->head->left.sym),
  };

  struct lisp_symtab reg[frame.tab.size];

yield:
  stack->sexp_trans(stack);
  enum lisp_stack_ev ev = stack->ev;

  if (STACK_PUSHED_VAR(ev)) {
push_var:
    assert(lisp_stack_sexp_frame_var(stack, &frame), 1);
    goto yield;
  }

  else if (STACK_PUSHED_FUNC(ev)) {
    /** in the case of a `quot'-like function, any
        GEXP ::= {SEXP | LEXP | SYM | <-} may return
        as literal, which means that something like:

                    (quot (not-a-function))

        shouldn't push `not-a-function' to the stack,
        just return its GEXP
     */
    if (STACK_QUOT(stack->ev)) {
      goto push_var;
    }

    if (stack->fun) {
      assert(lisp_stack_frame(stack), 1);
      assert(lisp_stack_frame_var(stack, &frame), 1);
      goto yield;
    }
    else {
      defer_as(1);
    }
  }

  else if (STACK_POPPED(ev)) {
    // NOTE: it's the job of `::fun' to set up the
    // `stack' variable to correctly give in the
    // `symtab' values for `lisp_stack_frame_var'
    assert(stack->fun(stack), 1);
  }

  done_for(ret);
}
  }

  assert(varp > 0, 1);
  done_for(ret);
}
// LEX stack: END
