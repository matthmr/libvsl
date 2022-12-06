// vslisp: a very simple lisp implementation;
//         not bootstrapped

// EDIT: ok... maybe not THAT simple, but still
//       simple enough as to not be bootstrappABLE

#include <unistd.h>
#include <stdlib.h>

#ifdef DEBUG
#  include <stdio.h>
#endif

#include "vslisp.h"
#include "pool.h"
//#include "prim.h"

static uint paren             = 0;
//static uint deep_paren        = 0;
static uint hash_i            = 0;
static char iobuf[IOBLOCK]    = {0};

static struct lisp_hash chash = {0};
static struct lisp_cps   lcps = {0};
static struct lisp_sexp* head = NULL,
                       * root = NULL;

struct MEMPOOL(lisp_sexp, SEXPPOOL);
//struct MEMPOOL(lisp_sym_hash, HASHPOOL);

struct pool_ret_t {
  void* mem;
  void* base;
  uint  idx;
  bool  new; /**
                0 -> in the same pool
                1 -> on a different pool
              */
};

struct sexp_pool_t {
  struct lisp_sexp*               master;
  struct MEMPOOL_TMPL(lisp_sexp)* slave;
};

static struct MEMPOOL_TMPL(lisp_sexp)  sexpmp = {0};
static struct MEMPOOL_TMPL(lisp_sexp)* sexpmpp = NULL;
//static struct MEMPOOL_TMPL(lisp_sym_hash) hashmp = {0};

static inline struct pool_ret_t pool_add_node(struct MEMPOOL_TMPL(lisp_sexp)* mpp) {
  // TODO: make this an `int' function because of
  //       ENOMEM in malloc
  struct pool_ret_t ret = {
    .mem  = NULL,
    .base = mpp,
    .new  = false,
  };

  if (mpp->used == mpp->total) {
    if (!mpp->next) {
      mpp->next = malloc(sizeof(struct MEMPOOL_TMPL(lisp_sexp)));

      mpp->next->idx   = (mpp->idx + 1);
      mpp->next->prev  = mpp;
      mpp->next->next  = NULL;

      struct MEMPOOL_TMPL(lisp_sexp)* pp = NULL;

      for (struct MEMPOOL_TMPL(lisp_sexp)* _ = mpp; _; _->prev) {
        pp = _;
      }

      mpp->next->base = pp->base;
    }

    mpp->next->total  = mpp->total;
    mpp->next->used   = 0;
    mpp               = mpp->next;
    ret.new           = true;
  }

  ret.mem = (mpp->mem + mpp->used);
  ++mpp->used;

  return ret;
}
static inline struct MEMPOOL_TMPL(lisp_sexp)* pool_get_idx(struct MEMPOOL_TMPL(lisp_sexp)* mpp) { }
static inline struct sexp_pool_t lisp_sexp_node_root(struct MEMPOOL_TMPL(lisp_sexp)* mpp,
                                                     struct lisp_sexp* head) {
  struct sexp_pool_t ret;
  int pidx = ret->root.pidx;

#ifdef DEBUG
  fprintf(stderr, ":: lisp_sexp_node_root: ret->root.am = %d\n", ret->root.am);
#endif

  if (!ret.master->root.pidx) {
    ret = ((struct lisp_sexp*)mpp->mem + ret.master->root.am);
  }
  else {
    pool_get_idx(pp);
    mpp = pp->base;

    for (uint i = 0; i < pi; i++) {
      mpp = mpp->next;
    }

    ret.master = ((struct lisp_sexp*)mpp->mem + ret->root.am);
  }

  ret.slave  = mpp;

  return ret;
}
#define TO_ROOT true
#define TO_CHILD true
static inline struct pos_t lisp_sexp_node_set_pos(bool to, struct pool_ret_t pr, struct lisp_sexp* head) {
  struct pos_t ret;

  ret.pidx = pr.new? (pr.idx + 1): 0;
  ret.am   = (head - (struct lisp_sexp*)pr.base);

  return ret;
}
static inline struct sexp_pool_t lisp_sexp_node_get_pos(struct MEMPOOL_TMPL(lisp_sexp)* mpp,
                                                        struct lisp_sexp* head, enum sexp_t t) {
  struct sexp_pool_t ret;
  struct MEMPOOL_TMPL(lisp_sexp)* pp = mpp;
  int pidx = 0;

  if (t & (__SEXP_RIGHT_SEXP | __SEXP_RIGHT_LEXP)) {
    pidx = ret.master->right.pos.pidx;
    if (!pidx) {
      ret.master = ((struct lisp_sexp*)pp->mem + ret->right.pos.am);
    }
    else {
      ret.master = ((struct lisp_sexp*)pp->mem + ret->right.pos.am);
    }
  }
  else if (t & (__SEXP_LEFT_SEXP | __SEXP_LEFT_LEXP)) {
    pidx = ret.master->left.pos.pidx;
    if (!pidx) {
      ret.master = ((struct lisp_sexp*)pp->mem + ret->left.pos.am);
    }
    else {
      ret.master = ((struct lisp_sexp*)pp->mem + ret->left.pos.am);
    }
  }

  ret.slave      = pp;

  return ret;
}
static void lisp_sexp_node(struct MEMPOOL_TMPL(lisp_sexp)** mpp) {
#ifdef DEBUG
    fputs("-> lisp_sexp_node()\n", stderr);
#endif

  if (!head) {
#ifdef DEBUG
    fputs("  -> EV: attach to root\n", stderr);
#endif
    head = root;
    return;
  }

  struct MEMPOOL_TMPL(lisp_sexp)* mppv = *mpp;

  struct pool_ret_t pr       = pool_add_node(mppv);
  struct lisp_sexp* new_head = pr.mem;

  if (pr.new) {
    mppv = *mpp = pr.mem;
  }

  new_head->t    = (__SEXP_SELF_SEXP | __SEXP_LEFT_EMPTY | __SEXP_RIGHT_EMPTY);

  if (head->t & __SEXP_LEFT_EMPTY) {
    /**
          ? <- HEAD
         /
        . [new_head]
     */
    new_head->root  = lisp_sexp_node_set_pos(TO_ROOT, pr, head);
    head->left.pos  = lisp_sexp_node_set_pos(TO_CHILD, pr, new_head);
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
    new_head->root   = lisp_sexp_node_set_pos(TO_ROOT, pr, head);
    head->right.pos  = lisp_sexp_node_set_pos(TO_CHILD, pr, new_head);
    head->t         &= ~__SEXP_RIGHT_EMPTY;
    head->t         |= __SEXP_RIGHT_SEXP;
    head             = new_head;
  }
  else {
    /**
       ? <- HEAD [old_head]           ?
      / \                    ===>    / \
     ?   ?                          ?   =   <- HEAD [lexp_head] `
                                       / \                      `
                        old memory -> ?   .         [new_head]  `- new memory
     */
#define OLD_RIGHT (__SEXP_RIGHT_SYM | __SEXP_RIGHT_SEXP | __SEXP_RIGHT_LEXP)
#define RIGHT_BECOMES_LEFT(x) (((x) & OLD_RIGHT) >> 1)

#ifdef DEBUG
    fputs("  -> EV: node lexp\n", stderr);
#endif
    struct lisp_sexp* lexp_head = new_head;

    lexp_head->root  = lisp_sexp_node_set_pos(TO_ROOT, pr, head);

    pr = pool_add_node(mppv);

    new_head                    = pr.mem;
    struct lisp_sexp  old_head  = *head;

    if (pr.new) {
      mppv = *mpp = pr.mem;
    }

    head->t        &= ~OLD_RIGHT;
    head->t        |= __SEXP_RIGHT_LEXP;

    new_head->root  = lisp_sexp_node_set_pos(TO_ROOT, pr, lexp_head);

    lexp_head->t    = (RIGHT_BECOMES_LEFT(old_head.t) | __SEXP_RIGHT_SEXP | __SEXP_SELF_LEXP);
    lexp_head->root = lisp_sexp_node_set_pos(pr, head);

    if (old_head.t & __SEXP_RIGHT_SYM) {
      lexp_head->left.sym   = old_head.right.sym;
    }
    else if (old_head.t & RIGHT_CHILD_T) {
      struct lisp_sexp* rch = lisp_sexp_node_get_pos(pr.base, head, RIGHT_CHILD_T);
      lexp_head->left.pos   = lisp_sexp_node_set_pos(TO_CHILD, pr, rch);
      rch->root             = lisp_sexp_node_set_pos(TO_ROOT, pr, lexp_head);
    }

    head->right.pos = lisp_sexp_node_set_pos(TO_CHILD, pr, lexp_head);
    head            = new_head;
  }
}
static void lisp_sexp_sym(struct MEMPOOL_TMPL(lisp_sexp)** mpp) {
  if (!head) {
#ifdef DEBUG
    fputs("-> lisp_do_sym()\n", stderr);
#endif
    goto done;
  }

#ifdef DEBUG
  fputs("-> lisp_sexp_sym()\n", stderr);
#endif

  struct MEMPOOL_TMPL(lisp_sexp)* mppv = *mpp;

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
     ?   ?                  ?   = <- HEAD [new_head]
                               / \
                              ?   *
     */
#ifndef OLD_RIGHT
#  define OLD_RIGHT (__SEXP_RIGHT_SYM | __SEXP_RIGHT_SEXP | __SEXP_RIGHT_LEXP)
#endif
#ifdef DEBUG
    fputs("  -> EV: sym lexp\n", stderr);
#endif

    struct lisp_sexp old_head = *head;
    struct pool_ret_t pr      = pool_add_node(mppv);

    if (pr.new) {
      *mpp      = pr.mem;
    }

    head->t    &= ~OLD_RIGHT;
    head->t    |= __SEXP_RIGHT_LEXP;

    struct lisp_sexp* lexp_head = pr.mem;
    lexp_head->root = lisp_sexp_node_set_pos(TO_ROOT, pr, head);
    lexp_head->t    = (RIGHT_BECOMES_LEFT(old_head.t) | __SEXP_RIGHT_SYM | __SEXP_SELF_LEXP);

    if (old_head.t & __SEXP_RIGHT_SYM) {
      lexp_head->left.sym = old_head.right.sym;
    }
    else if (old_head.t & (__SEXP_RIGHT_SEXP | __SEXP_RIGHT_LEXP)) {
      struct lisp_sexp* rch = lisp_sexp_node_get_pos(pr.base, head, RIGHT_CHILD_T);
      lexp_head->left.pos   = lisp_sexp_node_set_pos(TO_CHILD, pr, rch);
      rch->root             = lisp_sexp_node_set_pos(TO_ROOT, pr, lexp_head);
    }

    head->right.pos      = lisp_sexp_node_set_pos(TO_CHILD, pr, lexp_head);
    lexp_head->right.sym = chash;

    head = lexp_head;
  }

done:
  hash_i     = 0;
  chash.len  = 0;

  chash.body.hash = 0L;
  *(ulong*) chash.body.cmask = chash.body.hash;
}
static void lisp_do_sexp(struct MEMPOOL_TMPL(lisp_sexp)** mpp) {
  /**
     this algorithm is broken up into three states:

       1. leftwise
       2. rightwise
       3. rebound

     the default state is state 1. If the left child is a symbol,
     then state 3. is issued.

     state 3. can then either issue state 2. or itself.

     state 2. will almost always issue back state 1. or state 3.
   */

  struct MEMPOOL_TMPL(lisp_sexp)* p = *mpp;
  struct lisp_sexp* _head           = head;

  enum sexp_t t  = 0;
  bool done_side = false;

  for (; p; p = p->prev) {
    p->used = 0;
  }

  p    = (*mpp)->base;
  head = root;

  goto start_algo;

again_algo:
  /** issued another iteration:
        try to go rightwise, then let the leftwise algorithm figure out what to do
   */
    if (! done_side) {
      done_side = true;
      head      = lisp_sexp_node_get_pos(p, head, RIGHT_CHILD_T);
    }
    else {
      return;
    }

start_algo: ;
  for (;;) {
left_algo:
    t = head->t;
    if (!(t & LEFT_CHILD_T)) {
      /** left child of common root is SYM or NIL:
            go root; let the rebound algorithm figure out what to do
       */
#ifdef DEBUG
        fprintf(stderr, "-> lisp_do_sexp::left = %s\n", head->left.sym.body.cmask);
#endif
root_algo:
        if (head == root) {
          goto again_algo;
        }

        _head = head;
        head  = lisp_sexp_node_root(&p, head);

        if (_head == lisp_sexp_node_get_pos(p, head, LEFT_CHILD_T)) {
          /** came from the left: go root, then go right, then go left again
               .           ->      .
              / \                 / \
            >*<  ?               *  >?<
          */
          head = lisp_sexp_node_get_pos(p, head, RIGHT_CHILD_T);
          t    = head->t;
          if (t & RIGHT_CHILD_T) {
            /** right child of common root is SEXP or LEXP:
                  continue doing the algorithm leftwise
             */
            goto left_algo;
          }
          else {
            /** right child of common root is SYM or NIL:
                  go root; left the rootwise algorithm figure out what to do
             */
#ifdef DEBUG
            fprintf(stderr, "-> lisp_do_sexp::right = %s\n", head->right.sym.body.cmask);
#endif
            goto root_algo;
          }
        }

        /** came from the right: go root again; let the leftwise algorithm figure out what to do
              .                >.<
             / \    ^    ->    / \
            ?  >.<  |         ?   .
         */
        else {
          if (head == root) {
            goto again_algo;
          }
          else {
            goto root_algo;
          }
        }
    }
    else {
      head = lisp_sexp_node_get_pos(p, head, LEFT_CHILD_T);
    }
  }

  head = NULL;
  *mpp = p;
}
static inline void lisp_sexp_end(struct MEMPOOL_TMPL(lisp_sexp)* mpp) {
#ifdef DEBUG
  fputs("<- lisp_sexp_end()\n", stderr);
#endif

  struct lisp_sexp* phead = head;

  if (phead == root) {
    return;
  }

  for (;;) {
    phead = lisp_sexp_node_root(&mpp, phead);

    if (phead->t & (__SEXP_SELF_SEXP | __SEXP_SELF_ROOT)) {
      break;
    }
  }

  head = phead;
}
static inline struct lisp_hash_body do_chash(struct lisp_hash_body body,
                                             int i, char c) {
  ulong hash_byt = ((c + i) % 0x100);

  i         %= 8;
  hash_byt <<= (i*8);

  body.hash      |= hash_byt;
  body.cmask[i]   = c;

  return body;
}
static inline struct lisp_cps lisp_ev(struct lisp_cps pstat,
                                      enum lisp_pev sev) {
  if (sev & __LISP_SYMBOL_OUT) {
    if (pstat.master & __LISP_SYMBOL_IN) {
#ifdef DEBUG
      fputs("<- EV: symbol out\n", stderr);
#endif
      pstat.master &= ~__LISP_SYMBOL_IN;
      lisp_sexp_sym(&sexpmpp);
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

#ifdef DEBUG
    fputs("<- EV: paren out\n", stderr);
#endif

    if (paren) {
      pstat.master &= ~__LISP_PAREN_IN;
      --paren;
      lisp_sexp_end(sexpmpp);
      if (!paren) {
        lisp_do_sexp(&sexpmpp);
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
    lisp_sexp_node(&sexpmpp);
    ++paren;
  }

  pstat.master |= sstat;

  return pstat;
}
static inline struct lisp_cps lisp_whitespace(struct lisp_cps pstat) {
  if (pstat.master & __LISP_SYMBOL_IN) {
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

  ++chash.len;
  chash.body = do_chash(chash.body, hash_i, c);
  ++hash_i;

#ifdef DEBUG
  fprintf(stderr, "vslisp: character (%c) (0x%lx)\n", c, chash.body.hash);
#endif

 done:
  return pstat;
}
static int parse_ioblock(char* buf, uint size) {
  int ret = 0;

  for (uint i = 0; i < size; i++) {
    char c = buf[i];

    switch (c) {
    case __LISP_PAREN_OPEN:
#ifdef DEBUG
      fputs("-> EV: paren open\n", stderr);
#endif
      lcps = lisp_stat(lcps, __LISP_PAREN_IN);
      break;
    case __LISP_PAREN_CLOSE:
#ifdef DEBUG
      fputs("<- EV: paren close\n", stderr);
#endif
      lcps = lisp_ev(lcps, __LISP_PAREN_OUT);
      break;
    case __LISP_WHITESPACE:
#ifdef DEBUG
      fputs("-> EV: whitespace\n", stderr);
#endif
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

#ifdef DEBUG
  printf("vslisp: [ret = %d, paren = %d]\n", ret, paren);
#endif

 done:
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
  sexpmp.total      = SEXPPOOL;
  sexpmp.used       = 1;
  sexpmp.prev       = NULL;
  sexpmp.next       = NULL;

  sexpmpp           = &sexpmp;

  head              = NULL;
  root              = sexpmpp->mem;

  sexpmpp->base     = sexmpmpp;
  sexpmpp->used     = 1;

  root->root.pidx     = 0;
  root->t           = (__SEXP_SELF_ROOT | __SEXP_LEFT_EMPTY | __SEXP_RIGHT_EMPTY);

  int ret = parse_bytstream(STDIN_FILENO);

  if (ret) {
    write(STDERR_FILENO,
          MSG("[ !! ] vslisp: error while parsing file\n"));
  }

#ifdef DEBUG
#  ifdef DEBUG_DUMP_TREE
  fputs(iobuf, stdout);
  for (uint i = 0; i < SEXPPOOL; i++) {
    struct lisp_sexp* sexp = (struct lisp_sexp*) &sexpmp.mem[i];
    fprintf(stdout, "(type = %x) (root = %d) (left = %d, %s) (right = %d, %s)\n",
            sexp->t,
            sexp->root.am,
            sexp->left.pos.am,
            sexp->left.sym.body.cmask,
            sexp->right.pos.am,
            sexp->right.sym.body.cmask
      );
  }
#  endif
#endif

  return ret;
}
