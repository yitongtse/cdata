#! /usr/local/bin/perl -w

sub replace_INTERNAL_INC {
    $_[0] =~ s/(^#include\s+)(INTERNAL_INC\s*\()([\w.-_]+)\)/$1\"rfgw\/$3\"/;
    return (defined($1));
}


$orig = "#include INTERNAL_INC(abc.h);";

if (replace_INTERNAL_INC($replaced = $orig)) {
    print "Original: ",$orig,"\n";
    print "Replaced: ",$replaced,"\n";
} else {
    print "Not matched\n";
}


