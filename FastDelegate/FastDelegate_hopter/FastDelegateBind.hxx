//						FastDelegateBind.h 
//  Helper file for FastDelegates. Provides bind() function, enabling
//  FastDelegates to be rapidly compared to programs using boost::function and boost::bind.
//
//  Documentation is found at http://www.codeproject.com/cpp/FastDelegate.asp
//
//		Original author: Jody Hagins.
//		 Minor changes by Don Clugston.
//
// Warning: The arguments to 'bind' are ignored! No actual binding is performed.
// The behaviour is equivalent to boost::bind only when the basic placeholder 
// arguments _1, _2, _3, etc are used in order.
//
// HISTORY:
//	1.4 Dec 2004. Initial release as part of FastDelegate 1.4.


#ifndef FASTDELEGATEBIND_H
#define FASTDELEGATEBIND_H
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

////////////////////////////////////////////////////////////////////////////////
//						FastDelegate bind()
//
//				bind() helper function for boost compatibility.
//				(Original author: Jody Hagins).
//
// Add another helper, so FastDelegate can be a dropin replacement
// for boost::bind (in a fair number of cases).
// Note the elipses, because boost::bind() takes place holders
// but FastDelegate does not care about them.  Getting the place holder
// mechanism to work, and play well with boost is a bit tricky, so
// we do the "easy" thing...
// Assume we have the following code...
//      using boost::bind;
//      bind(&Foo:func, &foo, _1, _2);
// we should be able to replace the "using" with...
//      using fastdelegate::bind;
// and everything should work fine...
////////////////////////////////////////////////////////////////////////////////

#ifdef FASTDELEGATE_ALLOW_FUNCTION_TYPE_SYNTAX

namespace fastdelegate {

@VARARGS
template <class X, class Y, class RetType, @CLASSARGS>
FastDelegate< RetType ( @FUNCARGS ) >
bind(
    RetType (X::*func)( @FUNCARGS ),
    Y * y,
    ...)
{ 
  return FastDelegate< RetType ( @FUNCARGS ) >(y, func);
}

template <class X, class Y, class RetType, @CLASSARGS>
FastDelegate< RetType ( @FUNCARGS ) >
bind(
    RetType (X::*func)( @FUNCARGS ) const,
    Y * y,
    ...)
{ 
  return FastDelegate< RetType ( @FUNCARGS ) >(y, func);
}

@ENDVAR

#endif //FASTDELEGATE_ALLOW_FUNCTION_TYPE_SYNTAX

} // namespace fastdelegate

#endif // !defined(FASTDELEGATEBIND_H)

