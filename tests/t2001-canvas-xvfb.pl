#!/usr/bin/perl -w
# -----------------------------------------------------------------------------
# Run the canvas tests against a virtual X server.
#
# Xvfb is preferred, with Xephyr as a fallback.  Xephyr needs a parent
# DISPLAY.  The test is skipped when neither can be started here.  On
# success the canvas widget tests must actually have run; silently skipping
# them is a failure.
#
# Usage: t2001-canvas-xvfb.pl [--valgrind]

use strict;
use lib ($0 =~ m|^(.*/)| ? $1 : ".");
use GofficeTest;
use Fcntl;
use IO::Select;

my $cmd = './test-canvas';
my $valgrind = 0;
my $server_pid = 0;
my $server_log = 'virtual-x-server.log';

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
    if ($server_pid) {
	kill 'TERM', $server_pid;
	waitpid ($server_pid, 0);
    }
    unlink $server_log;
}

sub server_log_text {
    return -r $server_log ? GofficeTest::read_file ($server_log) : '';
}

# Start an X server that reports its display number on a pipe.
sub start_x_server {
    my ($name, $program, @args) = @_;

    # The server needs a place to put its unix socket.
    mkdir '/tmp/.X11-unix' unless -d '/tmp/.X11-unix';

    # Keep the server chatter in a log file; it is only interesting when the
    # server fails to come up.
    open (my $log, '>', $server_log) or die "Cannot create $server_log: $!\n";

    # Let the server pick a free display and report the number back to us
    # when it is ready to accept connections.
    my ($rfd, $wfd);
    pipe ($rfd, $wfd) or die "Cannot create a pipe: $!\n";
    fcntl ($wfd, F_SETFD, 0) or die "Cannot clear close-on-exec: $!\n";

    $server_pid = fork ();
    die "Cannot fork: $!\n" unless defined $server_pid;
    if ($server_pid == 0) {
	close $rfd;
	open (STDOUT, '>&', $log);
	open (STDERR, '>&', $log);
	exec ($program, '-displayfd', fileno ($wfd), @args);
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
	kill 'TERM', $server_pid;
	waitpid ($server_pid, 0);
	$server_pid = 0;
	return (undef, "$name failed to start", &server_log_text ());
    }

    return ($display, undef, undef);
}

sub start_xvfb {
    my $xvfb = &find_program ('Xvfb', 1);
    return (undef, 'Xvfb is not available', undef) unless defined $xvfb;

    # GLX is disabled: widget creation does not need it, and some drivers
    # crash the server while initializing it.
    return &start_x_server ('Xvfb', $xvfb,
			    '-screen', '0', '1280x1024x24',
			    '-extension', 'GLX', '-nolisten', 'tcp');
}

sub start_xephyr {
    my $xephyr = &find_program ('Xephyr', 1);
    return (undef, 'Xephyr is not available', undef) unless defined $xephyr;

    # Xephyr is nested, so it needs the inherited DISPLAY of a parent server.
    return (undef, 'Xephyr needs a parent DISPLAY', undef)
	unless defined $ENV{'DISPLAY'} && $ENV{'DISPLAY'} ne '';

    return &start_x_server ('Xephyr', $xephyr,
			    '-screen', '1280x1024x24',
			    '-extension', 'GLX', '-nolisten', 'tcp');
}

my ($display, @errors, @logs);
foreach my $start (\&start_xvfb, \&start_xephyr) {
    my ($got_display, $error, $log) = &$start ();
    if (defined $got_display) {
	$display = $got_display;
	last;
    }
    push @errors, $error if defined $error;
    push @logs, $log if defined $log && $log ne '';
}

if (!defined $display) {
    &dump_indented (join ('', @logs));
    &report_skip (@errors ? join ('; ', @errors)
		  : 'no virtual X server is available');
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
