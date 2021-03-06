/**************************************************************************/
/*                                                                        */
/*                              WWIV Version 5.x                          */
/*             Copyright (C)1998-2020, WWIV Software Services             */
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
#ifndef __INCLUDED_BBS_INTERPRET_H__
#define __INCLUDED_BBS_INTERPRET_H__

#include "bbs/bbs.h"
#include "sdk/ansi/ansi.h"
#include "sdk/user.h"
#include <string>

class MacroContext {
public:
  virtual ~MacroContext() = default;
  virtual const wwiv::sdk::User& u() const = 0;
  virtual const wwiv::sdk::files::directory_t& dir() const = 0;
  virtual bool mci_enabled() const = 0;
  virtual std::string interpret(char c) const;
};

class BbsMacroContext : public MacroContext {
public:
  BbsMacroContext(const wwiv::sdk::User* u, bool mci_enabled) : u_(u), mci_enabled_(mci_enabled) {}
  const wwiv::sdk::User& u() const override { return *u_; }
  const wwiv::sdk::files::directory_t& dir() const override { return a()->current_dir(); }
  bool mci_enabled() const override { return mci_enabled_; }

private:
  const wwiv::sdk::User* u_;
  bool mci_enabled_;
};

class BbsMacroFiilter : public wwiv::sdk::ansi::AnsiFilter {
public:
  BbsMacroFiilter(wwiv::sdk::ansi::AnsiFilter* chain, const BbsMacroContext* ctx)
      : chain_(chain), ctx_(ctx){};
  bool write(char c) override;
  bool attr(uint8_t a) override;

private:
  wwiv::sdk::ansi::AnsiFilter* chain_;
  const BbsMacroContext* ctx_;
  bool in_pipe_{false};
  bool in_macro_{false};
};

#endif // __INCLUDED_BBS_INTERPRET_H__
