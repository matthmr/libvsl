#include "debug.h"

#include "lex.h"
#include "lisp.h"
#include "sexp.h"
#include "stack.h"
#include "mm.h"

#include <string.h>

//// ERRORS

ECODE(EISNOTFUN, EARGTOOBIG, EARGTOOSMALL);

EMSG {
  [EISNOTFUN]    = ERR_STRING("libvsl: stack", "not a function"),
  [EARGTOOBIG]   = ERR_STRING("libvsl: stack",
                              "too many arguments for function"),
  [EARGTOOSMALL] = ERR_STRING("libvsl: stack",
                              "not enough arguments for function"),
};

////////////////////////////////////////////////////////////////////////////////

/** Unbind symbols from the symbol table, unless GC guarded */
static void lisp_stack_unbind(struct lisp_symtab* tab) {
  // TODO
  return;
}

/** Frees the stack memory whose GC guard is not set */
static void lisp_stack_free(struct lisp_sexp* argp) {
  struct lisp_yield yield = {
    .exp  = argp,
    .stat = __YIELD_OK,
  };

  struct lisp_obj* obj = NULL;

  // it's safe to free SEXP nodes even though we're technically accessing them
  // after free, because we're not allocating anything below this function
  while (yield = lisp_sexp_yield(yield, __YIELD_IGNORE_LEXP),
         yield.stat != __YIELD_DONE) {
    switch (yield.stat) {
    case __YIELD_LEFT_EXPR:
    case __YIELD_LEFT_OBJ:
      obj = yield.exp->left;

      if (yield.stat == __YIELD_LEFT_EXPR) {
        yield.stat = __YIELD_END_EXPR;
      }

      break;

    case __YIELD_RIGHT_EXPR:
    case __YIELD_RIGHT_OBJ:
      obj = yield.exp->right;

      if (yield.stat == __YIELD_RIGHT_EXPR) {
        yield.stat = __YIELD_END_EXPR;
      }

      break;

    case __YIELD_END_EXPR:
      if (yield.exp != argp) {
        mm_free(yield.exp);
      }
      break;

    default:
      break;
    }

    lisp_free_obj(obj);
    obj = NULL;
  }
}

////////////////////////////////////////////////////////////////////////////////

#if 0
struct lisp_ret
lisp_stack_frame_sexp(struct lisp_sexp* exp, struct lisp_symtab* envp) {
  register int ret = 0;
  struct lisp_ret f_ret = {0};

  struct lisp_yield yield = {
    .exp  = argp,
    .stat = __YIELD_OK,
  };

  struct lisp_stack f_stack = {0};

  struct lisp_sym* f_sym = NULL;
  struct lisp_obj f_obj = {0}, * p_obj = NULL;

  struct lisp_symtab f_symtab = {0};
  struct lisp_sexp      argp0 = {0};

  struct lisp_sexp* f_top = &argp0;

  lex.ev &= ~__LEX_EV_PAREN_IN;

  // we don't need to allocate a new scope right away
  f_stack.envp = f_envp;
  f_stack.inherit = true;

  //// FRAME

  DB_MSG("[ stack ] new frame");

  f_stack.argp = &argp0;
  f_top = f_stack.argp;

  DB_MSG("[ stack ] yielding for: argp[0]");


  //// YIELD ARGP0

  yield = lisp_sexp_yield(yield, 0);

  switch (yield.stat) {
    case __YIELD_LEFT_EXPR:
      break;

    case __YIELD_LEFT_OBJ:
      break;

    case __YIELD_END_EXPR:
      defer();
      break;

    default:
      break;
    }

  done_for(f_ret);
}
#endif

/* TODO: handle NULL obj as `nil' */ //
struct lisp_ret lisp_stack_frame_lex(struct lisp_symtab* f_envp) {
  register int ret = 0;
  struct lisp_ret f_ret = {0};

  struct lisp_stack f_stack = {0};

  struct lisp_sym* f_sym = NULL;
  struct lisp_obj f_obj = {0}, * p_obj = NULL;

  // for literal symbols. in the stack frame, literal symbols are treated as
  // symbols in symtab whose objects are themselves. if they pop, they become
  // a standard object. orphan symbols will always be freed
  struct lisp_symtab* f_symtab = NULL;
  struct lisp_sexp argp0 = {0};

  struct lisp_sexp* f_top = &argp0;

  bool ref = false;

  lex.ev &= ~__LEX_EV_PAREN_IN;

  // we don't need to allocate a new scope right away
  f_stack.envp = f_envp;
  f_stack.inherit = true;

  //// FRAME

  DB_MSG("[ stack ] new frame");

  f_stack.argp = &argp0;
  f_top = f_stack.argp;

  DB_MSG("[ stack ] yielding for: argp[0]");

  //// YIELD ARGP0

  do {
    ret = lisp_lex_yield();
  } while (ret == __LEX_NO_INPUT);

  assert(ret == __LEX_OK, OR_ERR());

  if (LEX_SYMBOL_OUT(lex.ev)) {
    lex.ev &= ~__LEX_EV_SYMBOL_OUT;
  }
  else if (LEX_PAREN_IN(lex.ev)) {
    lex.ev &= ~__LEX_EV_PAREN_IN;
    f_ret   = lisp_stack_frame_lex(f_stack.envp);

    assert(f_ret.succ, OR_ERR());
    assert(f_ret.obj && IS_FUN(f_ret.obj->typ), err(EISNOTFUN));

    p_obj = f_ret.obj;
    ref   = f_ret.ref;

    goto save_argp0;
  }
  else if (LEX_PAREN_OUT(lex.ev)) {
    lex.ev &= ~__LEX_EV_PAREN_OUT;

    f_ret.succ = true;

    f_ret.ref = true; // false;
    f_ret.obj = NULL; // pure `nil' is a NULL object pointer

    defer();
  }

  f_sym = lisp_symtab_get(lex.symbuf, f_stack.envp, 0);

  assert(f_sym, OR_ERR());
  assert(f_sym->obj && IS_FUN(f_sym->obj->typ), err(EISNOTFUN));

  p_obj = f_sym->obj;
  ref   = true; // false;

  INC_RESET(lex.symbuf);

save_argp0:
  f_stack.argp = lisp_sexp_obj(p_obj, ref, f_stack.argp);
  assert(f_stack.argp, OR_ERR());

  f_stack.lit = IS_LIT(f_sym->obj->typ);

  f_stack.argv++;

  //// FROM ARGP0

  for (;;) {
    DB_FMT("[ stack ] yielding for: argp[%d]", f_stack.argv);

    do {
      ret = lisp_lex_yield();
    } while (ret == __LEX_NO_INPUT);

    assert(ret == __LEX_OK, OR_ERR());

    if (LEX_SYMBOL_OUT(lex.ev)) {
      lex.ev &= ~__LEX_EV_SYMBOL_OUT;
      f_stack.argv++;

      if (f_stack.lit) {
        if (!f_symtab) {
          f_symtab = mm_alloc(sizeof(*f_symtab));
          assert(f_symtab, OR_ERR());
        }

        f_sym = lisp_symtab_set(lex.symbuf, NULL, f_symtab, __LISP_SYMTAB_SAFE);
      }
      else {
        f_sym = lisp_symtab_get(lex.symbuf, f_stack.envp, 0);
      }

      assert(f_sym, OR_ERR());

      // we create an object wrapper for symbol literals
      if (f_stack.lit) {
        if (!f_sym->obj) {
          p_obj = mm_alloc(sizeof(*p_obj));
          assert(p_obj, OR_ERR());

          f_obj = (struct lisp_obj) {
            .typ   = __LISP_OBJ_SYM,
            .m_typ = __LISP_OBJ_ORG,
            ._.sym = *f_sym,
            .refs  = 0,

            .cow.org.recip = 0,
          };

          // the object of the symbol literal is an object owning the symbol
          p_obj = memcpy(p_obj, &f_obj, sizeof(f_obj));

          f_sym->obj = p_obj;
        }

        ref = true;
      }

      // by default, symbols are passed by value. internally, this does not
      // allocate new memory unless the bound symbol is mutated, then it
      // copies the value before proceeding. pass by reference is possible with
      // anonymous return (see below)
      else {
        ref = false;
      }

      f_stack.argp = lisp_sexp_obj(f_sym->obj, ref, f_stack.argp);
      assert(f_stack.argp, OR_ERR());

      INC_RESET(lex.symbuf);
    }

    else if (LEX_PAREN_IN(lex.ev)) {
      lex.ev &= ~__LEX_EV_PAREN_IN;
      f_stack.argv++;

      if (f_stack.lit) {
        p_obj = mm_alloc(sizeof(*p_obj));
        assert(p_obj, OR_ERR());

        f_obj = (struct lisp_obj) {
          .typ   = __LISP_OBJ_EXP,
          .m_typ = __LISP_OBJ_ORG,
          ._.exp = mm_alloc(sizeof(struct lisp_sexp)),
          .refs  = 0,

          .cow.org.recip = 0,
        };
        assert(f_obj._.exp, OR_ERR());

        p_obj = memcpy(p_obj, &f_obj, sizeof(f_obj));
        ref   = true;
      }
      else {
        f_ret = lisp_stack_frame_lex(f_stack.envp);
        assert(f_ret.succ, OR_ERR());

        p_obj = f_ret.obj;
        ref   = f_ret.ref;
      }

      f_stack.argp = lisp_sexp_obj(p_obj, ref, f_stack.argp);
      assert(f_stack.argp, OR_ERR());
    }

    else if (LEX_PAREN_OUT(lex.ev)) {
      lex.ev &= ~__LEX_EV_PAREN_OUT;
      f_stack.argp = lisp_sexp_end(f_stack.argp);

      if (f_stack.argp == f_top) {
        goto apply;
      }
    }
  }

apply:
  DB_MSG("[ stack ] evaluating...");

  p_obj = argp0.left;

  // user defined functions are wrapped around `lisp_eval'. CLISP functions
  // are executed directly
  switch (p_obj->typ) {
  case __LISP_OBJ_CFUN:
  case __LISP_OBJ_CFUN_LIT:
    f_ret = CFUN_OF(*p_obj) (f_stack.argp, f_stack.argv, f_stack.envp);
    break;

  case __LISP_OBJ_LAMBDA:
  case __LISP_OBJ_MACRO:
    f_ret = lisp_eval(f_stack.argp, f_stack.argv, f_stack.envp);
    break;

  // apease the almighty compiler
  default:
    break;
  }

  // wtf
  ret = (int)f_ret.succ;
  assert(ret == 1, OR_ERR());
  ret = 0;

  lisp_stack_free(f_stack.argp);
  lisp_stack_unbind(f_symtab);

  done_for_with(f_ret, f_ret.succ = (ret == 0));
}
