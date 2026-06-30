#! /usr/local/bin/perl

$file = 'test.txt';
open(INFO, $file);
$line_cnt = 0;

while ($line = <INFO>)
{
    $line_cnt = $line_cnt + 1;
    print "$line_cnt: $line";
}

close(INFO);
print "\nTotal: $line_cnt lines\n";
