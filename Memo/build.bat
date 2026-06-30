#! /bin/csh

set USER = "ytse"
set VOB = "/vob/rmx"
set BRANCH = "dpi_integ"
set STORAGE = "/vws/nip"
set VIEW = $USER-$BRANCH
set VERSION = "2.5.6DP5"
set BUILDNO = "317"
set LABEL = "INT2_5_6DP5"


# mkview -i $BRANCH -v $VOB -s $STORAGE
ct setview $VIEW

# cd $VOB
# freezeview > freeze.note

## Record the freeze view time

## Build DSP binaries
# cd $VOB/dsp/app/dpi
# make VER=$BUILDNO all

# cd $VOB/dsp/app/statmux
# make all

# cd $VOB/dsp/app/bitshaper
# make all

# cd $VOB/ppc

## Check available compile licenses
# lmstat -a | more

# makeall $VERSION $BUILDNO

## Make a release directory visible to NT network
# mkdir $USER/TmpRel

# cd utils
# uudecode connect_exe.uue
# uudecode srec2fls_exe.uue
# cd ..
cp ./*/*.ld ./*/*.fls ./*/*.exe $USER/TmpRel

## Test the build with RateMux


# mklabel -l LABEL -co "Label for release 2.5.6DP5"
