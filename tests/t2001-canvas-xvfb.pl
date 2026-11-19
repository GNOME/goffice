#!/usr/bin/perl -w
# -----------------------------------------------------------------------------
# Run the canvas tests against a virtual X server.
#
# The test is skipped when Xvfb is not available, and also when Xvfb cannot
# bring up a server here.  On success the canvas widget tests must actually
# have run; silently skipping them is a failure.
#
# Usage: t2001-canvas-xvfb.pl [--valgrind]

use strict;
use lib ($0 =~ m|^(.*/)| ? $1 : ".");
use GofficeTest;
use Fcntl;
use IO::Select;

my $cmd = './test-canvas';
my $valgrind = 0;
my $xvfb_pid = 0;
my $xvfb_log = 'xvfb.log';

foreach my $arg (@ARGV) {
    $valgrind = 1 if $arg eq '--valgrind';
}

&message ("Run the canvas tests against a virtual X server.");

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

END {
    # Keep the exit status; waitpid() would clobber $? and mask a
    # valgrind induced die.
    local $?;
    if ($xvfb_pid) {
	kill 'TERM', $xvfb_pid;
	waitpid ($xvfb_pid, 0);
    }
    unlink $xvfb_log;
}

my $xvfb = &find_program ('Xvfb', 1);
&report_skip ("Xvfb is not available") unless defined $xvfb;

# The server needs a place to put its unix socket.
mkdir '/tmp/.X11-unix' unless -d '/tmp/.X11-unix';

# Keep the server chatter in a log file; it is only interesting when the
# server fails to come up.
open (my $log, '>', $xvfb_log) or die "Cannot create $xvfb_log: $!\n";

# Let Xvfb pick a free display and report the number back to us when it is
# ready to accept connections.
my ($rfd, $wfd);
pipe ($rfd, $wfd) or die "Cannot create a pipe: $!\n";
fcntl ($wfd, F_SETFD, 0) or die "Cannot clear close-on-exec: $!\n";

$xvfb_pid = fork ();
die "Cannot fork: $!\n" unless defined $xvfb_pid;
if ($xvfb_pid == 0) {
    close $rfd;
    open (STDOUT, '>&', $log);
    open (STDERR, '>&', $log);
    # GLX is disabled: widget creation does not need it, and some drivers
    # crash the server while initializing it.
    exec ($xvfb, '-displayfd', fileno ($wfd),
	  '-screen', '0', '1280x1024x24',
	  '-extension', 'GLX', '-nolisten', 'tcp');
    exit 127;
}
close $wfd;
close $log;

my $display;
{
    my $sel = IO::Select->new ($rfd);
    my $buf = '';

    if ($sel->can_read (20) &&
	defined (sysread ($rfd, $buf, 16)) &&
	$buf =~ /(\d+)/) {
	$display = $1;
    }
}
close $rfd;

if (!defined $display) {
    kill 'TERM', $xvfb_pid;
    waitpid ($xvfb_pid, 0);
    $xvfb_pid = 0;
    &dump_indented (GofficeTest::read_file ($xvfb_log));
    &report_skip ("Xvfb failed to start");
}

$ENV{'DISPLAY'} = ":$display";
delete $ENV{'WAYLAND_DISPLAY'};
$ENV{'GDK_BACKEND'} = 'x11';

if ($valgrind) {
    &test_valgrind ($cmd, 1);
} else {
    my $output = `$cmd 2>&1`;
    my $err = $?;

    &dump_indented ($output);
    &system_failure ($cmd, $err) if $err;

    die "the canvas tests were skipped despite the X server\n"
	if $output =~ /skipped canvas tests/;

    print STDERR "Pass\n";
}
