/*
	Genie/FS/sys/mac/vol/list.cc
	----------------------------
*/

#include "Genie/FS/sys/mac/vol/list.hh"

// POSIX
#include <sys/stat.h>

// Iota
#include "iota/strings.hh"

// mac-sys-utils
#include "mac_sys/gestalt.hh"

// gear
#include "gear/inscribe_decimal.hh"
#include "gear/parse_decimal.hh"

// plus
#include "plus/deconstruct.hh"
#include "plus/freeze.hh"
#include "plus/serialize.hh"
#include "plus/stringify.hh"
#include "plus/var_string.hh"
#include "plus/string/concat.hh"

// Nitrogen
#include "Nitrogen/Files.hh"
#include "Nitrogen/Folders.hh"

// MacIO
#include "MacIO/FSMakeFSSpec_Sync.hh"

// poseven
#include "poseven/types/errno_t.hh"

// vfs
#include "vfs/dir_contents.hh"
#include "vfs/dir_entry.hh"
#include "vfs/node.hh"
#include "vfs/functions/resolve_pathname.hh"
#include "vfs/node/types/fixed_dir.hh"
#include "vfs/node/types/symbolic_link.hh"

// relix-kernel
#include "relix/api/root.hh"

// Genie
#include "Genie/FS/basic_directory.hh"
#include "Genie/FS/Drives.hh"
#include "Genie/FS/FSSpec.hh"
#include "Genie/FS/FSTree_Property.hh"
#include "Genie/FS/link_method_set.hh"
#include "Genie/FS/node_method_set.hh"
#include "Genie/FS/property.hh"
#include "Genie/FS/serialize_Str255.hh"
#include "Genie/FS/Trigger.hh"
#include "Genie/FS/sys/mac/vol/list/N/dt.hh"
#include "Genie/FS/sys/mac/vol/list/N/parms.hh"
#include "Genie/FS/utf8_text_property.hh"
#include "Genie/Utilities/AsyncIO.hh"
#include "Genie/Utilities/canonical_positive_integer.hh"


namespace Genie
{
	
	namespace n = nucleus;
	namespace N = Nitrogen;
	namespace p7 = poseven;
	
	
	const uint32_t gestaltFSAttr = 'fs  ';
	
	const int gestaltFSSupports4GBVols = 4;
	const int gestaltFSSupports2TBVols = 5;
	
	
	static N::FSVolumeRefNum GetKeyFromParent( const vfs::node& parent )
	{
		return N::FSVolumeRefNum( -gear::parse_unsigned_decimal( parent.name().c_str() ) );
	}
	
	
#if TARGET_API_MAC_CARBON
	
	static inline bool Has_PBXGetVolInfo()
	{
		return true;
	}
	
	// never called
	static UInt16 GetTotalBlocks( const XVolumeParam& volume )  { return 0; }
	static UInt16 GetFreeBlocks ( const XVolumeParam& volume )  { return 0; }
	
#else
	
	static bool Has_PBXGetVolInfo()
	{
		static bool result = mac::sys::gestalt_bit_set( gestaltFSAttr, gestaltFSSupports2TBVols );
		
		return result;
	}
	
	static bool Has4GBVolumes()
	{
		static bool result = mac::sys::gestalt_bit_set( gestaltFSAttr, gestaltFSSupports4GBVols );
		
		return result;
	}
	
	static const VCB* FindVCB( ::FSVolumeRefNum vRefNum )
	{
		const VCB* vcb = (VCB*) ::GetVCBQHdr()->qHead;
		
		while ( vcb != NULL )
		{
			if ( vcb->vcbVRefNum == vRefNum )
			{
				break;
			}
			
			vcb = (VCB*) vcb->qLink;
		}
		
		return vcb;
	}
	
	static inline UInt16 GetTotalBlocks( const XVolumeParam& volume )
	{
		const VCB* vcb = Has4GBVolumes() ? FindVCB( volume.ioVRefNum ) : NULL;
		
		return vcb ? vcb->vcbNmAlBlks : volume.ioVNmAlBlks;
	}
	
	static inline UInt16 GetFreeBlocks( const XVolumeParam& volume )
	{
		const VCB* vcb = Has4GBVolumes() ? FindVCB( volume.ioVRefNum ) : NULL;
		
		return vcb ? vcb->vcbFreeBks : volume.ioVFrBlk;
	}
	
#endif
	
	
	static bool is_valid_VolumeRefNum( N::FSVolumeRefNum key )
	{
		try
		{
			(void) MacIO::FSMakeFSSpec< FNF_Throws >( key, N::fsRtDirID, NULL );
		}
		catch ( const Mac::OSStatus& err )
		{
			if ( err != nsvErr )
			{
				throw;
			}
			
			return false;
		}
		
		return true;
	}
	
	struct valid_name_of_vol_number
	{
		typedef canonical_positive_integer well_formed_name;
		
		static bool applies( const plus::string& name )
		{
			if ( well_formed_name::applies( name ) )
			{
				const int i = gear::parse_unsigned_decimal( name.c_str() );
				
				if ( (i & 0xffff8000) == 0 )
				{
					return is_valid_VolumeRefNum( N::FSVolumeRefNum( -i ) );
				}
			}
			
			return false;
		}
	};
	
	
	extern const vfs::fixed_mapping sys_mac_vol_N_Mappings[];
	
	static vfs::node_ptr vol_lookup( const vfs::node* parent, const plus::string& name )
	{
		if ( !valid_name_of_vol_number::applies( name ) )
		{
			poseven::throw_errno( ENOENT );
		}
		
		return fixed_dir( parent, name, sys_mac_vol_N_Mappings );
	}
	
	static void vol_iterate( const vfs::node* parent, vfs::dir_contents& cache )
	{
		N::Volume_Container sequence = N::Volumes();
		
		typedef N::Volume_Container::const_iterator Iter;
		
		const Iter end = sequence.end();
		
		for ( Iter it = sequence.begin();  it != end;  ++it )
		{
			const short vRefNum = *it;
			
			const ino_t inode = -vRefNum;
			
			plus::string name = gear::inscribe_decimal( -vRefNum );
			
			cache.push_back( vfs::dir_entry( inode, name ) );
		}
	}
	
	
	struct Volume_Accessor_Defaults
	{
		static const bool needsName = false;
		
		static const bool neverZero = false;
	};
	
	struct GetVolumeName : Volume_Accessor_Defaults,
	                       serialize_Str255_contents
	{
		static const bool needsName = true;
		
		static const unsigned char* Get( const XVolumeParam& volume )
		{
			return volume.ioNamePtr;
		}
	};
	
	struct GetVolumeBlockCount : Volume_Accessor_Defaults,
	                             plus::serialize_unsigned< UInt32 >
	{
		// will break on 16TB volumes
		
		static UInt32 Get( const XVolumeParam& volume )
		{
			return Has_PBXGetVolInfo() ? volume.ioVTotalBytes / volume.ioVAlBlkSiz
			                           : GetTotalBlocks( volume );
		}
	};
	
	struct GetVolumeBlockSize : Volume_Accessor_Defaults,
	                            plus::serialize_unsigned< UInt32 >
	{
		static UInt32 Get( const XVolumeParam& volume )
		{
			return volume.ioVAlBlkSiz;
		}
	};
	
	struct GetVolumeFreeBlockCount : Volume_Accessor_Defaults,
	                                 plus::serialize_unsigned< UInt32 >
	{
		static UInt32 Get( const XVolumeParam& volume )
		{
			return Has_PBXGetVolInfo() ? volume.ioVFreeBytes / volume.ioVAlBlkSiz
			                           : GetFreeBlocks( volume );
		}
	};
	
	struct GetVolumeCapacity : Volume_Accessor_Defaults,
	                           plus::serialize_unsigned< UInt64 >
	{
		static UInt64 Get( const XVolumeParam& volume )
		{
			return Has_PBXGetVolInfo() ? volume.ioVTotalBytes
			                           : GetTotalBlocks( volume ) * volume.ioVAlBlkSiz;
		}
	};
	
	struct GetVolumeFreeSpace : Volume_Accessor_Defaults,
	                            plus::serialize_unsigned< UInt64 >
	{
		static UInt64 Get( const XVolumeParam& volume )
		{
			return Has_PBXGetVolInfo() ? volume.ioVFreeBytes
			                           : GetFreeBlocks( volume ) * volume.ioVAlBlkSiz;
		}
	};
	
	struct GetVolumeSignature : Volume_Accessor_Defaults,
	                            plus::serialize_c_string_contents
	{
		static const char* Get( const XVolumeParam& volume )
		{
			static char sigWord[] = "ab";
			
			sigWord[ 0 ] = volume.ioVSigWord >> 8;
			sigWord[ 1 ] = volume.ioVSigWord & 0xff;
			
			return sigWord;
		}
	};
	
	struct GetVolumeFSID : Volume_Accessor_Defaults,
	                       plus::serialize_int< SInt16 >
	{
		static SInt16 Get( const XVolumeParam& volume )
		{
			return volume.ioVFSID;
		}
	};
	
	struct GetVolumeWriteCount : Volume_Accessor_Defaults,
	                             plus::serialize_int< SInt32 >
	{
		static const bool neverZero = true;
		
		static SInt32 Get( const XVolumeParam& volume )
		{
			return volume.ioVWrCnt;
		}
	};
	
	struct GetVolumeFileCount : Volume_Accessor_Defaults,
	                            plus::serialize_int< SInt32 >
	{
		static const bool neverZero = true;
		
		static SInt32 Get( const XVolumeParam& volume )
		{
			return volume.ioVFilCnt;
		}
	};
	
	struct GetVolumeDirCount : Volume_Accessor_Defaults,
	                           plus::serialize_int< SInt32 >
	{
		static const bool neverZero = true;
		
		static SInt32 Get( const XVolumeParam& volume )
		{
			return volume.ioVDirCnt;
		}
	};
	
	
	static void PBHGetVInfoSync( HVolumeParam& pb, N::FSVolumeRefNum vRefNum, StringPtr name = NULL )
	{
		pb.ioNamePtr  = name;
		pb.ioVRefNum  = vRefNum;
		pb.filler2    = 0;
		pb.ioVolIndex = 0;
		
		Mac::ThrowOSStatus( ::PBHGetVInfoSync( (HParamBlockRec*) &pb ) );
	}
	
	static inline void PBHGetVInfoSync( XVolumeParam& pb, N::FSVolumeRefNum vRefNum, StringPtr name = NULL )
	{
		PBHGetVInfoSync( (HVolumeParam&) pb, vRefNum, name );
	}
	
	static void PBXGetVolInfoSync( XVolumeParam& pb, N::FSVolumeRefNum vRefNum, StringPtr name = NULL )
	{
		pb.ioNamePtr  = name;
		pb.ioVRefNum  = vRefNum;
		pb.ioXVersion = 0;
		pb.ioVolIndex = 0;
		
		Mac::ThrowOSStatus( ::PBXGetVolInfoSync( &pb ) );
	}
	
	static void GetVolInfo( XVolumeParam& pb, const vfs::node& that, StringPtr name )
	{
		const N::FSVolumeRefNum vRefNum = GetKeyFromParent( that );
		
		if ( Has_PBXGetVolInfo() )
		{
			PBXGetVolInfoSync( pb, vRefNum, name );
		}
		else
		{
			PBHGetVInfoSync( pb, vRefNum, name );
		}
	}
	
	template < class Accessor >
	struct sys_mac_vol_N_Property : readonly_property
	{
		static const int fixed_size = Accessor::fixed_size;
		
		typedef N::FSVolumeRefNum Key;
		
		static typename Accessor::result_type Get( const vfs::node* that )
		{
			XVolumeParam pb;
			
			Str31 name;
			
			GetVolInfo( pb, *that, Accessor::needsName ? name : NULL );
			
			return Accessor::Get( pb );
		}
		
		static void get( plus::var_string& result, const vfs::node* that, bool binary )
		{
			const typename Accessor::result_type data = Get( that );
			
			if ( Accessor::neverZero  &&  data == 0 )
			{
				p7::throw_errno( ENOENT );
			}
			
			Accessor::deconstruct::apply( result, data, binary );
		}
	};
	
	struct sys_mac_vol_N_name : sys_mac_vol_N_Property< GetVolumeName >
	{
		static const bool can_set = true;
		
		static void set( const vfs::node* that, const char* begin, const char* end, bool binary )
		{
			const N::FSVolumeRefNum vRefNum = GetKeyFromParent( *that );
			
			N::Str27 name( begin, end - begin );
			
			Mac::ThrowOSStatus( ::HRename( vRefNum, fsRtDirID, "\p", name ) );
		}
	};
	
	static vfs::node_ptr folder_link_resolve( const vfs::node* that )
	{
		const char* name = that->name().c_str();
		
		const N::FolderType type = name[0] == 's' ? N::kSystemFolderType
		                         : name[0] == 't' ? N::kTemporaryFolderType
		                         :                  N::FolderType();
		
		const Mac::FSVolumeRefNum vRefNum = GetKeyFromParent( *that->owner() );
		
		return FSTreeFromFSDirSpec( N::FindFolder( vRefNum, type, false ) );
	}
	
	static const link_method_set folder_link_link_methods =
	{
		NULL,
		&folder_link_resolve
	};
	
	static const node_method_set folder_link_methods =
	{
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		&folder_link_link_methods
	};
	
	
	static vfs::node_ptr Root_Factory( const vfs::node*     parent,
	                                   const plus::string&  name,
	                                   const void*          args )
	{
		N::FSVolumeRefNum key = GetKeyFromParent( *parent );
		
		const Mac::FSDirSpec volume = n::make< Mac::FSDirSpec >( key, N::fsRtDirID );
		
		return FSTreeFromFSDirSpec( volume );
	}
	
	static vfs::node_ptr Drive_Link_Factory( const vfs::node*     parent,
	                                         const plus::string&  name,
	                                         const void*          args )
	{
		N::FSVolumeRefNum key = GetKeyFromParent( *parent );
		
		HVolumeParam pb;
		
		PBHGetVInfoSync( pb, key );
		
		if ( pb.ioVDrvInfo == 1 )
		{
			p7::throw_errno( ENOENT );
		}
		
		plus::string drive = gear::inscribe_decimal( pb.ioVDrvInfo );
		
		drive = "/sys/mac/drive/" + drive;
		
		return vfs::new_symbolic_link( parent, name, drive );
	}
	
	static vfs::node_ptr Driver_Link_Factory( const vfs::node*     parent,
	                                          const plus::string&  name,
	                                          const void*          args )
	{
		N::FSVolumeRefNum key = GetKeyFromParent( *parent );
		
		HVolumeParam pb;
		
		PBHGetVInfoSync( pb, key );
		
		if ( pb.ioVDRefNum == 0 )
		{
			p7::throw_errno( ENOENT );
		}
		
		plus::string unit = gear::inscribe_decimal( ~pb.ioVDRefNum );
		
		unit = "/sys/mac/unit/" + unit;
		
		return vfs::new_symbolic_link( parent, name, unit );
	}
	
	static vfs::node_ptr Folder_Link_Factory( const vfs::node*     parent,
	                                          const plus::string&  name,
	                                          const void*          args )
	{
		N::FSVolumeRefNum key = GetKeyFromParent( *parent );
		
		return new vfs::node( parent, name, S_IFLNK | 0777, &folder_link_methods );
	}
	
	static vfs::node_ptr volume_trigger_factory( const vfs::node*     parent,
	                                             const plus::string&  name,
	                                             const void*          args )
	{
		const Mac::FSVolumeRefNum vRefNum = GetKeyFromParent( *parent );
		
		const trigger_extra extra = { (trigger_function) args, vRefNum };
		
		return trigger_factory( parent, name, &extra );
	}
	
	
	#define PREMAPPED( map )  &vfs::fixed_dir_factory, (const void*) map
	
	#define PROPERTY( prop )  &new_property, &property_params_factory< prop >::value
	
	#define PROPERTY_ACCESS( access )  PROPERTY( sys_mac_vol_N_Property< access > )
	
	const vfs::fixed_mapping sys_mac_vol_N_Mappings[] =
	{
		{ ".mac-name", PROPERTY( sys_mac_vol_N_name ) },
		
		{ "name", PROPERTY( utf8_text_property< sys_mac_vol_N_name > ) },
		
		{ "block-size",  PROPERTY_ACCESS( GetVolumeBlockSize      ) },
		{ "blocks",      PROPERTY_ACCESS( GetVolumeBlockCount     ) },
		{ "blocks-free", PROPERTY_ACCESS( GetVolumeFreeBlockCount ) },
		
		{ "capacity",  PROPERTY_ACCESS( GetVolumeCapacity  ) },
		{ "freespace", PROPERTY_ACCESS( GetVolumeFreeSpace ) },
		
		{ "sig", PROPERTY_ACCESS( GetVolumeSignature ) },
		
		{ "drive",  &Drive_Link_Factory  },
		{ "driver", &Driver_Link_Factory },
		
		{ "fsid", PROPERTY_ACCESS( GetVolumeFSID ) },
		
		{ "writes", PROPERTY_ACCESS( GetVolumeWriteCount ) },
		{ "files",  PROPERTY_ACCESS( GetVolumeFileCount  ) },
		{ "dirs",   PROPERTY_ACCESS( GetVolumeDirCount   ) },
		
		{ "dt",    PREMAPPED( sys_mac_vol_list_N_dt_Mappings ) },
		{ "parms", PREMAPPED( sys_mac_vol_N_parms_Mappings   ) },
		
		// volume roots are named "mnt", not the volume name
		{ "mnt",  &Root_Factory },
		
		{ "flush",  &volume_trigger_factory, (void*) &volume_flush_trigger   },
		{ "umount", &volume_trigger_factory, (void*) &volume_unmount_trigger },
		
	#if !TARGET_API_MAC_CARBON
		
		{ "eject",  &volume_trigger_factory, (void*) &volume_eject_trigger   },
		
	#endif
		
		{ "sys", &Folder_Link_Factory },
		{ "tmp", &Folder_Link_Factory },
		
		{ NULL, NULL }
		
	};
	
	vfs::node_ptr New_FSTree_sys_mac_vol( const vfs::node*     parent,
	                                      const plus::string&  name,
	                                      const void*          args )
	{
		return new_basic_directory( parent, name, vol_lookup, vol_iterate );
	}
	
	vfs::node_ptr Get_sys_mac_vol_N( N::FSVolumeRefNum vRefNum )
	{
		vfs::node_ptr parent = vfs::resolve_absolute_path( *relix::root(), STR_LEN( "/sys/mac/vol/list" ) );
		
		const plus::string name = gear::inscribe_decimal( -vRefNum );
		
		return fixed_dir( parent.get(), name, sys_mac_vol_N_Mappings );
	}
	
}

