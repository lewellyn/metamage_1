/*	========
 *	touch.cc
 *	========
 */

// POSIX
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <utime.h>

// more-posix
#include "more/perror.hh"


#pragma exceptions off


int main( int argc, char *const *argv )
{
	// Check for sufficient number of args
	if ( argc < 2 )
	{
		more::perror( "touch", "missing arguments", 0 );
		
		return 1;
	}
	
	// Try to touch each item.  Return whether any errors occurred.
	int exit_status = 0;
	
	for ( size_t index = 1;  index < argc;  ++index )
	{
		const char* pathname = argv[ index ];
		
		int result = utime( pathname, NULL );
		
		if ( result < 0 )
		{
			if ( errno == ENOENT )
			{
				result = open( pathname, O_WRONLY | O_CREAT, 0666 );
				
				if ( result >= 0 )
				{
					close( result );
					
					continue;
				}
			}
			
			more::perror( "touch", pathname );
			
			exit_status = 1;
		}
	}
	
	return exit_status;
}

