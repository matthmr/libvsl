#ifndef LOCK_CGEN
#  define LOCK_CGEN

#  define PROVIDE_INC_STRING
#  define LOCK_ERR_UTILS

#  include "utils.h"
#  include "err.h"

#  ifndef CGEN_OUTBUF
#    define CGEN_OUTBUF (4096)
#  endif

extern const char autogen_notice[];

enum cgen_typ {
  CGEN_UNKNOWN = 0,

  CGEN_RECURSE = 1,
  CGEN_UCHAR,
  CGEN_SHORT,
  CGEN_INT,
  CGEN_STRING,
};

void cgen_notice(void);
void cgen_string(char* string);
void cgen_itoa_string(uint num);
void cgen_flush(void);

void cgen_index(uint idx);
void cgen_field(char* name, enum cgen_typ typ, void* dat);
void cgen_field_array(char* name, enum cgen_typ typ, void* dat, uint size);
void cgen_close_field(void);

INC_STRING(CGEN_OUTBUF);

struct string_is {
  string_ip inc;
  uint      size;
};

typedef struct string_is string_is;

string_is cgen_stris_from(char* string, uint size);
void cgen_itoa_for(uint num, string_is* stris);
void cgen_string_for(char* string, string_is* stris);
void cgen_flush_for(string_is* stris);
void cgen_index_for(uint idx, string_is* stris);
#endif
