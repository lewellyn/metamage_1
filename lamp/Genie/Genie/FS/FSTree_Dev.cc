/*	=============
 *	FSTree_Dev.cc
 *	=============
 */

#include "Genie/FS/FSTree_Dev.hh"

// Standard C
#include "errno.h"

// POSIX
#include "fcntl.h"
#include <sys/stat.h>

// Iota
#include "iota/strings.hh"

// poseven
#include "poseven/types/errno_t.hh"

// vfs
#include "vfs/filehandle.hh"
#include "vfs/node.hh"
#include "vfs/functions/new_static_symlink.hh"
#include "vfs/node/types/dynamic_group.hh"

// MacVFS
#include "MacVFS/file/gestalt.hh"

// relix-kernel
#include "relix/api/current_process.hh"
#include "relix/config/mini.hh"
#include "relix/config/pts.hh"
#include "relix/fs/con_tag.hh"
#include "relix/fs/pts_tag.hh"
#include "relix/task/process.hh"
#include "relix/task/process_group.hh"
#include "relix/task/session.hh"

// Genie
#include "Genie/FS/data_method_set.hh"
#include "Genie/FS/node_method_set.hh"
#include "Genie/IO/SerialDevice.hh"
#include "Genie/IO/SimpleDevice.hh"


#ifndef CONFIG_DEV_SERIAL
#define CONFIG_DEV_SERIAL  (!CONFIG_MINI)
#endif

namespace Genie
{
	
	namespace p7 = poseven;
	
	
	struct CallOut_Traits
	{
		static const bool isPassive = false;
	};
	
	struct DialIn_Traits
	{
		static const bool isPassive = true;
	};
	
	struct ModemPort_Traits
	{
		static const char* Name()  { return "A"; }
	};
	
	struct PrinterPort_Traits
	{
		static const char* Name()  { return "B"; }
	};
	
	template < class Mode, class Port >
	struct dev_Serial
	{
		static const mode_t perm = S_IRUSR | S_IWUSR;
		
		static vfs::filehandle_ptr open( const vfs::node* that, int flags, mode_t mode );
	};
	
	template < class Mode, class Port >
	vfs::filehandle_ptr dev_Serial< Mode, Port >::open( const vfs::node* that, int flags, mode_t mode )
	{
		const bool nonblocking = flags & O_NONBLOCK;
		
		return OpenSerialDevice( Port::Name(), Mode::isPassive, nonblocking );
	}
	
	typedef dev_Serial< CallOut_Traits, ModemPort_Traits   > dev_cumodem;
	typedef dev_Serial< CallOut_Traits, PrinterPort_Traits > dev_cuprinter;
	typedef dev_Serial< DialIn_Traits,  ModemPort_Traits   > dev_ttymodem;
	typedef dev_Serial< DialIn_Traits,  PrinterPort_Traits > dev_ttyprinter;
	
	
	struct dev_gestalt
	{
		static const mode_t perm = S_IRUSR;
		
		static vfs::filehandle_ptr open( const vfs::node* that, int flags, mode_t mode );
	};
	
	vfs::filehandle_ptr dev_gestalt::open( const vfs::node* that, int flags, mode_t mode )
	{
		return open_gestalt( that, flags, mode );
	}
	
	
	static vfs::filehandle_ptr simple_device_open( const vfs::node* that, int flags, mode_t mode )
	{
		return GetSimpleDeviceHandle( *that );
	}
	
	static const data_method_set simple_device_data_methods =
	{
		&simple_device_open
	};
	
	static const node_method_set simple_device_methods =
	{
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		&simple_device_data_methods
	};
	
	struct dev_tty
	{
		static const mode_t perm = S_IRUSR | S_IWUSR;
		
		static vfs::filehandle_ptr open( const vfs::node* that, int flags, mode_t mode );
	};
	
	vfs::filehandle_ptr dev_tty::open( const vfs::node* that, int flags, mode_t mode )
	{
		vfs::filehandle* tty = relix::current_process().get_process_group().get_session().get_ctty().get();
		
		if ( tty == NULL )
		{
			p7::throw_errno( ENOENT );
		}
		
		return tty;
	}
	
	
	template < class Opener >
	struct basic_device
	{
		static const data_method_set data_methods;
		static const node_method_set node_methods;
	};
	
	template < class Opener >
	const data_method_set basic_device< Opener >::data_methods =
	{
		&Opener::open
	};
	
	template < class Opener >
	const node_method_set basic_device< Opener >::node_methods =
	{
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		&data_methods
	};
	
	
	template < class Opener >
	static vfs::node_ptr BasicDevice_Factory( const vfs::node*     parent,
	                                          const plus::string&  name,
	                                          const void*          args )
	{
		return new vfs::node( parent,
		                      name,
		                      S_IFCHR | Opener::perm,
		                      &basic_device< Opener >::node_methods );
	}
	
	static vfs::node_ptr SimpleDevice_Factory( const vfs::node*     parent,
	                                           const plus::string&  name,
	                                           const void*          args )
	{
		return new vfs::node( parent, name, S_IFCHR | 0600, &simple_device_methods );
	}
	
	using vfs::dynamic_group_factory;
	using vfs::dynamic_group_element;
	
	const vfs::fixed_mapping dev_Mappings[] =
	{
		{ "null",    &SimpleDevice_Factory },
		{ "zero",    &SimpleDevice_Factory },
		{ "console", &SimpleDevice_Factory },
		
		{ "tty", &BasicDevice_Factory< dev_tty > },
		
	#if CONFIG_DEV_SERIAL
		
		{ "cu.modem",    &BasicDevice_Factory< dev_cumodem    > },
		{ "cu.printer",  &BasicDevice_Factory< dev_cuprinter  > },
		{ "tty.modem",   &BasicDevice_Factory< dev_ttymodem   > },
		{ "tty.printer", &BasicDevice_Factory< dev_ttyprinter > },
		
	#endif
		
		{ "gestalt", &BasicDevice_Factory< dev_gestalt > },
		
		{ "con", &dynamic_group_factory, &dynamic_group_element< relix::con_tag >::extra },
		
	#if CONFIG_PTS
		
		{ "pts", &dynamic_group_factory, &dynamic_group_element< relix::pts_tag  >::extra },
		
	#endif
		
		{ "fd", &vfs::new_static_symlink, "/proc/self/fd" },
		
		{ NULL, NULL }
	};
	
}

