/** Symbol table functionality */

#ifndef LOCK_SYMTAB
#  define LOCK_SYMTAB

#  include "utils.h"

#  include "lisp.h"

/* maximum length of LISP symbols */
#  define LISP_SYM_MAX_LEN (80)

/* ascii factor & ascii normalizer: normalize character encoded in ascii to fit
   the hasher */
#  define ASCII_FAC (32)
#  define ASCII_NORM(x) ((x) - ASCII_FAC)

#  define PRIME (97)
#  define HASH_SIZE (107) // 101

////////////////////////////////////////////////////////////////////////////////

typedef uint hash_t;

/** Symbol table node */
struct lisp_symtab_node {
  /* hash index */
  hash_t hash_idx;

  /* ourselves */
  //struct lisp_sym* self;
  struct lisp_sym self;

  /* children nodes */
  struct lisp_symtab_node* left, * right;
};

/** Symbol table */
struct lisp_symtab {
  /* parent scope */
  struct lisp_symtab* root;

  /* proper table */
  struct lisp_symtab_node tab;
};

////////////////////////////////////////////////////////////////////////////////

/** Hash string iterator @strip */
hash_t hash(string_ip strip);

/** Sets a symbol from object @sym_obj and string @sym_str as a symbol on a
    symbol table @sym_tab, returning its memory */
struct lisp_sym*
lisp_symtab_set(string_ip sym_str, struct lisp_obj sym_obj,
                struct lisp_symtab* sym_tab);

/** Sets a symbol from object @sym_obj and string @sym_str as a symbol on a
    symbol table @sym_tab returning its memory and assuring that, if one
    already exists, it won't overwrite memory, whereas `lisp_symtab_set' will
    always overwrite */
struct lisp_sym*
lisp_symtab_sets(string_ip sym_str, struct lisp_obj sym_obj,
                 struct lisp_symtab* sym_tab);

/** Gets a symbol from data @sym_data and hash @sym_hash as a symbol on a symbol
    table @sym_tab, returning its memory. If the table is NULL, returns the
    memory of the symbol as a primitive type */
struct lisp_sym*
lisp_symtab_get(string_ip sym_str, struct lisp_symtab* sym_tab);

#endif
