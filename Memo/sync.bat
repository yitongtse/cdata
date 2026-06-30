#! /bin/csh

set USER = "ytse"
set VOB = "/vob/rmx"
set PARENT_BR = "dpi_integ"
set PROJ = "dpi_newapi_integ"
set OLD_REF = "DPI01_03_27"
set NEW_REF = "INT2_3_8DP"
set STORAGE = "/vws/noa"
set PRESYNC_LBL = "DPI_NEWAPI_INTEG_PRESYNC_INT2_3_8DP"
set POSTSYNC_LBL = "DPI_NEWAPI_INTEG_POSTSYNC_INT2_3_8DP"


## 0
## Make sure CLEARCASE_AVOBS is set to the appropriate VOB
# env | grep CLEARCASE_AVOBS

## 1: Determine OLD_REF
## Set view to any view on the branch to be sync
# updbranch -p -b $PROJ -v $VOB | grep REF_LABEL

## 2.1: Create a sync view (make sure you run it in the view storage server)
## Make sure the master and replica name are the same
# ct describe vob:$VOB | grep "replica name"
# ct lstype -long brtype:$PROJ@$VOB | grep "master replica"
# mkview -i $PROJ -sync $NEW_REF -s $STORAGE -v $VOB
# ct setview $USER-$PROJ.sync

## 2.2: Create a temporary view from the sync label
# mkview -i $NEW_REF -s $STORAGE -v $VOB -t ref

## 3: Estimate the size of the merge
# cd $VOB
# ct findmerge -avobs -element '{ brtype('$PROJ') }' \
#     -ftag $USER-$NEW_REF.ref -log estimate.log -print
# wc -l estimate.log

## 4: Lock branch
# updbranch -p -b $PROJ -v $VOB
# updbranch -c DDTS_STATE:MER -b $PROJ -v $VOB
# cc_lock -n "commit.$PROJ.checklocks" -t W -s $VOB
# cc_lock -n "commit.$PROJ.checklocks" -t W -r $VOB

## 5: Lay pre-sync label
## Answer "y" to prompts
# mklabel -comment "Syncing with $PARENT_BR branch at $NEW_REF" \
#     -l $PRESYNC_LBL -v $VOB

## 6: Merge from new ref label
# ct findmerge -avobs -element '{ brtype('$PROJ') }' \
#     -ftag $USER-$NEW_REF.ref -log $PROJ.findlog \
#     -exec 'cc_merge -r -l /vob/rmx/sync.rcslog'


## 7: Reslove conflicts, compile and test !! 
##


## 8.1: Generate and review diffs
# cc_diff -avobs -qry '\!brtype('$PROJ')' -l $OLD_REF \
#     > /vob/rmx/newbase.diffs
# cc_diff -u -p > /vob/rmx/sync.diffs
# cc_diff -u -v $USER-$NEW_REF.ref > /vob/rmx/branch.diffs

## 8.2: Uncheckout identical files and record hyperlinks
# cc_rm_identical -hlink -f > /vob/rmx/ident.files

## 9.1
# thawview

## 9.2: Re-do merge
# ct findmerge -avobs -element '{ brtype('$PROJ') }' \
#     -ftag $USER-$NEW_REF.ref -log $VOB/$PROJ.zerolog \
#     -exec 'cc_merge -r -l $VOB/$PROJ.zerocclog'

## 9.3: Check in merged elements
## Use "sync" as bugid
# prepare -a -m LATEST
# commit

## 9.4: Move base label to new ref label
# updbranch -p -b $PROJ | grep BASE_LABEL

## 9.5: Update REF_LABEL
# updbranch -b $PROJ -c REF_LABEL:$NEW_REF -v $VOB

## 10: Lay post-sync label
# mklabel -comment "After syncing to $PARENT_BR branch at $NEW_REF" \
#     -l $POSTSYNC_LBL -v $VOB

## 11: Unlock child branch
# updbranch -c DDTS_STATE:"NEW,BID,MER" -v $VOB -b $PROJ


## NOTE: Proceed only after you no longer need the sync view !!!

## 12: Clean sync view
# cc_rmview -view $USER-$PROJ.sync -vob $VOB

## 13: Clean temporary reference view
# cc_rmview -view $USER-$NEW_REF.ref -vob $VOB

