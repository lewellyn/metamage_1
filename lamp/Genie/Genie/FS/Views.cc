/*	========
 *	Views.cc
 *	========
 */

#include "Genie/FS/Views.hh"

#ifndef __MACTYPES__
#include <MacTypes.h>
#endif

// POSIX
#include <sys/stat.h>

// Standard C++
#include <map>

// iota
#include "iota/swap.hh"

// Debug
#include "debug/assert.hh"

// poseven
#include "poseven/types/errno_t.hh"

// PEdestal
#include "Pedestal/EmptyView.hh"

// Genie
#include "Genie/FS/dir_method_set.hh"
#include "Genie/FS/file_method_set.hh"
#include "Genie/FS/node_method_set.hh"
#include "Genie/FS/gui/port/ADDR.hh"
#include "Genie/Utilities/simple_map.hh"


namespace Genie
{
	
	namespace p7 = poseven;
	namespace Ped = Pedestal;
	
	
	class canary
	{
		private:
			bool it_lives;
		
		public:
			canary() : it_lives( true )
			{
			}
			
			~canary()
			{
				it_lives = false;
			}
			
			bool lives() const
			{
				return it_lives;
			}
	};
	
	struct ViewParameters
	{
		FSTreePtr      itsDelegate;
		ViewFactory    itsFactory;
		const FSTree*  itsWindowKey;
		
		ViewParameters() : itsFactory(), itsWindowKey()
		{
		}
		
		void swap( ViewParameters& other );
	};
	
	void ViewParameters::swap( ViewParameters& other )
	{
		itsDelegate.swap( other.itsDelegate );
		
		using iota::swap;
		
		swap( itsFactory,   other.itsFactory   );
		swap( itsWindowKey, other.itsWindowKey );
	}
	
	typedef std::map< plus::string, ViewParameters > ViewParametersSubMap;
	
	typedef simple_map< const FSTree*, ViewParametersSubMap > ViewParametersMap;
	
	static ViewParametersMap gViewParametersMap;
	
	static canary the_canary;
	
	
	static const ViewParameters* FindView( const FSTree* parent, const plus::string& name )
	{
		if ( const ViewParametersSubMap* the_submap = gViewParametersMap.find( parent ) )
		{
			const ViewParametersSubMap& submap = *the_submap;
			
			ViewParametersSubMap::const_iterator it = submap.find( name );
			
			if ( it != submap.end() )
			{
				return &it->second;
			}
		}
		
		return NULL;
	}
	
	static bool ViewExists( const FSTree* parent, const plus::string& name )
	{
		return FindView( parent, name ) != NULL;
	}
	
	static void AddViewParameters( const FSTree*        parent,
	                               const plus::string&  name,
	                               const FSTreePtr&     delegate,
	                               ViewFactory          factory )
	{
		ASSERT( delegate.get() != NULL );
		
		ViewParameters& params = gViewParametersMap[ parent ][ name ];
		
		params.itsFactory  = factory;
		params.itsDelegate = delegate;
	}
	
	static void AddViewWindowKey( const FSTree* parent, const plus::string& name, const FSTree* windowKey )
	{
		ASSERT( FindView( parent, name ) != NULL );
		
		ASSERT( FindView( parent, name )->itsDelegate.get() != NULL );
		
		gViewParametersMap[ parent ][ name ].itsWindowKey = windowKey;
	}
	
	static void DeleteDelegate( FSTreePtr& delegate_ref )
	{
		if ( const FSTree* delegate = delegate_ref.get() )
		{
			FSTreePtr delegate_copy;
			
			delegate_copy.swap( delegate_ref );
			
			try
			{
				delegate->Delete();
			}
			catch ( ... )
			{
				if ( TARGET_CONFIG_DEBUGGING )
				{
					// This might happen in __destroy_global_chain(),
					// so don't ASSERT which relies on trashed infrastructure.
					
					::DebugStr( "\p" "Delegate's Delete() method may not throw" );
				}
			}
			
			RemoveAllViewParameters( delegate );
		}
	}
	
	static void RemoveViewParameters( const FSTree* parent, const plus::string& name )
	{
		if ( ViewParametersSubMap* it = gViewParametersMap.find( parent ) )
		{
			ViewParametersSubMap& submap = *it;
			
			ViewParametersSubMap::iterator jt = submap.find( name );
			
			ViewParameters temp;
			
			if ( jt != submap.end() )
			{
				temp.swap( jt->second );
				
				submap.erase( jt );
				
				if ( submap.empty() )
				{
					gViewParametersMap.erase( it );
				}
				
				notify_port_of_view_loss( temp.itsWindowKey, temp.itsDelegate.get() );
				
				DeleteDelegate( temp.itsDelegate );
			}
		}
	}
	
	void RemoveAllViewParameters( const FSTree* parent )
	{
		if ( !the_canary.lives() )
		{
			return;
		}
		
		if ( ViewParametersSubMap* it = gViewParametersMap.find( parent ) )
		{
			ViewParametersSubMap temp = *it;
			
			gViewParametersMap.erase( it );
			
			for ( ViewParametersSubMap::iterator jt = temp.begin();  jt != temp.end();  ++jt )
			{
				ViewParameters& params = jt->second;
				
				notify_port_of_view_loss( params.itsWindowKey, params.itsDelegate.get() );
				
				DeleteDelegate( params.itsDelegate );
			}
		}
	}
	
	static boost::intrusive_ptr< Ped::View > MakeView( const FSTree*        parent,
	                                                   const plus::string&  name )
	{
		if ( const ViewParameters* params = FindView( parent, name ) )
		{
			const FSTree* delegate = params->itsDelegate.get();
			
			ViewFactory factory = params->itsFactory;
			
			ASSERT( delegate != NULL );
			ASSERT( factory  != NULL );
			
			return factory( delegate );
		}
		
		return boost::intrusive_ptr< Ped::View >();
	}
	
	static inline const FSTreePtr& GetViewDelegate( const FSTree* parent, const plus::string& name )
	{
		const ViewParameters* params = FindView( parent, name );
		
		if ( params == NULL )
		{
			p7::throw_errno( ENOENT );
		}
		
		return params->itsDelegate;
	}
	
	static const FSTreePtr& GetViewDelegate( const FSTree* view )
	{
		const FSTreePtr& delegate = GetViewDelegate( view->ParentRef().get(), view->Name() );
		
		return delegate;
	}
	
	static inline const FSTree* GetViewWindowKey( const FSTree* parent, const plus::string& name )
	{
		const ViewParameters* params = FindView( parent, name );
		
		return params ? params->itsWindowKey : NULL;
	}
	
	const FSTree* GetViewWindowKey( const FSTree* view )
	{
		return GetViewWindowKey( view->ParentRef().get(), view->Name() );
	}
	
	
	bool InvalidateWindowForView( const FSTree* view )
	{
		const FSTree* windowKey = GetViewWindowKey( view );
		
		return invalidate_port_WindowRef( windowKey );
	}
	
	
	static void new_view_hardlink( const FSTree*     node,
	                               const FSTreePtr&  dest );
	
	static const file_method_set new_view_file_methods =
	{
		NULL,
		NULL,
		&new_view_hardlink
	};
	
	static const node_method_set new_view_methods =
	{
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		&new_view_file_methods
	};
	
	struct new_view_extra
	{
		premapped::mapping const*  mappings;
		premapped::destructor      destructor;
		ViewFactory                view_factory;
		DelegateFactory            delegate_factory;
	};
	
	FSTreePtr New_new_view( const FSTreePtr&           parent,
	                        const plus::string&        name,
	                        ViewFactory                factory,
	                        const premapped::mapping*  mappings,
	                        premapped::destructor      dtor,
	                        DelegateFactory            delegate_factory )
	
	{
		FSTree* result = new FSTree( parent,
		                             name,
		                             S_IFREG | 0,
		                             &new_view_methods,
		                             sizeof (new_view_extra) );
		
		new_view_extra& extra = *(new_view_extra*) result->extra();
		
		extra.mappings         = mappings;
		extra.destructor       = dtor;
		extra.view_factory     = factory;
		extra.delegate_factory = delegate_factory;
		
		return result;
	}
	
	FSTreePtr create_default_delegate_for_new_view( const FSTree*        node,
	                                                const FSTreePtr&     parent,
	                                                const plus::string&  name )
	{
		new_view_extra& extra = *(new_view_extra*) node->extra();
		
		FSTreePtr delegate = Premapped_Factory( parent, name, extra.mappings, extra.destructor );
		
		return delegate;
	}
	
	static void new_view_hardlink( const FSTree*     node,
	                               const FSTreePtr&  target )
	{
		new_view_extra& extra = *(new_view_extra*) node->extra();
		
		const FSTreePtr& parent = target->ParentRef();
		
		const FSTree* key = parent.get();
		
		const plus::string& name = target->Name();
		
		FSTreePtr delegate = extra.delegate_factory( node, parent, name );
		
		AddViewParameters( key, name, delegate, extra.view_factory );
		
		target->CreateDirectory( 0 );  // mode is ignored
	}
	
	
	class FSTree_Unview : public FSTree
	{
		private:
			ViewGetter itsGetter;
			
			// Non-copyable
			FSTree_Unview           ( const FSTree_Unview& );
			FSTree_Unview& operator=( const FSTree_Unview& );
		
		public:
			FSTree_Unview( const FSTreePtr&     parent,
			               const plus::string&  name,
			               ViewGetter           get );
			
			const FSTree* ParentKey() const  { return ParentRef().get(); }
			
			void CreateDirectory( mode_t mode ) const;
			
			boost::intrusive_ptr< Pedestal::View >& Get() const
			{
				ASSERT( itsGetter != NULL );
				
				return itsGetter( ParentKey(), Name() );
			}
	};
	
	static void unview_mkdir( const FSTree* node, mode_t mode )
	{
		const FSTree_Unview* file = static_cast< const FSTree_Unview* >( node );
		
		file->CreateDirectory( mode );
	}
	
	static const dir_method_set unview_dir_methods =
	{
		NULL,
		NULL,
		&unview_mkdir
	};
	
	static const node_method_set unview_methods =
	{
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		&unview_dir_methods
	};
	
	
	class FSTree_View : public FSTree
	{
		private:
			ViewGetter  itsGetter;
			ViewPurger  itsPurger;
			
			// Non-copyable
			FSTree_View           ( const FSTree_View& );
			FSTree_View& operator=( const FSTree_View& );
		
		public:
			FSTree_View( const FSTreePtr&     parent,
			             const plus::string&  name,
			             ViewGetter           get,
			             ViewPurger           purge );
			
			const FSTree* ParentKey() const  { return ParentRef().get(); }
			
			void Delete() const;
			
			boost::intrusive_ptr< Pedestal::View >& Get() const
			{
				ASSERT( itsGetter != NULL );
				
				return itsGetter( ParentKey(), Name() );
			}
	};
	
	FSTree_Unview::FSTree_Unview( const FSTreePtr&     parent,
	                              const plus::string&  name,
	                              ViewGetter           get )
	:
		FSTree( parent, name, 0, &unview_methods ),
		itsGetter( get )
	{
	}
	
	static void view_touch( const FSTree* node )
	{
		InvalidateWindowForView( node );
	}
	
	static void view_remove( const FSTree* node )
	{
		const FSTree_View* file = static_cast< const FSTree_View* >( node );
		
		return file->Delete();
	}
	
	static FSTreePtr view_lookup( const FSTree*        node,
	                              const plus::string&  name,
	                              const FSTree*        parent );
	
	static void view_listdir( const FSTree*  node,
	                          FSTreeCache&   cache );
	
	static const dir_method_set view_dir_methods =
	{
		&view_lookup,
		&view_listdir
	};
	
	static node_method_set view_methods =
	{
		NULL,
		NULL,
		&view_touch,
		NULL,
		&view_remove,
		NULL,
		NULL,
		NULL,
		&view_dir_methods
	};
	
	FSTree_View::FSTree_View( const FSTreePtr&     parent,
	                          const plus::string&  name,
	                          ViewGetter           get,
	                          ViewPurger           purge )
	:
		FSTree( parent, name, S_IFDIR | 0700, &view_methods ),
		itsGetter( get   ),
		itsPurger( purge )
	{
	}
	
	void FSTree_View::Delete() const
	{
		const FSTree* parent = ParentKey();
		
		const plus::string& name = Name();
		
		const FSTree* windowKey = GetViewWindowKey( this );
		
		uninstall_view_from_port( Get(), windowKey );
		
		Get() = Ped::EmptyView::Get();
		
		RemoveViewParameters( parent, name );
		
		if ( itsPurger )
		{
			itsPurger( parent, name );
		}
	}
	
	void FSTree_Unview::CreateDirectory( mode_t mode ) const
	{
		const FSTree* parent = ParentRef().get();
		
		const plus::string& name = Name();
		
		const FSTree* windowKey = GetViewWindowKey( parent );
		
		if ( windowKey == NULL )
		{
			windowKey = parent;
		}
		
		AddViewWindowKey( parent, name, windowKey );
		
		boost::intrusive_ptr< Ped::View > view = MakeView( parent, name );
		
		Get() = view;
		
		// Install and invalidate if window exists
		install_view_in_port( view, windowKey );
	}
	
	static FSTreePtr view_lookup( const FSTree*        node,
	                              const plus::string&  name,
	                              const FSTree*        parent )
	{
		const plus::string& real_name = name.empty() ? plus::string( "." ) : name;
		
		return GetViewDelegate( node )->Lookup( real_name, NULL );
	}
	
	static void view_listdir( const FSTree*  node,
	                          FSTreeCache&   cache )
	{
		GetViewDelegate( node )->IterateIntoCache( cache );
	}
	
	FSTreePtr New_View( const FSTreePtr&     parent,
	                    const plus::string&  name,
	                    ViewGetter           get,
	                    ViewPurger           purge )
	{
		const bool exists = ViewExists( parent.get(), name );
		
		typedef const FSTree* T;
		
		return exists ? T( new FSTree_View  ( parent, name, get, purge ) )
		              : T( new FSTree_Unview( parent, name, get        ) );
	}
	
}

