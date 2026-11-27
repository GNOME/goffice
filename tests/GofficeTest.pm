package GofficeTest;
use strict;
use Exporter;
use File::Basename qw(fileparse);
use Config;
use XML::Parser;
use Fcntl;
use POSIX ();
use IO::Select;
use IO::Socket::INET;
use File::Temp ();
use File::Path ();

$| = 1;

@GofficeTest::ISA = qw (Exporter);
@GofficeTest::EXPORT = qw(message test_valgrind
			 test_zip
			 find_program report_skip start_virtual_display
			 dump_indented system_failure
			 $topsrc $top_builddir
			 $gsf $PERL);
@GofficeTest::EXPORT_OK = qw(junkfile);

use vars qw($topsrc $top_builddir $PERL $verbose);

$PERL = $Config{'perlpath'};
$PERL .= $Config{'_exe'} if $^O ne 'VMS' && $PERL !~ m/$Config{'_exe'}$/i;

$topsrc = $0;
$topsrc =~ s|/[^/]+$|/..|;
$topsrc =~ s|^\./(.)|$1|;
$topsrc =~ s|/tests/\.\.$||;

$top_builddir = "..";
$verbose = 0;

# -----------------------------------------------------------------------------

my @tempfiles;
END {
    unlink @tempfiles;
}

sub junkfile {
    my ($fn) = @_;
    push @tempfiles, $fn;
}

sub removejunk {
    my ($fn) = @_;
    unlink $fn;

    if (@tempfiles && $fn eq $tempfiles[-1]) {
	scalar (pop @tempfiles);
    }
}

# -----------------------------------------------------------------------------

sub system_failure {
    my ($program,$code) = @_;

    if ($code == -1) {
	die "failed to run $program: $!\n";
    } elsif ($code >> 8) {
	my $sig = $code >> 8;
	die "$program died due to signal $sig\n";
    } else {
	die "$program exited with exit code $code\n";
    }
}

sub read_file {
    my ($fn) = @_;

    local (*FIL);
    open (FIL, $fn) or die "Cannot open $fn: $!\n";
    local $/ = undef;
    my $lines = <FIL>;
    close FIL;

    return $lines;
}

sub write_file {
    my ($fn,$contents) = @_;

    local (*FIL);
    open (FIL, ">$fn.tmp") or die "Cannot create $fn.tmp: $!\n";
    print FIL $contents;
    close FIL;
    rename "$fn.tmp", $fn;
}

sub update_file {
    my ($fn,$contents) = @_;

    my @stat = stat $fn;
    die "Cannot stat $fn: $!\n" unless @stat > 2;

    &write_file ($fn,$contents);

    chmod $stat[2], $fn or
	die "Cannot chmod $fn: $!\n";
}

# Print a string with each line prefixed by "| ".
sub dump_indented {
    my ($txt) = @_;
    return if $txt eq '';
    $txt =~ s/^/| /gm;
    $txt = "$txt\n" unless substr($txt, -1) eq "\n";
    print STDERR $txt;
}

sub find_program {
    my ($p,$qoptional) = @_;

    if ($p =~ m{/}) {
	return $p if -x $p;
    } else {
	my $PATH = exists $ENV{'PATH'} ? $ENV{'PATH'} : '';
	foreach my $dir (split (':', $PATH)) {
	    $dir = '.' if $dir eq '';
	    my $tentative = "$dir/$p";
	    return $tentative if -x $tentative;
	}
    }

    return undef if $qoptional;

    &report_skip ("$p is missing");
}

# -----------------------------------------------------------------------------

sub message {
    my ($message) = @_;
    print "-" x 79, "\n";
    my $me = $0;
    $me =~ s|^.*/||;
    foreach (split (/\n/, $message)) {
	print "$me: $_\n";
    }
}

# -----------------------------------------------------------------------------

sub test_command {
    my ($cmd,$test) = @_;

    my $output = `$cmd 2>&1`;
    my $err = $?;
    die "Failed command: $cmd\n" if $err;

    &dump_indented ($output);
    local $_ = $output;
    if (&$test ($output)) {
	print STDERR "Pass\n";
    } else {
	die "Fail\n";
    }
}

# -----------------------------------------------------------------------------

sub test_valgrind {
    my ($cmd,$uselibtool) = @_;

    local (%ENV) = %ENV;
    $ENV{'G_DEBUG'} .= ':gc-friendly:resident-modules';
    $ENV{'G_SLICE'} = 'always-malloc';
    delete $ENV{'VALGRIND_OPTS'};
    # Keep valgrind from hanging while trying to fetch debug info.
    $ENV{'DEBUGINFOD_URLS'} = '';

    # Per-wrapper log so parallel test runs do not clobber each other.
    my $outfile = $0;
    $outfile =~ s|.*/||;
    $outfile =~ s/\.pl$//;
    $outfile .= '-valgrind.log';
    unlink $outfile;
    die "Cannot remove $outfile.\n" if -f $outfile;
    &junkfile ($outfile);

    my $valhelp = `valgrind --help 2>&1`;
    &report_skip ("Valgrind is not available") unless defined $valhelp;
    die "Problem running valgrind.\n" unless $valhelp =~ /log-file/;

    my $valvers = `valgrind --version`;
    die "Problem running valgrind.\n"
	unless $valvers =~ /^valgrind-(\d+)\.(\d+)\.(\d+)/;
    $valvers = $1 * 10000 + $2 * 100 + $3;
    &report_skip ("Valgrind is too old") unless $valvers >= 30500;

    $cmd = "--gen-suppressions=all $cmd";

    {
	my $suppfile = "$topsrc/tests/common.supp";
	&report_skip ("file $suppfile does not exist") unless -r $suppfile;
	$cmd = "--suppressions=$suppfile $cmd" if -r $suppfile;
    }

    {
	my $suppfile = $0;
	$suppfile =~ s/\.pl$/.supp/;
	$cmd = "--suppressions=$suppfile $cmd" if -r $suppfile;
    }

    # $cmd = "--show-reachable=yes $cmd";
    $cmd = "--show-below-main=yes $cmd";
    $cmd = "--leak-check=full $cmd";
    $cmd = "--num-callers=20 $cmd";
    $cmd = "--track-fds=yes $cmd";
    if ($valhelp =~ /--log-file-exactly=/) {
	$cmd = "--log-file-exactly=$outfile $cmd";
    } else {
	$cmd = "--log-file=$outfile $cmd";
    }
    $cmd = "valgrind $cmd";
    $cmd = "../libtool --mode=execute $cmd" if $uselibtool;

    my $code = system ($cmd);
    &system_failure ('valgrind', $code) if $code;

    my $txt = &read_file ($outfile);
    &removejunk ($outfile);
    # A client that forks makes the child run under valgrind too, sharing
    # the log file, so use the last summary, not the first.
    my @errors = ($txt =~ /ERROR\s+SUMMARY:\s*(\d+)\s+errors?/ig);
    my $errors = @errors ? $errors[-1] : -1;
    if ($errors == 0) {
	&dump_indented ($txt) if $verbose;
	print STDERR "Pass\n";
	return;
    }

    &dump_indented ($txt);
    die "Fail\n";
}

# -----------------------------------------------------------------------------

sub quotearg {
    return join (' ', map { &quotearg1 ($_) } @_);
}

sub quotearg1 {
    my ($arg) = @_;

    return "''" if $arg eq '';
    my $res = '';
    while ($arg ne '') {
	if ($arg =~ m!^([-=/._a-zA-Z0-9]+)!) {
	    $res .= $1;
	    $arg = substr ($arg, length $1);
	} else {
	    $res .= "\\" . substr ($arg, 0, 1);
	    $arg = substr ($arg, 1);
	}
    }
    return $res;
}

# -----------------------------------------------------------------------------

sub report_skip {
    my ($txt) = @_;

    print "SKIP -- $txt\n";
    # 77 is magic for automake
    exit 77;
}

# -----------------------------------------------------------------------------
# Virtual displays, for the tests that need real GTK widgets.
#
# start_virtual_display() starts the first of Xvfb, Xephyr and Broadway
# (broadwayd) that works, and points the environment at it so that child
# processes use it.  It skips the whole test (exit 77) when none can be
# started.  The server is stopped, and its files removed, when the script
# ends.

my $vd_owner = 0;			# pid that started the server
my $vd_pid = 0;				# the server, 0 when not running
my $vd_log = 'virtual-x-server.log';	# its output
my $vd_dir;				# private runtime directory (Broadway)

END {
    # Keep the exit status; waitpid() would clobber $? and mask a
    # valgrind induced die.
    local $?;
    if ($vd_owner == $$) {
	&_vd_stop ();
	unlink $vd_log;
	File::Path::rmtree ($vd_dir) if defined $vd_dir;
    }
}

sub _vd_stop {
    return unless $vd_pid;
    kill 'TERM', $vd_pid;
    waitpid ($vd_pid, 0);
    $vd_pid = 0;
}

# True if the server has exited (and has been reaped).
sub _vd_exited {
    return 1 unless $vd_pid;
    my $r = waitpid ($vd_pid, POSIX::WNOHANG ());
    return 0 unless $r == $vd_pid || $r == -1;
    $vd_pid = 0;
    return 1;
}

sub _vd_log_text {
    return -r $vd_log ? &read_file ($vd_log) : '';
}

# Run a server with its output going to the log file.  CHILD_SETUP, if
# given, runs in the child just before exec.  Returns the pid.
sub _vd_spawn {
    my ($child_setup, @cmd) = @_;

    open (my $log, '>', $vd_log) or die "Cannot create $vd_log: $!\n";
    $vd_owner = $$;
    my $pid = fork ();
    die "Cannot fork: $!\n" unless defined $pid;
    if ($pid == 0) {
	open (STDOUT, '>&', $log);
	open (STDERR, '>&', $log);
	&$child_setup () if $child_setup;
	{
	    no warnings 'exec';
	    exec { $cmd[0] } @cmd;
	}
	# _exit, so that the child does not run our END block.
	POSIX::_exit (127);
    }
    close $log;
    return $pid;
}

# Each starter returns (\%env, $description) on success, where %env gives
# the environment to use (an undefined value means "unset it"), and
# (undef, $error, $server_log) on failure.

# Start an X server that reports its display number on a pipe.
sub _vd_x_server {
    my ($name, $program, @args) = @_;

    # The server needs a place to put its unix socket.
    mkdir '/tmp/.X11-unix' unless -d '/tmp/.X11-unix';

    # Let the server pick a free display and report the number back to us
    # when it is ready to accept connections.
    my ($rfd, $wfd);
    pipe ($rfd, $wfd) or die "Cannot create a pipe: $!\n";
    fcntl ($wfd, F_SETFD, 0) or die "Cannot clear close-on-exec: $!\n";

    $vd_pid = &_vd_spawn (sub { close $rfd },
			  $program, '-displayfd', fileno ($wfd), @args);
    close $wfd;

    my $display;
    {
	my $sel = IO::Select->new ($rfd);
	my $buf = '';
	my $deadline = time () + 20;

	# Some servers write the number and the terminating newline
	# separately.  Read until the newline (or EOF): closing our end
	# after the first piece would make the server's second write fail
	# with EPIPE, which is fatal to it.
	while ($buf !~ /\n/ && (my $left = $deadline - time ()) > 0) {
	    last unless $sel->can_read ($left);
	    my $n = sysread ($rfd, $buf, 16, length $buf);
	    last unless $n;
	}
	$display = $1 if $buf =~ /^(\d+)/;
    }
    close $rfd;

    if (!defined $display) {
	my $log = &_vd_log_text ();
	&_vd_stop ();
	return (undef, "$name failed to start", $log);
    }

    return ({ DISPLAY => ":$display",
	      WAYLAND_DISPLAY => undef,
	      GDK_BACKEND => 'x11' },
	    "$name on :$display");
}

sub _vd_xvfb {
    my $xvfb = &find_program ('Xvfb', 1);
    return (undef, 'Xvfb is not available', undef) unless defined $xvfb;

    # GLX is disabled: widget creation does not need it, and some drivers
    # crash the server while initializing it.
    return &_vd_x_server ('Xvfb', $xvfb,
			  '-screen', '0', '1280x1024x24',
			  '-extension', 'GLX', '-nolisten', 'tcp');
}

sub _vd_xephyr {
    my $xephyr = &find_program ('Xephyr', 1);
    return (undef, 'Xephyr is not available', undef) unless defined $xephyr;

    # Xephyr is nested, so it needs the inherited DISPLAY of a parent server.
    return (undef, 'Xephyr needs a parent DISPLAY', undef)
	unless defined $ENV{'DISPLAY'} && $ENV{'DISPLAY'} ne '';

    return &_vd_x_server ('Xephyr', $xephyr,
			  '-screen', '1280x1024x24',
			  '-extension', 'GLX', '-nolisten', 'tcp');
}

# A TCP port that is free right now.  Somebody else can grab it before we
# use it, so callers must be prepared to retry.
sub _vd_free_port {
    my ($address) = @_;

    my $sock = IO::Socket::INET->new (Listen => 1, Proto => 'tcp',
				      LocalAddr => $address, LocalPort => 0,
				      ReuseAddr => 1) or return undef;
    my $port = $sock->sockport ();
    close $sock;
    return $port;
}

# Wait for a freshly started broadwayd.  It has no way of telling us that
# it is ready, so wait for it to announce its unix socket and for its http
# port to accept connections.  (The socket is an abstract one on Linux, so
# there is no file to look for.)
sub _vd_broadway_ready {
    my ($address, $port) = @_;
    my $deadline = time () + 20;

    while (time () < $deadline) {
	return 0 if &_vd_exited ();
	my $txt = &_vd_log_text ();
	return 0 if $txt =~ /Unable to listen/;

	if ($txt =~ /^Listening on /m) {
	    my $c = IO::Socket::INET->new (PeerAddr => $address,
					   PeerPort => $port,
					   Proto => 'tcp', Timeout => 1);
	    if ($c) {
		close $c;
		# The port could belong to a stranger that grabbed it
		# before we did.  If so, broadwayd complains and exits
		# right away.
		select (undef, undef, undef, 0.2);
		return 0 if &_vd_exited ();
		return &_vd_log_text () !~ /Unable to listen/;
	    }
	}
	select (undef, undef, undef, 0.1);
    }
    return 0;
}

# Broadway is GTK's own display backend and needs no X server.  Two things
# make it more work than the X servers:
#
# * It always listens on http port 8080 on all addresses, which would
#   break parallel test runs.  We give it a private port on the loopback
#   interface instead.  (Port 0 does not mean "any" to it.)
#
# * Clients find it through a unix socket whose name is derived from
#   $XDG_RUNTIME_DIR, so use a private directory of our own.  The display
#   name then is always :0, and parallel runs do not see each other.
sub _vd_broadway {
    my $bw = &find_program ('broadwayd', 1) ||
	&find_program ('gtk3-broadwayd', 1);
    return (undef, 'broadwayd is not available', undef) unless defined $bw;

    $vd_dir = File::Temp::tempdir ('goffice-bw-XXXXXX', TMPDIR => 1);

    my $address = '127.0.0.1';
    my $log = '';
    foreach my $attempt (1 .. 3) {
	my $port = &_vd_free_port ($address);
	return (undef, 'no free TCP port for broadwayd', undef)
	    unless defined $port;

	$vd_pid = &_vd_spawn (sub { $ENV{'XDG_RUNTIME_DIR'} = $vd_dir },
			      $bw, ':0', "--address=$address", "--port=$port");
	return ({ XDG_RUNTIME_DIR => $vd_dir,
		  BROADWAY_DISPLAY => ':0',
		  GDK_BACKEND => 'broadway',
		  DISPLAY => undef,
		  WAYLAND_DISPLAY => undef },
		"broadwayd on port $port")
	    if &_vd_broadway_ready ($address, $port);

	$log = &_vd_log_text ();
	&_vd_stop ();
	# Only losing the race for the port is worth another try.
	last unless $log =~ /Unable to listen/;
    }
    return (undef, 'broadwayd failed to start', $log);
}

sub start_virtual_display {
    my (@errors, @logs);

    foreach my $start (\&_vd_xvfb, \&_vd_xephyr, \&_vd_broadway) {
	my ($env, $what, $log) = &$start ();

	if ($env) {
	    foreach my $key (keys %$env) {
		if (defined $env->{$key}) {
		    $ENV{$key} = $env->{$key};
		} else {
		    delete $ENV{$key};
		}
	    }
	    # The canberra sound module forks for PulseAudio when a Gdk
	    # event sound plays, which confuses valgrind's per-process
	    # accounting.  Tests have no business making noise anyway.
	    delete $ENV{'GTK_MODULES'};

	    print "Virtual display: $what\n";
	    return $what;
	}

	push @errors, $what if defined $what;
	push @logs, $log if defined $log && $log ne '';
    }

    &dump_indented (join ('', @logs));
    &report_skip (@errors ? join ('; ', @errors)
		  : 'no virtual display is available');
}

# -----------------------------------------------------------------------------
# Setup a consistent environment

&report_skip ("all tests skipped") if exists $ENV{'GOFFICE_SKIP_TESTS'};

delete $ENV{'G_SLICE'};
$ENV{'G_DEBUG'} = 'fatal_criticals';

delete $ENV{'LANG'};
delete $ENV{'LANGUAGE'};
foreach (keys %ENV) { delete $ENV{$_} if /^LC_/; }
$ENV{'LC_ALL'} = 'C';

# libgsf listens for this
delete $ENV{'WINDOWS_LANGUAGE'};

if (@ARGV && $ARGV[0] eq '--verbose') {
    $verbose = 1;
    scalar shift @ARGV;
}

1;
