#include "ConfigManager.h"
#include "stringhelper.h"

#include <fstream>

#include <llvm/Support/raw_ostream.h>

using namespace lksast;

ConfigManager::ConfigManager(const string &extdef2src_file,
                             const string &src2ast_file) {
  load_extdef2src_map(extdef2src_file);
  load_src2ast_map(src2ast_file);
}

ConfigManager::~ConfigManager() {
  src_set.clear();
  src2ast_map.clear();
  extdef2src_map.clear(); // shared_ptr maybe err?
}

/*
 * Some definition may in .h file,
 * so
 * FIXME: May has same val?
 */
int ConfigManager::InsertDefSrcAst(const string &defname, const string &srcfn,
                                   const string &astfn) {
  shared_ptr<string> tmp_src_fn_shareptr = std::make_shared<string>(srcfn);
  auto fn_shareptr = src_set.find(tmp_src_fn_shareptr);
  if (fn_shareptr == src_set.end()) {
    src_set.insert(tmp_src_fn_shareptr);
    extdef2src_map[defname] = tmp_src_fn_shareptr;
    // src2ast_map[tmp_src_fn_shareptr] = astfn;
  } else {
    extdef2src_map[defname] = *fn_shareptr;
    // src2ast_map[*fn_shareptr] = astfn;
  }
  src2ast_map[srcfn] = astfn;
  return 0;
}

int ConfigManager::UpdateConfigIntoFile() {
  llvm::errs() << "TODO: ConfigManager::UpdateConfigIntoFile()\n";
  return 0;
}

std::pair<string, string> ConfigManager::parse_one_line(const string &line) {
  size_t idx = line.find(" ");
  if (idx == 0 || idx == line.npos) {
    llvm::errs() << "[-] Err in parse_one_line: " << line << "\n";
    exit(1);
  }
  string _key = line.substr(0, idx);
  string _val = line.substr(idx + 1);
  _key = Trim(_key);
  _val = Trim(_val);
  return std::make_pair(_key, _val);
}

#define PREFIX_EXTDEF "c:@F@"
void ConfigManager::load_extdef2src_map(const string &extdef2src_file) {
  std::ifstream infile(extdef2src_file.c_str());
  string line;
  while (std::getline(infile, line)) {
    line = Trim(line);
    if (line == "") {
      continue;
    }
    auto kvpair = parse_one_line(line);
    string extdef = kvpair.first;
    string srcfile = kvpair.second;
    size_t idx = extdef.find(PREFIX_EXTDEF);
    if (idx != 0) {
      continue;
    }
    string funcname = extdef.substr(sizeof(PREFIX_EXTDEF) - 1);
    shared_ptr<string> tmp_fn_shareptr = std::make_shared<string>(srcfile);
    auto fn_shareptr = src_set.find(tmp_fn_shareptr);
    if (fn_shareptr == src_set.end()) {
      src_set.insert(tmp_fn_shareptr);
      extdef2src_map[funcname] = tmp_fn_shareptr;
    } else {
      extdef2src_map[funcname] = *fn_shareptr;
    }
  }
}

void ConfigManager::load_src2ast_map(const std::string &src2ast_file) {
  std::ifstream infile(src2ast_file.c_str());
  string line;
  while (std::getline(infile, line)) {
    line = Trim(line);
    if (line == "") {
      continue;
    }
    auto kvpair = parse_one_line(line);
    src2ast_map[kvpair.first] = kvpair.second;
    /*
    shared_ptr<string> tmp_fn_shareptr = std::make_shared<string>(kvpair.first);
    auto fn_shareptr = src_set.find(tmp_fn_shareptr);
    if (fn_shareptr == src_set.end()) {
      src_set.insert(tmp_fn_shareptr);
      src2ast_map[tmp_fn_shareptr] = kvpair.second;
    } else {
      src2ast_map[*fn_shareptr] = kvpair.second;
    }
    */
  }
}

/*
ConfigManager Dump
|- src_set:
|   |- xxx.c
|   `- yyy.c
|- extdef2src_map:
|   |- xxx <=> xxx.c
|   `- yyy <=> yyy.c
`- src2ast_map:
    |- xxx.c <=> xxx.ast
    `- yyy.c <=> yyy.ast
 */
void ConfigManager::dump() {
  llvm::errs() << "ConfigManager Dump\n";
  llvm::errs() << "|- src_set:\n";
  size_t i = 1, cnt = src_set.size();
  for (auto &src : src_set) {
    if (i != cnt) {
      llvm::errs() << "|  |- " << *src << "\n";
    } else {
      llvm::errs() << "   `- " << *src << "\n";
    }
    i++;
  }
  i = 1, cnt = extdef2src_map.size();
  llvm::errs() << "|- extdef2src_map:\n";
  for (auto &e2s : extdef2src_map) {
    if (i != cnt) {
      llvm::errs() << "|  |- " << e2s.first << " <=> " << *(e2s.second) << "\n";
    } else {
      llvm::errs() << "|  `- " << e2s.first << " <=> " << *(e2s.second) << "\n";
    }
    i++;
  }
  i = 1, cnt = src2ast_map.size();
  llvm::errs() << "`- src2ast_map:\n";
  for (auto &s2a : src2ast_map) {
    if (i != cnt) {
      llvm::errs() << "   |- " << s2a.first << " <=> " << s2a.second << "\n";
    } else {
      llvm::errs() << "   `- " << s2a.first << " <=> " << s2a.second << "\n";
    }
    i++;
  }
}
