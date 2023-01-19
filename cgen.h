#ifndef LOCK_CGEN
#  define LOCK_CGEN

#  ifndef CGEN_OUTBUF
#    define CGEN_OUTBUF (4096)
#  endif

extern const char autogen_notice[];

enum cgen_typ {
  CGEN_UNKNOWN = 0,

  CGEN_RECURSE = 1,
  CGEN_INT,
  CGEN_STRING,
};

void cgen_notice(void);
void cgen_atoi(uint num);
void cgen_index(uint idx);
void cgen_string(char* string);
void cgen_field(char* name, enum cgen_typ typ, void* dat);
void cgen_close_field(void);
void cgen_flush(void);

#endif
