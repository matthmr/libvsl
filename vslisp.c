// vslisp: a very simple lisp implementation;
//         not bootstrapped

#include <unistd.h>

#include "vslisp.h"
#include "debug.h"

static ulong chash            = 0;
static uint paren             = 0;
static char iobuf[IOBLOCK]    = {0};

static struct lisp_cps   lcps = {0};
static struct lisp_sexp* head = NULL,
                       * root = NULL;

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
  chash = 0L;
}
static void lisp_do_sexp(POOL_T* mpp) {
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

  POOL_RET_T pp;
  struct lisp_sexp* _head = head;

  pp.base  = mpp;
  pp.mem   = mpp;
  pp.entry = mpp->mem;
  head     = root;

  enum sexp_t t  = 0;

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
    DB_FMT("  -> (std) left = %lu", head->left.sym);

    /** stage3a: */
    if (t & RIGHT_CHILD) {
      DB_MSG("  -> (rebound-left) right = exp");
      pp   = lisp_sexp_node_get_pos(RIGHT_CHILD, pp, head);
      head = pp.entry;
      goto stage1;
    }

    /** right child of common root is SYM:
          stage3b
    */
    DB_FMT("  -> (rebound-left) right = %lu", head->right.sym);
  }

stage3b:
  if (head == root) {
    if (t == (__SEXP_RIGHT_SYM | __SEXP_RIGHT_EMPTY)) {
      DB_FMT("  -> (rebound-right) right = %lu", head->right.sym);
    }
    DB_MSG("  -> HIT ROOT");
    goto done;
  }

  _head = head;
  pp    = lisp_sexp_node_get_pos(ROOT, pp, head);
  head  = pp.entry;
  t     = head->t;

  if (_head == lisp_sexp_node_get_pos(RIGHT_CHILD, pp, head).entry) {
    goto stage3b;
  }

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
    DB_FMT("  -> (rebound-left) right = %lu", head->right.sym);
    goto stage3b;
  }

done:
  pool_clean(mpp);
  head = NULL;
}
static inline void lisp_sexp_end(POOL_T* mpp) {
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
static inline struct lisp_cps lisp_ev(struct lisp_cps pstat,
                                      enum lisp_pev sev) {
  if (sev & __LISP_SYMBOL_OUT) {
    if (pstat.master & __LISP_SYMBOL_IN) {
      DB_MSG("<- EV: symbol out");
      pstat.master &= ~__LISP_SYMBOL_IN;
      lisp_sexp_sym(&POOLP);
    }
    else {
      pstat.slave = 1;
      goto done;
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
      if (!paren) {
        lisp_do_sexp(POOLP);
      }
    }
    else {
      pstat.slave = 1;
      goto done;
    }
  }

 done:
  return pstat;
}
static inline struct lisp_cps lisp_stat(struct lisp_cps pstat,
                                        enum lisp_pstat sstat) {
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
    chash = done_chash(chash);
    pstat = lisp_ev(pstat, __LISP_SYMBOL_OUT);
  }

  return pstat;
}
static inline struct lisp_cps lisp_csym(struct lisp_cps pstat, char c) {
  pstat.master |= __LISP_SYMBOL_IN;

  if (!__LISP_ALLOWED_IN_NAME(c)) {
    pstat.slave = 1;
    goto done;
  }

  chash = do_chash(chash, c);

  DB_FMT("vslisp: character (%c) (0x%lu)", c, chash);

 done:
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
      ret = 1;
      goto done;
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
    int s = parse_ioblock(iobuf, r);
    if (s) {
      ret = s;
      goto done;
    }
  } while (r == IOBLOCK);

 done:
  if (paren) {
    ret = 1;
  }
  else if (lcps.master) {
    lcps = lisp_whitespace(lcps);
  }

  return ret;
}

int main(void) {
  if (frontend) {
    frontend();
  }

  root    = POOLP->mem;
  root->t = (__SEXP_SELF_ROOT | __SEXP_LEFT_EMPTY | __SEXP_RIGHT_EMPTY);

  int ret = parse_bytstream(STDIN_FILENO);

  if (ret) {
    write(STDERR_FILENO,
          MSG("[ !! ] vslisp: error while parsing file\n"));
  }

  return ret;
}
