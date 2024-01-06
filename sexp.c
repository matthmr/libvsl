#include "debug.h"

#include "lisp.h"
#include "symtab.h"
#include "sexp.h"
#include "mm.h"

#include <string.h>

/** Base 'end' algorithm. See the `lisp_sexp_end' function */
static inline struct lisp_sexp* __lisp_sexp_end(struct lisp_sexp* expr_head) {
  if (!expr_head->root) {
    goto done;
  }

  do {
    expr_head = expr_head->root;
  } while (expr_head->lexp);

done:
  return expr_head;
}

/** Sets up an addition of an element, returning an LEXP as `lexp_head' */
static struct lisp_sexp*
lisp_sexp_on_lexp(struct lisp_sexp* expr_head, struct lisp_obj* lexp_obj,
                  struct lisp_sexp* lexp_head) {
  lexp_head->lexp = true;

  lexp_head->root = expr_head;
  lexp_head->left = expr_head->right;

  *lexp_obj = (struct lisp_obj) {
    .typ    = __LISP_OBJ_EXP,
    .m_typ  = __LISP_OBJ_ORG,
    ._.exp  = lexp_head,
    .refs   = 1,

    .cow.org.recip = 0,
  };

  expr_head->right = lexp_obj;

  return lexp_head;
}

////////////////////////////////////////////////////////////////////////////////

/** The algorithm is broken up into three states:

    1. going leftwise
    2. going rightwise
    3. rebounding

    The default state is:

    (1.) will continue to issue itself unless the left child is a symbol or NIL,
    then (3.) is issued.

    (2.) will issue back (1.). If the right child is a symbol or NIL, then (3.)
    is issued.

    (3.) will either issue (2.) or itself again. (3.) is broken up into further
    states:
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
    +--------+--------+-------+------------+------------+ */ //
struct lisp_yield
lisp_sexp_yield(struct lisp_yield yield,
                const enum lisp_yield_ignore ignore) {
  register enum lisp_yield_t ret = yield.stat;

  struct lisp_sexp* _expr_head = NULL, * expr_head = yield.exp;
  struct lisp_yield ret_t = {0};

  //// HANDLE PREVIOUS STATE

  switch (ret) {
  case __YIELD_LEFT_OBJ:
    goto rightwise;
  case __YIELD_RIGHT_OBJ:
    goto rebound;
  case __YIELD_END_EXPR:
    expr_head = expr_head->root;
    goto rebound;
  default:
    goto leftwise;
  }

rebound:
  if (!expr_head || !expr_head->root) {
    DB_MSG("[ sexp ] yield: done");

    defer_as(__YIELD_DONE);
  }

  _expr_head = expr_head;
  expr_head  = expr_head->root;

  // from the left
  if (expr_head->left->_.exp == _expr_head) {
    DB_MSG("[ sexp ] yield: rebound: from-left");

    goto rightwise;
  }

  // from the right
  else {
    DB_MSG("[ sexp ] yield: rebound: from-right");

    if (IGNORE_REBOUND(ignore) && _expr_head->lexp) {
      goto rebound;
    }

    expr_head = _expr_head;
    defer_as(__YIELD_END_EXPR);
  }

rightwise:
  if (expr_head->right) {
    if (IS_EXP(expr_head->right->typ)) {
      expr_head = expr_head->right->_.exp;

      DB_FMT("[ sexp ] yield: (rebound-left) right -> expr (%p)", expr_head);

      if (IGNORE_NONEND(ignore) ||
          (IGNORE_LEXP(ignore) && expr_head->lexp)) {
        goto leftwise;
      }

      defer_as(__YIELD_RIGHT_EXPR);
    }
    else {
      DB_MSG("[ sexp ] yield: (rebound-left) right -> obj");

      // if (IGNORE_OBJ(ignore)) {
      //   goto rebound;
      // }

      defer_as(__YIELD_RIGHT_OBJ);
    }
  }

  else {
    DB_MSG("[ sexp ] yield: (rebound-left) right -> nil");
    defer_as(__YIELD_END_EXPR);
  }

leftwise:
  if (expr_head->left) {
    if (IS_EXP(expr_head->left->typ)) {
      expr_head = expr_head->left->_.exp;

      DB_FMT("[ sexp ] yield: left -> expr (%p)", expr_head);

      // we assume this tree doesn't have any left LEXPs
      if (IGNORE_NONEND(ignore)) {
        goto leftwise;
      }

      defer_as(__YIELD_LEFT_EXPR);
    }
    else {
      DB_MSG("[ sexp ] yield: left -> obj");

      // if (IGNORE_OBJ(ignore)) {
      //   goto rightwise;
      // }

      defer_as(__YIELD_LEFT_OBJ);
    }
  }

  else {
    DB_MSG("[ sexp ] yield: left -> nil");
    defer_as(__YIELD_END_EXPR);
  }

  done_for_with(ret_t, (ret_t.exp  = expr_head,
                        ret_t.stat = ret));
}

/** The algorithm follows:

    EMPTY LEFT:
        ? [expr_head] <- ret
       /
      . [new_obj]

    EMPTY RIGHT:
        . [expr_head] <- ret
       / \
      ?   . [new_obj]


    FULL TREE:
        ? [expr_head]         ?
       / \             ===>  / \
      ?   ? [old_obj]       ?   ,   [lexp_head] } new memory <- ret
                               / \
                   [old_obj]  ?   * [new_obj]
 */ //
struct lisp_sexp*
lisp_sexp_obj(struct lisp_obj* obj, bool ref, struct lisp_sexp* expr_head) {
  register int ret = 0;

  struct lisp_obj* e_obj = NULL;

  if (!obj) {
    obj = mm_alloc(sizeof(*obj));
    assert(obj, OR_ERR());

    *obj = (struct lisp_obj) {
      .refs = 1,
      .typ  = __LISP_OBJ_NIL,

      .cow.org.recip = 0,
    };
  }

  if (ref) {
    obj->refs++;

    e_obj = obj;
  }

  else {
    obj->m_typ |= (__LISP_OBJ_ORG | __LISP_OBJ_COW);
    obj->cow.org.recip++;

    e_obj = mm_alloc(sizeof(*e_obj));
    assert(e_obj, OR_ERR());

    if (obj) {
      e_obj = memcpy(e_obj, obj, sizeof(*e_obj));

      e_obj->m_typ &= ~__LISP_OBJ_ORG;
      e_obj->cow.from = obj;
    }
  }

  DB_FMT("[ sexp ] add obj: %p", e_obj);

  ////

  if (!expr_head->left) {
    expr_head->left = e_obj;
  }

  else if (!expr_head->right) {
obj_right:
    expr_head->right = e_obj;
  }

  else {
    struct lisp_obj* lexp_obj = mm_alloc(sizeof(*e_obj));
    assert(lexp_obj, OR_ERR());

    struct lisp_sexp* lexp_head = mm_alloc(sizeof(*expr_head));
    assert(lexp_head, OR_ERR());

    expr_head = lisp_sexp_on_lexp(expr_head, lexp_obj, lexp_head);

    DB_FMT("  -> add: sym: lexp (%p)", expr_head);

    goto obj_right;
  }

  // TODO: needed?
  // if (!(obj->m_typ | __LISP_OBJ_COW) && obj->refs == 1 &&
  //     e_obj->typ == __LISP_OBJ_EXP && e_obj->_.exp) {
  //   e_obj->_.exp->root = expr_head;
  // }

  done_for((expr_head = ret? NULL: expr_head));
}

struct lisp_sexp* lisp_sexp_end(struct lisp_sexp* expr_head) {
  if (expr_head->lexp) {
    expr_head = __lisp_sexp_end(expr_head);
  }

  if (expr_head->root) {
    expr_head = expr_head->root;
  }

  DB_FMT("[ sexp ] end (%p)", expr_head);

  return expr_head;
}
