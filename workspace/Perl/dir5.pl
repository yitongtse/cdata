#! /usr/local/bin/perl

$file_cnt = 0;
$dir_cnt = 0;
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
    my $file = shift;

    if (-d $file) {
        print "Dir : $file\n";
        $dir_cnt++;
    } else {
        print "File: $file\n";
        file_process($file);
        $file_cnt++;
    }
    return;
}


sub file_process {
    my $file = shift;
    my $match_cnt = 0;
    open(INFO, $file);

    while ($line = <INFO>) {
        if ($line =~ /INTERNAL_INC/) {
            $match_cnt++;
        }
    }

    close(INFO);
    print "  INTERNAL_INC: $match_cnt\n";
    $tot_match_cnt += $match_cnt;
    return;
}


dir_traverse("/vob/ios.comp/rfgw/lc", \&node_process);

print "\nProcessing summary:\n";
print "  Files  : $file_cnt \n";
print "  Dirs   : $dir_cnt \n";
print "  Matches: $tot_match_cnt \n";

