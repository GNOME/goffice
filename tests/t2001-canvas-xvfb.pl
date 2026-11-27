#!/usr/bin/perl -w
# -----------------------------------------------------------------------------
# Run the canvas tests against a virtual display.
#
# Xvfb is preferred, with Xephyr and then Broadway (broadwayd) as
# fallbacks; see start_virtual_display in GofficeTest.pm.  Xephyr needs a
# parent DISPLAY.  The test is skipped when no virtual display can be
# started here.  On success the canvas widget tests must actually have run;
# silently skipping them is a failure.
#
# Usage: t2001-canvas-xvfb.pl [--valgrind]

use strict;
use lib ($0 =~ m|^(.*/)| ? $1 : ".");
use GofficeTest;

my $cmd = './test-canvas';
my $valgrind = 0;

foreach my $arg (@ARGV) {
    $valgrind = 1 if $arg eq '--valgrind';
}

&message ("Run the canvas tests against a virtual display.");

if (!-x $cmd) {
    my $dir = $0 =~ m|^(.*/)| ? $1 : './';
    my $base = $0;

    die "test-canvas is not built; run make first.\n" unless -x "${dir}test-canvas";
    $base =~ s|^.*/||;
    chdir $dir or die "Cannot change to $dir: $!\n";
    # Pretend we were started from the build directory so that the
    # relative paths in GofficeTest keep working.
    $0 = "./$base";
    $topsrc = '..';
}

&start_virtual_display ();

if ($valgrind) {
    &test_valgrind ($cmd, 1);
} else {
    my $output = `$cmd 2>&1`;
    my $err = $?;

    &dump_indented ($output);
    &system_failure ($cmd, $err) if $err;

    die "the canvas tests were skipped despite the virtual display\n"
	if $output =~ /skipped canvas tests/;

    print STDERR "Pass\n";
}
