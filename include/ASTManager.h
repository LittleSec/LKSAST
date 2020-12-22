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
  std::vector<clang::FunctionDecl *> _FDs;

  uint64_t _DirectCallCnt;
  uint64_t _VarFPCnt;
  uint64_t _ParmVarFPCnt;
  uint64_t _FieldVarFPCnt;

public:
  // getter/setter
  CLASS_MEMBER_GETTER_PROTO(std::vector<std::string> &, AstFiles, _AFs);
  CLASS_MEMBER_GETTER_PROTO(std::vector<std::unique_ptr<clang::ASTUnit>> &,
                            AstUnits, _AUs);
  CLASS_MEMBER_GETTER_PROTO(std::vector<clang::FunctionDecl *> &, FunctionDecls,
                            _FDs);
  /*
    std::vector<std::string> &getAstFiles() { return _AFs; };
    std::vector<std::unique_ptr<clang::ASTUnit>> &getAstUnits() { return _AUs;
    }; std::vector<clang::FunctionDecl *> &getFunctionDecls() { return _FDs; };
  */

  PRIVATE_MEMBER_GETTER(uint64_t, DirectCallCnt);
  PRIVATE_MEMBER_GETTER(uint64_t, VarFPCnt);
  PRIVATE_MEMBER_GETTER(uint64_t, ParmVarFPCnt);
  PRIVATE_MEMBER_GETTER(uint64_t, FieldVarFPCnt);

public:
  ASTManager(const std::string &astListxt);
  void initAFsFromASTListxt(const std::string &astListxt);
  void initAUsFromAFs();
  void initFDsFromAUs();
  std::unique_ptr<clang::ASTUnit> loadAUFromAF(const std::string &astpath);
  void loadFDFromAU(const std::unique_ptr<clang::ASTUnit> &au);
};

} // namespace lksast

#endif /* LKSAST_AST_MANAGER_H */
