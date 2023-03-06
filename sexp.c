#include "debug.h"
#include "stack.h" // also includes `sexp.h'

#undef LOCK_POOL_DEF
#undef LOCK_POOL_THREAD

#include "pool.h"  // also includes `err.h'

static struct lisp_sexp* head = NULL;
struct lisp_sexp* root        = NULL;

POOL_T** sexp_pp = &POOL_P;

static inline void pool_clean(POOL_T* pp) {
  // TODO: stub
  return;
}

static inline POOL_RET_T
lisp_sexp_node_get_pos(enum sexp_t t, POOL_RET_T pr, struct lisp_sexp* head) {
  // NOTE: this function cannot error

  uint cidx = 0;
  uint pidx = 0;

  if (IS_ROOT(t)) {
    cidx = head->root.cidx;
    pidx = head->root.pidx;
  }
  else if (RIGHT_CHILD_EXPR(t)) {
    cidx = head->right.pos.cidx;
    pidx = head->right.pos.pidx;
  }
  else if (LEFT_CHILD_EXPR(t)) {
    cidx = head->left.pos.cidx;
    pidx = head->left.pos.pidx;
  }

  DB_FMT("[ == ] sexp(pool::get_pos): pidx = %d, cidx = %d", pidx, cidx);

  pr       = pool_from_idx(pr.new, cidx);
  pr.entry = (pr.new->mem + pidx);

  return pr;
}

////////////////////////////////////////////////////////////////////////////////

static void lisp_sexp_trans(struct lisp_stack* stack) {
  /**
     algorithm
     ---------

     this algorithm is broken up into three states:

       1. leftwise
       2. rightwise
       3. rebound

     the default state is state 1. If the left child is a symbol,
     then state 3. is issued.

     state 3. can then either issue state 2. or itself again.

     state 2. will issue back state 1. If the right child is a symbol,
     then state 3 is issued.

     stage 3.
     --------

     stage 3a: if rebounding from stage 1. do stage 2.
     stage 3b: if rebounding from stage 2. do stage 3.
     if hit root from stage 3b stop the algorithm.
   */

  struct lisp_sexp* _head;

  POOL_T*    mpp;
  POOL_RET_T pr;

  mpp      = stack->typ.sexp.mpp;

  pr.new   = pr.base = mpp;
  pr.entry = mpp->mem;
  head     = stack->typ.sexp.head;
  //head     = root;

  enum sexp_t t  = 0;

  if (stack->ev & __STACK_POP) {
    stack->ev   &= ~__STACK_POP;
    goto stage3b_popped;
  }

  // TODO: stage 1 done twice *should* be an error, implement that
stage1:
  t = head->t;
  if (LEFT_CHILD_EXPR(t)) {
    /** left child of common root is EXP:
          stage 1
    */
    DB_MSG("[ == ] sexp(trans): left -> exp");
    pr   = lisp_sexp_node_get_pos(__LEFT_CHILD, pr, head);
    head = pr.entry;
    goto stage1;
  }
  else {
    /** left child of common root is SYM:
          stage 3a
    */
    if (!STACK_PUSHED(stack->ev)) {
      DB_FMT("[ == ] sexp(trans): left(hash.sum) = 0x%x", head->left.sym.sum);

      // symbol under LEXP is just a symbol
      if (IS_LEXP(t)) {
        lisp_stack_sexp_push_var(stack, mpp, head, __STACK_PUSH_LEFT);
      }
      else {
        lisp_stack_sexp_push(stack, mpp, head);
      }

      return;
    }
    else {
      stack->ev &=
        ~(__STACK_PUSH_LEFT | __STACK_PUSH_RIGHT | __STACK_PUSH_FUNC);
    }

    /** stage3a: */
    if (RIGHT_CHILD_EXPR(t)) {
      DB_MSG("[ == ] sexp(trans): rebound-left::right -> exp");
      pr   = lisp_sexp_node_get_pos(__RIGHT_CHILD, pr, head);
      head = pr.entry;
      goto stage1;
    }
    else {
      /** right child of common root is SYM:
            stage3b
      */

      DB_FMT("[ == ] sexp(trans): rebound-left::right(hash.sym) = 0x%x",
             head->right.sym.sum);
      lisp_stack_sexp_push_var(stack, mpp, head, __STACK_PUSH_RIGHT);
      return;
    }
  }

stage3b:
  if (head == root) {
    if (t == __SEXP_RIGHT_SYM) {
      DB_FMT("[ == ] sexp(trans): rebound-right::right(hash.sym) = 0x%x",
             head->right.sym.sum);
    }
    DB_MSG("[ == ] sexp(trans): hit root");

    // hit the root from the right: there are no more elements,
    // pop for the last time
    lisp_stack_sexp_pop(stack, mpp, head);
    return;
  }

  // the current head is an SEXP: pop it, then go to `stage3b_popped'
  if (t & __SEXP_SELF_SEXP) {
    lisp_stack_sexp_pop(stack, mpp, head);
    return;
  }

stage3b_popped:
  _head = head;

  // go root
  pr   = lisp_sexp_node_get_pos(__ROOT, pr, head);
  head = pr.entry;
  t    = head->t;

  // came from the right: go root again
  if (_head == lisp_sexp_node_get_pos(__RIGHT_CHILD, pr, head).entry) {
    goto stage3b;
  }

  // came from the left: try to go right
  //  - can't: go root again
  if (RIGHT_CHILD_EXPR(t)) {
    /** right child of common root is EXP:
          stage2
    */

    DB_MSG("[ == ] sexp(trans): rebound-right::right -> exp");
    /** stage 2: */
    pr   = lisp_sexp_node_get_pos(__RIGHT_CHILD, pr, head);
    head = pr.entry;
    goto stage1;
  }
  else {
    DB_MSG("[ == ] sexp(trans): rebound-left::right -> exp");
    lisp_stack_sexp_push_var(stack, mpp, head, __STACK_PUSH_RIGHT);
    return;
    //goto stage3b;
  }
}

////////////////////////////////////////////////////////////////////////////////

int lisp_sexp_node_add(POOL_T** mpp) {
  register int ret = 0;

  DB_MSG("[ == ] sexp: lisp_sexp_node_add()");

  assert(head, (
           (head = root),
           ((*mpp)->p_idx = 1), 0));

  POOL_RET_T pr              = pool_add_node(*mpp);
  struct lisp_sexp* new_head = pr.entry;

  assert(pr.stat == 0, OR_ERR());

  if (pr.new != pr.base) {
    *mpp = pr.new;
  }

  new_head->self.pidx = IDX_HM(pr.new->p_idx);
  new_head->self.cidx = pr.new->c_idx;

  if (!LEFT_CHILD(head->t)) {
    /**
          ? <- HEAD
         /
        . [new_head]
     */
    new_head->root  = head->self;
    new_head->t     = __SEXP_SELF_SEXP;
    head->left.pos  = new_head->self;
    head->t        |= __SEXP_LEFT_SEXP;
    head            = new_head;
  }
  else if (!RIGHT_CHILD(head->t)) {
    /**
          . <- HEAD
         / \
        ?   . [new_head]
     */
    new_head->root   = head->self;
    new_head->t      = __SEXP_SELF_SEXP;
    head->right.pos  = new_head->self;
    head->t         |= __SEXP_RIGHT_SEXP;
    head             = new_head;
  }
  else {
    /**
       ? <- HEAD [old_head]           ?
      / \                    ===>    / \
     ?   ?                          ?   ,   <- HEAD [lexp_head] `
                                       / \                      `
                        old memory -> ?   .         [new_head]  `- new memory
     */
    DB_MSG("[ == ] sexp(lisp_sexp_node_add): lexp");

    /** save the old state of `head', swap the memory for `new_head' to
        `lexp_head'
     */
    struct lisp_sexp  old_head  = *head;
    struct lisp_sexp* lexp_head = new_head;

    /** set type and offset of `lexp_head'
     */
    lexp_head->root = head->self;
    lexp_head->t    =
      (SWAP_RL(old_head.t) | __SEXP_RIGHT_SEXP | __SEXP_SELF_LEXP);

    /** change the right type of head from whatever it was to `lexp'
     */
    head->t = ((head->t & ~RIGHT) | __SEXP_RIGHT_LEXP);
    head->right.pos = lexp_head->self;

    /** get new memory for `new_head'
          - possible chance of changing the thread of `mpp'
     */
    pr       = pool_add_node(*mpp);
    new_head = pr.entry;

    assert(pr.stat == 0, OR_ERR());

    if (pr.new != pr.base) {
      *mpp = pr.new;
    }

    new_head->self.pidx = IDX_HM(pr.new->p_idx);
    new_head->self.cidx = pr.new->c_idx;

    /** set type and offset of `new_head'
     */
    new_head->root       = lexp_head->self;
    new_head->t          = __SEXP_SELF_SEXP;
    lexp_head->right.pos = new_head->self;

    if (old_head.t & __SEXP_RIGHT_SYM) {
      /** copy the old right symbol of `head' to `lexp_head'
       */
      lexp_head->left.sym   = old_head.right.sym;
    }
    else if (old_head.t & (__SEXP_RIGHT_LEXP | __SEXP_RIGHT_SEXP)) {
      /** fix the root offset of the old right child of `head'
       */
      pr = lisp_sexp_node_get_pos(__RIGHT_CHILD, pr, &old_head);

      struct lisp_sexp* rch_pr = pr.entry;

      lexp_head->left.pos = rch_pr->self;
      rch_pr->root        = lexp_head->self;
    }

    head = new_head;
  }

  done_for(ret);
}

int lisp_sexp_sym(POOL_T** mpp, struct lisp_hash hash) {
  register int ret = 0;

  /** for now, top-level symbols are silently ignored
   */
  assert(head, 0);

  DB_MSG("[ == ] sexp: lisp_sexp_sym()");

  if (!LEFT_CHILD(head->t)) {
    /**
          ? <- HEAD
         /
        *
     */
    head->t         |= __SEXP_LEFT_SYM;
    head->left.sym   = hash;
  }
  else if (!RIGHT_CHILD(head->t)) {
    /**
          ? <- HEAD
         / \
        ?   *
     */
    head->t         |= __SEXP_RIGHT_SYM;
    head->right.sym  = hash;
  }
  else {
    /**
       ?  <- HEAD             ?
      / \            ===>    / \
     ?   ?                  ?   , <- HEAD [lexp_head]
                               / \
                              ?   *
     */
    DB_MSG("[ == ] sexp(lisp_sexp_sym): lexp");

    /** save the old state of `head', get new memory for `lexp_head'
          - possible chance of changing the thread of `mpp'
     */
    struct lisp_sexp old_head   = *head;
    POOL_RET_T pr               = pool_add_node(*mpp);
    struct lisp_sexp* lexp_head = pr.entry;

    assert(pr.stat == 0, OR_ERR());

    if (pr.new != pr.base) {
      *mpp = pr.new;
    }

    lexp_head->self.pidx = IDX_HM(pr.new->p_idx);
    lexp_head->self.cidx = pr.new->c_idx;

    /** set type and offset of `lexp_head'
     */
    lexp_head->t         =
      (SWAP_RL(old_head.t) | __SEXP_RIGHT_SYM | __SEXP_SELF_LEXP);
    lexp_head->root      = head->self;
    lexp_head->right.sym = hash;

    /** change the right type of head from whatever it was to `lexp'
     */
    head->t = ((head->t & ~RIGHT) | __SEXP_RIGHT_LEXP);
    head->right.pos = lexp_head->self;

    if (old_head.t & __SEXP_RIGHT_SYM) {
      /** copy the old right symbol of `head' to `lexp_head'
       */
      lexp_head->left.sym = old_head.right.sym;
    }
    else if (old_head.t & (__SEXP_RIGHT_SEXP | __SEXP_RIGHT_LEXP)) {
      /** fix the root offset of the old right child of `head'
       */
      pr = lisp_sexp_node_get_pos(__RIGHT_CHILD, pr, &old_head);

      struct lisp_sexp* rch_pr = pr.entry;

      lexp_head->left.pos = rch_pr->self;
      rch_pr->root        = lexp_head->self;
    }

    head = lexp_head;
  }

  done_for(ret);
}

void lisp_sexp_end(POOL_T** mpp) {
  // NOTE: this function cannot error

  DB_MSG("[ == ] sexp: lisp_sexp_end()");

  POOL_RET_T pr = {0};
  struct lisp_sexp* phead = head;

  pr.new   = pr.base = *mpp;
  pr.entry = phead;

  bool lexp_head = IS_LEXP(phead->t) && true;

again:
  if (!phead || phead == root) {
    return;
  }

  for (;;) {
    pr    = lisp_sexp_node_get_pos(__ROOT, pr, phead);
    phead = pr.entry;

    if (IS_ROOT(phead->t)) {
      break;
    }
  }

  if (lexp_head) {
    lexp_head = false;
    goto again;
  }

  head = phead;
}

// TODO: this is wrong with the new lexer; please refactor
int lisp_sexp_eval(POOL_T** mpp) {
  register int ret = 0;

  struct lisp_stack stack;

  stack.typ.sexp = (struct lisp_sexp_stack) {
    .mpp  = *mpp,
    .head = root,
  };

  lisp_sexp_trans(&stack);

  done_for_with(ret, pool_clean(*mpp));
}

// used on the stack to get the root of a temporary SEXP tree
struct lisp_sexp* lisp_sexp_get_head(void) {
  return head;
}

void sexp_init(void) {
  root    = POOL.mem;
  root->t = __ROOT;
};
