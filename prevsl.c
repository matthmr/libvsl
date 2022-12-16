#include <unistd.h>
#include <stdlib.h>

static int parse_ioblock(char* buf, uint size) {
  int ret = 0;

  for (uint i = 0; i < size; i++) {
    char c = buf[i];
  }
  goto done;

done:
  return ret;
}
static int parse_bytstream(int fd) {
  int ret = 0;

  int  s;
  uint r;

  do {
    r = read(fd, iobuf, IOBLOCK);
    s = parse_ioblock(iobuf, r);
    if (s) {
      ret = s;
      goto done;
    }
  } while (r == IOBLOCK);

done:
  return ret;
}

int main(void) {
  int ret = parse_bytestream(STDIN_FILENO);

  return ret;
}
