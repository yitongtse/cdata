#! /usr/local/bin/perl


# This is a directory traverse routine
#
sub traverse {
    my ($dir, $callback) = @_;
    my $present;

    opendir $present, $dir or return();

    for (grep { ! /^(\.|\.\.)$/ } readdir $present) {
        my $path = "$dir/$_";

        &$callback($path);
        traverse($path, $callback) if -d $path;
    }

    closedir $present;
    return();
}


sub file_proc {
  my $file = shift;
  if (-d $file) {
    print "Dir : $file\n";
    $dir_cnt++;
  } else {
    print "File: $file\n";
    $file_cnt++;
  }
  return;
}

traverse("/vob/ios.comp/rfgw/lc/", \&file_proc);

print "Files: $file_cnt \n";
print "Dirs : $dir_cnt \n";

