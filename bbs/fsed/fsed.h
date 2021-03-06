/**************************************************************************/
/*                                                                        */
/*                              WWIV Version 5.x                          */
/*                   Copyright (C)2020, WWIV Software Services            */
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
/**************************************************************************/
#ifndef __INCLUDED_BBS_FSED_FSED_H__
#define __INCLUDED_BBS_FSED_FSED_H__

#include "bbs/message_editor_data.h"
#include "bbs/fsed/model.h"
#include "bbs/fsed/line.h"
#include <filesystem>
#include <vector>
#include <string>

namespace wwiv::bbs::fsed {

bool fsed(const std::filesystem::path& path);
bool fsed(FsedModel& ed, MessageEditorData& data, bool file);
bool fsed(std::vector<std::string>& lin, int maxli, MessageEditorData& data, bool file);

}

#endif