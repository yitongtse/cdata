#! /usr/local/bin/perl -w

@ans = ("false", "true");

$_ = "#include INTERNAL_INC(abc.h);";
$a = (/INTERNAL_INC\((\w|.)*\)/);
print "$ans[$a] \n";

$_ = "#include INTERNAL_INC<abc.h);";
$a = (/INTERNAL_INC\((\w|.)*\)/);
print "$ans[$a] \n";

$_ = "#include INTERNAL_INC(abc.h;";
$a = (/INTERNAL_INC\((\w|.)*\)/);
print "$ans[$a] \n";


