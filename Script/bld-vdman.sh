#!/bin/tcsh -f
#
echo "\nBuilding vidman...\n"
cd $BINOS_ROOT/cable/video/vdman
gmk -f TARGET=mips64_cge7 Plx:=-ubr all-ubr
if ($status != '0') then
    echo "\nmake vidman failed...\n"
    exit 1
endif

echo "\nBuilding clc-mips packages...\n"
cd $BINOS_ROOT
rm -rf linkfarm/iso-cbr/*clcvideo*.pkg*
./build/package/package_new.pl --stage="module" --module="clc-mips" cbr $BINOS_ROOT
if ($status != '0') then
    echo "\nbuild clc-mips packages failed...\n"
    exit 1
endif
