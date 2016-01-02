/**************************************************************************/
/*                                                                        */
/*                              WWIV Version 5.x                          */
/*             Copyright (C)1998-2016, WWIV Software Services             */
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
#include "wwivutil/wwivutil.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "core/command_line.h"
#include "core/log.h"
#include "core/file.h"
#include "core/scope_exit.h"
#include "core/strings.h"
#include "core/stl.h"
#include "sdk/config.h"
#include "wwivutil/messages/messages.h"
#include "wwivutil/net/net.h"
#include "wwivutil/fix/fix.h"

using std::map;
using std::string;
using std::vector;
using namespace wwiv::core;
using namespace wwiv::strings;
using namespace wwiv::sdk;

namespace wwiv {
namespace wwivutil {

class WWIVUtil {
public:
  WWIVUtil(int argc, char *argv[]) : cmdline_(argc, argv, "network_number") {
    Logger::Init(argc, argv);
    cmdline_.AddStandardArgs();
  }
  ~WWIVUtil() {}

  int Main() {
    ScopeExit at_exit(Logger::ExitLogger);
    try {
      Add(new MessagesCommand());
      Add(new NetCommand());
      Add(new FixCommand());

      if (!cmdline_.Parse()) { return 1; }
      const std::string bbsdir(cmdline_.arg("bbsdir").as_string());
      Config config(bbsdir);
      if (!config.IsInitialized()) {
        LOG << "Unable to load CONFIG.DAT.";
        return 1;
      }
      command_config_.reset(new Configuration(bbsdir, &config));
      SetConfigs();
      return cmdline_.Execute();
    } catch (std::exception& e) {
      LOG << e.what();
    }
    return 1;
  }

private:
  void Add(UtilCommand* cmd) {
    cmdline_.add(cmd);
    cmd->AddStandardArgs();
    cmd->AddSubCommands();
    subcommands_.push_back(cmd);
  }

  void SetConfigs() {
    for (auto s : subcommands_) {
      s->set_config(command_config_.get());
    }
  }
  std::vector<UtilCommand*> subcommands_;
  CommandLine cmdline_;
  std::unique_ptr<Configuration> command_config_;
};

}  // namespace wwivutil
}  // namespace wwiv

int main(int argc, char *argv[]) {
  wwiv::wwivutil::WWIVUtil wwivutil(argc, argv);
  return wwivutil.Main();
}