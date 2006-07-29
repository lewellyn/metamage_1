/*	=========
 *	OSUtils.h
 *	=========
 */

#ifndef NITROGEN_OSUTILS_H
#define NITROGEN_OSUTILS_H

#ifndef NITROGEN_FRAMEWORKHEADER_H
#include "Nitrogen/FrameworkHeader.h"
#endif
#ifndef __OSUTILS__
#include FRAMEWORK_HEADER(CarbonCore,OSUtils.h)
#endif
#ifndef NITROGEN_MACTYPES_H
#include "Nitrogen/MacTypes.h"
#endif
#ifndef NITROGEN_MIXEDMODE_H
#include "Nitrogen/MixedMode.h"
#endif
#ifndef NITROGEN_MACMEMORY_H
#include "Nitrogen/MacMemory.h"
#endif
#ifndef NITROGEN_DATETIMEUTILS_H
#include "Nitrogen/DateTimeUtils.h"
#endif
#ifndef NITROGEN_CFSTRING_H
#include "Nitrogen/CFString.h"
#endif
#ifndef NITROGEN_UPP_H
#include "Nitrogen/UPP.h"
#endif


namespace Nitrogen
{
	
	void RegisterDeferredTaskManagerErrors();
	
	// ...
	
	namespace Private
	{
	#if TARGET_CPU_68K && !TARGET_RT_MAC_CFM
		
		inline void InvokeDeferredTaskUPP( long dtParam, ::DeferredTaskUPP userUPP )
		{
			::InvokeDeferredTaskUPP( dtParam, userUPP );
		}
		
	#else
		
		using ::InvokeDeferredTaskUPP;
		
	#endif
	}
	
	struct DeferredTaskUPP_Details : Basic_UPP_Details< ::DeferredTaskUPP,
	                                                    ::DeferredTaskProcPtr,
	                                                    ::NewDeferredTaskUPP,
	                                                    ::DisposeDeferredTaskUPP,
	                                                    Private::InvokeDeferredTaskUPP >
	{};
	
	typedef UPP< DeferredTaskUPP_Details > DeferredTaskUPP;
	
	inline Nucleus::Owned< DeferredTaskUPP > NewDeferredTaskUPP( ::DeferredTaskProcPtr p )
	{
		return NewUPP< DeferredTaskUPP >( p );
	}
	
	inline void DisposeDeferredTaskUPP( Nucleus::Owned< DeferredTaskUPP > )  {}
	
#if !TARGET_CPU_68K || TARGET_RT_MAC_CFM
	
	inline void InvokeDeferredTaskUPP( long             dtParam,
	                                   DeferredTaskUPP  userUPP )
	{
		userUPP( dtParam );
	}
	
#endif
	
	using ::DeferredTask;
	using ::DeferredTaskPtr;
	
	// ...
	
	void DTInstall( DeferredTask& dtTaskPtr );
	
	// ...
	
	inline UInt32 Delay( UInt32 ticks )
	{
		UInt32 finalTicks;
		::Delay( ticks, &finalTicks );
		
		return finalTicks;
	}
	
	// ...

#if	TARGET_RT_MAC_MACHO
/*	
	There's a long note in technical Q & A #1078
		http://developer.apple.com/qa/qa2001/qa1078.html
	about how to make CSCopyMachineName and CSCopyUserName work in all environments (OS 9, CFM, etc)
	I'm going the easy way here, and making it work for Mach-O.
*/

	class CSCopyUserName_Failed {};
	inline Nucleus::Owned<CFStringRef> CSCopyUserName ( Boolean useShortName ) {
		CFStringRef result = ::CSCopyUserName ( useShortName );
		if ( NULL == result ) throw CSCopyUserName_Failed();
		return Nucleus::Owned<CFStringRef>::Seize ( result );
		}

	class CSCopyMachineName_Failed {};
	inline Nucleus::Owned<CFStringRef> CSCopyMachineName (void) {
		CFStringRef result = ::CSCopyMachineName();
		if ( NULL == result ) throw CSCopyMachineName_Failed();
		return Nucleus::Owned<CFStringRef>::Seize ( result );
		}
#endif

}

#endif

