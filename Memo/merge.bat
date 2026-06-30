#! /bin/csh

set USER = "ytse"
set VOB = "/vob/qnx"
set PARENT_BR = "pb"
set PROJ = "flo_vnbu_ph2"
set STORAGE = "/vws/nmg"
set PRETERM_LBL = "FLO_VNBU_PH2_PRESYNC_TERM"


## 1
## Evaluate size of integration branch
# cd $VOB
ct find -avobs -element '{ brtype('$PROJ') }' -print | wc -l

## 2
## Perform incremental sync

## 2.1
## To find all possible view server
# sync_get_host -branch $PARENT_BR -v $VOB

## Lock the parent branch
# updbranch -c DDTS_STATE:MER -b $PARENT_BR -v $VOB

## 3
# mkview -i $PROJ -s $STORAGE -t commit -v $VOB
# ct setview $USER-$PROJ.commit

## 4
# updbranch -c DDTS_STATE:MER -b $PROJ -v $VOB
# cc_lock -n "commit.$PROJ.checklocks" -t W -s $VOB
# cc_lock -n "commit.$PROJ.checklocks" -t W -r $VOB

## 5
## Answer "y" on both prompts
# cd $VOB
# mklabel -comment "Terminal sync prior to commit"  -l $PRETERM_LBL -v $VOB

## 6
# cc_int_to_task

## 7
# prepare -s

## 8
## Answer "n" on prompt
# setenv CC_FMERGE_OPTS "-print"
# prepare -m LATEST
# unsetenv CC_FMERGE_OPTS

## 9
# prepare -m LATEST

## 10
## Compile and test !!!

## 11
# cc_diff -n -u > diff_out
## Remove identical files from the change list in
##    $STORAGE/$USER-$PROJ.commit/cisco/audit/ViewAudit
# cc_rm_identical

## 12
# cc_fix_copyright -c

## 13
# prepare -a -m LATEST -r
# commit

## 14
# updbranch -b $PROJ -v $VOB -a BRANCH_COLLAPSE:DONE
# updbranch -b $PROJ -v $VOB -c TASK_BRANCH:FALSE
# updbranch -b $PROJ -v $VOB -c DDTS_STATE:NONE

