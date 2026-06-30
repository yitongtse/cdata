#! /usr/local/bin/perl

$dir_cnt = 0;
$proc_file_cnt = 0;
$skip_file_cnt = 0;
$tot_match_cnt = 0;


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
        print "Directory: $node\n";
        $dir_cnt++;
    } else {
        if ($node =~ /[ch]$/) {
            print "File: $node\n";
            file_process($node);
            $proc_file_cnt++;
        } else {
            print "Skip file: $node\n";
            $skip_file_cnt++;
        }
    }
    return;
}


sub file_process {
    my $file = shift;
    my $match_cnt = 0;
    open(INFO, $file);

    while (<INFO>) {
        if (/INTERNAL_INC\((\w|.)*\)/) {
            print "  $_";
            $match_cnt++;
        }
    }

    close(INFO);
    if ($match_cnt > 0) {
        print "  $match_cnt matches\n";
    }
    $tot_match_cnt += $match_cnt;
    return;
}



dir_traverse("/vob/ios.comp/rfgw/lc", \&node_process);

print "\nProcessing summary:\n";
print "  Directories   : $dir_cnt \n";
print "  Files skipped : $skip_file_cnt \n";
print "  Files proessed: $proc_file_cnt \n";
print "  Matches       : $tot_match_cnt \n";

