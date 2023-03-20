/** NOTE: errors in this module just exit out the program
 */

#include <unistd.h>
#include <stdlib.h>

#include "libcgen.h"
#include "debug.h"

static string_i outbuf = {
  .idx = 0,
};

static const char autogen_notice[] = \
  "// THIS FILE IS AUTO-GENERATED. DO NOT EDIT\n\n";

static inline void cgen_err(char* msg, uint size) {
  write(STDERR_FILENO, msg, size);
  exit(1);
}

static inline void write_file(int fd) {
  if (!outbuf.idx) {
    return;
  }

  int wstat  = write(fd, outbuf.string, outbuf.idx);
  outbuf.idx = 0;

  if (wstat == -1) {
    cgen_err(ERR_MSG("cgen", "IO error in libc's `write'"));
  }
}

static void write_string(char* string) {
  char* stri    = outbuf.string;
  uint  inc_idx = outbuf.idx;

  for (uint idx = 0; string[idx] != '\0'; ++inc_idx, ++idx) {
    if (inc_idx == CGEN_OUTBUF) {
      outbuf.idx = CGEN_OUTBUF;
      write_file(STDOUT_FILENO);
      inc_idx = 0;
    }

    stri[inc_idx] = string[idx];
  }

  outbuf.idx = inc_idx;
}

static inline char conv_lisp_c(char c) {
  switch (c) {
  case '-':
    c = '_';
    break;
  }

  return c;
}

static void write_string_from_lisp(char* string) {
  char* stri    = outbuf.string;
  uint  inc_idx = outbuf.idx;

  for (uint idx = 0; string[idx] != '\0'; ++inc_idx, ++idx) {
    if (inc_idx == CGEN_OUTBUF) {
      outbuf.idx = CGEN_OUTBUF;
      write_file(STDOUT_FILENO);
      inc_idx = 0;
    }

    // C is not as lenient as lisp we it comes to identifiers, so we need to
    // convert some characters
    stri[inc_idx] = conv_lisp_c(string[idx]);
  }

  outbuf.idx = inc_idx;
}

static inline void cgen_clear(void) {
  uint idx = outbuf.idx;

  if (idx == CGEN_OUTBUF) {
    return;
  }

  for (; idx < CGEN_OUTBUF; ++idx) {
    outbuf.string[idx] = '\0';
  }
}

////////////////////////////////////////////////////////////////////////////////

void cgen_itoa_string(uint num) {
  if ((int) num == -1) {
    write_string("-1u");
    return;
  }


  uint exp = 1;
  for (uint _num = num; _num >= 10; _num /= 10) {
    exp *= 10;
  }

  char string[2] = {0};

  while (exp) {
    uint dig = num / exp;
    *string  = ITOA(dig);
    write_string(string);
    num -= (dig * exp);
    exp /= 10;
  }
}

void cgen_notice(void) {
  write_string((char*)autogen_notice);
}

void cgen_index(uint idx) {
  write_string("[");
  cgen_itoa_string(idx);
  write_string("]");
}

void cgen_close_field(void) {
  write_string("}, ");
}

void cgen_string(char* string) {
  write_string(string);
}

void cgen_field(char* name, enum cgen_typ typ, void* dat) {
  write_string(".");
  write_string(name);
  write_string(" = ");

  switch (typ) {
  case CGEN_UNKNOWN:
    cgen_err(ERR_MSG("cgen", "unknown type for field"));
    exit(1);
    break;
  case CGEN_RECURSE:
    write_string("{ ");
    return;
  case CGEN_INT:
    cgen_itoa_string(*(int*) dat);
    break;
  case CGEN_SHORT:
    cgen_itoa_string(*(short*) dat);
    break;
  case CGEN_UCHAR:
    cgen_itoa_string(*(uchar*) dat);
    break;
  case CGEN_STRING:
    if (dat) {
      write_string_from_lisp(dat);
    }
    break;
  }

  write_string(", ");
}

void cgen_field_array(char* name, enum cgen_typ typ, void* dat, uint size) {
  write_string(".");
  write_string(name);
  write_string(" = {");

  switch (typ) {
  case CGEN_INT:
    for (uint i = 0; i < size; ++i) {
      cgen_itoa_string(((int*) dat)[i]);
      write_string(", ");
    }
    break;
  case CGEN_SHORT:
    for (uint i = 0; i < size; ++i) {
      cgen_itoa_string(((short*) dat)[i]);
      write_string(", ");
    }
    break;
  case CGEN_UNKNOWN: default:
    cgen_err(ERR_MSG("cgen", "unknown or unsupported type for array field"));
    exit(1);
    break;
  }

  write_string("}, ");
}

void cgen_flush(void) {
  cgen_clear();
  write_file(STDOUT_FILENO);
}

////////////////////////////////////////////////////////////////////////////////

string_is cgen_stris_from(char* string, uint size) {
  string_is ret = {0};

  ret.inc.string = string;
  ret.inc.idx    = 0;
  ret.size       = size;

  return ret;
}

void cgen_itoa_for(uint num, string_is* stris) {
  char* buf    = stris->inc.string;
  uint buf_idx = stris->inc.idx;
  uint size    = stris->size;

  if ((int) num == -1) {
    write_string("-1u");
    return;
  }

  uint exp = 1;
  for (uint _num = num; _num >= 10; _num /= 10) {
    exp *= 10;
  }

  while (exp) {
    uint dig = num / exp;
    buf[buf_idx] = ITOA(dig);
    num -= (exp * dig);
    exp /= 10;
    ++buf_idx;

    if (buf_idx == size) {
      cgen_err(ERR_MSG("cgen", "error in cgen"));
    }
  }

  stris->inc.idx = buf_idx;
}

static void write_string_for(char* string, string_is* stris) {
  char* stri    = stris->inc.string;
  uint  inc_idx = stris->inc.idx;
  uint  size    = stris->size;

  for (uint idx = 0; string[idx] != '\0'; ++inc_idx, ++idx) {
    if (inc_idx == size) {
      cgen_err(ERR_MSG("cgen", "error in cgen"));
    }

    stri[inc_idx] = string[idx];
  }

  stris->inc.idx = inc_idx;
}

void cgen_string_for(char* string, string_is* stris) {
  write_string_for(string, stris);
}

static inline void cgen_clear_for(string_is* stris) {
  char* buf    = stris->inc.string;
  uint buf_idx = stris->inc.idx;
  uint size    = stris->size;

  if (buf_idx == size) {
    return;
  }

  for (; buf_idx < size; ++buf_idx) {
    buf[buf_idx] = '\0';
  }
}

void cgen_index_for(uint idx, string_is* stris) {
  write_string_for("[", stris);
  cgen_itoa_for(idx, stris);
  write_string_for("]", stris);
}

void cgen_flush_for(string_is* stris) {
  cgen_clear_for(stris);
}
