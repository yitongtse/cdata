#! /usr/local/bin/perl

use strict;
use warnings;

my $in_file = "/ws/ytse/Content/Capture/d-602-239-214-66-2.pcap";
my $out_file = "test.out";
my $hdr_sz = 82;
my $pkt_sz = 188 * 7;
my $gap_sz = 58;


open(IN, $in_file);
open(OUT, "> $out_file");
binmode IN;
binmode OUT;
my $buf;
my $i;
my $j;
my $sync;


# Skip file header
read IN, $buf, $hdr_sz or die "File read failed\n";


for ($i=0; $i<10000; $i++) {
#for ($i=0; 1; $i++) {
    # Read the payload
    read IN, $buf, $pkt_sz or die "File read failed\n";
#    print OUT $buf;

    print "\nIP $i:";
    for ($j=0; $j<7; $j++) {
        $sync = sprintf("%x", ord(substr($buf, $j*188, 1)));
        print " $sync";
    }


    # Skip inter-packet gap
    read IN, $buf, $gap_sz or die "File read failed\n";
}

print "\n";
