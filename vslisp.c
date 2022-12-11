 // vslisp: a very simple lisp implementation;
//         not bootstrapped

#include <unistd.h>
#include <stdlib.h>

#ifdef DEBUG
#  include <stdio.h>
#endif

#include "vslisp.h"
#include "pool.h"

#ifndef LIBVSL
//  #include "prim.h"
#else
//  #include "libvsl.h"
#endif

static uint paren             = 0;
static uint hash_i            = 0;
static char iobuf[IOBLOCK]    = {0};

static struct lisp_hash chash = {0};
static struct lisp_cps   lcps = {0};
static struct lisp_sexp* head = NULL,
                       * root = NULL;

struct MEMPOOL(lisp_sexp, SEXPPOOL);
struct MEMPOOL_RET(lisp_sexp);
//struct MEMPOOL(lisp_sym_hash, HASHPOOL);

static struct MEMPOOL_TMPL(lisp_sexp)  sexpmp = {0};
static struct MEMPOOL_TMPL(lisp_sexp)* sexpmpp = NULL;
//static struct MEMPOOL_TMPL(lisp_sym_hash) hashmp = {0};

static inline void pool_clean(struct MEMPOOL_TMPL(lisp_sexp)* pp) {
  for (; pp; pp = pp->next) {
    pp->used = 0;
  }
}
static inline struct MEMPOOL_RET_TMPL(lisp_sexp)
pool_add_node(struct MEMPOOL_TMPL(lisp_sexp)* mpp) {
  struct MEMPOOL_RET_TMPL(lisp_sexp) ret = {
    .mem   = NULL,
    .base  = mpp,
    .entry = NULL,
    .same  = true,
  };

  if (mpp->used == mpp->total) {
    if (!mpp->next) {
      mpp->next        = malloc(sizeof(struct MEMPOOL_TMPL(lisp_sexp)));

      mpp->next->idx   = (mpp->idx + 1);
      mpp->next->prev  = mpp;
      mpp->next->next  = NULL;
    }

    ret.same          = false;
    mpp->next->total  = mpp->total;
    mpp->next->used   = 0;
  }

  ret.mem   = mpp;
  ret.entry = (mpp->mem + mpp->used);
  ++mpp->used;

  return ret;
}
static inline struct MEMPOOL_RET_TMPL(lisp_sexp)
pool_from_idx(struct MEMPOOL_TMPL(lisp_sexp)* mpp,
              uint idx) {
  struct MEMPOOL_RET_TMPL(lisp_sexp) ret = {0};
  struct MEMPOOL_TMPL(lisp_sexp)* pp     = mpp;
  int diff = (idx - mpp->idx);

  if (diff > 0) {
    for (; diff; --diff) {
      pp = mpp->next;
    }
  }
  else if (diff < 0) {
    for (diff = -diff; diff; --diff) {
      pp = mpp->prev;
    }
  }

  ret.entry  =
    (ret.mem = pp)->mem;
  ret.base   = mpp;
  ret.same   = (bool) (pp == mpp);

  return ret;
}
static inline struct pos_t lisp_sexp_node_set_pos(enum sexp_t t,
                                                  struct MEMPOOL_RET_TMPL(lisp_sexp) pr,
                                                  struct lisp_sexp* head) {
  struct pos_t ret      = {0};

  uint off              = 0;
  struct lisp_sexp* mem = NULL;

  if (!pr.same) {
    if (t == __SEXP_CHILD) {
      mem = pr.mem->mem;
      off = pr.mem->idx;
    }
    else if (t == __SEXP_ROOT) {
      mem = pr.base->mem;
      off = pr.base->idx;
    }
  }
  else {
    mem = pr.mem->mem;
    off = pr.mem->idx;
  }

  ret.am   = (head - mem);
  ret.pidx = off;

  return ret;
}
static inline struct MEMPOOL_RET_TMPL(lisp_sexp)
lisp_sexp_node_get_pos(enum sexp_t t,
                       struct MEMPOOL_RET_TMPL(lisp_sexp) pr,
                       struct lisp_sexp* head) {
  uint pidx = 0;
  uint am   = 0;

  if (t == __SEXP_ROOT) {
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

  // TODO: signal that the memory is in another section
  pr       = pool_from_idx(pr.mem, pidx);
  pr.entry = (pr.mem->mem + am);

  return pr;
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

  struct MEMPOOL_RET_TMPL(lisp_sexp) pr
                             = pool_add_node(*mpp);
  struct lisp_sexp* new_head = pr.entry;

  if (!pr.same) {
    *mpp = pr.mem;
  }

  new_head->t = (__SEXP_SELF_SEXP | __SEXP_LEFT_EMPTY | __SEXP_RIGHT_EMPTY);

  if (head->t & __SEXP_LEFT_EMPTY) {
    /**
          ? <- HEAD
         /
        . [new_head]
     */
    new_head->root  = lisp_sexp_node_set_pos(__SEXP_ROOT, pr, head);
    head->left.pos  = lisp_sexp_node_set_pos(__SEXP_CHILD, pr, new_head);
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
    new_head->root   = lisp_sexp_node_set_pos(__SEXP_ROOT, pr, head);
    head->right.pos  = lisp_sexp_node_set_pos(__SEXP_CHILD, pr, new_head);
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

    struct lisp_sexp  old_head  = *head;
    struct lisp_sexp* lexp_head = new_head;

    lexp_head->root = lisp_sexp_node_set_pos(__SEXP_ROOT, pr, head);
    lexp_head->t    = (RIGHT_BECOMES_LEFT(old_head.t) | __SEXP_RIGHT_SEXP | __SEXP_SELF_LEXP);

    head->t        &= ~OLD_RIGHT;
    head->t        |= __SEXP_RIGHT_LEXP;

    pr        = pool_add_node(*mpp);
    new_head  = pr.entry;

    if (!pr.same) {
      *mpp    = pr.mem;
    }

    new_head->root          = lisp_sexp_node_set_pos(__SEXP_ROOT, pr, lexp_head);

    if (old_head.t & __SEXP_RIGHT_SYM) {
      lexp_head->left.sym   = old_head.right.sym;
    }
    else if (old_head.t & RIGHT_CHILD_T) {
      struct MEMPOOL_TMPL(lisp_sexp)* mem = pr.mem;

      pr = lisp_sexp_node_get_pos(RIGHT_CHILD_T, pr, head);
      struct lisp_sexp* rch = pr.entry;
      pr.mem                = mem;

      if (!pr.same) {
        *mpp = pr.mem;
      }

      lexp_head->left.pos   = lisp_sexp_node_set_pos(__SEXP_CHILD, pr, rch);
      rch->root             = lisp_sexp_node_set_pos(__SEXP_ROOT, pr, lexp_head);
    }

    head->right.pos = lisp_sexp_node_set_pos(__SEXP_CHILD, pr, lexp_head);
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

    struct lisp_sexp old_head   = *head;
    struct MEMPOOL_RET_TMPL(lisp_sexp) pr
                                = pool_add_node(*mpp);
    struct lisp_sexp* lexp_head = pr.entry;

    if (! pr.same) {
      *mpp = pr.mem;
    }

    lexp_head->t    = (RIGHT_BECOMES_LEFT(old_head.t) | __SEXP_RIGHT_SYM | __SEXP_SELF_LEXP);
    lexp_head->root = lisp_sexp_node_set_pos(__SEXP_ROOT, pr, head);

    head->t        &= ~OLD_RIGHT;
    head->t        |= __SEXP_RIGHT_LEXP;

    if (old_head.t & __SEXP_RIGHT_SYM) {
      lexp_head->left.sym   = old_head.right.sym;
    }
    else if (old_head.t & (__SEXP_RIGHT_SEXP | __SEXP_RIGHT_LEXP)) {
      pr = lisp_sexp_node_get_pos(RIGHT_CHILD_T, pr, head);
      struct lisp_sexp* rch = pr.entry;
      lexp_head->left.pos   = lisp_sexp_node_set_pos(__SEXP_CHILD, pr, rch);
      rch->root             = lisp_sexp_node_set_pos(__SEXP_ROOT, pr, lexp_head);
    }

    head->right.pos      = lisp_sexp_node_set_pos(__SEXP_CHILD, pr, lexp_head);
    lexp_head->right.sym = chash;

    head = lexp_head;
  }

done:
  hash_i     = 0;
  chash.len  = 0;

  chash.body.hash = 0L;
  *(ulong*) chash.body.cmask = chash.body.hash;
}
static void lisp_do_sexp(struct MEMPOOL_TMPL(lisp_sexp)* mpp) {
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

  pool_clean(mpp);

  struct MEMPOOL_RET_TMPL(lisp_sexp) pp;
  struct lisp_sexp* _head = head;

  pp.base  = mpp;
  pp.mem   = mpp;
  pp.entry = mpp->mem;

  enum sexp_t t  = 0;
  bool done_side = false;

  head = root;

  goto start_algo;

issue_algo:
  /** issued another iteration:
        try to go rightwise, then let the leftwise algorithm figure out what to do

      NOTE:
        only called when one of the branches hit root
   */
  if (! done_side) {
    done_side = true;

    t = head->t;
    if (t & (__SEXP_RIGHT_SYM | __SEXP_RIGHT_EMPTY)) {
      /** right child of common root is SYM or NIL:
            end it here, there won't be another iteration
       */
#ifdef DEBUG
      fprintf(stderr, "  -> lisp_do_sexp::right = %lx\n", head->right.sym.body.hash);
#endif
      goto end_algo;
    }
    else {
#ifdef DEBUG
      fputs("  -> rebound root\n", stderr);
#endif
      pp   = lisp_sexp_node_get_pos(RIGHT_CHILD_T, pp, head);
      head = pp.entry;
    }
  }
  else {
end_algo:
    /** second time rebounding:
          there's nothing left to seek through; end the algorithm
     */
    // TODO
    return;
  }

start_algo: ;
  for (;;) {
    t = head->t;
    if (t & LEFT_CHILD_T) {
      /** left child of common root is SEXP or LEXP:
            go left again
       */
#ifdef DEBUG
      fputs("  -> left_child = exp\n", stderr);
#endif
      pp   = lisp_sexp_node_get_pos(LEFT_CHILD_T, pp, head);
      head = pp.entry;
    }
    else {
      /** left child of common root is SYM or NIL:
            try to go right
       */
#ifdef DEBUG
        fprintf(stderr, "  -> lisp_do_sexp::left = %lx\n", head->left.sym.body.hash);
#endif

rebound_algo:
        if (head == root) {
#ifdef DEBUG
          fputs("  -> hit root\n", stderr);
#endif
          goto issue_algo;
        }

        if (t & RIGHT_CHILD_T) {
#ifdef DEBUG
          fputs("  -> right_child = exp\n", stderr);
#endif

          pp   = lisp_sexp_node_get_pos(RIGHT_CHILD_T, pp, head);
          head = pp.entry;
          continue;
        }
        else {
          /** right child of common root is SYM or NIL:
                go root; let the rebound algorithm figure out what to do
           */
#ifdef DEBUG
          fprintf(stderr, "  -> lisp_do_sexp::right = %lx\n", head->right.sym.body.hash);
#endif

          _head = head;
          pp    = lisp_sexp_node_get_pos(__SEXP_ROOT, pp, head);
          head  = pp.entry;

          if (_head == lisp_sexp_node_get_pos(LEFT_CHILD_T, pp, head).entry) {
#ifdef DEBUG
            fputs("  -> rebound: from_left\n", stderr);
#endif
            /** rebound: came from the left:
                go right, then let the leftwise algorithm

                 .           ->      .
                / \                 / \
              >*<  ?               *  >?<
            */
            t    = head->t;

            if (t & RIGHT_CHILD_T) {
              /** right child of common root is SEXP or LEXP:
                  continue doing the leftwise algorithm
              */
#ifdef DEBUG
              fputs("  -> right_child = exp\n", stderr);
#endif
              pp   = lisp_sexp_node_get_pos(RIGHT_CHILD_T, pp, head);
              head = pp.entry;
              continue;
            }
            else {
              /** right child of common root is SYM or NIL:
                  go root; let the rebound algorithm figure out what to do
              */
#ifdef DEBUG
              fprintf(stderr, "  -> lisp_do_sexp::right = %lx\n", head->right.sym.body.hash);
#endif
              goto rebound_algo;
            }
          }

          else {
#ifdef DEBUG
            fputs("  -> rebound: from_right\n", stderr);
#endif
            /** rebound: came from the right:
                go root again; let the leftwise algorithm figure out what to do
                 .                >.<
                / \    ^    ->    / \
               ?  >.<  |         ?   .
            */
            if (head == root) {
#ifdef DEBUG
              fputs("  -> hit root\n", stderr);
#endif
              goto issue_algo;
            }
            else {
              goto rebound_algo;
            }
          }
        }
    }
  }

  head = NULL;
}
static inline void lisp_sexp_end(struct MEMPOOL_TMPL(lisp_sexp)* mpp) {
  // NOTE: `head' will always be on either an orphan SEXP or a paren SEXP,
  //       and probably on a different section than `root'

#ifdef DEBUG
  fputs("<- lisp_sexp_end()\n", stderr);
#endif

  struct MEMPOOL_RET_TMPL(lisp_sexp) pp = {0};
  struct lisp_sexp* phead = head;

  pp.mem   =
  pp.base  = mpp;
  pp.entry = phead;
  pp.same  = true;

  if (phead == root) {
    return;
  }

  for (;;) {
    pp    = lisp_sexp_node_get_pos(__SEXP_ROOT, pp, phead);
    phead = pp.entry;

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
        lisp_do_sexp(sexpmpp);
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

#ifndef LIBVSL
int main(void) {
  sexpmp.total      = SEXPPOOL;
  sexpmp.used       = 1;
  sexpmp.prev       = NULL;
  sexpmp.next       = NULL;

  sexpmpp           = &sexpmp;

  head              = NULL;
  root              = sexpmpp->mem;

  sexpmpp->used     = 1;

  root->t           = (__SEXP_SELF_ROOT | __SEXP_LEFT_EMPTY | __SEXP_RIGHT_EMPTY);

  int ret = parse_bytstream(STDIN_FILENO);

  if (ret) {
    write(STDERR_FILENO,
          MSG("[ !! ] vslisp: error while parsing file\n"));
  }

#  ifdef DEBUG
#    ifdef DEBUG_DUMP_TREE
  struct MEMPOOL_TMPL(lisp_sexp)* pp = &sexpmp;

loop:
  for (uint i = 0; i < SEXPPOOL; i++) {
    struct lisp_sexp* sexp = ((struct lisp_sexp*) pp->mem + i);
    fprintf(stdout, "(type = %x) (root = %d) (left = %d, %s) (right = %d, %s)\n",
            sexp->t,
            sexp->root.am,
            sexp->left.pos.am,
            sexp->left.sym.body.cmask,
            sexp->right.pos.am,
            sexp->right.sym.body.cmask
      );
  }

  if ((pp = pp->next)) {
    goto loop;
  }
#    endif
#  endif

  return ret;
}
#endif
