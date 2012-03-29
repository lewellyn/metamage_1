package Compile::Driver;

use Compile::Driver::Configuration;
use Compile::Driver::Job;
use Compile::Driver::Job::Compile;
use Compile::Driver::Job::Link::Archive;
use Compile::Driver::Job::Link::Binary;
use Compile::Driver::Module;
use Compile::Driver::Options;

use warnings;
use strict;


sub jobs_for
{
	my ( $module ) = @_;
	
	return if !$module->is_buildable;
	
	my @jobs;
	
	my $target = $module->target;
	
	my @sources = $module->sources;
	
	my $name = $module->name;
	
	my @objs;
	
	foreach my $path ( @sources )
	{
		my $obj = Compile::Driver::Job::obj_pathname( $target, $name, $path );
		
		push @objs, $obj;
		
		my $compile = Compile::Driver::Job::Compile::->new
		(
			TYPE => "CC",
			FROM => $module,
			PATH => $path,
			DEST => $obj,
		);
		
		push @jobs, $compile;
	}
	
	my $link;
	
	if ( $module->is_static_lib )
	{
		$link = Compile::Driver::Job::Link::Archive::->new
		(
			TYPE => "AR",
			FROM => $module,
			OBJS => \@objs,
			DEST => Compile::Driver::Job::lib_pathname( $target, $name ),
		);
	}
	elsif ( $module->is_executable )
	{
		$link = Compile::Driver::Job::Link::Binary::->new
		(
			TYPE => "LINK",
			FROM => $module,
			OBJS => \@objs,
			PREQ => $module->recursive_prerequisites,
			DEST => Compile::Driver::Job::bin_pathname( $target, $name, $module->program_name ),
		);
	}
	
	push @jobs, $link;
	
	return @jobs;
}

sub main
{
	my @args = Compile::Driver::Options::set_options( @_ );
	
	my $configuration = Compile::Driver::Configuration::->new( Compile::Driver::Options::specs );
	
	foreach my $name ( @args )
	{
		my $module = $configuration->get_module( $name, "[mandatory]" );
	}
	
	my $desc = { NAME => "[program args]", DATA => { use => [ @args ] } };
	
	my $module = Compile::Driver::Module::->new( $configuration, $desc );
	
	my @prereqs = @{ $module->recursive_prerequisites };
	
	pop @prereqs;
	
	my @jobs = map { Compile::Driver::jobs_for( $configuration->get_module( $_ ) ) } @prereqs;
	
	foreach my $job ( @jobs )
	{
		$job->perform;
	}
	
}

1;
