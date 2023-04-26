#!/usr/bin/bash -e

case $1 in
  '-h'|'--help')
    echo -e \
'Usage:       scripts/make-targets.sh
Description: Makes the targets
Variables:   M4=[m4-like command]
Note:        Make sure to call this script from the repository root'
    exit 1
    ;;
esac

echo '[ .. ] Generating targets'

[[ -z $M4 ]] && M4=m4

echo "[ == ] $M4 make/Targets.m4 | awk | sed > make/Targets.mk"

eval "$M4 $M4FLAGS make/Targets.m4" |\

awk -F: '
BEGIN {
  cmsg="";
  ccmd="";
  link_cmd="";
  link_lib="";
}
/^LINK_LOCAL: .+/ {
  link_cmd=$2;
  link_lib=$2;
  gsub(/ $/, "", link_cmd);
  gsub(/ $/, "", link_lib);
  gsub(/\.a/, "", link_cmd);
  gsub(/ /, " -l", link_cmd);
  gsub(/ /, " lib", link_lib);
  link_cmd="-L. " link_cmd;
  next;
}
/^LINK_LOCAL: $/ {
  next;
}
/^$/ {
  next;
}
{
  base=$1;
  base_include=base;

  if (base ~ /.*\/.*/) {
    sub(/\//, "I", base_include);
  }

  cmsg = "@echo \"CC " base "\"";
  ccmd = "@$(CC) M4FLAG_include_" base_include " $(CFLAGS) $(CFLAGSADD) " link_cmd " $^ -o $@";

  printf "%s %s\n\t%s\n\t%s\n", $0, link_lib, cmsg, ccmd;
  link_lib="";
  link_cmd="";
}
' |\
  sed -e 's/M4FLAG_include_[0-9A-Za-z_]*/ /g' > make/Targets.mk

echo "[ == ] cat make/Targets.mk | sed >> make/Targets.mk"
cat make/Targets.mk |\
  cut -d: -f1       |\
  sed -n '/^[^\t]/p' |\
  tr '\n' ' '       |\
  sed 's: $:\n:'    |\
  sed -e 's/^/TARGETS:=&/' >> make/Targets.mk

echo '[ OK ] scripts/make-targets.sh: Done'
