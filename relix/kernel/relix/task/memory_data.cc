/*
	memory_data.cc
	--------------
*/

#include "relix/task/memory_data.hh"

// standard C/C++
#include <cstring>

// debug
#include "debug/assert.hh"
#include "debug/boost_assert.hh"

// Boost
#include <boost/intrusive_ptr.hpp>

// plus
#include "plus/argv.hh"
#include "plus/simple_map.hh"

// vfs
#include "vfs/memory_mapping.hh"


namespace relix
{
	
	struct program_parameters
	{
		plus::argv its_cmdline;
		plus::argv its_environ;
	};
	
	static void swap( program_parameters& a, program_parameters& b )
	{
		swap( a.its_cmdline, b.its_cmdline );
		swap( a.its_environ, b.its_environ );
	}
	
	class memory_data_impl : public memory_data
	{
		public:
			typedef plus::simple_map< addr_t, boost::intrusive_ptr< const vfs::memory_mapping > > mmap_map;
			
			program_parameters  its_parameters;
			mmap_map            its_memory_mappings;
		
		public:
			memory_data_impl()
			{
			}
			
			memory_data_impl( const memory_data_impl& x )
			{
				/*
					Don't copy its_parameters -- it exists only to store data
					that argv and initial environ point to.
					
					For vfork(), environ is reallocated in userspace upon
					modification, and argv is simply not handled -- modifying
					argv in a vforked child modifies it in the parent -- which
					is POSIX-conformant since anything between vfork() and exec
					is undefined behavior.
					
					For fork(), we'll need to multiplex all memory belonging to
					the process, including argv/envp pointers and string data
					and private mmap regions.
				*/
			}
			
			void swap( memory_data_impl& other );
	};
	
	void memory_data_impl::swap( memory_data_impl& other )
	{
		using relix::swap;
		
		swap( its_parameters,      other.its_parameters      );
		swap( its_memory_mappings, other.its_memory_mappings );
	}
	
	inline void swap( memory_data_impl& a, memory_data_impl& b )
	{
		a.swap( b );
	}
	
	
	void memory_data::swap( memory_data& other )
	{
		relix::swap( static_cast< memory_data_impl& >( *this ),
		             static_cast< memory_data_impl& >( other ) );
	}
	
	memory_data* memory_data::create()
	{
		return new memory_data_impl();
	}
	
	memory_data* duplicate( const memory_data& x )
	{
		return new memory_data_impl( static_cast< const memory_data_impl& >( x ) );
	}
	
	void destroy( const memory_data* x )
	{
		delete static_cast< const memory_data_impl* >( x );
	}
	
	static inline const memory_data_impl* impl_cast( const memory_data* x )
	{
		return static_cast< const memory_data_impl* >( x );
	}
	
	static inline memory_data_impl* impl_cast( memory_data* x )
	{
		return static_cast< memory_data_impl* >( x );
	}
	
	const plus::string& memory_data::get_cmdline() const
	{
		return impl_cast( this )->its_parameters.its_cmdline.get_string();
	}
	
	
	void memory_data::set_argv( const char *const *argv )
	{
		impl_cast( this )->its_parameters.its_cmdline.assign( argv );
	}
	
	void memory_data::set_envp( const char *const *envp )
	{
		impl_cast( this )->its_parameters.its_environ.assign( envp );
	}
	
	int memory_data::get_argc() const
	{
		return impl_cast( this )->its_parameters.its_cmdline.get_argc();  // don't count trailing NULL
	}
	
	char* const* memory_data::get_argv()
	{
		return impl_cast( this )->its_parameters.its_cmdline.get_argv();
	}
	
	char* const* memory_data::get_envp()
	{
		return impl_cast( this )->its_parameters.its_environ.get_argv();
	}
	
	void* memory_data::add_memory_mapping( const vfs::memory_mapping* mapping )
	{
		ASSERT( mapping != NULL );
		
		void* key = mapping->get_address();
		
		impl_cast( this )->its_memory_mappings[ key ] = mapping;
		
		return key;
	}
	
	void memory_data::msync_memory_mapping( addr_t key, size_t len, int flags )
	{
		memory_data_impl::mmap_map& mappings = impl_cast( this )->its_memory_mappings;
		
		if ( boost::intrusive_ptr< const vfs::memory_mapping >* it = mappings.find( key ) )
		{
			it->get()->msync( key, len, flags );
		}
	}
	
	void memory_data::remove_memory_mapping( addr_t key )
	{
		impl_cast( this )->its_memory_mappings.erase( key );
	}
	
	void memory_data::clear_memory_mappings()
	{
		impl_cast( this )->its_memory_mappings.clear();
	}
	
}

