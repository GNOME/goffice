#!/usr/bin/perl -w
# -----------------------------------------------------------------------------
# Run the canvas tests without any display connection.
#
# The display independent parts must pass.  In a Gtk build the canvas widget
# tests are expected to report themselves as skipped, as Gtk cannot create a
# GtkLayout without a display.
#
# Usage: t2000-canvas-headless.pl [--valgrind]

use strict;
use lib ($0 =~ m|^(.*/)| ? $1 : ".");
use GofficeTest;

my $cmd = './test-canvas';
my $valgrind = 0;

foreach my $arg (@ARGV) {
    $valgrind = 1 if $arg eq '--valgrind';
}

&message ("Run the canvas tests without a display connection.");

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

delete $ENV{'DISPLAY'};
delete $ENV{'WAYLAND_DISPLAY'};

if ($valgrind) {
    &test_valgrind ($cmd, 1);
} else {
    my $output = `$cmd 2>&1`;
    my $err = $?;

    &dump_indented ($output);
    &system_failure ($cmd, $err) if $err;

    print STDERR "Pass\n";
}
