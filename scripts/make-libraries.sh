#!/usr/bin/bash -e

# TODO: this build script is not compliant with cproj, add and `Include.m4'
#       pass in the future (it's not needed now)

case $1 in
  '-h'|'--help')
    echo -e \
'Usage:       scripts/make-libraries.sh
Description: Makes the libraries for the targets
Variables:   M4=[m4-like command]
             SED=[sed-like command]
Note:        Make sure to call this script from the repository root'
    exit 1
    ;;
esac

echo '[ .. ] Generating libraries'

[[ -z $M4 ]]  && M4=m4
[[ -z $SED ]] && SED=sed

echo "[ == ] $M4 make/Libraries.m4 > make/Libaries.mk"
eval "$M4 $M4FLAGS make/Libraries.m4" > make/Libraries.mk

echo "[ == ] cat make/Libraries.m4 | $SED >> make/Libaries.mk"
eval "$M4 $M4FLAGS make/Libraries.m4" |\
  cut -d: -f1                         |\
  tr '\n' ' '                         |\
  $SED 's: $:\n:'                     |\
  $SED -e 's/^/LIBRARIES:=&/g' >> make/Libraries.mk

echo '[ OK ] make-libraries.sh: Done'
