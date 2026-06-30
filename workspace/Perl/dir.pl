#! /usr/local/bin/perl -w

$file_cnt = 0;
$dir_cnt = 0;

@files = glob("*");

foreach $file (@files) {
    print "$file \n";
    $file_cnt++;
}

print "Total $file_cnt files.\n";
