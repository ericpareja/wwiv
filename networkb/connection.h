/**************************************************************************/
/*                                                                        */
/*                          WWIV Version 5.x                              */
/*             Copyright (C)2015-2016, WWIV Software Services             */
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
#pragma once
#ifndef __INCLUDED_NETWORKB_CONNECTION_H__
#define __INCLUDED_NETWORKB_CONNECTION_H__

#include <chrono>
#include <cstdint>
#include <string>

namespace wwiv {
namespace net {

class Connection
{
public:
  Connection();
  virtual ~Connection();

  virtual int receive(void* data, int size, std::chrono::milliseconds d) = 0;
  virtual std::string receive(int size, std::chrono::milliseconds d) = 0;
  virtual int send(const void* data, int size, std::chrono::milliseconds d) = 0;

  virtual uint16_t read_uint16(std::chrono::milliseconds d) = 0;
  virtual uint8_t read_uint8(std::chrono::milliseconds d) = 0;
  virtual bool is_open() const = 0;
  virtual bool close() = 0;
};

}  // namespace net
}  // namespace wwiv

#endif  // __INCLUDED_NETWORKB_CONNECTION_H__
