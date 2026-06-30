#! /bin/csh

cd /vob/ios.comp/rfgw/lc/mv/src/images

# Build Cobia image
/vob/cisco.comp/rfgw/util/objects/tool.sunos.sparc/mkcfsbin mv_cob \
/auto/videoeng2/mb_fpga_cpld_images/beta_files/cb_fpga_23_080804_0276_0029_beta.bin  \
`date '+%Y%m%d'` `date '+00%H%M%S'`

# Build Yellowfin image
# /vob/cisco.comp/rfgw/util/objects/tool.sunos.sparc/mkcfsbin mv_yel /nfs/videoeng2/mb_syn_pnr/yf/080129_YF_2.0/syn_rev_1/pnr_rev_1/yf_fpga_top.bin `date '+%Y%m%d'` `date '+00%H%M%S'`

