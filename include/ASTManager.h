#ifndef LKSAST_AST_MANAGER_H
#define LKSAST_AST_MANAGER_H

#include "Common.h"

#include <memory>
#include <string>
#include <vector>

#include <clang/AST/Decl.h>
#include <clang/Frontend/ASTUnit.h>

namespace lksast {

class ASTManager {
private:
  std::vector<std::string> _AFs;
  std::vector<std::unique_ptr<clang::ASTUnit>> _AUs;

public:
  // getter/setter
  // std::vector<std::string> &getAstFiles() { return _AFs; }
  CLASS_MEMBER_GETTER_PROTO(std::vector<std::string> &, AstFiles, _AFs);

  // CLASS_MEMBER_GETTER_PROTO(std::vector<std::unique_ptr<clang::ASTUnit>> &,
  //                           AstUnits, _AUs);
  std::vector<std::unique_ptr<clang::ASTUnit>> &getAstUnits();

public:
  ASTManager(const std::string &astListxt, bool isLimitMem = true);
  void initAFsFromASTListxt(const std::string &astListxt);
  void initAUsFromAFs();
  std::unique_ptr<clang::ASTUnit> loadAUFromAF(const std::string &astpath);
};

} // namespace lksast

#endif /* LKSAST_AST_MANAGER_H */
