/*	==========
 *	Caption.hh
 *	==========
 */

#ifndef PEDESTAL_CAPTION_HH
#define PEDESTAL_CAPTION_HH

// Standard C++
#include <string>

// Pedestal
#include "Pedestal/View.hh"


namespace Pedestal
{
	
	class Caption : public View
	{
		public:
			virtual std::string Text() const = 0;
			
			virtual bool Wrapped() const  { return true; }
			
			void Draw( const Rect& bounds, bool erasing );
	};
	
}

#endif

