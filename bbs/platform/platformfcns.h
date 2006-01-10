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

#ifndef __INCLUDED_PLATFORM_FCNS_H__
#define __INCLUDED_PLATFORM_FCNS_H__

#if defined (_UNIX)
#include "linux/linuxplatform.h"
#endif // _UNIX

// $PLATFORM/filesupp.cpp

bool WWIV_CopyFile(const char * szSourceFileName, const char * szDestFileName);
double WWIV_GetFreeSpaceForPath(const char * szPath);
void WWIV_ChangeDirTo(const char *s);
void WWIV_GetDir(char *s, bool be);
void WWIV_GetFileNameFromPath(const char *pszPath, char *pszFileName);



// $PLATFORM/reboot.cpp

void WWIV_RebootComputer();


// $PLATFORM/reboot.cpp

void show_files( const char *pszFileName, const char *pszDirectoryName );


// $PLATFORM/utility2.cpp
char *WWIV_make_abs_cmd(char *out);
int WWIV_make_path(char *s);
void WWIV_Delay(unsigned long usec);
void WWIV_OutputDebugString( const char *pszString );


// $PLATFORM/exec.cpp
int ExecExternalProgram( const char *pszCommandLine, int flags );


#endif // __INCLUDED_PLATFORM_FCNS_H__
