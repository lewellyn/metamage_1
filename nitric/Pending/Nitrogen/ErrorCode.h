// ErrorCode.h

/*
   One of the more distressing problems arising when using C code from C++
   is that C functions return error codes, and C++ code gets along better
   with exceptions.  A common approach to makng the transition is to wrap
   error codes in a class which is then thrown.
   
   But this approach doesn't produce exceptions with the fine granularity
   one expects from C++ exceptions.  Often one must catch the error class,
   test the code it contains, and rethrow those exceptions one didn't want
   to catch.
   
   These templates extend the error class approch to provide fine-grained
   exception throwing.  Consider an error code type ErrorNumber, and an
   exception class wrapper ErrorClass for that type.

   
   The classes generated by the template
   
      template < class ErrorClass, ErrorNumber number > class ErrorCode
   
   each represent a particular, numbered error.   These classes all
   descend from ErrorClass, and initialize that base class with their
   particular number.
   
   
   The function template
   
      template < class ErrorClass > void ThrowErrorCode( ErrorClass n )
   
   throws an exception representing the error given by n.  The type of the
   exception thrown is either ErrorClass or a subclass of ErrorClass.
      
      -- If the error number n has been registered, the exception has type
            ErrorCode< ErrorClass, n >
      -- Otherwise, the exception thrown has type ErrorClass.
   
   In most cases, this function isn't called directly; instead, one calls a
   wrapper function which filters out those numbers which indicate success.
   
   
   The functions
   
      template < class ErrorClass, ErrorNumber number > void RegisterErrorCode()
   
   regsiter particular error numbers, ensuring that they will be thrown as
   ErrorCode< ErrorClass, number > by ThrowErrorCode< ErrorClass >.
   
   
   --- Advanced usage ---
   
   The ErrorNumber type is inferred from the ErrorClass type by a traits class
   
      template < class ErrorClass >
      struct ErrorClassTraits
        {
         typedef typename ErrorClass::ErrorNumber ErrorNumber;
        };
   
   Error classes may specify their underlying error code type with a typedef,
   or one may use this sceme with an existing error class by specializing
   ErrorClassTraits.
   
   The template function Convert is used to convert ErrorNumber to ErrorClass
   and vice versa.  By default, it attempts an implicit conversion; if
   these conversions are not allowed, Convert may be extended to provide
   the necessary conversions.
   
   The class template ErrorCode may be specialized to provide a richer
   representation of particular errors.  For example, one might use a
   specialization derived from both ErrorClass and std::bad_alloc to
   represent a memory allocation error.

   Likewise, the function template used to throw the specific exceptions,
   
      template < class Exception > void Throw()
   
   may be specialized to modify the way a perticular code is thrown.
*/

#ifndef NITROGEN_ERRORCODE_H
#define NITROGEN_ERRORCODE_H

#ifndef NITROGEN_CONVERT_H
#include "Nitrogen/Convert.h"
#endif

#include <map>

namespace Nitrogen
  {   
   template < class ErrorClass >
   struct ErrorClassTraits
     {
      typedef typename ErrorClass::ErrorNumber ErrorNumber;
     };   
   
   template < class ErrorClass, typename ErrorClassTraits<ErrorClass>::ErrorNumber number >
   class ErrorCode: public ErrorClass
     {
      public:
         ErrorCode()
           : ErrorClass( Convert<ErrorClass>( number ) )
           {}
     };


   template < class Exception >
   void Throw()
     {
      throw Exception();
     }

   
   template < class ErrorClass >
   class ErrorCodeThrower
     {
      private:
         typedef typename ErrorClassTraits< ErrorClass >::ErrorNumber ErrorNumber;
         
         typedef std::map< ErrorNumber, void(*)() > Map;
      
         Map map;

         // not implemented:
            ErrorCodeThrower( const ErrorCodeThrower& );
            ErrorCodeThrower& operator=( const ErrorCodeThrower& );
      
      public:
         ErrorCodeThrower()
           {}
         
         template < ErrorNumber number >
         void Register()
           {
            map[ number ] = Nitrogen::Throw< ErrorCode< ErrorClass, number > >;
           }
         
         void Throw( ErrorClass error ) const
           {
            typename Map::const_iterator found = map.find( Convert<ErrorNumber>( error ) );
            if ( found != map.end() )
               return found->second();
            throw error;
           }
     };


   template < class ErrorClass >
   ErrorCodeThrower<ErrorClass>& TheGlobalErrorCodeThrower()
     {
      static ErrorCodeThrower<ErrorClass> theGlobalErrorCodeThrower;
      return theGlobalErrorCodeThrower;
     }
   
   template < class ErrorClass, typename ErrorClassTraits<ErrorClass>::ErrorNumber number >
   void RegisterErrorCode()
     {
      TheGlobalErrorCodeThrower<ErrorClass>().template Register<number>();
     }
   
   template < class ErrorClass >
   void ThrowErrorCode( ErrorClass error )
     {
      TheGlobalErrorCodeThrower<ErrorClass>().Throw( error );
     }
  }

#endif
