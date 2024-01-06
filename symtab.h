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
#  define HASH_SIZE (101) // 107

#  define HASH_COLSOFF (2)
#  define HASH_CHKSIZE (7)

#  define UPPERBIT(x) ((x) & (1 << sizeof(int)*8))

////////////////////////////////////////////////////////////////////////////////

typedef uint hash_t;

/** Symbol table node. Hash table element */
struct lisp_symtab_node {
  /* hash index */
  hash_t idx;

  /* current symbol */
  struct lisp_sym sym;

  /* spread offset for this index. upper bit set means another node */
  uint soff;
};

/** Symbol table chunk */
struct lisp_symtab_chk {
  /* next chunk linked list. should have (HASH_SIZE / HASH_CHKSIZE) elements */
  struct lisp_symtab_chk* next;

  /* table chunk. each chunk should have increasing order of elements */
  struct lisp_symtab_node _[HASH_CHKSIZE];

  /* chunk size */
  uint chk_size;
};

/** Symbol table interface for hash collision and chunking */
struct lisp_symtab_if {
  /* next table. for hash collision */
  struct lisp_symtab_if* next;

  /* table chunk inteface */
  struct lisp_symtab_chk chk;

  /* bitfield of elements on this table */
  char elems[HASH_SIZE / (sizeof(char)*8) + 1];
};

/** Main symbol table interface */
struct lisp_symtab {
  /* parent scope */
  struct lisp_symtab* root;

  /* symbol table proper */
  struct lisp_symtab_if _;
};

////////////////////////////////////////////////////////////////////////////////

/** Symbol table type. Changes how get/set acts given a table */
enum lisp_symtab_t {
  // overwrites on set
  __LISP_SYMTAB_STD = 0,

  // doesn't cross the scope boundary
  __LISP_SYMTAB_SCOPE = BIT(0),

  // doesn't overwrite on set
  __LISP_SYMTAB_SAFE = BIT(1),
};

/** Sets a symbol from the string representation @sym_str as a symbol on a
    symbol table @sym_tab with data @sym_obj, returning its memory. Affected by
    the table type */
struct lisp_sym*
lisp_symtab_set(string_ip sym_str, struct lisp_obj* sym_obj,
                struct lisp_symtab* sym_tab, const enum lisp_symtab_t typ);

/** Gets a symbol from the string representation @sym_str on a symbol table
    @sym_tab, returning its memory. Affected by the table type */
struct lisp_sym*
lisp_symtab_get(string_ip sym_str, struct lisp_symtab* sym_tab,
                const enum lisp_symtab_t typ);

/** Deletes a symbol from the string representation @sym_str on a symbol table
    @sym_tab. Affected by the table type */
void
lisp_symtab_del(string_ip sym_str, struct lisp_symtab* sym_tab,
                const enum lisp_symtab_t typ);

#endif
