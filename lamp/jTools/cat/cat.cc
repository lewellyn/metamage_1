/*	======
 *	cat.cc
 *	======
 */

// POSIX
#include <fcntl.h>
#include <unistd.h>

// poseven
#include "poseven/extras/pump.hh"
#include "poseven/functions/open.hh"
#include "poseven/functions/perror.hh"

// Orion
#include "Orion/Main.hh"


namespace tool
{
	
	namespace n = nucleus;
	namespace p7 = poseven;
	
	
	static bool PathnameMeansStdIn( const char* pathname )
	{
		return    pathname[0] == '-'
			   && pathname[1] == '\0';
	}
	
	static const char* EvaluateMetaFilename( const char* pathname )
	{
		if ( PathnameMeansStdIn( pathname ) )
		{
			return "/dev/fd/0";
		}
		
		return pathname;
	}
	
	int Main( int argc, char** argv )
	{
		const char* argv0 = argv[0];
		
		const char *const * args = argv + 1;
		
		// Check for sufficient number of args
		if ( *args == NULL )
		{
			static const char *const default_args[] = { "-", NULL };
			
			args = default_args;
		}
		
		// Print each file in turn.  Return whether any errors occurred.
		int exit_status = 0;
		
		while ( *args != NULL )
		{
			const char* pathname = EvaluateMetaFilename( *args++ );
			
			try
			{
				n::owned< p7::fd_t > fd = p7::open( pathname, p7::o_rdonly );
				
				p7::pump( fd, p7::stdout_fileno );
			}
			catch ( const p7::errno_t& error )
			{
				p7::perror( argv0, pathname, error );
				
				exit_status = 1;
				
				continue;
			}
		}
		
		return exit_status;
	}
	
}

