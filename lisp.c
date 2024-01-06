#include "mm.h"
#include "lisp.h"
#include "stack.h"

/** Free an expression wholly */
static void lisp_free_exp(struct lisp_sexp* exp) {
  struct lisp_yield yield = {
    .exp  = exp,
    .stat = __YIELD_OK,
  };

  struct lisp_sexp* root = yield.exp->root;
  yield.exp->root = NULL;

  while (yield =
           lisp_sexp_yield(yield, __YIELD_IGNORE_LEXP|__YIELD_IGNORE_NONEND),
         yield.stat != __YIELD_DONE) {
    switch (yield.stat) {
    case __YIELD_LEFT_OBJ:
      yield.exp->left->refs--;
      lisp_free_obj(yield.exp->left);
      break;

    case __YIELD_RIGHT_OBJ:
      yield.exp->right->refs--;
      lisp_free_obj(yield.exp->right);
      break;

    default:
      break;
    }
  }

  yield.exp->root = root;

  return;
}

/** Frees the memory of the object based on the type */
static void lisp_free_obj_mem(struct lisp_obj* obj) {
  switch (obj->typ) {
  case __LISP_OBJ_CFUN:
  case __LISP_OBJ_CFUN_LIT:
    mm_free(obj->_.cfun);
    break;

  case __LISP_OBJ_LAMBDA:
  case __LISP_OBJ_MACRO:
  case __LISP_OBJ_EXP:
    lisp_free_exp(obj->_.exp);
    break;

  case __LISP_OBJ_SYM:
    mm_free((void*)obj->_.sym.str);
    break;

  case __LISP_OBJ_FOREIGN:
    mm_free(obj->_.gen.dat);
    break;

  default:
    break;
  }
}

////////////////////////////////////////////////////////////////////////////////

struct lisp_ret
lisp_eval(struct lisp_sexp* argp, uint argv, struct lisp_symtab* envp) {
  register int       ret = 0;
  struct lisp_ret  ret_t = {0};

  struct lisp_arg_t args = {
    .exp = argp,
    .arg = NULL,
  };

  struct lisp_obj  argp0 = {0};
  struct lisp_obj*   arg = NULL;

  // argp0: points to an SEXP which contains all other SEXPs part of the lambda
  // we pass this to the stack after setting up the environment
  FOR_ARG(arg, args) {
    argp0 = *arg;
    break;
  }

  done_for((ret_t.succ = (ret == 0), ret_t));
}

struct lisp_arg_t lisp_argp_next(struct lisp_arg_t argp) {
  struct lisp_arg_t ret = argp;

  if (!argp.exp) {
    ret.arg = NULL;
    ret.exp = NULL;
    return ret;
  }

  ret.arg = argp.exp->left;

  // already yielded the left
  if (ret.arg == argp.arg) {
    struct lisp_obj* obj = argp.exp->right;

    // in the SEXP tree, nil object is still an object. NULL objects aren't
    // part of the tree at all (see `sexp.c')
    if (!obj) {
      ret.arg = NULL;
      ret.exp = NULL;
    }

    else if (obj->typ == __LISP_OBJ_EXP && obj->_.exp->lexp) {
      ret.arg = obj->_.exp->left;
      ret.exp = obj->_.exp;
    }

    else {
      ret.arg = obj;
      ret.exp = NULL;
    }
  }

  return ret;
}

// TODO:
void* lisp_obj_dat(struct lisp_obj* obj) {
  return NULL;
}

void lisp_free_obj(struct lisp_obj* obj) {
  if (!obj) {
    return;
  }

  bool cmem = obj->m_typ & __LISP_OBJ_C;

  // it's the job of the function to assure the GCr that the memory is theirs
  // THE GARBAGE COLLECTOR HAS NO MERCY!
  if (obj->m_typ & __LISP_OBJ_NOGC) {
    obj->m_typ &= ~__LISP_OBJ_NOGC;
    return;
  }

  obj->refs--;

  //// REF FREE

  if (!(obj->m_typ & __LISP_OBJ_COW)) {
    if (!cmem) {
      // we're freeing an origin
      if (!obj->refs) {
        lisp_free_obj_mem(obj);
        mm_free(obj);
      }

      // the object still has references: tag free
      else {
        lisp_free_obj_mem(obj);
        obj->typ = __LISP_OBJ_NIL;
      }
    }

    return;
  }

  //// COW FREE

  // we're freeing a COW recipient
  if (!(obj->m_typ & __LISP_OBJ_ORG)) {
    struct lisp_obj* from = obj->cow.from;

    // free the interface
    if (!obj->refs) {
      if (!(from->cow.org.recip--) && !cmem) {
        // no references on orgin: do as we would below
        if (!from->refs) {
          from->typ = from->cow.org.typ;

          lisp_free_obj_mem(from);
          mm_free(from);
        }

        // the reference has been tagged free: actually free it
        else if (from->typ == __LISP_OBJ_NIL) {
          from->typ = from->cow.org.typ;

          lisp_free_obj_mem(from);

          from->typ = __LISP_OBJ_NIL;
        }
      }

      mm_free(obj);
    }

    // there are references to this COW: keep the memory and the interface, but
    // set the type to NIL so that write operations fail. both will be freed
    // once the last reference is freed
    else {
      obj->typ = __LISP_OBJ_NIL;
    }

    return;
  }

  // we're freeing a COW origin: tag free
  if (obj->cow.org.recip) {
    obj->cow.org.typ = obj->typ;
    obj->typ = __LISP_OBJ_NIL;

    return;
  }
}
