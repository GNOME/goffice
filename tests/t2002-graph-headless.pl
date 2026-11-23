#!/usr/bin/perl -w
# -----------------------------------------------------------------------------
# Run the graph tests without any display connection.
#
# The graph core (model, geometry, rendering to an image surface) is
# display independent.  Plot-type plugins are loaded from the build tree,
# so they must have been built already: tests are run after the plugins
# directory in SUBDIRS order.
#
# Usage: t2002-graph-headless.pl [--valgrind]

use strict;
use lib ($0 =~ m|^(.*/)| ? $1 : ".");
use GofficeTest;
use Cwd;

my $cmd = './test-graph';
my $valgrind = 0;

foreach my $arg (@ARGV) {
    $valgrind = 1 if $arg eq '--valgrind';
}

&message ("Run the graph tests without a display connection.");

if (!-x $cmd) {
    my $dir = $0 =~ m|^(.*/)| ? $1 : './';
    my $base = $0;

    die "test-graph is not built; run make first.\n" unless -x "${dir}test-graph";
    $base =~ s|^.*/||;
    chdir $dir or die "Cannot change to $dir: $!\n";
    # Pretend we were started from the build directory so that the
    # relative paths in GofficeTest keep working.
    $0 = "./$base";
    $topsrc = '..';
}

delete $ENV{'DISPLAY'};
delete $ENV{'WAYLAND_DISPLAY'};

# Plot plugins come from the source tree (plugin.xml descriptors) and the
# build tree (modules); in an in-tree build the two coincide.  Automake
# supplies the directories from the Makefile; fall back on the paths
# relative to the tests directory.
{
    my $srcdir = exists $ENV{'GOFFICE_TEST_SRCDIR'}
	? $ENV{'GOFFICE_TEST_SRCDIR'} : $topsrc;
    my $builddir = exists $ENV{'GOFFICE_TEST_BUILDDIR'}
	? $ENV{'GOFFICE_TEST_BUILDDIR'} : $top_builddir;
    my @dirs;
    my %seen;

    foreach my $d ("$srcdir/plugins", "$builddir/plugins") {
	my $key = -d $d ? Cwd::abs_path ($d) : $d;
	next if $seen{$key}++;
	push @dirs, $d;
    }
    $ENV{'GOFFICE_TEST_PLUGIN_DIRS'} = join (':', @dirs);
}

if ($valgrind) {
    $ENV{'GOFFICE_TEST_UNDER_VALGRIND'} = 1;
    &test_valgrind ($cmd, 1);
} else {
    my $output = `$cmd 2>&1`;
    my $err = $?;

    &dump_indented ($output);
    &system_failure ($cmd, $err) if $err;

    die "the graph tests were skipped despite plugins being built\n"
	if $output =~ /skipped plot plugins/;

    print STDERR "Pass\n";
}
