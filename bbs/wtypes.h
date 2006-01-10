/**************************************************************************/
/*                                                                        */
/*                              WWIV Version 5.0x                         */
/*             Copyright (C)1998-2004, WWIV Software Services             */
/*                                                                        */
/*    Licensed  under the  Apache License, Version  2.0 (the "License");  */
/*    you may not use this  file  except in compliance with the License.  */
/*    You may obtain a copy of the License at                             */
/*                                                                        */
/*                http://www.apache.org/licenses/LICENSE-2.0              */
/*                                                                        */
/*    Unless  required  by  applicable  law  or agreed to  in  writing,   */
/*    software  distributed  under  the  License  is  distributed on an   */
/*    "AS IS"  BASIS, WITHOUT  WARRANTIES  OR  CONDITIONS OF ANY  KIND,   */
/*    either  express  or implied.  See  the  License for  the specific   */
/*    language governing permissions and limitations under the License.   */
/*                                                                        */
/**************************************************************************/

#ifndef __INCLUDED_WTYPES_H__
#define __INCLUDED_WTYPES_H__

//
// Defined for all platforms
//
typedef unsigned char       UCHAR8;
typedef signed char         INT8;
typedef signed char         CHAR8;
typedef short               INT16;
typedef unsigned char       UINT8;
typedef unsigned short      UINT16;
// This should only be used in files, where size matters, not for
// function return values, etc.


//
// Defined on everything except for WIN32
//
#if !defined (_WIN32)
typedef unsigned char       BYTE;
typedef unsigned char       UCHAR;
typedef unsigned short      WORD;
typedef unsigned long       DWORD;
typedef unsigned long       DWORD32;

typedef unsigned short      USHORT;
typedef unsigned int        UINT;
typedef int                 INT;
typedef long                INT32;
typedef unsigned long       UINT32;
typedef long                LONG32;
typedef unsigned long       ULONG32;

#ifndef LOBYTE
#define LOBYTE(w)           ((BYTE)(w))
#endif  // LOBYTE

#ifndef HIBYTE
#define HIBYTE(w)           ((BYTE)(((WORD)(w) >> 8) & 0xFF))
#endif  // HIBYTE

#endif // !_WIN32


//
// MSDOS specific defines
//
#if defined ( __MSDOS__ )
typedef unsigned char bool;
#endif // __MSDOS__


//
// WIN32 Specific
//

#if defined (__GNUC__) && defined (_WIN32)
typedef long                INT32;
typedef unsigned long       UINT32;
typedef unsigned long       ULONG32;
#endif // __GNUC__ && _WIN32


#if defined ( _WIN32 ) && ( _MSC_VER < 1300 )

#undef min
#undef max

// Add std::min and std::max because of MSVC 6.0 lameness.
namespace std
{

	template<class _Ty> inline
		const _Ty& max(const _Ty& a, const _Ty& b)
	{
		return a < b ? b : a;
	}

	template<class _Ty> inline
		const _Ty& min(const _Ty& a, const _Ty& b)
	{
		return b < a ? b : a;
	}

}

#endif // _MSC_VER < 1300


#endif	// __INCLUDED_WTYPES_H__
