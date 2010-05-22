// Nitrogen/MacTypes.hh
// --------------------
//
// Maintained by Joshua Juran

// Part of the Nitrogen project.
//
// Written 2002-2007 by Lisa Lippincott and Joshua Juran.
//
// This code was written entirely by the above contributors, who place it
// in the public domain.


#ifndef NITROGEN_MACTYPES_HH
#define NITROGEN_MACTYPES_HH

#ifndef __MACTYPES__
#include <MacTypes.h>
#endif
#ifndef __SCRIPT__
#include <Script.h>
#endif

// nucleus
#include "nucleus/enumeration_traits.hh"
#include "nucleus/flag_ops.hh"
#include "nucleus/make.hh"
#include "nucleus/overloaded_math.hh"
#include "nucleus/scribe.hh"

// Nitrogen
#include "Mac/Resources/Types/ResType.hh"
#include "Mac/Toolbox/Types/OSType.hh"

#ifndef NITROGEN_OSSTATUS_HH
#include "Nitrogen/OSStatus.hh"
#endif

#include <cstddef>
#include <string>
#include <cmath>


namespace Nitrogen
  {
	
	using ::VersRec;
	
	inline std::size_t SizeOf_VersRec( const VersRec& version )
	{
		UInt8 shortVersionLength = version.shortVersion[ 0 ];
		
		// The long version string immediately follows the short version string.
		const UInt8* longVersion = version.shortVersion + 1 + shortVersionLength;
		UInt8 longVersionLength  = longVersion[ 0 ];
		
		return sizeof (::NumVersion)
		     + sizeof (SInt16)
		     + 1 + shortVersionLength
		     + 1 + longVersionLength;
	}
	
   using ::UInt8;
   using ::SInt8;
   using ::UInt16;
   using ::SInt16;
   using ::UInt32;
   using ::SInt32;

   typedef long long                wide;
   typedef unsigned long long       UnsignedWide;
	
	
   // Nitrogen uses floating point types in preference to fixed-point types.
   template < class Floating, int fractionBits, class Integral >
   inline Floating FixedToFloatingPoint( Integral in )
     {
      return std::ldexp( static_cast<Floating>(in), -fractionBits );
     }

   template < class Integral, int fractionBits, class Floating >
   inline Integral FloatingToFixedPoint( Floating in )
     {
      return static_cast< Integral >( nucleus::c_std::nearbyint( std::ldexp( in, fractionBits ) ) );
     }
   
   inline double FixedToDouble( ::Fixed in )                   { return FixedToFloatingPoint< double,  16 >( in ); }
   inline ::Fixed DoubleToFixed( double in )                   { return FloatingToFixedPoint< ::Fixed, 16 >( in ); }

   inline double UnsignedFixedToDouble( ::UnsignedFixed in )   { return FixedToFloatingPoint< double,          16 >( in ); }
   inline ::UnsignedFixed DoubleToUnsignedFixed( double in )   { return FloatingToFixedPoint< ::UnsignedFixed, 16 >( in ); }

   inline double FractToDouble( ::Fract in )                   { return FixedToFloatingPoint< double,  30 >( in ); }
   inline ::Fract DoubleToFract( double in )                   { return FloatingToFixedPoint< ::Fract, 30 >( in ); }

   inline double ShortFixedToDouble( ::ShortFixed in )         { return FixedToFloatingPoint< double,       8 >( in ); }
   inline ::ShortFixed DoubleToShortFixed( double in )         { return FloatingToFixedPoint< ::ShortFixed, 8 >( in ); }
   
   struct FixedFlattener
     {
      typedef double Put_Parameter;
      
      template < class Putter >
      static void Put( Put_Parameter toPut, Putter put )
        {
         const Fixed fixed = DoubleToFixed( toPut );
         put( &fixed, &fixed + 1 );
        }
      
      typedef double Get_Result;
      
      template < class Getter >
      static Get_Result Get( Getter get )
        {
         Fixed fixed;
         get( &fixed, &fixed + 1 );
         return FixedToDouble( fixed );
        }
      
      typedef Put_Parameter Parameter;
      typedef Get_Result    Result;
      
      static const bool hasStaticSize = true;
      
      typedef ::Fixed Buffer;
     };
   
   
   using ::Float32;
   using ::Float64;
   using ::Float80;
   using ::Float96;
   
   typedef ::std::size_t  Size;
	
	enum OptionBits
	{
		kNilOptions = ::kNilOptions,
		
		kOptionBits_Max = nucleus::enumeration_traits< ::OptionBits >::max
	};
	
	NUCLEUS_DEFINE_FLAG_OPS( OptionBits )
	
	enum ScriptCode
	{
		smSystemScript = ::smSystemScript,
		
		kScriptCode_Max = nucleus::enumeration_traits< ::ScriptCode >::max
	};
	
	enum LangCode
	{
		langUnspecified = ::langUnspecified,
		
		kLangCode_Max = nucleus::enumeration_traits< ::LangCode >::max
	};
	
	enum RegionCode
	{
		verUS = ::verUS,
		
		kRegionCode_Max = nucleus::enumeration_traits< ::RegionCode >::max
	};
	
	using Mac::OSType;
	using Mac::kUnknownType;
	
	using Mac::ResType;
	using Mac::kVersionResType;
	
	typedef bool Boolean;
	
	typedef nucleus::converting_POD_scribe< bool, ::Boolean > BooleanFlattener;
	
	enum Style
	{
		kStyle_Max = nucleus::enumeration_traits< ::Style >::max
	};
	
	NUCLEUS_DEFINE_FLAG_OPS( Style )

   using ::UnicodeScalarValue;
   using ::UTF32Char;
   using ::UniChar;
   using ::UTF16Char;
   using ::UTF8Char;

   typedef std::basic_string< UTF32Char > UTF32String;
   typedef std::basic_string< UTF16Char > UTF16String;
   typedef std::basic_string< UTF8Char > UTF8String;
   typedef std::basic_string< UniChar > UniString;
   
	template < class UTFChar >
	struct UnicodeFlattener
	{
		typedef const std::basic_string< UTFChar >& Put_Parameter;
		
		template < class Putter >
		static void Put( Put_Parameter toPut, Putter put )
		{
			const UTFChar* begin = &toPut[0];
			const std::size_t size = toPut.size() * sizeof (UTFChar);
			
			put( begin, begin + size );
		}
		
		typedef std::basic_string< UTFChar > Get_Result;
		
		template < class Getter >
		static Get_Result Get( Getter get )
		{
			const std::size_t size = get.size();
			Get_Result result;
			
			result.resize( size / sizeof (UTFChar) );
			
			UTFChar* begin = &result[0];
			
			get( begin, begin + size );
			
			return result;
		}
		
		typedef Put_Parameter Parameter;
		typedef Get_Result    Result;
		
		static const bool hasStaticSize = false;
		struct Buffer {};
	};
   
   using ::Point;
   using ::Rect;
  }

namespace nucleus
  {
   template <>
   struct maker< Point >
     {
      Point operator()( short v, short h ) const
        {
         Point result;
         result.v = v;
         result.h = h;
         return result;
        }
      
      Point operator()() const
        {
         return operator()( 0, 0 );
        }
     };

   template <>
   struct maker< Rect >
     {
      Rect operator()( short top, short left, short bottom, short right ) const
        {
         Rect result;
         result.top = top;
         result.left = left;
         result.bottom = bottom;
         result.right = right;
         return result;
        }
      
      Rect operator()() const
        {
         return operator()( 0, 0, 0, 0 );
        }
     };
  }

#endif
