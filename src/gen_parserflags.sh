#!/bin/bash

# Generate parser flags and messages
# to be used in conjunction with error.c and error.h
# searches the sources for commentlines following the pattern:
# // PARSEFLAG <KEYWORD> <FUNCTION-NAME> "<arguments>"
#
# it generates a table with keywords and functions in parsedef.h"
FILE="parsedef.h"
echo "/* File generated by gen_parserflags.sh, do not edit by hand */" >  $FILE
NFLAGS=0;
echo "#ifndef PARSEDEF_H" >>  $FILE
echo "#define PARSEDEF_H" >>  $FILE
# first collect all parsing flags in one file
if [ -z "$1" ]
then
	echo gen_parseflags.sh needs arguments!
	exit 1
fi
srcdir="$1"
shift
for s in $@
do
	echo Collecting parsing flags from $s
	egrep -o 'PARSEFLAG.*' "$srcdir/$s" | sort | uniq >>tmpflags
done

# create the parse flag defines
awk '{print "void "$3"(char *in);"}' tmpflags | sort | uniq>>$FILE

echo "typedef void (*ParserFun)(char *in);">>$FILE
echo "typedef struct {">>$FILE
echo "	char *key;">>$FILE
echo "	ParserFun fun;">>$FILE
echo "} KeyWord;">>$FILE

echo "const KeyWord KeyTable[] = {">>$FILE
awk '{print "\t{\""$2"\", &"$3"},"}' tmpflags>>$FILE
echo "	{NULL, NULL}">>$FILE
echo "};">>$FILE


echo "char *Usage[] = {"  >>  $FILE
sed -n 's/.*PARSEFLAG.*\(\".*\"\)/	\1,/gp' tmpflags>> $FILE
echo "	NULL"  >>  $FILE
echo "};"   >>  $FILE

echo "#endif /*PARSEDEF_H*/" >>  $FILE

rm tmpflags
