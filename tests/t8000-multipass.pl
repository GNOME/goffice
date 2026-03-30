#!/usr/bin/perl -w
# -----------------------------------------------------------------------------

use strict;
use lib ($0 =~ m|^(.*/)| ? $1 : ".");
use GofficeTest;

&message ("Check that SUFFIX is being used where it needs to be.");

my $res = system("$topsrc/tools/check-multipass");
die "Fail\n" if $res;

print STDERR "Pass\n";
