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
/*                                                                        */
/**************************************************************************/

#include "bbs/xferovl.h"
#include "bbs/application.h"
#include "bbs/batch.h"
#include "bbs/bbs.h"
#include "bbs/bbsutl.h"
#include "bbs/bgetch.h"
#include "bbs/com.h"
#include "bbs/conf.h"
#include "bbs/confutil.h"
#include "bbs/defaults.h"
#include "bbs/dirlist.h"
#include "bbs/input.h"
#include "bbs/listplus.h"
#include "bbs/mmkey.h"
#include "bbs/sr.h"
#include "bbs/sysoplog.h"
#include "bbs/utility.h"
#include "bbs/xfer.h"
#include "bbs/xferovl1.h"
#include "core/findfiles.h"
#include "core/numbers.h"
#include "core/strings.h"
#include "core/textfile.h"
#include "fmt/printf.h"
#include "local_io/keycodes.h"
#include "local_io/wconstants.h"
#include "sdk/files/allow.h"
#include "sdk/names.h"
#include "sdk/status.h"
#include "sdk/files/files.h"

#include <string>

using std::string;
using namespace wwiv::core;
using namespace wwiv::sdk;
using namespace wwiv::strings;

extern char str_quit[];

void move_file() {
  int d1 = 0;

  bool ok = false;
  bout.nl();
  const auto fm = file_mask("|#2Filename to move: ");
  dliscan();
  int nCurRecNum = recno(fm);
  if (nCurRecNum < 0) {
    bout << "\r\nFile not found.\r\n";
    return;
  }
  bool done = false;
  tmp_disable_conf(true);

  while (!a()->hangup_ && nCurRecNum > 0 && !done) {
    int nCurrentPos = nCurRecNum;
    auto f = a()->current_file_area()->ReadFile(nCurRecNum);
    const auto dir = a()->dirs()[a()->current_user_dir().subnum];
    bout.nl();
    printfileinfo(&f.u(), dir);
    bout.nl();
    bout << "|#5Move this (Y/N/Q)? ";
    const auto ch = ynq();
    std::filesystem::path src_fn;
    if (ch == 'Q') {
      done = true;
    } else if (ch == 'Y') {
      src_fn = FilePath(dir.path, f);
      string ss;
      do {
        bout.nl(2);
        bout << "|#2To which directory? ";
        ss = mmkey(MMKeyAreaType::dirs);
        if (ss[0] == '?') {
          dirlist(1);
          dliscan();
        }
      }
      while (!a()->hangup_ && ss[0] == '?');
      d1 = -1;
      if (!ss.empty()) {
        for (auto i1 = 0; i1 < a()->dirs().size() && a()->udir[i1].subnum != -1; i1++) {
          if (ss == a()->udir[i1].keys) {
            d1 = i1;
          }
        }
      }
      if (d1 != -1) {
        ok = true;
        d1 = a()->udir[d1].subnum;
        dliscan1(d1);
        if (recno(f.aligned_filename()) > 0) {
          ok = false;
          bout << "\r\nFilename already in use in that directory.\r\n";
        }
        if (a()->current_file_area()->number_of_files() >= a()->dirs()[d1].maxfiles) {
          ok = false;
          bout << "\r\nToo many files in that directory.\r\n";
        }
        if (File::freespace_for_path(a()->dirs()[d1].path) <
            static_cast<long>(f.numbytes() / 1024L) + 3) {
          ok = false;
          bout << "\r\nNot enough disk space to move it.\r\n";
        }
        dliscan();
      } else {
        ok = false;
      }
    } else {
      ok = false;
    }
    if (ok && !done) {
      bout << "|#5Reset upload time for file? ";
      if (yesno()) {
        f.set_date(DateTime::now());
      }
      --nCurrentPos;
      string ss = a()->current_file_area()->ReadExtendedDescriptionAsString(f).value_or("");
      if (a()->current_file_area()->DeleteFile(nCurRecNum)) {
        a()->current_file_area()->Save();
      }

      dliscan1(d1);
      auto dest_fn = FilePath(a()->dirs()[d1].path, f);
      if (a()->current_file_area()->AddFile(f, ss)) {
        a()->current_file_area()->Save();
      }
      if (src_fn != dest_fn && File::Exists(src_fn)) {
        File::Rename(src_fn, dest_fn);
      }
      bout << "\r\nFile moved.\r\n";
    }
    dliscan();
    nCurRecNum = nrecno(fm, nCurrentPos);
  }

  tmp_disable_conf(false);
}

static files::FileAreaSortType sort_type_from_wwiv_type(int t) {
  switch (t) {
  case 0:
    return files::FileAreaSortType::FILENAME_ASC;
  case 1:
    return files::FileAreaSortType::DATE_ASC;
  case 2:
    return files::FileAreaSortType::DATE_DESC;
  default:
    return files::FileAreaSortType::FILENAME_ASC;
  }
}

void sortdir(int directory_num, int type) {
  dliscan1(directory_num);

  if (a()->current_file_area()->Sort(sort_type_from_wwiv_type(type))) {
    a()->current_file_area()->Save();
  }
}

void sort_all(int type) {
  tmp_disable_conf(true);
  for (auto i = 0;
       i < a()->dirs().size() && a()->udir[i].subnum != -1 && !a()->localIO()->KeyPressed();
       i++) {
    bout << "\r\n|#1Sorting " << a()->dirs()[a()->udir[i].subnum].name << wwiv::endl;
    sortdir(i, type);
  }
  tmp_disable_conf(false);
}

void rename_file() {
  bout.nl(2);
  bout << "|#2File to rename: ";
  auto s = input(12);
  if (s.empty()) {
    return;
  }
  if (strchr(s.c_str(), '.') == nullptr) {
    s += ".*";
  }
  s = aligns(s);
  dliscan();
  bout.nl();
  const auto original_filename = s;
  int nRecNum = recno(s);
  while (nRecNum > 0 && !a()->hangup_) {
    int nCurRecNum = nRecNum;
    auto f = a()->current_file_area()->ReadFile(nRecNum);
    const auto& dir = a()->dirs()[a()->current_user_dir().subnum];
    bout.nl();
    printfileinfo(&f.u(), dir);
    bout.nl();
    bout << "|#5Change info for this file (Y/N/Q)? ";
    const char ch = ynq();
    if (ch == 'Q') {
      break;
    }
    if (ch == 'N') {
      nRecNum = nrecno(original_filename, nCurRecNum);
      continue;
    }
    bout.nl();
    bout << "|#2New filename? ";
    s = input(12);
    if (!okfn(s)) {
      s.clear();
    }
    if (!s.empty()) {
      s = aligns(s);
      if (!iequals(s, "        .   ")) {
        const std::string p = dir.path;
        auto dest_fn = FilePath(p, s);
        if (File::Exists(dest_fn)) {
          bout << "Filename already in use; not changed.\r\n";
        } else {
          File::Rename(FilePath(p, f), dest_fn);
          if (File::Exists(dest_fn)) {
            auto* area = a()->current_file_area();
            auto ss = area->ReadExtendedDescriptionAsString(f).value_or("");
            if (!ss.empty()) {
              area->DeleteExtendedDescription(f, nCurRecNum);
              area->AddExtendedDescription(s, ss);
              f.set_extended_description(true);
            }
            f.set_filename(s);
          } else {
            bout << "Bad filename.\r\n";
          }
        }
      }
    }
    bout << "\r\nNew description:\r\n|#2: ";
    auto* area = a()->current_file_area();
    auto desc = input_text(58);
    if (!desc.empty()) {
      f.set_description(desc);
    }
    auto ss = area->ReadExtendedDescriptionAsString(f).value_or("");
    bout.nl(2);
    bout << "|#5Modify extended description? ";
    if (yesno()) {
      bout.nl();
      if (!ss.empty()) {
        bout << "|#5Delete it? ";
        if (yesno()) {
          area->DeleteExtendedDescription(f, nCurRecNum);
          f.set_extended_description(false);
        } else {
          f.set_extended_description(true);
          modify_extended_description(&ss, a()->dirs()[a()->current_user_dir().subnum].name);
          if (!ss.empty()) {
            area->DeleteExtendedDescription(f, nCurRecNum);
            area->AddExtendedDescription(f, nCurRecNum, ss);
          }
        }
      } else {
        modify_extended_description(&ss, a()->dirs()[a()->current_user_dir().subnum].name);
        if (!ss.empty()) {
          area->AddExtendedDescription(f, nCurRecNum, ss);
          f.set_extended_description(true);
        } else {
          f.set_extended_description(false);
        }
      }
    } else if (!ss.empty()) {
      f.set_extended_description(true);
    } else {
      f.set_extended_description(false);
    }
    if (a()->current_file_area()->UpdateFile(f, nRecNum)) {
      a()->current_file_area()->Save();
    }
    nRecNum = nrecno(original_filename, nCurRecNum);
  }
}

static bool upload_file(const std::string& file_name, uint16_t directory_num,
                        const std::string& description) {
  auto d = a()->dirs()[directory_num];
  const auto temp_filename = aligns(file_name);
  files::FileRecord f{};
  f.set_filename(temp_filename);
  f.u().ownerusr = static_cast<uint16_t>(a()->usernum);
  if (!(d.mask & mask_cdrom) && !check_ul_event(directory_num, &f.u())) {
    bout << file_name << " was deleted by upload event.\r\n";
  } else {
    const auto unaligned_filename = files::unalign(file_name);
    const auto full_path = FilePath(d.path, unaligned_filename);

    File fileUpload(full_path);
    if (!fileUpload.Open(File::modeBinary | File::modeReadOnly)) {
      if (!description.empty()) {
        bout << "ERR: " << unaligned_filename << ":" << description << wwiv::endl;
      } else {
        bout << "|#1" << unaligned_filename << " does not exist." << wwiv::endl;
      }
      return true;
    }
    const auto fs = fileUpload.length();
    f.set_numbytes(fs);
    fileUpload.Close();
    to_char_array(f.u().upby, a()->names()->UserName(a()->usernum));
    f.set_date(DateTime::now());

    auto t = DateTime::from_time_t(File::last_write_time(full_path));
    f.set_actual_date(t);

    if (d.mask & mask_PD) {
      d.mask = mask_PD;
    }
    bout.nl();

    bout << "|#9File name   : |#2" << f << wwiv::endl;
    bout << "|#9File size   : |#2" << humanize(f.numbytes()) << wwiv::endl;
    if (!description.empty()) {
      f.set_description(description);
      bout << "|#1 Description: " << f.description() << wwiv::endl;
    } else {
      bout << "|#9Enter a description for this file.\r\n|#7: ";
      auto desc = input_text(58);
      f.set_description(desc);
    }
    bout.nl();
    if (f.description().empty()) {
      return false;
    }
    get_file_idz(f, a()->dirs()[directory_num]);
    a()->user()->SetFilesUploaded(a()->user()->GetFilesUploaded() + 1);
    if (!(d.mask & mask_cdrom)) {
      add_to_file_database(f);
    }
    a()->user()->set_uk(a()->user()->uk() + bytes_to_k(fs));
    f.set_date(DateTime::now());
    if (a()->current_file_area()->AddFile(f)) {
      a()->current_file_area()->Save();
    }
    auto status = a()->status_manager()->BeginTransaction();
    status->IncrementNumUploadsToday();
    status->IncrementFileChangedFlag(WStatus::fileChangeUpload);
    a()->status_manager()->CommitTransaction(std::move(status));
    sysoplog() << "+ '" << f << "' uploaded on " << d.name;
    a()->UpdateTopScreen();
  }
  return true;
}

bool maybe_upload(const std::string& file_name, uint16_t directory_num, const std::string& description) {
  auto abort = false;
  const auto i = recno(aligns(file_name));

  if (i == -1) {
    if (!is_uploadable(file_name) && dcs()) {
      bout.format("{:<12}: |#5In filename database - add anyway? ", file_name);
      const auto ch = ynq();
      if (ch == *str_quit) {
        return false;
      }
      if (ch == YesNoString(false)[0]) {
        bout << "|#5Delete it? ";
        if (yesno()) {
          File::Remove(FilePath(a()->dirs()[directory_num].path, file_name));
        }
        bout.nl();
        return true;
      }
    }
    if (!upload_file(file_name, a()->udir[directory_num].subnum, description)) {
      return false;
    }
  } else {
    auto f = a()->current_file_area()->ReadFile(i);
    const auto ocd = a()->current_user_dir_num();
    a()->set_current_user_dir_num(directory_num);
    printinfo(&f.u(), &abort);
    a()->set_current_user_dir_num(ocd);
    if (abort) {
      return false;
    }
  }
  return true;
}

/* This assumes the file holds listings of files, one per line, to be
 * uploaded.  The first word (delimited by space/tab) must be the filename.
 * after the filename are optional tab/space separated words (such as file
 * size or date/time).  After the optional words is the description, which
 * is from that position to the end of the line.  the "type" parameter gives
 * the number of optional words between the filename and description.
 * the optional words (size, date/time) are ignored completely.
 */
void upload_files(const std::string& file_name, uint16_t directory_num, int type) {
  char s[255], *fn1, *description = nullptr, *ext = nullptr;
  bool abort = false;
  bool ok = true;

  std::string last_fn;
  const auto& dir = a()->dirs()[a()->udir[directory_num].subnum];
  dliscan1(dir);

  auto file = std::make_unique<TextFile>(file_name, "r");
  if (!file->IsOpen()) {
    const auto default_fn = FilePath(dir.path, file_name);
    file.reset(new TextFile(default_fn, "r"));
  }
  if (!file->IsOpen()) {
    bout << file_name << ": not found.\r\n";
  } else {
    while (ok && file->ReadLine(s, 250)) {
      if (s[0] < SPACE) {
        continue;
      }
      if (s[0] == SPACE) {
        if (!last_fn.empty()) {
          if (!ext) {
            ext = static_cast<char*>(BbsAllocA(4096L));
            *ext = 0;
          }
          for (description = s; *description == ' ' || *description == '\t'; description++)
            ;
          if (*description == '|') {
            do {
              description++;
            }
            while (*description == ' ' || *description == '\t');
          }
          fn1 = strchr(description, '\n');
          if (fn1) {
            *fn1 = 0;
          }
          strcat(ext, description);
          strcat(ext, "\r\n");
        }
      } else {
        int ok1 = 0;
        fn1 = strtok(s, " \t\n");
        if (fn1) {
          ok1 = 1;
          for (int i = 0; ok1 && (i < type); i++) {
            if (strtok(nullptr, " \t\n") == nullptr) {
              ok1 = 0;
            }
          }
          if (ok1) {
            description = strtok(nullptr, "\n");
            if (!description) {
              ok1 = 0;
            }
          }
        }
        if (ok1) {
          if (!last_fn.empty() && ext && *ext) {
            auto f = a()->current_file_area()->ReadFile(1);
            if (iequals(last_fn, f.aligned_filename())) {
              add_to_file_database(f);
              if (a()->current_file_area()->UpdateFile(f, 1, ext)) {
                a()->current_file_area()->Save();
              }
            }
            *ext = 0;
          }
          while (*description == ' ' || *description == '\t') {
            ++description;
          }
          ok = maybe_upload(fn1, directory_num, description);
          checka(&abort);
          if (abort) {
            ok = false;
          }
          if (ok) {
            last_fn = fn1;
            last_fn = aligns(last_fn);
            if (ext) {
              *ext = 0;
            }
          }
        }
      }
    }
    file->Close();
    if (ok && last_fn[0] && ext && *ext) {
      auto f = a()->current_file_area()->ReadFile(1);
      if (iequals(last_fn, f.aligned_filename())) {
        add_to_file_database(f);
        if (a()->current_file_area()->UpdateFile(f, 1, ext)) {
          a()->current_file_area()->Save();
        }
      }
    }
  }

  if (ext) {
    free(ext);
  }
}

// returns false on abort
bool uploadall(uint16_t directory_num) {
  const auto& dir = a()->dirs()[a()->udir[directory_num].subnum];
  dliscan1(dir);

  const auto path_mask = FilePath(dir.path, "*.*");
  const int maxf = dir.maxfiles;

  FindFiles ff(path_mask, FindFilesType::files);
  auto aborted = false;
  for (const auto& f : ff) {
    aborted = checka();
    if (aborted || a()->hangup_ || a()->current_file_area()->number_of_files() >= maxf) {
      break;
    }
    if (!maybe_upload(f.name, directory_num, "")) {
      break;
    }
  }
  if (aborted) {
    bout << "|#6Aborted.\r\n";
    return false;
  }
  if (a()->current_file_area()->number_of_files() >= maxf) {
    bout << "directory full.\r\n";
  }
  return true;
}

void relist() {
  char s[85];
  bool next, abort = false;
  int16_t tcd = -1;

  if (a()->filelist.empty()) {
    return;
  }
  bout.cls();
  bout.clear_lines_listed();
  if (a()->HasConfigFlag(OP_FLAGS_FAST_TAG_RELIST)) {
    bout.Color(FRAME_COLOR);
    bout << string(78, '-') << wwiv::endl;
  }
  for (size_t i = 0; i < a()->filelist.size(); i++) {
    auto& f = a()->filelist[i];
    if (!a()->HasConfigFlag(OP_FLAGS_FAST_TAG_RELIST)) {
      if (tcd != f.directory) {
        bout.Color(FRAME_COLOR);
        if (tcd != -1) {
          bout << "\r" << string(78, '-') << wwiv::endl;
        }
        tcd = f.directory;
        auto tcdi = -1;
        for (auto i1 = 0; i1 < a()->dirs().size(); i1++) {
          if (a()->udir[i1].subnum == tcd) {
            tcdi = i1;
            break;
          }
        }
        bout.Color(2);
        if (tcdi == -1) {
          bout << a()->dirs()[tcd].name << "." << wwiv::endl;
        } else {
          bout << a()->dirs()[tcd].name << " - #" << a()->udir[tcdi].keys << ".\r\n";
        }
        bout.Color(FRAME_COLOR);
        bout << string(78, '-') << wwiv::endl;
      }
    }
    sprintf(s, "%c%d%2d%c%d%c", 0x03, a()->batch().contains_file(f.u.filename) ? 6 : 0, i + 1, 0x03,
            FRAME_COLOR,
            okansi() ? '\xBA' : ' '); // was |
    bout.bputs(s, &abort, &next);
    bout.Color(1);
    strncpy(s, f.u.filename, 8);
    s[8] = 0;
    bout.bputs(s, &abort, &next);
    strncpy(s, &f.u.filename[8], 4);
    s[4] = 0;
    bout.Color(1);
    bout.bputs(s, &abort, &next);
    bout.Color(FRAME_COLOR);
    bout.bputs((okansi() ? "\xBA" : ":"), &abort, &next);

    auto numbytes = humanize(f.u.numbytes);
    if (!a()->HasConfigFlag(OP_FLAGS_FAST_TAG_RELIST)) {
      if (!(a()->dirs()[tcd].mask & mask_cdrom)) {
        files::FileName fn(f.u.filename);
        auto filepath = FilePath(a()->dirs()[tcd].path, fn);
        if (!File::Exists(filepath)) {
          numbytes = "N/A";
        }
      }
    }
    if (numbytes.size() < 5) {
      size_t i1 = 0;
      for (; i1 < 5 - numbytes.size(); i1++) {
        s[i1] = SPACE;
      }
      s[i1] = 0;
    }
    strcat(s, numbytes.c_str());
    bout.Color(2);
    bout.bputs(s, &abort, &next);

    bout.Color(FRAME_COLOR);
    bout.bputs((okansi() ? "\xBA" : "|"), &abort, &next);
    auto numdloads = std::to_string(f.u.numdloads);

    size_t i1 = 0;
    for (; i1 < 4 - numdloads.size(); i1++) {
      s[i1] = SPACE;
    }
    s[i1] = 0;
    strcat(s, numdloads.c_str());
    bout.Color(2);
    bout.bputs(s, &abort, &next);

    bout.Color(FRAME_COLOR);
    bout.bputs((okansi() ? "\xBA" : "|"), &abort, &next);
    sprintf(s, "%c%d%s", 0x03, f.u.mask & mask_extended ? 1 : 2, f.u.description);
    bout.bpla(trim_to_size_ignore_colors(s, a()->user()->GetScreenChars() - 28), &abort);
  }
  bout.Color(FRAME_COLOR);
  bout << "\r" << string(78, '-') << wwiv::endl;
  bout.clear_lines_listed();
}

/*
 * Allows user to add or remove ALLOW.DAT entries.
 */
void edit_database() {
  do {
    bout.nl();
    bout << "|#2A|#7)|#9 Add to ALLOW.DAT\r\n";
    bout << "|#2R|#7)|#9 Remove from ALLOW.DAT\r\n";
    bout << "|#2Q|#7)|#9 Quit\r\n";
    bout.nl();
    bout << "|#7Select: ";
    const auto ch = onek("QAR");
    switch (ch) {
    case 'A': {
      bout.nl();
      bout << "|#2Filename: ";
      auto s = input(12, true);
      if (!s.empty()) {
        add_to_file_database(s);
      }
    } break;
    case 'R': {
      bout.nl();
      bout << "|#2Filename: ";
      auto s = input(12, true);
      if (!s.empty()) {
        remove_from_file_database(s);
      }
    } break;
    case 'Q':
      return;
    }
  }
  while (!a()->hangup_);
}

void add_to_file_database(const std::string& file_name) {
  files::Allow allow(*a()->config());
  allow.Add(file_name);
  allow.Save();
}

void add_to_file_database(const files::FileRecord& f) {
  add_to_file_database(f.aligned_filename());
}

void remove_from_file_database(const std::string& file_name) {
  files::Allow allow(*a()->config());
  allow.Remove(file_name);
  allow.Save();
}

/*
 * Returns 1 if file not found in filename database.
 */

bool is_uploadable(const std::string& file_name) {
  files::Allow allow(*a()->config());
  return allow.IsAllowed(file_name);
}

static void l_config_nscan() {
  bool abort = false;
  bout.nl();
  bout << "|#9Directories to new-scan marked with '|#2*|#9'\r\n\n";
  for (auto i = 0; i < a()->dirs().size() && a()->udir[i].subnum != -1 && !abort;
       i++) {
    const int i1 = a()->udir[i].subnum;
    std::string s{"  "};
    if (a()->context().qsc_n[i1 / 32] & (1L << (i1 % 32))) {
      s = "* ";
    }
    bout.bpla(fmt::format("{}{}. {}", s, a()->udir[i].keys, a()->dirs()[i1].name), &abort);
  }
  bout.nl(2);
}

static void config_nscan() {
  char ch;
  bool abort = false;

  if (okansi()) {
    config_scan_plus(NSCAN);
    return;
  }
  bool done1 = false;
  const int oc = a()->current_user_dir_conf_num();
  const int os = a()->current_user_dir().subnum;

  do {
    if (ok_multiple_conf(a()->user(), a()->uconfdir)) {
      abort = false;
      std::string s1 = " ";
      bout.nl();
      bout << "Select Conference: \r\n\n";
      size_t i = 0;
      while (i < a()->dirconfs.size() && has_userconf_to_dirconf(i) && !abort) {
        const auto confnum = a()->uconfdir[i].confnum;
        const auto cn = stripcolors(a()->dirconfs[confnum].conf_name);
        const auto s2 = StrCat(a()->dirconfs[confnum].designator, ") ", cn);
        bout.bpla(s2, &abort);
        s1.push_back(static_cast<char>(a()->dirconfs[confnum].designator));
        i++;
      }
      bout.nl();
      bout << " Select [" << s1.substr(1) << ", <space> to quit]: ";
      ch = onek(s1);
    } else {
      ch = '-';
    }
    switch (ch) {
    case ' ':
      done1 = true;
      break;
    default:
      if (ok_multiple_conf(a()->user(), a()->uconfdir)) {
        size_t i = 0;
        while (ch != a()->dirconfs[a()->uconfdir[i].confnum].designator &&
               i < a()->dirconfs.size()) {
          i++;
        }
        if (i >= a()->dirconfs.size()) {
          break;
        }

        setuconf(ConferenceType::CONF_DIRS, i, -1);
      }
      l_config_nscan();
      bool done = false;
      do {
        bout.nl();
        bout << "|#9Enter directory number (|#1C=Clr All, Q=Quit, S=Set All|#9): |#0";
        auto s = mmkey(MMKeyAreaType::dirs);
        if (s[0]) {
          for (auto i = 0; i < a()->dirs().size(); i++) {
            const int i1 = a()->udir[i].subnum;
            if (s == a()->udir[i].keys) {
              a()->context().qsc_n[i1 / 32] ^= 1L << (i1 % 32);
            }
            if (s == "S") {
              a()->context().qsc_n[i1 / 32] |= 1L << (i1 % 32);
            }
            if (s == "C") {
              a()->context().qsc_n[i1 / 32] &= ~(1L << (i1 % 32));
            }
          }
          if (s == "Q") {
            done = true;
          }
          if (s == "?") {
            l_config_nscan();
          }
        }
      }
      while (!done && !a()->hangup_);
      break;
    }
    if (!ok_multiple_conf(a()->user(), a()->uconfdir)) {
      done1 = true;
    }
  }
  while (!done1 && !a()->hangup_);

  if (okconf(a()->user())) {
    setuconf(ConferenceType::CONF_DIRS, oc, os);
  }
}

void xfer_defaults() {
  char s[81];
  int i;
  bool done = false;

  do {
    bout.cls();
    bout << "|#7[|#21|#7]|#1 Set New-Scan Directories.\r\n";
    bout << "|#7[|#22|#7]|#1 Set Default Protocol.\r\n";
    bout << "|#7[|#23|#7]|#1 New-Scan Transfer after Message Base ("
        << YesNoString(a()->user()->IsNewScanFiles()) << ").\r\n";
    bout << "|#7[|#24|#7]|#1 Number of lines of extended description to print ["
        << a()->user()->GetNumExtended() << " line(s)].\r\n";
    const std::string onek_options = "Q12345";
    bout << "|#7[|#2Q|#7]|#1 Quit.\r\n\n";
    bout << "|#5Which? ";
    char ch = onek(onek_options);
    switch (ch) {
    case 'Q':
      done = true;
      break;
    case '1':
      config_nscan();
      break;
    case '2':
      bout.nl(2);
      bout << "|#9Enter your default protocol, |#20|#9 for none.\r\n\n";
      i = get_protocol(xfertype::xf_down);
      if (i >= 0) {
        a()->user()->SetDefaultProtocol(i);
      }
      break;
    case '3':
      a()->user()->ToggleStatusFlag(User::nscanFileSystem);
      break;
    case '4':
      bout.nl(2);
      bout << "|#9How many lines of an extended description\r\n";
      bout << "|#9do you want to see when listing files (|#20-" << a()->max_extend_lines
          << "|#7)\r\n";
      bout << "|#9Current # lines: |#2" << a()->user()->GetNumExtended() << wwiv::endl;
      bout << "|#7: ";
      input(s, 3);
      if (s[0]) {
        i = to_number<int>(s);
        if ((i >= 0) && (i <= a()->max_extend_lines)) {
          a()->user()->SetNumExtended(i);
        }
      }
      break;
    }
  }
  while (!done && !a()->hangup_);
}

void finddescription() {
  if (okansi()) {
    listfiles_plus(LP_SEARCH_ALL);
    return;
  }

  bout.nl();
  bool ac = false;
  if (ok_multiple_conf(a()->user(), a()->uconfdir)) {
    bout << "|#5All conferences? ";
    ac = yesno();
    if (ac) {
      tmp_disable_conf(true);
    }
  }
  bout << "\r\nFind description -\r\n\n";
  bout << "Enter string to search for in file description:\r\n:";
  const auto search_string = input(58);
  if (search_string.empty()) {
    tmp_disable_conf(false);
    return;
  }
  const auto ocd = a()->current_user_dir_num();
  auto abort = false;
  auto count = 0;
  auto color = 3;
  bout << "\r|#2Searching ";
  bout.clear_lines_listed();
  for (auto i = 0; i < a()->dirs().size() && !abort && !a()->hangup_ && (a()->udir[i].subnum != -1);
       i++) {
    const auto ii1 = a()->udir[i].subnum;
    int pts;
    auto need_title = true;
    if (a()->context().qsc_n[ii1 / 32] & (1L << (ii1 % 32))) {
      pts = 1;
    }
    pts = 1;
    // remove pts=1 to search only marked directories
    if (pts && !abort) {
      count++;
      bout << static_cast<char>(3) << color << ".";
      if (count == NUM_DOTS) {
        bout << "\r|#2Searching ";
        color++;
        count = 0;
        if (color == 4) {
          color++;
        }
        if (color == 10) {
          color = 0;
        }
      }
      a()->set_current_user_dir_num(i);
      dliscan();
      for (auto i1 = 1;
           i1 <= a()->current_file_area()->number_of_files() && !abort && !a()->hangup_; i1++) {
        auto f = a()->current_file_area()->ReadFile(i1);
        auto desc = ToStringUpperCase(f.description());

        if (desc.find(search_string) != std::string::npos) {
          if (need_title) {
            if (bout.lines_listed() >= a()->screenlinest - 7 && !a()->filelist.empty()) {
              tag_files(need_title);
            }
            if (need_title) {
              printtitle(&abort);
              need_title = false;
            }
          }

          printinfo(&f.u(), &abort);
        } else if (bkbhit()) {
          checka(&abort);
        }
      }
    }
  }
  if (ac) {
    tmp_disable_conf(false);
  }
  a()->set_current_user_dir_num(ocd);
  endlist(1);
}

void arc_l() {
  bout.nl();
  bout << "|#2File for listing: ";
  auto file_spec = input(12);
  if (file_spec.find('.') == std::string::npos) {
    file_spec += ".*";
  }
  if (!okfn(file_spec)) {
    file_spec.clear();
  }
  file_spec = aligns(file_spec);
  dliscan();
  bool abort = false;
  int nRecordNum = recno(file_spec);
  do {
    if (nRecordNum > 0) {
      auto f = a()->current_file_area()->ReadFile(nRecordNum);
      int i1 = list_arc_out(f.unaligned_filename(),
                            a()->dirs()[a()->current_user_dir().subnum].path);
      if (i1) {
        abort = true;
      }
      checka(&abort);
      nRecordNum = nrecno(file_spec, nRecordNum);
    }
  }
  while (nRecordNum > 0 && !a()->hangup_ && !abort);
}
