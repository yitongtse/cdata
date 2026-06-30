#! /usr/bin/perl -w

$line_cnt = 0;

print "Enter some text\n";

while () {
    chomp($line = <STDIN>);
    if ($line eq "") {
        last;
    }
    $line_cnt++;
}

print "You have entered $line_cnt lines\n";

