/*
	get_option.cc
	-------------
*/

#include "command/get_option.hh"

// Standard C
#include <string.h>

// command
#include "command/errors.hh"


#pragma exceptions off


#define STR_LEN( s )  "" s, (sizeof s - 1)

#define USAGE( text, arg )  usage( STR_LEN( text ": " ), arg )


namespace command
{
	
	option_result global_result;
	
	
	static const char* find_char( const char* begin, char c )
	{
		while ( *begin != '\0'  &&  *begin != c )
		{
			++begin;
		}
		
		return begin;
	}
	
	static bool matches( const char* a, size_t a_size, const char* b )
	{
		return memcmp( a, b, a_size ) == 0  &&  b[ a_size ] == '\0';
	}
	
	static const option* find_option( const option*  options,
	                                  const char*    name,
	                                  size_t         name_size )
	{
		for ( const option* o = options;  o->name != 0;  ++o )  // NULL
		{
			if ( matches( name, name_size, o->name ) )
			{
				return o;
			}
		}
		
		return 0;  // NULL
	}
	
	static const option* find_option( const option*  options,
	                                  short          code )
	{
		for ( const option* o = options;  o->name != 0;  ++o )  // NULL
		{
			if ( o->code == code )
			{
				return o;
			}
		}
		
		return 0;  // NULL
	}
	
	short get_option_( char* const*&   argv,
	                   const option*   options,
	                   int             flags,
	                   option_result&  result )
	{
		if ( const char* arg = *argv )
		{
			if ( result.mark )
			{
				arg = result.mark;
				
				goto next;
			}
			
			if ( arg[ 0 ] == '-' )
			{
				if ( arg[ 1 ] == '\0' )
				{
					// An "-" argument is not an option and means stdin
					return 0;
				}
				
				if ( arg[ 1 ] == '-' )
				{
					// long option or "--"
					
					const char* name = arg + 2;
					
					if ( *name == '\0' )
					{
						// skip "--" and end
						
						++argv;
						
						return 0;
					}
					
					const char* equals = find_char( name, '=' );
					
					const size_t name_size = equals - name;
					
					const option* opt = find_option( options, name, name_size );
					
					if ( opt == 0 )  // NULL
					{
						if ( flags & Flag_return_errors )
						{
							return Option_undefined;  // no such option (long)
						}
						
						USAGE( "Undefined option", arg );
					}
					
					result.param = 0;  // NULL
					
					if ( *equals != '\0' )
					{
						result.param = equals + 1;
						
						if ( opt->mode == Param_unwanted )
						{
							if ( flags & Flag_return_errors )
							{
								return Option_param_bogus;  // e.g. --verbose=foo
							}
							
							USAGE( "Bogus parameter in option", arg );
						}
						
						++argv;
						
						return opt->code;
					}
					
					++argv;
					
					if ( opt->mode == Param_required )
					{
						if ( *argv == 0 )  // NULL
						{
							if ( flags & Flag_return_errors )
							{
								return Option_param_missing;
							}
							
							USAGE( "Missing parameter for option", arg );
						}
						
						result.param = *argv++;
					}
					
					return opt->code;
				}
				
				// short option(s)
				
				++arg;
				
			next:
				
				result.param = 0;  // NULL
				
				const unsigned char c = *arg++;
				
				const option* opt = find_option( options, c );
				
				if ( opt == 0 )  // NULL
				{
					if ( flags & Flag_return_errors )
					{
						return Option_undefined;  // no such option (short)
					}
					
					char buffer[ sizeof "-?" ] = { '-', c };
					
					USAGE( "Undefined option", buffer );
				}
				
				const bool more = *arg != '\0';
				
				if ( more  &&  opt->mode != Param_required )
				{
					result.mark = arg;
				}
				else
				{
					result.mark = 0;  // NULL
					
					++argv;
					
					if ( opt->mode == Param_required )
					{
						if ( more )
						{
							result.param = arg;
						}
						else if ( *argv )
						{
							result.param = *argv++;
						}
						else
						{
							if ( flags & Flag_return_errors )
							{
								return Option_param_missing;
							}
							
							char buffer[ sizeof "-?" ] = { '-', c };
							
							USAGE( "Missing parameter for option", buffer );
						}
					}
				}
				
				return opt->code;
			}
		}
		
		return 0;
	}
	
}
