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
        print "Directory: $node\n";
        $dir_cnt++;
    } else {
        if ($node =~ /[ch]$/) {
            print "File: $node\n";
            file_process($node);
            $proc_file_cnt++;
        } else {
            print "File: $node (skipped)\n";
            $skip_file_cnt++;
        }
    }
    return;
}


sub file_process {
    my $file = shift;
    my $line_cnt = 0;
    my $match_cnt = 0;
    open(IN, $file);
    open(OUT, "> $file.tmp");

    while (<IN>) {
        $line_cnt++;
        $replaced = $_;
        if (replace_INTERNAL_INC($replaced) ||
            replace_COMP_INC($replaced)) {

            print "  Line ",$line_cnt,": ",$_;
            print "  => ",$replaced;
            $match_cnt++;
        }
        print OUT $replaced;
    }

    close(IN);
    close(OUT);

    if ($match_cnt > 0) {
        system("cleartool co -nc $file") == 0
            or print "Failed to check out file $file\n";
        system("mv $file.tmp $file") == 0
            or print "Failed to replace original file $file\n";
    } else {
        system("rm $file.tmp") == 0
            or print "Failed to remove temp file $file.tmp\n";
    }

    $tot_match_cnt += $match_cnt;
    return;
}


sub replace_INTERNAL_INC {
    $_[0] =~ s/(^#include\s+)(INTERNAL_INC\s*\()([\w.-_\/]+)\)/$1\"rfgw\/$3\"/;
    return (defined($1));
}


sub replace_COMP_INC {
    $_[0] =~ s/(^#include\s+)(COMP_INC\s*\(\s*)([\w.-_]+)(\s*,\s*)([\w.-_]+)\)/$1\<$3\/include\/$5\>/;
    return (defined($1));
}

