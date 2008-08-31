/*	=============
 *	Exceptions.hh
 *	=============
 */

#pragma once

// Standard C++
#include <string>


namespace ALine
{
	
	struct NoSuchUsedProject
	{
		NoSuchUsedProject( const std::string& projName, const std::string& used )
		:
			projName( projName ), 
			used    ( used )
		{}
		
		std::string projName;
		std::string used;
	};
	
	/*
	struct BadSourceAlias
	{
		BadSourceAlias( const Project& proj, const FSSpec& alias ) 
		:
			proj ( proj ), 
			alias( alias )
		{}
		
		const Project& proj;
		FSSpec alias;
	};
	*/
	
}

