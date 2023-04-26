#!/usr/bin/bash -e

case $1 in
  '-h'|'--help')
    echo -e \
'Usage:       scripts/make-deps.sh
Description: Makes the dependencies for objects
Variables:   CC=[C compiler]
             M4=[m4-like command]
Note:        Make sure to call this script from the repository root'
    exit 1
    ;;
esac

echo '[ .. ] Generating dependencies'

[[ -z $CC ]]  && CC=cc
[[ -z $M4 ]]  && M4=m4

# sed: fix-up for newlines put by the compiler
# awk: fix-up files in nested directories: put the first source on base
echo "[ == ] $CC -MM | sed | awk > make/Deps.mk"
find . -name '*.c' -not -path './dev/*' -type f |\
  xargs $CC -MM |\
  sed -z 's/\\\x0a *//g' |\
  awk '
$2 ~ /.*\/.*/ {
  basedir=$2;
  sub(/\/.*?$/, "", basedir);
  printf "%s/%s\n", basedir, $0;
  next;
}
{
  print;
}' > make/Deps.mk

# create the `OBJECTS' variable
echo "[ == ] cat make/Deps.mk | sed >> make/Deps.mk"
cat make/Deps.mk |\
  cut -d: -f1    |\
  tr '\n' ' '    |\
  sed 's: $:\n:' |\
  sed -e 's/^/OBJECTS:=&/' >> make/Deps.mk

echo '[ .. ] Generating objects'

echo "[ == ] awk make/Deps.mk | sed > make/Objects.mk"

# set up future template for `make-makefile':
#  - replace `/' with `I' for the objects for the M4FLAGs
awk -F: '
BEGIN {
  cmsg="";
  ccmd="";
}
/^OBJECTS/ {
  print;
  next;
}
{
  base=$1;
  sub(/\.o/, "", base);
  base_include=base;

  if (base ~ /.*\/.*/) {
    sub(/\//, "I", base_include);
  }

  cmsg = "@echo \"CC " base ".c\"";
  ccmd = "@$(CC) -c M4FLAG_include_" base_include " $(CFLAGS) $(CFLAGSADD) $< -o $@";
  printf "%s\n\t%s\n\t%s\n", $0, cmsg, ccmd;
}
' make/Deps.mk |\
  sed -e 's/M4FLAG_include_[0-9A-Za-z_]*/ /g' > make/Objects.mk

echo '[ OK ] scripts/make-deps.sh: Done'
