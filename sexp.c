#include "debug.h"

#include "stack.h" // also includes `sexp.h', `lisp.h'
#include "err.h"   // also includes `utils.h'
#include "mm.h"    // also includes `utils.h'

/**
   Base 'end' algorithm. See the `lisp_sexp_end' function
 */
static inline struct lisp_sexp* __lisp_sexp_end(struct lisp_sexp* expr_head) {
  if (IS_ROOT(expr_head->t)) {
    goto done;
  }

  do {
    expr_head = expr_head->root;
  } while (!IS_SEXP(expr_head->t));

done:
  return expr_head;
}

/**
   Applies the 'transversal' algorithm, used to transverse the SEXP tree
   applying an action on each expression. This has the effect of 'yielding'
   expressions.

   NOTE: we assume this is an expression tree generated by calling
   `lisp_sexp_node', `lisp_sexp_sym' and `lisp_sexp_end'. PRIMVSL allows for
   arbitrarily modifying the tree. Note that that *will* make this function
   useless if trying to parse that modified tree instead.

   ----

   The algorithm is broken up into three states:

   1. going leftwise
   2. going rightwise
   3. rebounding

   The default state is (1.), it will continue to issue itself unless the left
   child is a symbol or NIL (doesn't exist), then (3.) is issued.

   (3.) can then either issue (2.) or itself again.

   (2.) will issue back (1.) If the right child is a symbol or NIL, then (3). is
   issued.

   (3.) is broken up into further states:
     (3a.): if rebounding from the left, do (2.).
     (3b.): if rebounding from the right, do (3.).

   If root was hit from (3b.) stop the algorithm.

   -----

   Here's a visualization (the examples below go from `a' to `b' to `c'):

   +--------+--------+-------+------------+------------+
   | 1.   a | 2. a   | 3. b  | 3a.   b    | 3b. [c]    |
   |     /  |     \  |    |  |      / \   |       \    |
   |    b   |      b |    a  |     a  [c] |        b   |
   |        |     /  |       |            |         \  |
   |        |   [c]  |       | 3 -> 2     | 3 -> 3   a |
   +--------+--------+-------+------------+------------+
*/
static struct lisp_trans
lisp_sexp_trans(struct lisp_trans trans, bool stack, bool ignore_lexp) {
  register enum lisp_sexp_t    t = 0;
  register enum lisp_trans_t ret = trans.stat;

  struct lisp_sexp*   _expr_head = NULL,
                  *    expr_head = trans.exp;

  struct lisp_trans        ret_t = {0};

  if (ret & __TRANS_LEFT_SYM) {
    ret &= ~__TRANS_LEFT_SYM;
    goto rightwise;
  }
  else if (ret & (__TRANS_REBOUND_LEFT | __TRANS_REBOUND_RIGHT)) {
    ret &= ~(__TRANS_REBOUND_LEFT | __TRANS_REBOUND_RIGHT);
    goto rebound;
  }

  //// PREAMBLE

leftwise:
  t = expr_head->t;

  if (LEFT_EXPR(t)) {
    DB_MSG("[ sexp ] trans: left -> expr");
    _expr_head = expr_head;
    expr_head  = expr_head->left.exp;

    defer_as(__TRANS_LEFT_EXPR);
    // goto leftwise;
  }

  else if (LEFT_SYM(t)) {
    DB_FMT("[ sexp ] trans: left -> sym (0x%x)", expr_head->left.sym.sum);

    defer_as(__TRANS_LEFT_SYM);

rightwise:
    t = expr_head->t;

    if (RIGHT_EXPR(t)) {
      DB_MSG("[ sexp ] trans: (rebound-left) right -> expr");

      _expr_head = expr_head;
      expr_head  = expr_head->right.exp;

      if (ignore_lexp && IS_LEXP(expr_head->t)) {
        goto leftwise;
      }

      defer_as(__TRANS_RIGHT_EXPR);
    }

    else if (RIGHT_SYM(t)) {
      DB_FMT("[ sexp ] trans: (rebound-left) right -> sym (0x%x)",
             expr_head->left.sym.sum);
      defer_as(__TRANS_REBOUND_LEFT | __TRANS_RIGHT_SYM);

      // goto rebound;
    }

    else {
      DB_MSG("[ sexp ] trans: (rebound-left) right -> nil");
      defer_as(__TRANS_REBOUND_LEFT);

      // goto rebound;
    }
  }

  else {
    DB_MSG("[ sexp ] trans: left -> nil");

    bool hit_root = false;

rebound:
    _expr_head = expr_head;
    expr_head  = expr_head->root;

    if (!expr_head) {
      DB_MSG("[ sexp ] trans: hit root");
      expr_head = _expr_head;

      hit_root = true;
    }

    // from the left
    if (expr_head->left.exp == _expr_head) {
      DB_MSG("[ sexp ] trans: rebound: from-left");

      goto rightwise;
    }

    // from the right
    else {
      // hit root from the right: done
      if (hit_root) {
        DB_MSG("[ sexp ] trans: done");
        defer_as(__TRANS_REBOUND_RIGHT | __TRANS_DONE);
      }

      DB_MSG("[ sexp ] trans: rebound: from-right");

      // `stack' set ignores LEXP-rebound-from-right
      if (stack && IS_LEXP(_expr_head->t)) {
        goto rebound;
      }

      defer_as(__TRANS_REBOUND_RIGHT);
    }
  }

  done_for_with(ret_t, (ret_t.exp  = expr_head,
                        ret_t.stat = ret));
}

static struct lisp_trans lisp_sexp_yield_clear(struct lisp_trans trans) {
  return lisp_sexp_trans(trans, false, true);
}

static struct lisp_trans lisp_sexp_yield_lit(struct lisp_trans trans) {
  return lisp_sexp_trans(trans, true, false);
}

////////////////////////////////////////////////////////////////////////////////

/**
   Yield expressions from the SEXP tree
 */
struct lisp_trans lisp_sexp_yield(struct lisp_trans trans) {
  return lisp_sexp_trans(trans, true, true);
}

struct lisp_sexp* lisp_sexp_copy(struct lisp_sexp* expr_head) {
  register int             ret = 0;

  struct lisp_sexp* _expr_head = NULL;
  struct lisp_sexp*      _head = NULL;
  struct lisp_trans       tret = {0};

  tret.exp  = expr_head;
  tret.stat = __TRANS_OK;

  _head      = mm_alloc(sizeof(struct lisp_sexp));
  _expr_head = _head;
  assert(_head, OR_ERR());

  _expr_head->t    = expr_head->t;
  _expr_head->root = NULL;

  do {
    tret = lisp_sexp_yield_lit(tret);

    if (tret.stat & __TRANS_LEFT_EXPR) {
      _head = mm_alloc(sizeof(struct lisp_sexp));
      assert(_head, OR_ERR());

      _head->t = tret.exp->t;

      _expr_head->left.exp = _head;
      _head->root          = _expr_head;

      _expr_head = _head;
    }
    else if (tret.stat & __TRANS_RIGHT_EXPR) {
      _head = mm_alloc(sizeof(struct lisp_sexp));
      assert(_head, OR_ERR());

      _head->t = tret.exp->t;

      _expr_head->right.exp = _head;
      _head->root           = _expr_head;

      _expr_head = _head;
    }

    //// SET

    else if (tret.stat & (__TRANS_REBOUND_LEFT | __TRANS_REBOUND_RIGHT)) {
      if (tret.stat & __TRANS_LEFT_SYM) {
        _expr_head->left.exp  = tret.exp->left.exp;
      }
      else if (tret.stat & __TRANS_RIGHT_SYM) {
        _expr_head->right.exp = tret.exp->right.exp;
      }

      _expr_head = _expr_head->root;
    }
  } while (!(tret.stat & __TRANS_DONE));

  done_for((_expr_head = ret? NULL: _expr_head));
}

/**
   Clears the SEXP tree rooted by @expr_head. @expr_head is not returned back,
   so the caller has to be aware that the memory it had before is probably
   garbage now
 */
void lisp_sexp_clear(struct lisp_sexp* expr_head) {
  if (!expr_head) {
    return;
  }

  struct lisp_sexp* _expr_head = expr_head;
  struct lisp_trans       tret = {0};

  tret.exp  = expr_head;
  tret.stat = __TRANS_OK;

  do {
    _expr_head = expr_head;

    tret      = lisp_sexp_yield_clear(tret);
    expr_head = tret.exp;

    if (tret.stat & __TRANS_REBOUND_RIGHT) {
      mm_free(_expr_head);
    }
  } while (!(tret.stat & __TRANS_DONE));

  return;
}

/**
   Adds a node expression to @expr_head
 */
struct lisp_sexp* lisp_sexp_node(struct lisp_sexp* expr_head) {
  register int ret = 0;

  DB_MSG("[ sexp ] add: node");

  // calling with @expr_head as NULL: init it, then defer
  if (!expr_head) {
    expr_head = mm_alloc(sizeof(struct lisp_sexp));
    assert(expr_head, OR_ERR());

    expr_head->root      = NULL; // only root has `->root' as NULL
    expr_head->left.exp  = NULL;
    expr_head->right.exp = NULL;

    expr_head->t         = (__SEXP_SELF_ROOT | __SEXP_SELF_SEXP);

    defer_as(0);
  }

  struct lisp_sexp* new_head = mm_alloc(sizeof(struct lisp_sexp));
  assert(new_head, OR_ERR());

  if (!LEFT(expr_head->t)) {
    /**
         ? [expr_head]
        /
       . [new_head]
     */
    new_head->root      = expr_head;
    new_head->left.exp  = NULL;
    new_head->right.exp = NULL;

    new_head->t    = __SEXP_SELF_SEXP;

    expr_head->left.exp = new_head;
    expr_head->t  |= __SEXP_LEFT_SEXP;
  }

  //////////////////////////////////////////////////////////////////////////////

  else if (!RIGHT(expr_head->t)) {
    /**
         . [expr_head]
        / \
       ?   . [new_head]
     */
    new_head->root      = expr_head;
    new_head->left.exp  = NULL;
    new_head->right.exp = NULL;

    new_head->t    = __SEXP_SELF_SEXP;

    expr_head->right.exp = new_head;
    expr_head->t  |= __SEXP_RIGHT_SEXP;
  }

  //////////////////////////////////////////////////////////////////////////////

  else {
    /**
         ? [expr_head]         ?
        / \             ===>  / \
       ?   ? [old_head]      ?   ,   [lexp_head] }
                                / \              } new memory
                    [old_head] ?   . [new_head]  }
     */
    DB_MSG("  -> add: node: lexp");

    // save the old state of `expr_head', save the address of the right child of
    // `expr_head', swap the memory of `new_head' to `lexp_head'
    struct lisp_sexp _expr_head = *expr_head;
    struct lisp_sexp* lexp_head = new_head;

    lexp_head->t = __SEXP_SELF_LEXP;

    new_head = mm_alloc(sizeof(struct lisp_sexp));
    assert(new_head, OR_ERR());

    new_head->root      = lexp_head;
    new_head->left.exp  = NULL;
    new_head->right.exp = NULL;

    new_head->t    = __SEXP_SELF_SEXP;

    // setup `lexp_head' as the right child of `expr_head'
    expr_head->right.exp = lexp_head;
    expr_head->t         = ((expr_head->t & ~__RIGHT) | __SEXP_RIGHT_LEXP);
    lexp_head->root      = expr_head;

    // setup `old_head' as the left child of `lexp_head'
    if (RIGHT_EXPR(_expr_head.t)) {
      lexp_head->left.exp  = _expr_head.right.exp;
      lexp_head->t        |= SWAP_RL(_expr_head.t);

      _expr_head.right.exp->root = lexp_head;
    }
    else {
      lexp_head->left.sym  = _expr_head.right.sym;
      lexp_head->t        |= __SEXP_LEFT_SYM;
    }

    // setup `new_head' as the right child of `lexp_head'
    lexp_head->right.exp  = new_head;
    lexp_head->t         |= __SEXP_RIGHT_SEXP;
  }

  expr_head = new_head;

  done_for((expr_head = ret? NULL: expr_head));
}

/**
   Adds a symbol of hash @sym_hash to @expr_head
 */
struct lisp_sexp*
lisp_sexp_sym(struct lisp_sexp* expr_head, struct lisp_hash sym_hash) {
  register int ret = 0;

  // for now, top-level symbols are silently ignored
  assert(expr_head, 0);

  DB_MSG("[ sexp ] add: sym");

  if (!LEFT(expr_head->t)) {
    /**
         ? [expr_head]
        /
       *
     */
    expr_head->t        |= __SEXP_LEFT_SYM;
    expr_head->left.sym  = sym_hash;
  }

  //////////////////////////////////////////////////////////////////////////////

  else if (!RIGHT(expr_head->t)) {
    /**
         ? [expr_head]
        / \
       ?   *
     */
    expr_head->t         |= __SEXP_RIGHT_SYM;
    expr_head->right.sym  = sym_hash;
  }

  //////////////////////////////////////////////////////////////////////////////

  else {
    /**
         ? [expr_head]         ?
        / \             ===>  / \
       ?   ? [old_head]      ?   ,   [lexp_head] } new memory
                                / \
                    [old_head] ?   * [new_sym]
     */
    DB_MSG("  -> add: sym: lexp");

    // save the old state of `expr_head', save the address of the right child of
    // `expr_head'
    struct lisp_sexp _expr_head = *expr_head;

    struct lisp_sexp* lexp_head = mm_alloc(sizeof(struct lisp_sexp));
    assert(lexp_head, OR_ERR());

    lexp_head->t = __SEXP_SELF_LEXP;

    // setup `lexp_head' as the right child of `expr_head'
    expr_head->right.exp = lexp_head;
    expr_head->t         = ((expr_head->t & ~__RIGHT) | __SEXP_RIGHT_LEXP);
    lexp_head->root      = expr_head;

    // setup `old_head' as the left child of `lexp_head'
    if (RIGHT_EXPR(_expr_head.t)) {
      lexp_head->left.exp  = _expr_head.right.exp;
      lexp_head->t        |= SWAP_RL(_expr_head.t);

      _expr_head.right.exp->root = lexp_head;
    }
    else {
      lexp_head->left.sym  = _expr_head.right.sym;
      lexp_head->t        |= __SEXP_LEFT_SYM;
    }

    // setup `new_head' as the right child of `lexp_head'
    lexp_head->right.sym  = sym_hash;
    lexp_head->t         |= __SEXP_RIGHT_SYM;

    expr_head = lexp_head;
  }

  done_for((expr_head = ret ? NULL: expr_head));
}

/**
   Applies the 'end' algorithm to @expr_head

   This algorithm will try to get the parent of @expr_head once. If the current
   expression head is a LEXP, then it will recurse until the parent is either an
   SEXP, then it will try again to get the parent unless it has hit root

   NOTE: it's the job of the language engine to stop the calls to the SEXP
   module once the last paren has been issued, otherwise this *will* have a bug
   where expressions that are supposed to be separate are counted as child of
   the root @expr_head. It's a design concession: this prevents the caller from
   always checking NULL against a copy of the expression head
*/
struct lisp_sexp* lisp_sexp_end(struct lisp_sexp* expr_head) {
  DB_MSG("[ sexp ] end");

  if (IS_LEXP(expr_head->t)) {
    expr_head = __lisp_sexp_end(expr_head);
  }

  expr_head = __lisp_sexp_end(expr_head);

  return expr_head;
}
