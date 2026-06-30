#! /usr/local/bin/perl -w

sub replace_INTERNAL_INC {
    $_ = shift;
    s/(^#include\s+)(INTERNAL_INC\s*\()((\w|.)*)\)/$1\"rfgw\/$3\"/;
    return (defined($1), $_);
}


$orig = "#include INTERNAL_INC(abc.h);";

my ($matched, $replaced) = replace_INTERNAL_INC($orig);

if ($matched) {
    print "Original: ",$orig,"\n";
    print "Replaced: ",$replaced,"\n";
} else {
    print "Not matched\n";
}


