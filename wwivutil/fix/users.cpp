/**************************************************************************/
/*                                                                        */
/*                              WWIV Version 5.x                          */
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
#include "wwivutil/fix/users.h"

#include <algorithm>
#include <vector>
#include <set>

#include "core/command_line.h"
#include "core/file.h"
#include "core/strings.h"
#include "core/version.h"
#include "sdk/filenames.h"
#include "sdk/user.h"
#include "sdk/usermanager.h"

using std::clog;
using std::cout;
using std::endl;
using namespace wwiv::core;
using namespace wwiv::sdk;
using namespace wwiv::strings;

statusrec status;

namespace wwiv {
namespace wwivutil {

static char *dateFromTimeT(time_t t) {
  static char date_string[11];
  struct tm * pTm = localtime(&t);

  snprintf(date_string, sizeof(date_string), "%02d/%02d/%02d", pTm->tm_mon + 1, pTm->tm_mday, pTm->tm_year % 100);
  return date_string;
}

static char *dateFromTimeTForLog(time_t t) {
  static char date_string[11];
  struct tm * pTm = localtime(&t);

  snprintf(date_string, sizeof(date_string), "%02d%02d%02d", pTm->tm_year % 100, pTm->tm_mon + 1, pTm->tm_mday);
  return date_string;
}

static bool checkFileSize(File &file, unsigned long len) {
  if (!file.IsOpen()) {
    int nFileMode = File::modeReadOnly | File::modeBinary;
    file.Open(nFileMode);
  }
  unsigned long actual = file.GetLength();
  file.Close();
  if (actual < len) {
    cout << file.full_pathname() << " too short (" << actual << "<"
      << len << ")." << endl;
    return false;
  }
  if (actual > len) {
    cout << file.full_pathname() << " too long (" << actual << ">"
      << len << ")." << endl;
    cout << "Attempting to continue." << endl;
  }
  return true;
}

static void saveStatus(const std::string& datadir) {
  File statusDat(datadir, STATUS_DAT);

  statusDat.Open(File::modeReadWrite | File::modeBinary);
  statusDat.Write(&status, sizeof(statusrec));
  statusDat.Close();
}

static bool initStatusDat(const std::string& datadir) {
  int nFileMode = File::modeReadOnly | File::modeBinary;
  bool update = false;
  File statusDat(datadir, STATUS_DAT);
  if (!statusDat.Exists()) {
    cout << statusDat.full_pathname() << " NOT FOUND!" << endl;
    cout << "Recreating " << statusDat.full_pathname() << endl;
    memset(&status, 0, sizeof(statusrec));
    strcpy(status.date1, "00/00/00");
    strcpy(status.date2, status.date1);
    strcpy(status.date3, status.date1);
    strcpy(status.log1, "000000.log");
    strcpy(status.log2, "000000.log");
    strcpy(status.gfiledate, "00/00/00");
    status.callernum = 65535;
    status.wwiv_version = wwiv_num_version;
    update = true;
  } else {
    checkFileSize(statusDat, sizeof(statusrec));
    cout << "Reading " << statusDat.full_pathname() << "..." << endl;
    if (!statusDat.Open(nFileMode)) {
      cout << statusDat.full_pathname() << " NOT FOUND." << endl;
      return false;
    }
    statusDat.Read(&status, sizeof(statusrec));
    statusDat.Close();

    // version check
    if (status.wwiv_version > wwiv_num_version) {
      cout << "Incorrect version of fix (this is for "
           << wwiv_num_version << ", you need " << status.wwiv_version << ")" << endl;
    }

    time_t val = time(nullptr);
    char *curDate = dateFromTimeT(val);
    if (strcmp(status.date1, curDate)) {
      strcpy(status.date1, curDate);
      update = true;
      cout << "Date error in STATUS.DAT (status.date1) corrected" << endl;
    }

    val -= 86400L;
    curDate = dateFromTimeT(val);
    if (strcmp(status.date2, curDate)) {
      strcpy(status.date2, curDate);
      update = true;
      cout << "Date error in STATUS.DAT (status.date2) corrected" << endl;
    }
    char logFile[512];
    snprintf(logFile, sizeof(logFile), "%s.log", dateFromTimeTForLog(val));
    if (strcmp(status.log1, logFile)) {
      strcpy(status.log1, logFile);
      update = true;
      cout << "Log filename error in STATUS.DAT (status.log1) corrected" << endl;
    }

    val -= 86400L;
    curDate = dateFromTimeT(val);
    if (strcmp(status.date3, curDate)) {
      strcpy(status.date3, curDate);
      update = true;
      cout << "Date error in STATUS.DAT (status.date3) corrected" << endl;
    }
    snprintf(logFile, sizeof(logFile), "%s.log", dateFromTimeTForLog(val));
    if (strcmp(status.log2, logFile)) {
      strcpy(status.log2, logFile);
      update = true;
      cout << "Log filename error in STATUS.DAT (status.log2) corrected" << endl;
    }
  }
  if (update) {
    saveStatus(datadir);
  }
}

static void usage() {
  cout << "Usage:   fix users" << endl;
  cout << "Example: WWIVUTIL fix users" << endl;
}

bool FixUsersCommand::AddSubCommands() {
  add_argument(BooleanCommandLineArgument("exp", 'x', "Enable experimental features.", false));
  add_argument(BooleanCommandLineArgument("verbose", 'v', "Enable verbose output.", false));

  return true;
}

int FixUsersCommand::Execute() {
  std::cout << "Runnning FixUsersCommand::Execute" << std::endl;

  initStatusDat(config()->config()->datadir());

  File userFile(config()->config()->datadir(), USER_LST);
	if (!userFile.Exists()) {
    cout << userFile.full_pathname() << " does not exist." << endl;
    return 1;
	}

	UserManager userMgr(config()->config()->datadir(), sizeof(userrec), 
      config()->config()->config()->maxusers);
  cout << "Checking USER.LST... found " << userMgr.GetNumberOfUserRecords() << " user records." << endl;
  cout << "TBD: Check for trashed user recs." << endl;
	if (userMgr.GetNumberOfUserRecords() > config()->config()->config()->maxusers) {
    cout << "Might be too many." << endl;
    if (!arg("exp").as_bool()) {
      return 1;
    }
	} else {
    cout << "Reasonable number." << endl;
	}

	std::vector<smalrec> smallrecords;
	std::set<std::string> names;

  const int num_user_records = userMgr.GetNumberOfUserRecords();
	for(int i = 1; i <= num_user_records; i++) {
		User user;
		userMgr.ReadUser(&user, i);
		user.FixUp();
		userMgr.WriteUser(&user, i);
		if (!user.IsUserDeleted() && !user.IsUserInactive()) {
			smalrec sr = { 0 };
			strcpy((char*) sr.name, user.GetName());
			sr.number = static_cast<uint16_t>(i);
			std::string namestring((char*) sr.name);
			if (names.find(namestring) == names.end()) {
				smallrecords.push_back(sr);
				names.insert(namestring);
        if (arg("verbose").as_bool()) {
          cout << "Keeping user: " << sr.name << " #" << sr.number << endl;
        }
			} else {
				std::cout << "[skipping duplicate user: " << namestring << " #" << sr.number << "]";
			}
		}
	};

	std::sort(smallrecords.begin(), smallrecords.end(), [](const smalrec& a, const smalrec& b) -> bool {
		int equal = strcmp((char*)a.name, (char*)b.name);

		// Sort by user number if names match.
		if (equal == 0) {
			return a.number < b.number;
		}

		// Otherwise sort by name comparison.
		return equal < 0;
	});

	printf("size=%lu %lu\n", smallrecords.size(), sizeof(smalrec) * smallrecords.size());
  cout << "Checking NAMES.LST" << endl;
	File nameFile(config()->config()->datadir(), NAMES_LST);
	if (!nameFile.Exists()) {
    cout << nameFile.full_pathname() << " does not exist, regenerating with "
         << smallrecords.size() << " names" << endl;
		nameFile.Open(File::modeCreateFile | File::modeBinary | File::modeWriteOnly);
		nameFile.Write(&smallrecords[0], sizeof(smalrec) * smallrecords.size());
		nameFile.Close();
	} else {
		if (nameFile.Open(File::modeReadOnly | File::modeBinary)) {
			long size = nameFile.GetLength();
      uint16_t recs = static_cast<uint16_t>(size / sizeof(smalrec));
			if (recs != status.users) {
				status.users = recs;
        cout << "STATUS.DAT contained an incorrect user count." << endl;
			} else {
        cout << "STATUS.DAT matches expected user count of " << status.users << " users." << endl;
			}
		}
		nameFile.Close();
	}
  return 0;
}

}  // namespace wwivutil
}  // namespace wwiv
