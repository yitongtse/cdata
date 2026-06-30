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


sub textP {
  my $file = shift;
  if (-T $file) {
    print "[$file] is a text file\n";
  } else {
    print "[$file] is NOT a text file\n";
  }
  return;
}

traverse("/vob/ios.comp/rfgw/lc/mv", \&textP);

