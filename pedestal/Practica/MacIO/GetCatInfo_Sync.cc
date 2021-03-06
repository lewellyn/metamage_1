/*	==================
 *	GetCatInfo_Sync.cc
 *	==================
 */

#include "MacIO/GetCatInfo_Sync.hh"

// Nitrogen
#include "Nitrogen/Files.hh"

// MacIO
#include "MacIO/Routines.hh"
#include "MacIO/Sync.hh"


namespace MacIO
{
	
	namespace N = Nitrogen;
	
	
	template < class Policy >
	typename Policy::Result
	//
	GetCatInfo( CInfoPBRec&     pb,
	            SInt16          vRefNum,
	            SInt32          dirID,
	            unsigned char*  name,
	            SInt16          index )
	{
		//Str255 dummyName = "\p";  // clang hates this
		
		Str255 dummyName;
		       dummyName[ 0 ] = 0;
		
		if ( index == 0  &&  (name == NULL  ||  name[ 0 ] == '\0') )
		{
			if ( name == NULL )
			{
				name = dummyName;
			}
			
			index = -1;
		}
		
		nucleus::initialize< CInfoPBRec >( pb, vRefNum, dirID, name, index );
		
		return PBSync< GetCatInfo_Traits, Policy >( pb );
	}
	
	
	template
	void GetCatInfo< Throw_All >( CInfoPBRec&     pb,
	                              SInt16          vRefNum,
	                              SInt32          dirID,
	                              unsigned char*  name,
	                              SInt16          index );
	
	template
	bool GetCatInfo< Return_FNF >( CInfoPBRec&     pb,
	                               SInt16          vRefNum,
	                               SInt32          dirID,
	                               unsigned char*  name,
	                               SInt16          index );
	
}

