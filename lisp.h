/** Main interface for LISP objects */

#ifndef LOCK_LISP
#  define LOCK_LISP

#  include "utils.h"

#  define IS_SEXP(x) ((x) == __LISP_OBJ_SEXP)
#  define IS_LEXP(x) ((x) == __LISP_OBJ_LEXP)
#  define IS_EXPR(x) (IS_SEXP(x) || IS_LEXP(x))
#  define IS_SYM(x) ((x) == __LISP_OBJ_SYM)
#  define IS_LIT(x) ((x) == __LISP_OBJ_CFUN_LIT | (x) == __LISP_OBJ_MACRO)
#  define IS_FUN(x) ((x) == __LISP_OBJ_CFUN | (x) == __LISP_OBJ_CFUN_LIT | \
                     (x) == __LISP_OBJ_LAMBDA | (x) == __LISP_OBJ_MACRO)
#  define IS_NIL(x)  ((x) == __LISP_OBJ_NIL)

#  define RIGHT_EXPR(x) (IS_EXPR((x).right.typ))
#  define LEFT_EXPR(x)  (IS_EXPR((x).left.typ))
#  define RIGHT_NIL(x)  (IS_NIL((x).right.typ))
#  define LEFT_NIL(x)   (IS_NIL((x).left.typ))

// #  define RIGHT(x) (IS_HASH((x).right.typ) || IS_EXPR((x).right.typ))
// #  define LEFT(x)  (IS_HASH((x).left.typ) || IS_EXPR((x).left.typ))

struct lisp_sexp;
struct lisp_sym;

/** 'Foreign memory' interface for `lisp_obj'. Neither @typ nor @dat are set by
     default */
struct lisp_foreign {
  int   typ;
  void* dat;
};

//// OBJ

enum lisp_obj_mt {
  __LISP_ARG_OK       = 0,
  __LISP_ARG_KEEP     = BIT(0),
  __LISP_ARG_TRY_KEEP = BIT(1),
};

/** Discriminant for `lisp_obj' */
enum lisp_obj_t {
  __LISP_OBJ_NIL = 0,

  // C-like functions
  __LISP_OBJ_CFUN,
  __LISP_OBJ_CFUN_LIT,

  // lambda-like functions
  __LISP_OBJ_LAMBDA,
  __LISP_OBJ_MACRO,

  __LISP_OBJ_SYM,

  __LISP_OBJ_SEXP,
  __LISP_OBJ_LEXP,

  __LISP_OBJ_FOREIGN,
};

/** Main object interface. For most LISP top-level objects. See also
    `lisp_foreign' */
union lisp_obj_m {
  /* a CLISP function. to prevent infinite recursion, this type is left to be
     casted by the the caller */
  void* fun;

  /* a symbol */
  struct lisp_sym* sym;

  /* an expression. user-defined functions are of this type */
  struct lisp_sexp* exp;

  /* foreign memory */
  struct lisp_foreign gen;
};

/** Interface for objects stored as ends of an SEXP tree */
struct lisp_obj {
  /* the proper memory of the object */
  union lisp_obj_m _;

  /* the object type */
  enum lisp_obj_t typ;

  /* the argument type, as seen by the stack (memory-type) */
  enum lisp_obj_mt m_typ;
};

//// SEXP

/** Main SEXP type, used for both the SEXP tree and the AST */
struct lisp_sexp {
  /* root node */
  struct lisp_sexp* root;

  /* children node */
  struct lisp_obj left, right;

  /* self node type */
  enum lisp_obj_t typ;
};

//// SYM

/** Symbol 'node' type. Binary tree used for the symbol table and same-hash
    chain. The left node is always less, or less than half */
struct lisp_sym_node {
  struct lisp_sym* left, * right;
};

/** Symbol data wrapper. With string representation and type. The actual data
    is to be casted */
struct lisp_sym_dat {
  /* symbol string representation */
  const char* str;

  /* data for the symbol */
  // struct lisp_obj* obj;
  struct lisp_obj obj;
};

union lisp_sym_m {
  struct lisp_sym_node node;
  struct lisp_sym_dat   dat;
};

/** Symbol discretion. Assuming we've gotten the symbol from the table, this
    determines if the node is to be taken as is, or if it has collisions */
enum lisp_sym_dis {
  __LISP_SYM_NODE,
  __LISP_SYM_DAT,
};

/** Main symbol structure */
struct lisp_sym {
  /* symbol discretion */
  enum lisp_sym_dis dis;

  /* symbol proper */
  union lisp_sym_m _;
};

//// FUN

enum lisp_ret_t {
  __LISP_ERR = -1,
  __LISP_OK  =  0,
};

/** Type wrapper for `lisp_argp_next' */
struct lisp_arg_t {
  struct lisp_obj*   arg;
  struct lisp_sexp* expr;
};

/** Packaged return type of every LISP function */
struct lisp_ret {
  /* object returned */
  struct lisp_obj master;

  /* return status */
  enum lisp_ret_t slave;
};

struct lisp_symtab;

/** LISP function type */
typedef struct lisp_ret
(*lisp_fun) (struct lisp_sexp* argp, const uint argv, struct lisp_symtab* envp);

#  define CFUN(x) ((lisp_fun) ((x)._.fun))
#  define FOR_ARG(x,y) \
  for (struct lisp_obj* x = ((y = lisp_argp_next(y.expr)), NULL); \
       (x = y.arg); \
       y = lisp_argp_next(y.expr)) \

/** Returns the next argument object. By the way the tree is constructed, the
    end objects should always be on the left, while the rest of the tree
    continues as a LEXP on the right. If `RIGHT_NIL' yields, then the `expr'
    field can return as NULL, signifying the end of the expression */
struct lisp_arg_t lisp_argp_next(struct lisp_sexp* expr);

#endif
