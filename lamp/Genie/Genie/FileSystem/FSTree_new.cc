/*	=============
 *	FSTree_new.cc
 *	=============
 */

#include "Genie/FileSystem/FSTree_new.hh"

// Genie
#include "Genie/FileSystem/FSTree_new_caption.hh"
#include "Genie/FileSystem/FSTree_new_frame.hh"
#include "Genie/FileSystem/FSTree_new_icon.hh"
#include "Genie/FileSystem/FSTree_new_iconid.hh"
#include "Genie/FileSystem/FSTree_new_window.hh"


namespace Genie
{
	
	const FSTree_Premapped::Mapping new_Mappings[] =
	{
		{ "caption", &Singleton_Factory< FSTree_new_caption > },
		{ "frame",   &Singleton_Factory< FSTree_new_frame   > },
		{ "icon",    &Singleton_Factory< FSTree_new_icon    > },
		{ "iconid",  &Singleton_Factory< FSTree_new_iconid  > },
		{ "window",  &Singleton_Factory< FSTree_new_window  > },
		
		{ NULL, NULL }
	};
	
}

