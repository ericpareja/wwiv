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

#include "wwiv.h"
#include "WStringUtils.h"


/** Converts string to uppercase */
char *strupr(char *s)
{
	for(int i = 0; s[i] != 0; i++)
	{
		s[i] = wwiv::UpperCase<char>(s[i]);
	}

	return s;
}

/** Converts string to lowercase */
char *strlwr(char *s)
{
	for(int i = 0; s[i] != 0; i++)
	{
		s[i] = wwiv::LowerCase(s[i]);
	}

	return s;
}




