/** NOTE: errors in this module just exit out the program
 */

#include <unistd.h>
#include <stdlib.h>

#define PROVIDE_INC_STRING

#include "utils.h"
#include "cgen.h"
#include "err.h"

INC_STRING(CGEN_OUTBUF);

static string_i outbuf;

const char autogen_notice[] = \
  "// THIS FILE IS AUTO-GENERATED. DO NOT EDIT\n\n";

static inline void write_file(int fd, string_i* buf, int size) {
  buf->idx = 0;

  int ret = write(fd, buf->string, size);

  if (ret == -1) {
    write(STDERR_FILENO, ERR_MSG("cgen", "error in cgen"));
    exit(1);
  }
}

static uint inc_buf_idx(string_i* buf, char* string, uint idx) {
  uint size    = buf->size,
       inc_idx = buf->idx;
  char* stri   = buf->string;

  for (;;) {
    if (string[idx] == '\0') {
      break;
    }
    else if (size > 0 && inc_idx == CGEN_OUTBUF) {
      write_file(STDOUT_FILENO, buf, buf->size);
      return idx;
    }

    stri[inc_idx] = string[idx];
    ++inc_idx, ++idx;
  }

  buf->size += idx;
  buf->idx   = inc_idx;

  return 0;
}

static inline void write_string(char* string) {
  int ret = inc_buf_idx(&outbuf, string, 0);

  while (ret) {
    ret = inc_buf_idx(&outbuf, string, 0);
  }
}

void cgen_notice(void) {
  write_string((char*)autogen_notice);
}

void cgen_atoi(uint num) {
  uint num_idx   = num;
  uint idx       = 0;

  char string[1] = {0};

  do {
    num_idx /= 10;
    idx++;
  } while (num_idx);

  uint exp = 1;
  for (uint _ = 1; _ < idx; _++) {
    exp *= 10;
  }

  idx = 0;

  while (exp) {
    uint div = num / exp;
    *string = ITOA(div);
    write_string(string);
    num -= exp * div;
    exp /= 10;
  }
}

void cgen_index(uint idx) {
  write_string("[");
  cgen_atoi(idx);
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
  case CGEN_RECURSE:
    write_string("{ ");
    return;
  case CGEN_INT:
    cgen_atoi(*(int*) dat);
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
  write_file(STDOUT_FILENO, &outbuf, outbuf.size);
}
