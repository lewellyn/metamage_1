/*	=================
 *	AESendBlocking.cc
 *	=================
 */

// Relix
#include "relix/AESendBlocking.h"
#include "relix/syscall/registry.hh"

// Nitrogen
#include "Mac/Toolbox/Types/OSStatus.hh"

#include "Nitrogen/AEDataModel.hh"
#include "Nitrogen/AEInteraction.hh"

// Nitrogen Extras / AEFramework
#include "AEFramework/AEFramework.h"

// Genie
#include "Genie/current_process.hh"
#include "Genie/Process.hh"


namespace N = Nitrogen;


OSStatus AESendBlocking( const AppleEvent* appleEventPtr, AppleEvent* replyPtr )
{
	using namespace Genie;
	
	try
	{
		Mac::AppleEvent const& appleEvent = static_cast< const Mac::AppleEvent& >( *appleEventPtr );
		Mac::AppleEvent      & reply      = static_cast<       Mac::AppleEvent& >( *replyPtr      );
		
		(void) N::AESend( appleEvent,
		                  Mac::kAEQueueReply | Mac::kAECanInteract );
		
		// Now that we've sent the event, retrieve the return ID
		N::AEReturnID_32Bit returnID = N::AEGetAttributePtr< Mac::keyReturnIDAttr >( appleEvent );
		
		// Subscribe to AEFramework's queued reply delivery and wake-up service
		N::ExpectReply( returnID, &reply );
		
		// Sleep until the reply is delivered
		current_process().Stop();
	}
	catch ( const Mac::OSStatus& err )
	{
		return err;
	}
	
	return noErr;
}

#pragma force_active on

REGISTER_SYSTEM_CALL( AESendBlocking );

#pragma force_active reset
