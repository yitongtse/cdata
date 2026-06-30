#! /usr/local/bin/perl

$dir_cnt = 0;
$proc_file_cnt = 0;
$skip_file_cnt = 0;
$tot_match_cnt = 0;

$root_dir = "/vob/ios.comp/rfgw/lc";
dir_traverse($root_dir, \&node_process);

print "\nSummary:\n";
print "    Directories   : $dir_cnt \n";
print "    Files skipped : $skip_file_cnt \n";
print "    Files proessed: $proc_file_cnt \n";
print "    Matches       : $tot_match_cnt \n";



# This is a directory traverse routine
#
sub dir_traverse {
    my ($dir, $callback) = @_;
    my $present;

    opendir $present, $dir or return();

    for (grep { ! /^(\.|\.\.)$/ } readdir $present) {
        my $path = "$dir/$_";

        &$callback($path);
        dir_traverse($path, $callback) if -d $path;
    }

    closedir $present;
    return();
}


sub node_process {
    my $node = shift;

    if (-d $node) {
#        print "Dir: $node\n";
        $dir_cnt++;
    } else {
        if ($node =~ /[ch]$/) {
#            print "File: $node\n";
            file_scan($node);
            $proc_file_cnt++;
        } else {
#            print "File: $node (skipped)\n";
            $skip_file_cnt++;
        }
    }
    return;
}


sub file_scan {
    my $file = shift;
    my $line_cnt = 0;
    my $match_cnt = 0;
    open(IN, $file);

    while (<IN>) {
        $line_cnt++;
        if (match($_)) {
#            print "  Line ",$line_cnt,": ",$_;
            print "$file: $line_cnt: ",$_;
            $match_cnt++;
        }
    }

    close(IN);

    $tot_match_cnt += $match_cnt;
    return;
}


sub match {
    $_ = shift;
    return /default\:\s*}/;
}


