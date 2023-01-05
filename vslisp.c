// vslisp: a very simple lisp implementation;
//         not bootstrapped

#include <unistd.h>

#include "vslisp.h"
#include "stack.h"
#include "debug.h"

static uint paren               = 0;
static char iobuf[IOBLOCK]      = {0};

static struct lisp_symtab chash = {0};
static struct lisp_cps    lcps  = {0};
static struct lisp_sexp*  head  = NULL,
                       *  root  = NULL;

static inline void pool_clean(POOL_T* pp) {
  uint pn = pp->total;

  for (; pp; pp = pp->next) {
    pp->used = 0;
    for (uint i = 0; i < pn; ++i) {
      pp->mem[i].t = 0;
    }
  }
}

static inline struct pos_t
lisp_sexp_node_set_pos(enum sexp_t t,
                       POOL_RET_T pr,
                       struct lisp_sexp* head) {
  struct pos_t ret      = {0};

  uint off              = 0;
  struct lisp_sexp* mem = NULL;

  if (!pr.same && (t == ROOT)) {
    mem = pr.base->mem;
    off = pr.base->idx;
  }
  else {
    mem = pr.mem->mem;
    off = pr.mem->idx;
  }

  ret.am   = (head - mem);
  ret.pidx = off;

  return ret;
}

static inline POOL_RET_T
lisp_sexp_node_get_pos(enum sexp_t t,
                       POOL_RET_T pr,
                       struct lisp_sexp* head) {
  uint pidx = 0;
  uint am   = 0;

  if (t == ROOT) {
    pidx = head->root.pidx;
    am   = head->root.am;
  }
  else if (t & (__SEXP_RIGHT_SEXP | __SEXP_RIGHT_LEXP)) {
    pidx = head->right.pos.pidx;
    am   = head->right.pos.am;
  }
  else if (t & (__SEXP_LEFT_SEXP | __SEXP_LEFT_LEXP)) {
    pidx = head->left.pos.pidx;
    am   = head->left.pos.am;
  }

  pr       = pool_from_idx(pr.mem, pidx);
  pr.entry = (pr.mem->mem + am);

  return pr;
}

static void lisp_sexp_node_add(POOL_T** mpp) {
  DB_MSG("-> lisp_sexp_node_add()");

  if (!head) {
    DB_MSG("  -> EV: attach to root");
    root->t      = (__SEXP_SELF_ROOT | __SEXP_LEFT_EMPTY | __SEXP_RIGHT_EMPTY);
    head         = root;
    (*mpp)->used = 1;
    return;
  }

  POOL_RET_T pr              = pool_add_node(*mpp);
  struct lisp_sexp* new_head = pr.entry;

  if (!pr.same) {
    *mpp = pr.mem;
  }

  if (head->t & __SEXP_LEFT_EMPTY) {
    /**
          ? <- HEAD
         /
        . [new_head]
     */
    new_head->root  = lisp_sexp_node_set_pos(ROOT, pr, head);
    new_head->t     = (__SEXP_SELF_SEXP | __SEXP_LEFT_EMPTY | __SEXP_RIGHT_EMPTY);
    head->left.pos  = lisp_sexp_node_set_pos(CHILD, pr, new_head);
    head->t        &= ~__SEXP_LEFT_EMPTY;
    head->t        |= __SEXP_LEFT_SEXP;
    head            = new_head;
  }
  else if (head->t & __SEXP_RIGHT_EMPTY) {
    /**
          . <- HEAD
         / \
        ?   . [new_head]
     */
    new_head->root   = lisp_sexp_node_set_pos(ROOT, pr, head);
    new_head->t     = (__SEXP_SELF_SEXP | __SEXP_LEFT_EMPTY | __SEXP_RIGHT_EMPTY);
    head->right.pos  = lisp_sexp_node_set_pos(CHILD, pr, new_head);
    head->t         &= ~__SEXP_RIGHT_EMPTY;
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
    DB_MSG("  -> EV: node lexp");

    /** save the old state of `head', swap the memory for `new_head' to `lexp_head'
     */
    struct lisp_sexp  old_head  = *head;
    struct lisp_sexp* lexp_head = new_head;

    /** set type and offset of `lexp_head'
     */
    lexp_head->root = lisp_sexp_node_set_pos(ROOT, pr, head);
    lexp_head->t    = (SWAP_RL(old_head.t) | __SEXP_RIGHT_SEXP | __SEXP_SELF_LEXP);

    /** change the right type of head from whatever it was to `lexp'
     */
    head->t = ((head->t & ~RIGHT) | __SEXP_RIGHT_LEXP);
    head->right.pos  = lisp_sexp_node_set_pos(CHILD, pr, lexp_head);

    /** get new memory for `new_head'
          - possible chance of changing the thread of `mpp'
     */
    POOL_RET_T ppr
              = pr;
    pr        = pool_add_node(*mpp);
    new_head  = pr.entry;

    if (!pr.same) {
      *mpp    = pr.mem;
    }

    /** set type and offset of `new_head'
     */
    new_head->root       = lisp_sexp_node_set_pos(ROOT, pr, lexp_head);
    new_head->t          = (__SEXP_SELF_SEXP | __SEXP_LEFT_EMPTY | __SEXP_RIGHT_EMPTY);
    lexp_head->right.pos = lisp_sexp_node_set_pos(CHILD, pr, new_head);

    if (old_head.t & __SEXP_RIGHT_SYM) {
      /** copy the old right symbol of `head' to `lexp_head'
       */
      lexp_head->left.sym   = old_head.right.sym;
    }
    else if (old_head.t & RIGHT_CHILD) {
      /** fix the root offset of the old right child of `head'
       */
      ppr = lisp_sexp_node_get_pos(RIGHT_CHILD, ppr, &old_head);

      struct lisp_sexp* rch = ppr.entry;
      lexp_head->left.pos   = lisp_sexp_node_set_pos(CHILD, ppr, rch);
      rch->root             = lisp_sexp_node_set_pos(ROOT, ppr, lexp_head);
    }

    /** swap head for `lexp_head'
     */
    head = new_head;
  }
}

static void lisp_sexp_sym(POOL_T** mpp) {
  if (!head) {
    // TODO: implement this
    DB_MSG("-> lisp_do_sym()");
    goto done;
  }

  DB_MSG("-> lisp_sexp_sym()");

  if (head->t & __SEXP_LEFT_EMPTY) {
    /**
          ? <- HEAD
         /
        *
     */
    head->t         &= ~__SEXP_LEFT_EMPTY;
    head->t         |= __SEXP_LEFT_SYM;
    head->left.sym   = chash;
  }
  else if (head->t & __SEXP_RIGHT_EMPTY) {
    /**
          ? <- HEAD
         / \
        ?   *
     */
    head->t         &= ~__SEXP_RIGHT_EMPTY;
    head->t         |= __SEXP_RIGHT_SYM;
    head->right.sym  = chash;
  }
  else {
    /**
       ?  <- HEAD             ?
      / \            ===>    / \
     ?   ?                  ?   , <- HEAD [lexp_head]
                               / \
                              ?   *
     */
    DB_MSG("  -> EV: sym lexp");

    /** save the old state of `head', get new memory for `lexp_head'
          - possible chance of changing the thread of `mpp'
     */
    struct lisp_sexp old_head   = *head;
    POOL_RET_T pr               = pool_add_node(*mpp);
    struct lisp_sexp* lexp_head = pr.entry;

    if (!pr.same) {
      *mpp = pr.mem;
    }

    /** set type and offset of `lexp_head'
     */
    lexp_head->t    = (SWAP_RL(old_head.t) | __SEXP_RIGHT_SYM | __SEXP_SELF_LEXP);
    lexp_head->root = lisp_sexp_node_set_pos(ROOT, pr, head);
    lexp_head->right.sym = chash;

    /** change the right type of head from whatever it was to `lexp'
     */
    head->t = ((head->t & ~RIGHT) | __SEXP_RIGHT_LEXP);
    head->right.pos = lisp_sexp_node_set_pos(CHILD, pr, lexp_head);

    if (old_head.t & __SEXP_RIGHT_SYM) {
      /** copy the old right symbol of `head' to `lexp_head'
       */
      lexp_head->left.sym   = old_head.right.sym;
    }
    else if (old_head.t & (__SEXP_RIGHT_SEXP | __SEXP_RIGHT_LEXP)) {
      /** fix the root offset of the old right child of `head'
       */
      pr = lisp_sexp_node_get_pos(RIGHT_CHILD, pr, &old_head);

      struct lisp_sexp* rch = pr.entry;
      lexp_head->left.pos   = lisp_sexp_node_set_pos(CHILD, pr, rch);
      rch->root             = lisp_sexp_node_set_pos(ROOT, pr, lexp_head);
    }

    /** swap head for `lexp_head'
     */
    head = lexp_head;
  }

done:
  chash.sum     = 0;
  chash.len     = 0;
  chash.com_pos = 0;
}

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
  POOL_RET_T pp;

  mpp      = stack->mpp;

  pp.base  = mpp;
  pp.mem   = mpp;
  pp.entry = mpp->mem;
  head     = stack->head;
  //head     = root;

  enum sexp_t t  = 0;

  if (stack->ev & __STACK_POP) {
    stack->ev   &= ~__STACK_POP;
    goto stage3b_popped;
  }

  // TODO: stage 1 done twice *should* be an error, implement that
stage1:
  t = head->t;
  if (t & LEFT_CHILD) {
    /** left child of common root is EXP:
          stage 1
    */
    DB_MSG("  -> (std) left = exp");
    pp   = lisp_sexp_node_get_pos(LEFT_CHILD, pp, head);
    head = pp.entry;
    goto stage1;
  }
  else {
    /** left child of common root is SYM:
          stage 3a
    */
    if (!STACK_PUSHED(stack->ev)) {
      DB_FMT("  -> (std) left = 0x%x", head->left.sym.sum);

      // symbol under LEXP is just a symbol
      if (t & __SEXP_SELF_LEXP) {
        lisp_stack_push_var(stack, mpp, head, __STACK_PUSH_LEFT);
      }
      else {
        lisp_stack_push(stack, mpp, head);
      }

      return;
    }
    else {
      stack->ev &=
        ~(__STACK_PUSH_LEFT | __STACK_PUSH_RIGHT | __STACK_PUSH_FUNC);
    }

    /** stage3a: */
    if (t & RIGHT_CHILD) {
      DB_MSG("  -> (rebound-left) right = exp");
      pp   = lisp_sexp_node_get_pos(RIGHT_CHILD, pp, head);
      head = pp.entry;
      goto stage1;
    }
    else {
      /** right child of common root is SYM:
            stage3b
      */

      DB_FMT("  -> (rebound-left) right = 0x%x", head->right.sym.sum);
      lisp_stack_push_var(stack, mpp, head, __STACK_PUSH_RIGHT);
      return;
    }
  }

stage3b:
  if (head == root) {
    if (t == (__SEXP_RIGHT_SYM | __SEXP_RIGHT_EMPTY)) {
      DB_FMT("  -> (rebound-right) right = 0x%x", head->right.sym.sum);
    }
    DB_MSG("  -> HIT ROOT");

    // hit the root from the right: there are no more elements,
    // pop for the last time
    lisp_stack_pop(stack, mpp, head);
    return;
  }

  // the current head is an SEXP: pop it, then go to `stage3b_popped'
  if (t & __SEXP_SELF_SEXP) {
    lisp_stack_pop(stack, mpp, head);
    return;
  }

stage3b_popped:
  _head = head;

  // go root
  pp   = lisp_sexp_node_get_pos(ROOT, pp, head);
  head = pp.entry;
  t    = head->t;

  // came from the right: go root again
  if (_head == lisp_sexp_node_get_pos(RIGHT_CHILD, pp, head).entry) {
    goto stage3b;
  }

  // came from the left: try to go right
  //  - can't: go root again
  if (t & RIGHT_CHILD) {
    /** right child of common root is EXP:
          stage2
    */

    DB_MSG("  -> (rebound-right) right = exp");
    /** stage 2: */
    pp   = lisp_sexp_node_get_pos(RIGHT_CHILD, pp, head);
    head = pp.entry;
    goto stage1;
  }
  else {
    DB_FMT("  -> (rebound-left) right = 0x%x", head->right.sym.sum);
    lisp_stack_push_var(stack, mpp, head, __STACK_PUSH_RIGHT);
    return;
    //goto stage3b;
  }
}

static void lisp_sexp_end(POOL_T* mpp) {
  // NOTE: `head' will always be on either an orphan EXP or a paren EXP,
  //       and probably on a different section than `root'

  DB_MSG("<- lisp_sexp_end()");

  POOL_RET_T pp = {0};
  struct lisp_sexp* phead = head;

  pp.mem   =
  pp.base  = mpp;
  pp.entry = phead;
  pp.same  = true;

  bool lexp_head = ((phead->t & __SEXP_SELF_LEXP) && true);

again:
  if (phead == root) {
    return;
  }

  for (;;) {
    pp    = lisp_sexp_node_get_pos(ROOT, pp, phead);
    phead = pp.entry;

    if (phead->t & (__SEXP_SELF_SEXP | __SEXP_SELF_ROOT)) {
      break;
    }
  }

  if (lexp_head) {
    lexp_head = false;
    goto again;
  }

  head = phead;
}

static int lisp_do_sexp(POOL_T* mpp) {
  int ret = 0;

  struct lisp_stack stack = {
    .mpp        = mpp,
    .head       = root,
    .sexp_trans = &lisp_sexp_trans,
  };

  lisp_sexp_trans(&stack);

  if (stack.fun) {
    maybe(lisp_stack_frame(&stack));
  }
  else {
    defer(1);
  }

done:
  pool_clean(mpp);
  head = NULL;

  return ret;
}

static struct lisp_cps lisp_ev(struct lisp_cps pstat, enum lisp_pev sev) {
  if (sev & __LISP_SYMBOL_OUT) {
    if (pstat.master & __LISP_SYMBOL_IN) {
      DB_MSG("<- EV: symbol out");
      pstat.master &= ~__LISP_SYMBOL_IN;
      lisp_sexp_sym(&POOLP);
    }
    else {
      defer_var(pstat.slave, 1);
    }
  }
  if (sev & __LISP_PAREN_OUT) {
    if (pstat.master & __LISP_SYMBOL_IN) {
      pstat = lisp_ev(pstat, __LISP_SYMBOL_OUT);
    }

    DB_MSG("<- EV: paren out");

    if (paren) {
      pstat.master &= ~__LISP_PAREN_IN;
      --paren;
      lisp_sexp_end(POOLP);
    }
    else {
      defer_var(pstat.slave, 1);
    }

    // out of parens: start executing
    if (!paren) {
      pstat.slave = lisp_do_sexp(&POOL);
    }
  }

 done:
  return pstat;
}

static inline struct lisp_cps
lisp_stat(struct lisp_cps pstat, enum lisp_pstat sstat) {
  if (sstat & __LISP_PAREN_IN) {
    if (pstat.master & __LISP_SYMBOL_IN) {
      pstat = lisp_ev(pstat, __LISP_SYMBOL_OUT);
    }
    lisp_sexp_node_add(&POOLP);
    ++paren;
  }

  pstat.master |= sstat;

  return pstat;
}

static inline struct lisp_cps lisp_whitespace(struct lisp_cps pstat) {
  if (pstat.master & __LISP_SYMBOL_IN) {
    done_chash();
    pstat = lisp_ev(pstat, __LISP_SYMBOL_OUT);
  }

  return pstat;
}

static inline struct lisp_cps lisp_csym(struct lisp_cps pstat, char c) {
  pstat.master |= __LISP_SYMBOL_IN;

  if (!__LISP_ALLOWED_IN_NAME(c)) {
    pstat.slave = 1;
  }
  else {
    pstat.slave = do_chash(&chash, c);
  }

  DB_FMT("vslisp: character (%c) (0x%x)", c, chash.sum);

  return pstat;
}

static int parse_ioblock(char* buf, uint size) {
  int ret = 0;

  for (uint i = 0; i < size; i++) {
    char c = buf[i];

    switch (c) {
    case __LISP_PAREN_OPEN:
      DB_MSG("-> EV: paren open");
      lcps = lisp_stat(lcps, __LISP_PAREN_IN);
      break;
    case __LISP_PAREN_CLOSE:
      DB_MSG("<- EV: paren close");
      lcps = lisp_ev(lcps, __LISP_PAREN_OUT);
      break;
    case __LISP_WHITESPACE:
      DB_MSG("-> EV: whitespace");
      lcps = lisp_whitespace(lcps);
      break;
    default:
      lcps = lisp_csym(lcps, c);
      break;
    }

    if (lcps.slave) {
      defer(1);
    }
  }

done:
  DB_FMT("vslisp: [ret = %d, paren = %d]", ret, paren);
  return ret;
}

static int parse_bytstream(int fd) {
  int ret = 0;

  uint r;

  do {
    r = read(fd, iobuf, IOBLOCK);
    maybe(parse_ioblock(iobuf, r));
  } while (r == IOBLOCK);

  if (paren) {
    defer(1);
  }
  else if (lcps.master) {
    lcps = lisp_whitespace(lcps);
  }

done:
  return ret;
}

int main(void) {
  int ret = 0;

  if (frontend) {
    maybe(frontend());
  }

  root    = POOLP->mem;
  root->t = (__SEXP_SELF_ROOT | __SEXP_LEFT_EMPTY | __SEXP_RIGHT_EMPTY);

  ret = parse_bytstream(STDIN_FILENO);

  if (ret) {
    write(STDERR_FILENO,
          MSG("[ !! ] vslisp: error while parsing file\n"));
    defer(1);
  }

done:
  return ret;
}
