/*
	
	PatchUtils.hh
	
	Joshua Juran
	
*/

#ifndef SILVER_PATCHUTILS_HH
#define SILVER_PATCHUTILS_HH

// Mac OS X
#ifdef __APPLE__
#include <CoreServices/CoreServices.h>
#endif

// Mac OS
#ifndef __MACTYPES__
#include <MacTypes.h>
#endif


namespace Silver
{
	
	UniversalProcPtr ApplyTrapPatch_( short trap, UniversalProcPtr patchPtr );
	
	inline void RemoveTrapPatch_( short trap, UniversalProcPtr patchPtr )
	{
		ApplyTrapPatch_( trap, patchPtr );
	}
	
	template < class PatchedProcPtr >
	inline PatchedProcPtr ApplyTrapPatch( short trap, PatchedProcPtr patchPtr )
	{
		ProcPtr trapPtr = ApplyTrapPatch_( trap,
		                                   (UniversalProcPtr) patchPtr );
		
		return reinterpret_cast< PatchedProcPtr >( trapPtr );
	}
	
	template < class PatchedProcPtr >
	inline void RemoveTrapPatch( short trap, PatchedProcPtr patchPtr )
	{
		RemoveTrapPatch_( trap, (UniversalProcPtr) patchPtr );
	}
	
}

#endif

