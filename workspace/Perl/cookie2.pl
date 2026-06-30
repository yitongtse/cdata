#! /usr/bin/perl -w
#
# Cookie Monster

$cookie = "";

while ($cookie !~ /cookie/i) {
    print 'Give me a cookie: ';
    chomp ($cookie = <STDIN>);
}

print "Mmmm.  Cookie.\n";
