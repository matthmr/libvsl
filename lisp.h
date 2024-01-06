/** Main interface for LISP objects */

#ifndef LOCK_LISP
#  define LOCK_LISP

#  include "utils.h"

#  define IS_EXP(x) ((x) == __LISP_OBJ_EXP)
#  define IS_SYM(x) ((x) == __LISP_OBJ_SYM)
#  define IS_LIT(x) ((x) == __LISP_OBJ_CFUN_LIT || (x) == __LISP_OBJ_MACRO)
#  define IS_FUN(x) ((x) == __LISP_OBJ_CFUN || (x) == __LISP_OBJ_CFUN_LIT | \
                     (x) == __LISP_OBJ_LAMBDA || (x) == __LISP_OBJ_MACRO)
#  define IS_NIL(x) ((x) == __LISP_OBJ_NIL)

struct lisp_lambda;
struct lisp_symtab;
struct lisp_sexp;
struct lisp_sym;
struct lisp_obj;

/** 'Foreign memory' interface for `lisp_obj'. Neither @typ nor @dat are set by
     default */
struct lisp_foreign {
  int   typ;
  void* dat;
};

//// LAMBDA

/** Main lambda interface */
struct lisp_lambda {
  /* anonymous scope. in the case the lambda was defined in a scope that is
     orphaned. basis for currying */
  struct lisp_symtab* anon;

  /* expression body. The left child is special, as it is a list of symbols
     for which we bind the right child of our calling expression */
  struct lisp_sexp* body;

  /* are we a macro intead? if so, we bind the literal value of the right child
     to the left child */
  bool macro;
};

//// SYM

/** Main symbol structure. NULL data means this is a symbol literal */
struct lisp_sym {
  /* symbol string representation */
  const char* str;

  /* data of the symbol */
  struct lisp_obj* obj;
};

//// SEXP

/** Main SEXP type, used for both the SEXP tree and the AST */
struct lisp_sexp {
  /* root node */
  struct lisp_sexp* root;

  /* children node */
  struct lisp_obj* left, * right;

  /* are we an LEXP instead? */
  bool lexp;
};

//// OBJ

/** Discriminant for `lisp_obj' */
enum lisp_obj_t {
  // this exists for when objects with references are cleared. when that
  // happens, we free the memory and set this as the type. only when the last
  // reference is freed do we free the object interface.
  // this is also equal to owning a NULL object
  __LISP_OBJ_NIL = 0,

  // C-like functions
  __LISP_OBJ_CFUN,
  __LISP_OBJ_CFUN_LIT,

  // lambda-like functions
  __LISP_OBJ_LAMBDA,
  __LISP_OBJ_MACRO,

  // literal (atomic) symbol
  __LISP_OBJ_SYM,

  // symbol alias
  __LISP_OBJ_ALIAS,

  // standard reference
  __LISP_OBJ_REF,

  // LIBVSL SEXP-like type
  __LISP_OBJ_EXP,

  __LISP_OBJ_FOREIGN,
};

/** Memory type of the object. What to do in memory changing operations */
enum lisp_obj_mt {
  __LISP_OBJ_STD,

  // COW object: `lisp_obj_dat' will always copy
  __LISP_OBJ_COW  = BIT(0),

  // GC guard: always cleared by any `free'-like function
  __LISP_OBJ_NOGC = BIT(1),

  // origin object: used on reference bases
  __LISP_OBJ_ORG = BIT(2),

  // C obj: no need to free
  __LISP_OBJ_C = BIT(3),
};

/** Main object interface. For most LISP top-level objects. See also
    `lisp_foreign' */
union lisp_obj_m {
  /* a CLISP function. to prevent infinite recursion, this type is left to be
     casted by the the caller */
  void* cfun;

  /* a symbol (atomic). Alias-like operations copy the pointer of the object of
     the symbol being copied, instead of creating an object pointing to the
     symbol */
  struct lisp_sym sym;

  /* symbol alias */
  struct lisp_sym* alias;

  /* standard reference */
  struct lisp_obj* ref;

  /* an expression */
  struct lisp_sexp* exp;

  /* a lambda. macro or otherwise */
  struct lisp_lambda lambda;

  /* foreign memory */
  struct lisp_foreign gen;
};

/** Object COW interface */
union lisp_obj_cow {
  /* object COW origin */
  struct {
    uint recip;
    enum lisp_obj_t typ;
  } org;

  /* object COW recipient */
  struct lisp_obj* from;
};

/** Interface for LIBVSL objects */
struct lisp_obj {
  /* proper memory of the object */
  union lisp_obj_m _;

  /* object type */
  enum lisp_obj_t typ;

  /* memory type */
  enum lisp_obj_mt m_typ;

  /* number of references to this object */
  uint refs;

  /* cow interface */
  union lisp_obj_cow cow;
};

//// FUN

/// RET

/** Packaged return type of every LISP function */
struct lisp_ret {
  /* object returned */
  struct lisp_obj* obj;

  /* is it an object reference? */
  bool ref;

  /* LIBVSL return success. should only be used by LIBVSL */
  bool succ;
};

/// ARGS

/** Type wrapper for `lisp_argp_next' */
struct lisp_arg_t {
  struct lisp_obj*  arg;
  struct lisp_sexp* exp;
};

/** LISP function type */
typedef struct lisp_ret
(*lisp_fun) (struct lisp_sexp* argp, const uint argv, struct lisp_symtab* envp);

#  define CFUN_OF(x) ((lisp_fun) ((x)._.cfun))
#  define FOR_ARG(x,y) \
  for ((x) = ((y) = lisp_argp_next(y), (y).arg); \
       ((x) = (y).arg); \
       ((y) = lisp_argp_next(y))) \

/** Returns the next argument object. By the way the tree is constructed, the
    end objects should always be on the left, while the rest of the tree
    continues as a LEXP on the right. If `RIGHT_NIL' yields, then the `expr'
    field can return as NULL, signifying the end of the expression */
struct lisp_arg_t lisp_argp_next(struct lisp_arg_t argp);

/** Tries to free a LISP object */
void lisp_free_obj(struct lisp_obj* obj);

/** Returns a pointer to the data of an object. Has to be used for COW */
void* lisp_obj_dat(struct lisp_obj* obj);

/** Evals an SEXP tree as a function */
struct lisp_ret lisp_eval
(struct lisp_sexp* argp, uint argv, struct lisp_symtab* envp);

#endif
