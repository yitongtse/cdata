#! /usr/local/bin/perl -w

# Trying to replace:
#   INTERNAL_INC(file.h)
# with
#   "rfgw/file.h"
#

@ans = ("false", "true");

$_ = "#include INTERNAL_INC(abc.h);  // testing";
$a = (/INTERNAL_INC\((\w|.)*\)/);
print "$ans[$a] \n";
if ($a) {
   print "Original: ",$_,"\n";
   s/(INTERNAL_INC\()((\w|.)*)\)/\"rfgw\/$2\"/;
   print "Replaced: ",$_,"\n";
}



