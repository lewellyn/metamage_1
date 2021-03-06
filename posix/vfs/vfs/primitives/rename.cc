/*
	rename.cc
	---------
*/

#include "vfs/primitives/rename.hh"

// poseven
#include "poseven/types/errno_t.hh"

// vfs
#include "vfs/node.hh"
#include "vfs/methods/node_method_set.hh"


namespace vfs
{
	
	namespace p7 = poseven;
	
	
	void rename( const node& that, const node& target )
	{
		const node_method_set* methods = that.methods();
		
		if ( methods  &&  methods->rename )
		{
			methods->rename( &that, &target );
		}
		else
		{
			p7::throw_errno( EPERM );
		}
	}
	
}

