/**************************************************************************/
/*                                                                        */
/*                          WWIV Version 5.x                              */
/*           Copyright (C)2016-2020, WWIV Software Services               */
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
#ifndef __INCLUDED_SDK_FIDO_FIDO_UTIL_H__
#define __INCLUDED_SDK_FIDO_FIDO_UTIL_H__

#include "core/datetime.h"
#include "core/file.h"
#include "sdk/config.h"
#include "sdk/fido/fido_address.h"
#include "sdk/fido/fido_callout.h"
#include <ctime>
#include <filesystem>
#include <string>
#include <vector>

namespace wwiv {
namespace sdk {
namespace fido {
class FidoStoredMessage;
struct packet_header_2p_t;
class FidoPackedMessage;

// FTN Naming
std::string packet_name(wwiv::core::DateTime& dt);
std::string bundle_name(const FidoAddress& source, const FidoAddress& dest, int dow, int bundle_number);
std::string bundle_name(const FidoAddress& source, const FidoAddress& dest, const std::string& extension);
std::string net_node_name(const FidoAddress& dest, const std::string& extension);
std::string flo_name(const FidoAddress& dest, fido_bundle_status_t status);
std::vector<std::string> dow_prefixes();
std::string dow_extension(int dow, int bundle_number);
bool is_bundle_file(const std::string& name);
bool is_packet_file(const std::string& name);
std::string control_file_name(const FidoAddress& dest, fido_bundle_status_t status);
bool exists_bundle(const wwiv::sdk::Config& config, const net_networks_rec& net);
bool exists_bundle(const std::string& dir);

// FTN DateTime
std::string daten_to_fido(time_t t);
daten_t fido_to_daten(std::string d);
std::string tz_offset_from_utc();

// FTN Addressing
std::string to_net_node(const FidoAddress& a);
std::string to_zone_net_node(const FidoAddress& a);
std::string to_zone_net_node_point(const FidoAddress& a);

bool RoutesThroughAddress(const FidoAddress& a, const std::string& routes);
FidoAddress FindRouteToAddress(const FidoAddress& a, const FidoCallout& callout);

// FTN Text Handling
/** Splits a message to find a specific line. This will strip blank lines. */
std::vector<std::string> split_message(const std::string& string);

std::string FidoToWWIVText(const std::string& ft, bool convert_control_codes = true);
std::string WWIVToFidoText(const std::string& wt);
std::string WWIVToFidoText(const std::string& wt, int8_t max_optional_val_to_include);

/**
 * Gets the FidoAddress from a single line of text of the form:
 * "Name (zone:node/net)"
 */
FidoAddress get_address_from_single_line(const std::string& line);

/**
 * Gets the FidoAddress from an entire message by looking for the "* Origin: "
 * line in the message text.  The Origin line is expected to be of the form:
 * "* Origin: Some Origin Line Text (zone:node/net)"
 */
FidoAddress get_address_from_origin(const std::string& text);

/**
 * Gets the FidoAddress from an packet.
 *
 * Prefer the address from the Origin line of possible, otherwise use the zone
 * from the packet header, and net/node from the packed message.
 */
FidoAddress get_address_from_packet(const FidoPackedMessage& msg, const packet_header_2p_t& header);

/**
 * Gets the FidoAddress from a stored message (*.MSG file).
 *
 * Prefer the address from the Origin line of possible, otherwise use the zone
 * from the packet header, and net/node from the packed message.
 */
FidoAddress get_address_from_stored_message(const FidoStoredMessage& msg);

}  // namespace fido
}  // namespace net
}  // namespace wwiv

#endif  // __INCLUDED_SDK_FIDO_FIDO_UTIL_H__
