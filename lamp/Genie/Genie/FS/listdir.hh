/*
	listdir.hh
	----------
*/

// vfs
#include "vfs/dir_contents_fwd.hh"

// Genie
#include "Genie/FS/FSTree_fwd.hh"


namespace Genie
{
	
	void listdir( const FSTree* node, vfs::dir_contents& contents );
	
}
