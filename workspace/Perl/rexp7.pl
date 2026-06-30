#! /usr/local/bin/perl -w


$orig = "#include COMP_INC(rfgw , common.h);";

if (replace_COMP_INC($replaced = $orig)) {
    print "Original: ",$orig,"\n";
    print "Replaced: ",$replaced,"\n";
} else {
    print "Not matched\n";
}


# COMP_INC ( x , y )
# <x/include/y>

sub replace_COMP_INC {
    $_[0] =~ s/(^#include\s+)(COMP_INC\s*\(\s*)([\w.-_]+)(\s*,\s*)([\w.-_]+)\)/$1\<$3\/include\/$5\>/;
    return (defined($1));
}


