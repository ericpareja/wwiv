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

#ifndef __INCLUDED_PLATFORM_LINUX_LINUXPLATFORM_H__
#define __INCLUDED_PLATFORM_LINUX_LINUXPLATFORM_H__


// stringstuff.cpp

char *strupr(char *s);
char *strlwr(char *s);

// filestuff.cpp

long filelength(int f);
void chsize(int f, size_t size);
bool WWIV_CopyFile(const char * szSourceFileName, const char * szDestFileName);

// utility2.cpp

char * strrev(char *s);
void WWIV_RebootComputer();


#endif	// __INCLUDED_PLATFORM_LINUX_LINUXPLATFORM_H__


