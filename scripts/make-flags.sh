#!/usr/bin/bash

case $1 in
  '-h'|'--help')
    echo -e \
'Usage:       scripts/make-flags.sh
Description: Makes Makefile flags for the build
Variables:   M4=[m4-like command]
             M4FLAGS=[m4 flags]
Note:        Make sure to call this script from the repository root'
    exit 1
    ;;
esac

[[ -z $M4 ]]  && M4=m4

echo "[ .. ] Applying configuration flags to build"
echo "[ == ] $M4 $M4FLAGS make/Flags.m4 > make/Flags.mk"

INCLUDE=''

# look for include flags: `[+/-]debug -> CFLAGS += -[D/U]DEBUG'
DEBUG=$(echo "$M4FLAGS" | grep -o "M4FLAG_include_DEBUG=.")
DEBUG_STATE=$(echo "$DEBUG" | sed -z 's/M4FLAG_include_DEBUG=\(.\)/\1/g')

if [[ $DEBUG_STATE = '1' ]]; then
  INCLUDE+='CFLAGS += -DDEBUG'
elif [[ $DEBUG_STATE = '0' ]]; then
  INCLUDE+='CFLAGS += -UDEBUG'
fi

eval "$M4 $M4FLAGS make/Flags.m4" > make/Flags.mk

echo "$INCLUDE" >> make/Flags.mk

echo "[ OK ] scripts/make-flags.sh: Done"
