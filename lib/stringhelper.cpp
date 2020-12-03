#include "stringhelper.h"

#include <deque>
#include <llvm/Support/raw_ostream.h>

using namespace std;

namespace lksast {

string Trim(const string &s) {
  string result = s;
  result.erase(0, result.find_first_not_of(" \t\r\n"));
  result.erase(result.find_last_not_of(" \t\r\n") + 1);
  return result;
}

void SplitStr2Vec(const string &s, vector<string> &result, const string delim) {
  result.clear();

  if (s.empty()) {
    return;
  }

  size_t pos_start = 0, pos_end = s.find(delim), len = 0;

  // 1 substr, no delim
  if (pos_end == string::npos) {
    result.push_back(s);
    return;
  }

  // delimited args
  while (pos_end != string::npos) {
    len = pos_end - pos_start;
    result.push_back(s.substr(pos_start, len));
    pos_start = pos_end + delim.size();
    pos_end = s.find(delim, pos_start);
  }

  if (pos_start != s.size()) {
    result.push_back(s.substr(pos_start));
  }
};

string GetRealPurePath(const string &path) {
  vector<string> strvec;
  SplitStr2Vec(path, strvec, "/");
  if (strvec.empty()) {
    llvm::errs() << "[!] Maybe Err(Empty String?)\n";
    return "";
  }
  if (!strvec[0].empty()) {
    llvm::errs() << "[!] Warning(Not absolute path)\n";
    return "";
  }

  deque<string> fakestack;
  // Note: start from idx = 1
  for (vector<string>::const_iterator it = strvec.begin() + 1,
                                      ed = strvec.end();
       it != ed; it++) {
    string s = *it;
    if (s == ".") {
      continue;
    } else if (s == "..") {
      if (fakestack.empty()) {
        llvm::errs() << "[-] Err, Invail path(too many '..')\n";
      } else {
        fakestack.pop_back();
      }
    } else {
      fakestack.push_back(s);
    }
  }

  string purepath;
  for (string &s : fakestack) {
    purepath += ('/' + s);
  }
  // realpath is '/'
  if (purepath.empty()) {
    purepath += '/';
  }

  // llvm::errs() << purepath << "\n";
  return purepath;
}

} // namespace lksast
