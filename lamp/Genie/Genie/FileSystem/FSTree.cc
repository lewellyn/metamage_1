/*	=========
 *	FSTree.cc
 *	=========
 */

#include "Genie/FileSystem/FSTree.hh"

// Standard C++
#include <algorithm>
#include <functional>
#include <map>
#include <vector>

// MoreFiles
#include "MoreFilesExtras.h"

// Nucleus
#include "Nucleus/NAssert.h"

// Nitrogen Extras / Iteration
#include "Iteration/FSContents.h"

// Nitrogen Extras / Utilities
#include "Utilities/Files.h"

// Genie
#include "Genie/FileSystem/StatFile.hh"
#include "Genie/Process.hh"


namespace Genie
{
	
	typedef std::vector< FSNode > FSTreeCache;
	
	typedef boost::shared_ptr< const FSTreeCache > FSTreeCachePtr;
	
	class FSIterator_Cache : public FSIterator
	{
		private:
			FSTreeCachePtr  contents;
			std::size_t     nextOffset;
		
		public:
			FSIterator_Cache( const FSTreeCachePtr& cache ) : contents( cache ), nextOffset( 0 )  {}
			
			FSNode Get() const  { return nextOffset < contents->size() ? contents->at( nextOffset ) : FSNode(); }
			
			void Advance()  { ++nextOffset; }
			
			void Rewind()  { nextOffset = 0; }
			
			void Seek( off_t index )  { nextOffset = index; }
			
			off_t Tell() const  { return nextOffset; }
	};
	
	class FSTree_Regular : public FSTree
	{
		private:
			typedef std::map< std::string, FSTreePtr > MountPoints;
			
			MountPoints mountPoints;
		
		public:
			virtual ~FSTree_Regular()  {}
			
			FSTreePtr Lookup( const std::string& name ) const;
			
			void Mount( const std::string& name, FSTreePtr tree );
			
			FSTreePtr Lookup_MountPoint( const std::string& name ) const;
			
			virtual FSTreePtr Lookup_Regular( const std::string& name ) const = 0;
	};
	
	class FSTree_FSSpec : public FSTree_Regular
	{
		protected:
			FSSpec fileSpec;
		
		public:
			FSTree_FSSpec( const FSSpec& item ) : fileSpec( item )  {}
			
			virtual void Stat( struct ::stat& sb ) const;
			
			FSTreePtr Lookup_Regular( const std::string& name ) const;
			
			FSIteratorPtr Iterate() const;
	};
	
	class FSTree_FSSpec_ResFile : public FSTree_FSSpec
	{
		public:
			FSTree_FSSpec_ResFile( const FSSpec& file ) : FSTree_FSSpec( file )  {}
			
			void Stat( struct ::stat& sb ) const;
	};
	
	class FSTree_Virtual : public FSTree_Regular
	{
	};
	
	class FSTree_Volumes : public FSTree
	{
		public:
			void Stat( struct ::stat& sb ) const;
			
			FSTreePtr Lookup( const std::string& name ) const;
			
			FSIteratorPtr Iterate() const;
	};
	
	class FSTree_proc : public FSTree
	{
		public:
			void Stat( struct ::stat& sb ) const;
			
			FSTreePtr Lookup( const std::string& name ) const;
			
			FSIteratorPtr Iterate() const;
	};
	
	
	static N::FSDirSpec FindJDirectory()
	{
		FSSpec result;
		
		// Try current directory first
		if ( N::FSpTestDirectoryExists( result = N::FSDirSpec() & "j" ) )
		{
			return NN::Convert< N::FSDirSpec >( result );
		}
		
		// Then root, or bust
		return N::RootDirectory( N::BootVolume() ) << "j";
	}
	
	static FSTreePtr MakeFSRoot()
	{
		FSTree_FSSpec* tree = NULL;
		
		FSTreePtr result( tree = new FSTree_FSSpec( FindJDirectory() & "" ) );
		
		tree->Mount( "Volumes", FSTreePtr( new FSTree_Volumes() ) );
		tree->Mount( "proc",    FSTreePtr( new FSTree_proc()    ) );
		
		return result;
	}
	
	FSTreePtr FSRoot()
	{
		static FSTreePtr root = MakeFSRoot();
		
		return root;
	}
	
	FSTreePtr FSTreeFromFSSpec( const FSSpec& item )
	{
		return FSTreePtr( new FSTree_FSSpec( item ) );
	}
	
	
	namespace N = Nitrogen;
	namespace NN = Nucleus;
	
	static N::FSVolumeRefNum DetermineVRefNum( ConstStr255Param   name,
	                                           N::FSVolumeRefNum  vRefNum = N::FSVolumeRefNum() )
	{
		::FSVolumeRefNum actualVRefNum;
		N::ThrowOSStatus( ::DetermineVRefNum( name, vRefNum, &actualVRefNum ) );
		
		return N::FSVolumeRefNum( actualVRefNum );
	}
	
	static N::FSVolumeRefNum DetermineVRefNum( const std::string&  name,
	                                           N::FSVolumeRefNum   vRefNum = N::FSVolumeRefNum() )
	{
		return DetermineVRefNum( N::Str27( name ), vRefNum );
	}
	
	void FSTree_Regular::Mount( const std::string& name, FSTreePtr tree )
	{
		mountPoints[ name ] = tree;
	}
	
	FSTreePtr FSTree_Regular::Lookup( const std::string& name ) const
	{
		FSTreePtr result = Lookup_MountPoint( name );
		
		if ( result == NULL )
		{
			result = Lookup_Regular( name );
		}
		
		return result;
	}
	
	FSTreePtr FSTree_Regular::Lookup_MountPoint( const std::string& name ) const
	{
		MountPoints::const_iterator found = mountPoints.find( name );
		
		return found != mountPoints.end() ? found->second : FSTreePtr();
	}
	
	
	static FSNode MakeFSNode_FSSpec( const FSSpec& item )
	{
		std::string name = NN::Convert< std::string >( item.name );
		
		FSTreePtr tree( new FSTree_FSSpec( item ) );
		
		return FSNode( name, tree );
	}
	
	static FSNode MakeFSNode_Volume( N::FSVolumeRefNum vRefNum )
	{
		FSSpec volume = N::FSMakeFSSpec( vRefNum, N::FSDirID( long( fsRtDirID ) ), "\p" );
		
		std::string name = NN::Convert< std::string >( volume.name );
		
		FSTreePtr tree( new FSTree_FSSpec( volume ) );
		
		return FSNode( name, tree );
	}
	
	static FSNode MakeFSNode_proc( GenieProcessTable::ProcessMap::value_type proc )
	{
		pid_t pid = proc.first;
		
		std::string name = NN::Convert< std::string >( pid );
		
		//FSTreePtr tree( new FSTree_pid( pid ) );
		FSTreePtr tree;
		
		return FSNode( name, tree );
	}
	
	
	void FSTree_FSSpec::Stat( struct ::stat& sb ) const
	{
		StatFile( fileSpec, &sb );
	}
	
	FSTreePtr FSTree_FSSpec::Lookup_Regular( const std::string& name ) const
	{
		if ( N::FSpTestFileExists( fileSpec )  &&  name == "rsrc" )
		{
			return FSTreePtr( new FSTree_FSSpec_ResFile( fileSpec ) );
		}
		
		N::FSDirSpec dir = NN::Convert< N::FSDirSpec >( fileSpec );
		
		FSSpec item = dir & name;
		
		return FSTreePtr( new FSTree_FSSpec( item ) );
	}
	
	FSIteratorPtr FSTree_FSSpec::Iterate() const
	{
		FSTreePtr thisDir( new FSTree_FSSpec( *this ) );
		
		FSTreePtr parentDir( new FSTree_FSSpec( NN::Convert< FSSpec >( N::FSpGetParent( fileSpec ) ) ) );
		
		N::FSDirSpec dir( NN::Convert< N::FSDirSpec >( fileSpec ) );
		
		N::FSSpecContents_Container contents = N::FSContents( dir );
		
		FSTreeCache cache( 2 + contents.size() );
		
		cache[0] = FSNode( ".",  thisDir   );
		cache[1] = FSNode( "..", parentDir );
		
		std::transform( contents.begin(),
		                contents.end(),
		                cache.begin() + 2,
		                std::ptr_fun( MakeFSNode_FSSpec ) );
		
		FSTreeCache* newCache = new FSTreeCache();
		
		FSTreeCachePtr cachePtr( newCache );
		
		std::swap( cache, *newCache );
		
		return FSIteratorPtr( new FSIterator_Cache( cachePtr ) );
	}
	
	void FSTree_FSSpec_ResFile::Stat( struct ::stat& sb ) const
	{
		StatFile( fileSpec, &sb, true );
	}
	
	
	void FSTree_Volumes::Stat( struct ::stat& sb ) const
	{
		StatGeneric( &sb );
	}
	
	FSTreePtr FSTree_Volumes::Lookup( const std::string& name ) const
	{
		if ( name == "." )
		{
			return FSTreePtr( new FSTree_Volumes( *this ) );
		}
		else if ( name == ".." )
		{
			return FSRoot();
		}
		
		return FSTreePtr( new FSTree_FSSpec( N::RootDirectory( DetermineVRefNum( name + ":" ) ) & "" ) );
	}
	
	FSIteratorPtr FSTree_Volumes::Iterate() const
	{
		FSTreePtr thisDir( new FSTree_Volumes( *this ) );
		
		FSTreePtr parentDir( FSRoot() );
		
		N::Volume_Container volumes = N::Volumes();
		
		FSTreeCache cache( 2 );
		
		cache[0] = FSNode( ".",  thisDir   );
		cache[1] = FSNode( "..", parentDir );
		
		std::transform( volumes.begin(),
		                volumes.end(),
		                std::back_inserter( cache ),
		                std::ptr_fun( MakeFSNode_Volume ) );
		
		FSTreeCache* newCache = new FSTreeCache();
		
		FSTreeCachePtr cachePtr( newCache );
		
		std::swap( cache, *newCache );
		
		return FSIteratorPtr( new FSIterator_Cache( cachePtr ) );
	}
	
	
	void FSTree_proc::Stat( struct ::stat& sb ) const
	{
		StatGeneric( &sb );
	}
	
	FSTreePtr FSTree_proc::Lookup( const std::string& name ) const
	{
		if ( name == "." )
		{
			return FSTreePtr( new FSTree_proc( *this ) );
		}
		else if ( name == ".." )
		{
			return FSRoot();
		}
		
		pid_t pid = std::atoi( name.c_str() );
		
		//return FSTreePtr( new FSTree_pid( pid ) );
		return FSTreePtr();
	}
	
	FSIteratorPtr FSTree_proc::Iterate() const
	{
		FSTreePtr thisDir( new FSTree_proc( *this ) );
		
		FSTreePtr parentDir( FSRoot() );
		
		FSTreeCache cache( 2 );
		
		cache[0] = FSNode( ".",  thisDir   );
		cache[1] = FSNode( "..", parentDir );
		
		std::transform( gProcessTable.begin(),
		                gProcessTable.end(),
		                std::back_inserter( cache ),
		                std::ptr_fun( MakeFSNode_proc ) );
		
		FSTreeCache* newCache = new FSTreeCache();
		
		FSTreeCachePtr cachePtr( newCache );
		
		std::swap( cache, *newCache );
		
		return FSIteratorPtr( new FSIterator_Cache( cachePtr ) );
	}
	
}

