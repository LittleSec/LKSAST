#ifndef LKSAST_CONFIG_MANAGER_H
#define LKSAST_CONFIG_MANAGER_H

#include <functional>
#include <list>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/Basic/SourceManager.h>

#include "nlohmann/json.hpp"

using std::list;
using std::set;
using std::shared_ptr;
using std::string;
using std::unordered_map;
using std::unordered_set;
using json = nlohmann::json;

namespace lksast {

template <typename T> struct sharedptrLess {
  bool operator()(const shared_ptr<T> &lhs, const shared_ptr<T> &rhs) {
    return *lhs < *rhs;
  };
};

class ConfigManager {
public:
  unordered_set<string> hasAnalysisFunc_set;
  unordered_set<string> ignoredFunc_set;

  ConfigManager(const std::string &config_file);
  ~ConfigManager();
  void dump(); // for debug
  bool isNeedToAnalysis(clang::SourceManager &SM, clang::SourceLocation SL);
  bool isNeedToAnalysis(clang::Decl *D);
  bool isSyscall(const std::string &funcname);
  std::string getFnAstList() {
    return configJson["Input"]["AstList"].get<std::string>();
  };
  std::string getFnExtdefMapping() {
    return configJson["Input"]["ExtdefMapping"].get<std::string>();
  };
  std::string getFnSrc2Ast() {
    return configJson["Input"]["Src2Ast"].get<std::string>();
  };
  std::string getFnFun2Json() {
    return configJson["Output"]["Fun2Json"].get<std::string>();
  };
  std::string getFnNeed2AnalysisPtrInfo() {
    return configJson["Output"]["Need2AnalysisPtrInfo"].get<std::string>();
  };
  std::string getFnHasAnalysisPtrInfo() {
    return configJson["Output"]["HasAnalysisPtrInfo"].get<std::string>();
  };

private:
  list<string> ignorePath;
  json configJson;

  void HandleCfgJson();
  void HandleInputCfg(json &inj);
  void HandleOutputCfg(json &outj);
  void HandleRunningCfg();

  /*************************
   * Maybe Unused any more
   *************************/
private:
  std::pair<string, string> parse_one_line(const string &line);
  void load_extdef2src_map(const string &extdef2src_file);
  void load_src2ast_map(const string &src2ast_file);

public:
  unordered_map<string, shared_ptr<string>> extdef2src_map;
  // TODO: can use shared_ptr?(has some problems)
  unordered_map<string, string> src2ast_map;
  set<shared_ptr<string>, sharedptrLess<string>> src_set;
  int InsertDefSrcAst(const string &defname, const string &srcfn,
                      const string &astfn);
};

} // namespace lksast

#endif
