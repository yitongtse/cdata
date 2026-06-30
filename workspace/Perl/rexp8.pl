#! /usr/local/bin/perl -w

@ans = ("false", "true");

$_ = "#include IOS_INC(abc.h);";
print $ans[match_INC($_)],"\n";


sub match_INC {
    return (shift =~ /^#include.*_INC/);
}




