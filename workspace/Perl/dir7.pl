#! /usr/local/bin/perl

$dir_cnt = 0;
$proc_file_cnt = 0;
$skip_file_cnt = 0;
$tot_match_cnt = 0;

dir_traverse("/vob/ios.comp/rfgw/lc/mv/src/root", \&node_process);

print "\nProcessing summary:\n";
print "  Directories   : $dir_cnt \n";
print "  Files skipped : $skip_file_cnt \n";
print "  Files proessed: $proc_file_cnt \n";
print "  Matches       : $tot_match_cnt \n";



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
    my $line_cnt = 0;
    my $match_cnt = 0;
    open(INFO, $file);

    while (<INFO>) {
        $line_cnt++;
        if (replace_INTERNAL_INC($replaced = $_)) {
            print "  Line ",$line_cnt,": ",$_;
            print "      => :",$replaced;
            $match_cnt++;
        }
    }

    close(INFO);
    $tot_match_cnt += $match_cnt;
    return;
}


sub replace_INTERNAL_INC {
    $_[0] =~ s/(^#include\s+)(INTERNAL_INC\s*\()((\w|.)*)\)/$1\"rfgw\/$3\"/;
    return (defined($1));
}


