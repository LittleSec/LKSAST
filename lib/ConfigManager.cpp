#include "ConfigManager.h"
#include "stringhelper.h"

#include <fstream>

#include "llvm/Support/raw_ostream.h"

using namespace lksast;

#define JSON_FORMAT_ERR_CODE(errno)                                            \
  do {                                                                         \
    llvm::errs() << "[Err] Format Err in Config Json!\n";                      \
    llvm::errs() << "      More information see README.md\n";                  \
    exit(errno);                                                               \
  } while (false)

ConfigManager::ConfigManager(const string &config_file) {
  std::ifstream infile(config_file);
  infile >> configJson;
  HandleCfgJson();
  const std::string &extdef2src_file =
      configJson["Input"]["ExtdefMapping"].get<std::string>();
  const std::string &src2ast_file =
      configJson["Input"]["Src2Ast"].get<std::string>();
  if (extdef2src_file != "") {
    load_extdef2src_map(extdef2src_file);
  }
  if (src2ast_file != "") {
    load_src2ast_map(src2ast_file);
  }
  infile.close();
}

void ConfigManager::HandleCfgJson() {
  if (configJson.contains("Input") && configJson.contains("Output")) {
    json &inputJ = configJson["Input"];
    if (inputJ.contains("AstList")) {
      json &outputJ = configJson["Output"];
      HandleInputCfg(inputJ);
      HandleOutputCfg(outputJ);
      HandleRunningCfg();
    } else {
      JSON_FORMAT_ERR_CODE(1);
    }
  } else {
    JSON_FORMAT_ERR_CODE(1);
  }
}

void ConfigManager::HandleInputCfg(json &inj) {
  if (!inj.contains("ExtdefMapping")) {
    inj["ExtdefMapping"] = string("");
  }
  if (!inj.contains("Src2Ast")) {
    inj["Src2Ast"] = string("");
  }
}

void ConfigManager::HandleOutputCfg(json &outj) {
  if (!outj.contains("Fun2Json")) {
    outj["Fun2Json"] = "fun2json.json";
  }
  if (!outj.contains("Need2AnalysisPtrInfo")) {
    outj["Need2AnalysisPtrInfo"] = "Need2AnalysisPtrInfo.json";
  }
  if (!outj.contains("HasAnalysisPtrInfo")) {
    outj["HasAnalysisPtrInfo"] = "PtrInfo.json";
  }
}

void ConfigManager::HandleRunningCfg() {
  if (configJson.contains("Running")) {
    json &runJ = configJson["Running"];
    if (runJ.contains("IgnorePaths")) {
      for (auto &p : runJ["IgnorePaths"]) {
        ignorePath.push_back(p.get<std::string>());
      }
    } else { // TODO: maybe unused
      runJ["IgnorePaths"] = json::array();
    }
  } else {
    json tmpj;
    tmpj["IgnorePaths"] = json::array();
    configJson["Running"] = tmpj;
  }
}

ConfigManager::~ConfigManager() {
  src_set.clear();
  src2ast_map.clear();
  extdef2src_map.clear(); // shared_ptr maybe err?
  configJson.clear();
}

/*************************
 * Maybe Unused any more
 *************************/
/* FIXME:
 * temporary just support x86
 */
bool ConfigManager::isSyscall(const string &funcname) {
  /*
  if (funcname.find("__x64_sys_") == 0 || funcname.find("__ia32_sys_") == 0 ||
      funcname.find("__ia32_compat_sys_") == 0 ||
      funcname.find("__se_sys_") == 0) {
  */
  if (funcname.find("__x64_sys_") == 0) {
    return true;
  } else {
    return false;
  }
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

bool ConfigManager::isNeedToAnalysis(clang::SourceManager &SM,
                                     clang::SourceLocation SL) {
  if (SM.isInSystemHeader(SL)) {
    return false;
  }
  if (SM.isInExternCSystemHeader(SL)) {
    return false;
  }
  if (SM.isInSystemMacro(SL)) {
    return false;
  }
  std::string locstr = SL.printToString(SM);
  for (auto &igp : ignorePath) {
    if (locstr.find(igp) != string::npos) {
      return false;
    }
  }
  return true;
}

bool ConfigManager::isNeedToAnalysis(clang::FunctionDecl *FD) {
  if (FD == nullptr || !FD->hasBody()) {
    return false;
  }
  std::string funcname = FD->getName().str();
  // TODO: ignore builtin
  if (funcname.find("__builtin") == 0) {
    return false;
  }
  if (funcname.find("atomic") == 0) {
    return false;
  }
  // ignore **_printk/printf(many device drivers have these function for debug)
  if (funcname.find("printk") != funcname.npos) {
    return false;
  }
  if (funcname.find("printf") != funcname.npos) {
    return false;
  }
  if (hasAnalysisFunc_set.find(funcname) != hasAnalysisFunc_set.end()) {
    // llvm::errs() << "[!] has been handled: [" << funcname << " ]\n";
    return false;
  }
  /* Note:
   * It is decl loc, not defination loc !!!
   * Because ign path always just include header file,
   * not include the impl.c file path.
   */
  clang::SourceLocation sl1 = FD->getFirstDecl()->getLocation();
  clang::SourceLocation sl2 = FD->getLocation();
  clang::SourceManager &sm = FD->getASTContext().getSourceManager();
  if (isNeedToAnalysis(sm, sl1) && isNeedToAnalysis(sm, sl2)) {
    return true;
  } else {
    // TODO: not hasAnalysisFunc, is ignFunc
    // hasAnalysisFunc_set.insert(funcname);
    return false;
  }
}

bool ConfigManager::isNeedToAnalysis(clang::RecordDecl *RD, bool isFunPtr) {
  if (RD == nullptr) {
    return false;
  }
  if (RD->getName().startswith("pt_regs")) {
    return false;
  }
  if (isFunPtr == false) {
    if (RD->getName().startswith("trace_event_raw")) {
      return false;
    }
  }
  clang::SourceLocation sl = RD->getLocation();
  clang::SourceManager &sm = RD->getASTContext().getSourceManager();
  return isNeedToAnalysis(sm, sl);
}

bool ConfigManager::isNeedToAnalysis(clang::FieldDecl *FD, bool isFunPtr) {
  if (FD == nullptr) {
    return false;
  }
  if (FD->getName().startswith("pt_regs")) {
    return false;
  }
  if (isFunPtr == false) {
    if (FD->getName().startswith("trace_event_raw")) {
      return false;
    }
  }
  clang::QualType Ty = FD->getType();
  if (clang::RecordDecl *rd = Ty->getAsRecordDecl()) {
    if (isNeedToAnalysis(rd, isFunPtr) == false) {
      return false;
    }
  }
  clang::SourceLocation sl = FD->getLocation();
  clang::SourceManager &sm = FD->getASTContext().getSourceManager();
  return isNeedToAnalysis(sm, sl);
}
