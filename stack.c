#include "debug.h"

#include "lex.h"
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

static inline struct lisp_ret
lisp_emptyisnil(struct lisp_sexp* _argp, uint argv, struct lisp_symtab* envp) {
  return (struct lisp_ret) {
    .master = (struct lisp_obj) {
      .typ = __LISP_OBJ_NIL,
    },
    .slave = __LISP_OK,
  };
}

////////////////////////////////////////////////////////////////////////////////

struct lisp_ret
lisp_eval(struct lisp_sexp* argp, uint argv, struct lisp_symtab* envp) {
  register int       ret = 0;
  struct lisp_ret  ret_t = {0};
  struct lisp_arg_t args = {0};
  struct lisp_obj  argp0 = {0};

  // argp0: points to an SEXP which contains all other SEXPs part of the lambda
  // we pass this to the stack after setting up the environment
  FOR_ARG(arg, args) {
    argp0 = *arg;
    break;
  }

  done_for((ret_t.slave = ret, ret_t));
}

////////////////////////////////////////////////////////////////////////////////

/** Handle events yielded by the SEXP tree transversal */
static inline enum lisp_stack_stat
lisp_stack_handle_ev(struct lisp_yield st_stat) {
  switch (st_stat.stat) {
  case __YIELD_LEFT_OBJ:
  case __YIELD_RIGHT_OBJ:
    return __STACK_ELEM;
  case __YIELD_LEFT_EXPR:
  case __YIELD_RIGHT_EXPR:
    return __STACK_NEW;
  case __YIELD_END_EXPR:
    return __STACK_DONE;
  default:
    return __STACK_OK;
  }
}

// NOTE: We apply a 'copy-on-write' policy with the argument data; we only
// allocate memory for the argument if we need to change its memory, otherwise
// it's kept the same.

/** Handle the pop event. The expression @exp is the one we popped *from*, and
    @ret is *what* we popped */
static inline void
lisp_stack_pop(struct lisp_stack* frame, struct lisp_obj ret,
               struct lisp_sexp* exp) {
  struct lisp_sexp* exp_into = exp->root;
  bool from_left = (exp_into->left._.exp == exp);

  frame->argv++;

  if (from_left) {
    exp_into->left  = ret;
  }
  else {
    exp_into->right = ret;
  }
}

/** Resolute @obj as an argument of the stack frame */
static inline void
lisp_stack_res(struct lisp_stack* frame, struct lisp_obj obj) {
  register struct lisp_sexp* argp = frame->argp;

  if (IS_NIL(argp->right.typ)) {
    argp->left  = obj;
  }
  else {
    argp->right = obj;
  }

  return;
}

#if 0
/** Handle the symbol/literal event. @st_stat has as the .exp field the
    parent expression for the argument */
static inline void
lisp_stack_arg(struct lisp_stack* frame, bool lit, struct lisp_trans st_stat) {
  struct lisp_sexp* exp = st_stat.exp;

  frame->argv++;

  if (lit) {
    return;
  }

  // DEBUG
  return;

  if (LEFT_HASH(*exp)) {
    exp->left._.sym  = *lisp_symtab_get(exp->left._.hash);
  }
  else {
    exp->right._.sym = *lisp_symtab_get(exp->right._.hash);
  }
}
#endif

/** Frees the stack memory not tagged as `keep' */
// TODO: this
static void lisp_stack_free(struct lisp_stack* frame) {
  // DEBUG
  // register int        ret = __STACK_DONE;
  // struct lisp_trans s_ret = {0};

  // s_ret.stat = __TRANS_OK;
  // s_ret.exp  = frame->argp;

  // while (s_ret = lisp_sexp_yield_for_clear(s_ret),
  //        ret = lisp_stack_handle_ev(s_ret),
  //        ret != __STACK_DONE) {
  //   if ()
  // }

  lisp_sexp_clear(frame->argp);

  return;
}

////////////////////////////////////////////////////////////////////////////////

struct lisp_ret lisp_stack_frame_lex(struct lisp_symtab* f_envp) {
  register int      ret = __STACK_DONE;
  struct lisp_ret f_ret = {0};

  struct lisp_stack f_stack = {0};

  struct lisp_sym* f_sym = NULL;
  struct lisp_symtab f_symtab = {0}; // for literal symbols
  struct lisp_obj f_obj = {0};

  struct lisp_sexp argp0 = {
    .root      = NULL,
    .typ       = __LISP_OBJ_SEXP,
    .left.typ  = __LISP_OBJ_NIL,
    .right.typ = __LISP_OBJ_NIL,
  };

  struct lisp_sexp* f_top = &argp0;

  bool   lit = false;
  bool scope = f_envp && true;

  lex.ev &= ~__LEX_EV_PAREN_IN;

  // we implement a copy-on-write policy for the symbol table when creating new
  // scopes
  if (!scope) {
    f_envp = mm_alloc(sizeof(*f_envp));
    assert(f_envp, OR_ERR());

    f_envp->root = NULL;
    f_envp->tab = (struct lisp_symtab_node) {
      .hash_idx = 0,
      .self     = {0},
      .left     = NULL,
      .right    = NULL,
    };
  }

  f_stack.envp = f_envp;

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
  // TODO: anonymous return should be cleared by us, given the chance
  else if (LEX_PAREN_IN(lex.ev)) {
    lex.ev &= ~__LEX_EV_PAREN_IN;
    f_ret   = lisp_stack_frame_lex(f_stack.envp);

    assert(f_ret.slave == __STACK_DONE, OR_ERR());
    // assert(IS_FUN(f_ret.master.typ), err(EISNOTFUN));

    // f_hash = f_ret.master._.sym;
  }
  else if (LEX_PAREN_OUT(lex.ev)) {
    lex.ev &= ~__LEX_EV_PAREN_OUT;

    f_ret = lisp_emptyisnil(f_stack.argp, f_stack.argv, f_stack.envp);

    goto free;
  }

  f_sym = lisp_symtab_get(lex.symbuf, f_stack.envp);

  assert(f_sym, OR_ERR());
  assert(IS_FUN(f_sym->_.dat.obj.typ), err(EISNOTFUN));

  INC_RESET(lex.symbuf);

  f_obj.typ   = __LISP_OBJ_SYM;
  f_obj._.sym = memcpy(f_obj._.sym, f_sym, sizeof(*f_sym));

  f_stack.argp = lisp_sexp_obj(f_obj, f_stack.argp);

  lit = IS_LIT(f_sym->_.dat.obj.typ) && true;

  f_stack.argv++;

  //// FROM ARGP0

  for (;;) {
    do {
      ret = lisp_lex_yield();
    } while (ret == __LEX_NO_INPUT);

    assert(ret == __LEX_OK, OR_ERR());

    DB_FMT("[ stack ] yielding for: argp[%d]", f_stack.argv);

    if (LEX_SYMBOL_OUT(lex.ev)) {
      lex.ev &= ~__LEX_EV_SYMBOL_OUT;
      f_stack.argv++;

      f_sym = lit?
        lisp_symtab_sets(lex.symbuf,
                         (f_obj.typ = __LISP_OBJ_NIL, f_obj), &f_symtab):
        lisp_symtab_get(lex.symbuf, f_stack.envp);

      assert(f_sym, OR_ERR());

      f_stack.argp = lisp_sexp_obj(
        (lit? (struct lisp_obj) {
            ._.sym = f_sym,
            .typ   = __LISP_OBJ_SYM,
          }: f_sym->_.dat.obj), f_stack.argp);

      assert(f_stack.argp, OR_ERR());

      INC_RESET(lex.symbuf);
    }

    else if (LEX_PAREN_IN(lex.ev)) {
      lex.ev &= ~__LEX_EV_PAREN_IN;
      f_stack.argv++;

      if (lit) {
        f_obj.typ   = __LISP_OBJ_SEXP;
        f_obj._.exp = mm_alloc(sizeof(*f_obj._.exp));
        assert(f_obj._.exp, OR_ERR());

        *f_obj._.exp = (struct lisp_sexp) {
          .root      = f_stack.argp,
          .typ       = __LISP_OBJ_SEXP,
          .left.typ  = __LISP_OBJ_NIL,
          .right.typ = __LISP_OBJ_NIL,
        };

        f_stack.argp = lisp_sexp_obj(f_obj, f_stack.argp);
        assert(f_stack.argp, OR_ERR());

        f_stack.argp = RIGHT_NIL(*f_stack.argp)?
          f_stack.argp->left._.exp: f_stack.argp->right._.exp;
      }
      else {
        f_ret = lisp_stack_frame_lex(f_stack.envp);

        assert(f_ret.slave == __STACK_DONE, OR_ERR());
        lisp_stack_res(&f_stack, f_ret.master);
      }
    }

    else if (LEX_PAREN_OUT(lex.ev)) {
      lex.ev      &= ~__LEX_EV_PAREN_OUT;
      f_stack.argp = lisp_sexp_end(f_stack.argp);

      if (f_stack.argp == f_top) {
        goto apply;
      }
    }
  }

apply:
  f_obj = f_stack.argp[0].left;

  // user defined functions are wrapped around `lisp_eval'. CLISP functions
  // are executed directly
  switch (f_obj.typ) {
  case __LISP_OBJ_CFUN:
  case __LISP_OBJ_CFUN_LIT:
    f_ret = CFUN(f_obj._.sym->_.dat.obj)
      (f_stack.argp, f_stack.argv, f_stack.envp);
    break;

  case __LISP_OBJ_LAMBDA:
  case __LISP_OBJ_MACRO:
    f_ret = lisp_eval(f_stack.argp, f_stack.argv, f_stack.envp);
    break;

  // this never happens, but the compiler doesn't know that
  default:
    break;
  }

  assert((int) f_ret.slave == __LISP_OK, OR_ERR());

free:
  lisp_stack_free(&f_stack);
  defer_as(__STACK_DONE);

  done_for_with(f_ret, f_ret.slave = ret);
}
