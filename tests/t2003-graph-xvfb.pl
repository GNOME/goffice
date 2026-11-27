#!/usr/bin/perl -w
# -----------------------------------------------------------------------------
# Run the graph tests against a virtual display.
#
# This runs the display-dependent part of test-graph (Gtk dialogs and
# widgets); those tests never synthesize events, they only build, realize
# and destroy widgets.  Xvfb is preferred, with Xephyr and then Broadway
# (broadwayd) as fallbacks; see start_virtual_display in GofficeTest.pm.
# Xephyr needs a parent DISPLAY.  The test is skipped when no virtual
# display can be started here.  On success the display tests must actually
# have run; silently skipping them is a failure.
#
# Usage: t2003-graph-xvfb.pl [--valgrind]

use strict;
use lib ($0 =~ m|^(.*/)| ? $1 : ".");
use GofficeTest;
use Cwd;

my $cmd = './test-graph';
my $valgrind = 0;

foreach my $arg (@ARGV) {
    $valgrind = 1 if $arg eq '--valgrind';
}

&message ("Run the graph tests against a virtual display.");

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

&start_virtual_display ();

if ($valgrind) {
    &test_valgrind ($cmd, 1);
} else {
    my $output = `$cmd 2>&1`;
    my $err = $?;

    &dump_indented ($output);
    &system_failure ($cmd, $err) if $err;

    die "the graph display tests were skipped despite the virtual display\n"
	if $output =~ /skipped graph display tests/;

    print STDERR "Pass\n";
}
