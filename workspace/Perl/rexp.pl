#! /usr/local/bin/perl -w

@ans = ("false", "true");

# $_ = "abcd";
# $a = (/a..d/);
# print "$ans[$a] \n";

# $_ = "abce";
# $a = (/a..d/);
# print "$ans[$a] \n";

$_ = "check.c";
$a = (/[ch]$/);
print "$ans[$a] \n";

$_ = "check.h";
$a = (/[ch]$/);
print "$ans[$a] \n";

$_ = "check.o";
$a = (/[ch]$/);
print "$ans[$a] \n";

