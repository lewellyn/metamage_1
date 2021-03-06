/*
	process.cc
	----------
*/

#include "relix/task/process.hh"

// relix
#include "relix/task/process_group.hh"
#include "relix/task/process_image.hh"
#include "relix/task/process_resources.hh"
#include "relix/task/signal_handlers.hh"


namespace relix
{
	
	process::process()
	:
		its_id           ( 1 ),
		its_ppid         ( 0 ),
		its_name( "init" ),
		its_last_activity(   ),
		its_process_group( new process_group( 1 ) ),
		its_process_image( new process_image() ),
		its_process_resources( new process_resources() ),
		its_signal_handlers( signal_handlers::create() )
	{
		// Reset resource utilization on fork
		
		its_times.tms_utime  = 0;
		its_times.tms_stime  = 0;
		its_times.tms_cutime = 0;
		its_times.tms_cstime = 0;
	}
	
	process::process( int id, process& parent )
	:
		its_id           ( id   ),
		its_ppid         ( parent.id() ),
		its_name( parent.name() ),
		its_last_activity(      ),
		its_process_group( &parent.get_process_group() ),
		its_process_image( &parent.get_process_image() ),
		its_process_resources( new process_resources( parent.get_process_resources() ) ),
		its_signal_handlers( parent.its_signal_handlers )
	{
		// Reset resource utilization on fork
		
		its_times.tms_utime  = 0;
		its_times.tms_stime  = 0;
		its_times.tms_cutime = 0;
		its_times.tms_cstime = 0;
	}
	
	process::~process()
	{
	}
	
	void process::accumulate_child_times( const struct tms& times )
	{
		its_times.tms_cutime += times.tms_utime + times.tms_cutime;
		its_times.tms_cstime += times.tms_stime + times.tms_cstime;
	}
	
	const plus::string& process::get_cmdline() const
	{
		
		return its_process_image.get() ? its_process_image.get()->get_cmdline()
		                               : plus::string::null;
	}
	
	process_group& process::get_process_group() const
	{
		return *its_process_group;
	}
	
	process_image& process::get_process_image() const
	{
		return *its_process_image;
	}
	
	process_rsrcs& process::get_process_resources() const
	{
		return *its_process_resources;
	}
	
	void process::set_process_group( process_group& pg )
	{
		its_process_group = &pg;
	}
	
	void process::set_process_image( process_image& image )
	{
		its_process_image = &image;
	}
	
	void process::reset_process_image()
	{
		its_process_image.reset();
	}
	
	void process::reset_process_resources()
	{
		its_process_resources.reset();
	}
	
	const struct sigaction& process::get_sigaction( int signo ) const
	{
		return its_signal_handlers->get( signo - 1 );
	}
	
	void process::set_sigaction( int signo, const struct sigaction& action )
	{
		its_signal_handlers->set( signo - 1, action );
	}
	
	void process::unshare_signal_handlers()
	{
		its_signal_handlers = duplicate( *its_signal_handlers );
	}
	
	void process::reset_signal_handlers()
	{
		its_signal_handlers->reset_handlers();
	}
	
}

