/*
	link.cc
	-------
*/

// Standard C
#include <errno.h>

// POSIX
#include "sys/stat.h"
#include "unistd.h"

// poseven
#include "poseven/types/errno_t.hh"

// vfs
#include "vfs/node.hh"
#include "vfs/functions/file-tests.hh"
#include "vfs/functions/resolve_links_in_place.hh"
#include "vfs/primitives/hardlink.hh"

// relix-kernel
#include "relix/api/root.hh"
#include "relix/fs/resolve_path_at.hh"
#include "relix/syscall/registry.hh"

// Genie
#include "Genie/current_process.hh"
#include "Genie/Kernel/make_alias.hh"


#ifndef AT_SYMLINK_FOLLOW
#define AT_SYMLINK_FOLLOW  0
#endif

#ifndef AT_LINK_ALIAS
#define AT_LINK_ALIAS  0
#endif


namespace Genie
{
	
	namespace p7 = poseven;
	
	
	static int linkat( int olddirfd, const char* oldpath, int newdirfd, const char* newpath, int flags )
	{
		try
		{
			vfs::node_ptr oldFile = relix::resolve_path_at( olddirfd, oldpath );
			
			if ( const bool follow = flags & AT_SYMLINK_FOLLOW )
			{
				vfs::resolve_links_in_place( *relix::root(), oldFile );
			}
			
			vfs::node_ptr newFile = relix::resolve_path_at( newdirfd, newpath );
			
			// Do not resolve links.  If there's a symlink in this location, throw EEXIST.
			
			if ( exists( *newFile ) )
			{
				return set_errno( EEXIST );
			}
			
			if ( flags & AT_LINK_ALIAS )
			{
				make_alias( *oldFile, *newFile );
			}
			else
			{
				hardlink( *oldFile, *newFile );
			}
		}
		catch ( ... )
		{
			return set_errno_from_exception();
		}
		
		return 0;
	}
	
	
	#pragma force_active on
	
	REGISTER_SYSTEM_CALL( linkat );
	
	#pragma force_active reset
	
}
