#ifndef LKSAST_CONFIG_MANAGER_H
#define LKSAST_CONFIG_MANAGER_H

#include <functional>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>

using std::set;
using std::shared_ptr;
using std::string;
using std::unordered_map;

namespace lksast {

template <typename T> struct sharedptrLess {
  bool operator()(const shared_ptr<T> &lhs, const shared_ptr<T> &rhs) {
    return *lhs < *rhs;
  };
};

class ConfigManager {
public:
  unordered_map<string, shared_ptr<string>> extdef2src_map;
  // TODO: can use shared_ptr?(has some problems)
  unordered_map<string, string> src2ast_map;
  set<shared_ptr<string>, sharedptrLess<string>> src_set;

  ConfigManager(const string &extdef2src_file, const string &src2ast_file);
  ~ConfigManager();
  int InsertDefSrcAst(const string &defname, const string &srcfn,
                      const string &astfn);
  int UpdateConfigIntoFile();
  void dump();

private:
  std::pair<string, string> parse_one_line(const string &line);
  void load_extdef2src_map(const string &extdef2src_file);
  void load_src2ast_map(const string &src2ast_file);
};

} // namespace lksast

#endif
