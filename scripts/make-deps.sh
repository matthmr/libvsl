#!/usr/bin/bash -e

case $1 in
  '-h'|'--help')
    echo -e \
'Usage:       scripts/make-deps.sh
Description: Makes the dependencies for objects
Variables:   CC=[C compiler]
             M4=[m4-like command]
             SED=[sed-like command]
Note:        Make sure to call this script from the repository root'
    exit 1
    ;;
esac

echo '[ .. ] Generating dependencies'

[[ -z $CC ]]  && CC=cc
[[ -z $M4 ]]  && M4=m4
[[ -z $SED ]] && SED=sed

# fix-up for local development branch: all test files have `-test' prefix
# fix-up files in nested directories: put the first source on base
echo "[ == ] $CC -MM | awk > make/Deps.mk"
find . -name '*.c' -and -not -name '*-test*' -type f |\
  xargs $CC -MM |\
awk '
$2 ~ /.*\/.*/ {
  basedir=$2
  sub(/\/.*?$/, "", basedir);
  printf "%s/%s\n", basedir, $0;
  next
}
{
  print
}' > make/Deps.mk

# create the `OBJECTS' variable
echo "[ == ] cat make/Deps.mk | $SED >> make/deps.mk"
cat make/Deps.mk  |\
  cut -d: -f1     |\
  tr '\n' ' '     |\
  $SED 's: $:\n:' |\
  $SED -e 's/^/OBJECTS:=&/' >> make/Deps.mk

echo '[ .. ] Generating objects'

echo "[ == ] awk make/Deps.mk | $M4 make/Objects.m4 | $SED | awk > make/Objects.mk"

# set up future template for `make-makefile':
#  - replace `/' with `I' for the objects for the M4FLAGs
awk -F: '
BEGIN {
  cmsg="";
  ccmd="";
}
/^OBJECTS/ {
  print
  next
}
{
  base=$1;
  sub(/\.o/, "", base);
  base_include=base;

  if (base ~ /.*\/.*/) {
    sub(/\//, "I", base_include);
  }

  ocmd = "M4FLAG_override_" base_include;
  cmsg = "@echo \"CC " base ".c\"";
  ccmd = "@$(CC) -c M4FLAG_include_" base_include " $(CFLAGS) $(CFLAGSADD) $< -o $@";
  printf "%s\n\t%s\n\t%s\n\t%s\n", $0, ocmd, cmsg, ccmd;
}
' make/Deps.mk |\
  eval "$M4 $M4FLAGS make/Objects.m4" /dev/stdin     |\
    $SED -e 's/M4FLAG_include_[0-9A-Za-z_]*/ /g'      \
         -e 's/M4FLAG_override_[0-9A-Za-z_]*/ /g'    |\
      awk '
BEGIN {
  del=0;
}
/^[ 	]*$/ {
  next;
}
/M4FLAG_DELETE_ME/ {
  del=2;
  next;
}
{
  if (del > 0) {
    del -= 1;
    next;
  }
}
{
  print;
}' > make/Objects.mk

echo '[ OK ] make-deps.sh: Done'
