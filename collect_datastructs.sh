#!/bin/bash

# collect data structures to export in libssdp

echo "/* File generated by collect_datastructs.sh, do not edit by hand */" >  "libssdp_structs.h"

# first collect all error flags in one file
for s in $@
do
	echo Collecting data structures from $s
	awk '/BEGIN_SSDP_EXPORT/{flag=1;next}/END_SSDP_EXPORT/{flag=0}flag' $s >>  "libssdp_structs.h"
done
