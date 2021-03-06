/*
	plus/argv.hh
	------------
*/

#ifndef PLUS_ARGV_HH
#define PLUS_ARGV_HH

// Standard C++
#include <vector>

// plus
#include "plus/var_string.hh"


namespace plus
{
	
	class argv
	{
		private:
			var_string            its_string;
			std::vector< char* >  its_vector;
		
		public:
			argv();
			
			argv( const plus::string& s );
			
			argv( const char *const *args );
			
			argv( const argv& that );
			
			argv& operator=( const argv& that );
			
			void swap( argv& that );
			
			int get_argc() const  { return its_vector.size() - 1; }
			
			char* const* get_argv() const  { return &its_vector[ 0 ]; }
			
			const plus::string& get_string() const  { return its_string; }
			
			argv& assign( const plus::string& s );
			
			argv& assign( const char *const *args );
	};
	
	void swap( argv& a, argv& b );
	
}

#endif

