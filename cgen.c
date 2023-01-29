/** NOTE: errors in this module just exit out the program
 */

#include <unistd.h>
#include <stdlib.h>

#define PROVIDE_INC_STRING

#include "utils.h"
#include "cgen.h"
#include "err.h"

INC_STRING(CGEN_OUTBUF);

static string_i outbuf = {
  .idx = 0,
};

const char autogen_notice[] = \
  "// THIS FILE IS AUTO-GENERATED. DO NOT EDIT\n\n";

static inline void write_file(int fd) {
  if (!outbuf.idx) {
    return;
  }

  int wstat  = write(fd, outbuf.string, outbuf.idx);
  outbuf.idx = 0;

  if (wstat == -1) {
    write(STDERR_FILENO, ERR_MSG("cgen", "error in cgen"));
    exit(1);
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

void cgen_notice(void) {
  write_string((char*)autogen_notice);
}

void cgen_itoa_string(uint num) {
  uint num_idx = num, idx = 0;
  do {
    num_idx /= 10;
    idx++;
  } while (num_idx);

  uint exp = 1;
  for (uint _ = 1; _ < idx; ++_) {
    exp *= 10;
  }

  char string[1] = {0};
  while (exp) {
    uint div = num / exp;
    *string = ITOA(div);
    write_string(string);
    num -= exp * div;
    exp /= 10;
  }
}

char* cgen_itoa(char* buf, int buf_size, uint num) {
  uint num_idx = num, idx = 0;
  do {
    num_idx /= 10;
    idx++;
  } while (num_idx);

  uint exp = 1;
  for (uint _ = 1; _ < idx; ++_) {
    exp *= 10;
  }

  uint buf_idx = 0;
  while (exp) {
    uint div = num / exp;
    buf[buf_idx] = ITOA(div);
    num -= exp * div;
    exp /= 10;
    ++buf_idx;
  }

  for (; buf_idx < buf_size; ++buf_idx) {
    buf[buf_idx] = '\0';
  }

  return buf;
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
    write(STDERR_FILENO, ERR_MSG("cgen", "error in cgen"));
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
  case CGEN_CHAR:
    cgen_itoa_string(*(char*) dat);
    break;
  case CGEN_STRING:
    if (dat) {
      write_string(dat);
    }
    break;
  }

  write_string(", ");
}

void cgen_flush(void) {
  cgen_clear();
  write_file(STDOUT_FILENO);
}
