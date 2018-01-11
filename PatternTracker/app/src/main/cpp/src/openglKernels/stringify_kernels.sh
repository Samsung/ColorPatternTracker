#!/bin/bash

kernels="colConversion getColorPurity copyInGL getTransValidity getLineCrossing getPatternIndicator plotCornersInput plotCorners markDetectedCorners markDetectedCornersInput cornersZero getBlockCorners refineCorners getNCorners getCorners getLinePtAssignment"
OUT_MAIN=computeShaderKernels.h
echo " " >$OUT_MAIN

for name in $kernels
do
    IN=$name.cs
    OUT=$name.h

    #if [ $IN -ot $OUT ]
    #then
        #echo "Skipping generation of OpenCL $name kernel"
        #continue
    #fi
    echo "Generating OpenCL $name kernel"

    echo "static const char *"$name"_kernel =" >$OUT
    sed -e 's/\\/\\\\/g;s/"/\\"/g;s/  /\\t/g;s/^/"/;s/$/\\n"/' $IN >>$OUT
    if [ $? -ne 0 ]
    then
        exit 1
    fi
    echo ";" >>$OUT
	echo "#include \""$name".h\"" >>$OUT_MAIN
done
