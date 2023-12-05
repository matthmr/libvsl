#include "lisp.h"

struct lisp_arg_t lisp_argp_next(struct lisp_sexp* expr) {
  return expr? (struct lisp_arg_t) {
    .arg  = &expr->left,
    .expr = RIGHT_NIL(*expr)? NULL: expr->right._.exp,
  }: (struct lisp_arg_t) {
    .arg  = NULL,
    .expr = NULL,
  };
}
