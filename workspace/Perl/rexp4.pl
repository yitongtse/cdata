#! /usr/local/bin/perl -w

# Trying to replace:
#   INTERNAL_INC(file.h)
# with
#   "rfgw/file.h"
#

$_ = "#include INTERNAL_INC(abc.h);  // testing";

$orig = $_;
# s/(INTERNAL_INC\()((\w|.)*)\)/\"rfgw\/$2\"/;
# s/(INTERNAL_INC\s*\()((\w|.)*)\)/\"rfgw\/$2\"/;
s/(^#include\s+)(INTERNAL_INC\s*\()((\w|.)*)\)/$1\"rfgw\/$3\"/;

if (defined($2)) {
    print "Original: ",$orig,"\n";
    print "Replaced: ",$_,"\n";
} else {
    print "Not matched\n";
}


