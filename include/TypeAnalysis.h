#ifndef LKSAST_TPYE_ANALYSIS_H
#define LKSAST_TPYE_ANALYSIS_H

#include "ConfigManager.h"

#include <fstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/StmtVisitor.h>
#include <clang/Frontend/ASTUnit.h>

namespace lksast {

std::string TrimAttr(const std::string &s);

struct TACGNode {
public:
  enum CallType {
    NULLCGNode,  // cast CGNode into bool
    DirectCall,  // id is funcname
    InDirectCall // id is functype
  };
  CallType type;
  std::string identifier;
  std::string declloc;
  TACGNode() : type(NULLCGNode){};
  TACGNode(CallType t, const std::string &id) : type(t), identifier(id) {}
  TACGNode(clang::FunctionDecl *FD)
      : type(DirectCall), identifier(FD->getName().str()) {}
  const char *CallType2String() const;
  bool operator<(const TACGNode &ref) const;
  bool operator==(const TACGNode &ref) const;
  operator bool() { return type != NULLCGNode; };
  bool operator!() { return type == NULLCGNode; };
  struct Hash {
    size_t operator()(const TACGNode &n) const;
  };
};

using TACGsType = std::unordered_set<TACGNode, TACGNode::Hash>;

using TAPtr2InfoType =
    std::unordered_map<std::string, std::unordered_set<std::string>>;

class TAFunctionAnalyzer : public clang::StmtVisitor<TAFunctionAnalyzer> {
private:
  clang::FunctionDecl *_FD;
  clang::SourceManager &_SM;
  clang::PrintingPolicy &_PP;
  ConfigManager &_CfgMgr;
  TACGsType _Callees;

public:
  TAFunctionAnalyzer(clang::FunctionDecl *fd, ConfigManager &cfgmgr,
                     clang::PrintingPolicy &pp)
      : _FD(fd), _CfgMgr(cfgmgr), _SM(_FD->getASTContext().getSourceManager()),
        _PP(pp){};
  void VisitCallExpr(clang::CallExpr *CE);
  void Visit(clang::Stmt *S);
  TACGsType &getCGs() { return _Callees; }
}; // class TAFunctionAnalyzer

class TATUAnalyzer : public clang::ASTConsumer,
                     public clang::RecursiveASTVisitor<TATUAnalyzer> {
private:
  clang::ASTContext &_ASTCtx;
  clang::PrintingPolicy &_PP;
  ConfigManager &_CfgMgr;
  TAPtr2InfoType &_PtrInfo;
  std::unordered_map<std::string, TACGsType> _TUResult;

public:
  TATUAnalyzer(const std::unique_ptr<clang::ASTUnit> &au, ConfigManager &cfgmgr,
               TAPtr2InfoType &ptrinfo, clang::PrintingPolicy &pp)
      : _CfgMgr(cfgmgr), _ASTCtx(au->getASTContext()), _PtrInfo(ptrinfo),
        _PP(pp) {}

  void check() { HandleTranslationUnit(_ASTCtx); }
  void HandleTranslationUnit(clang::ASTContext &Context) override;
  bool TraverseFunctionDecl(clang::FunctionDecl *FD); // Traverse
  void dumpTree(const std::string &filename);
  void dumpTree(std::ofstream &of);
  void dumpJSON(const std::string &filename);
  void dumpJSON(std::ofstream &of);
}; // class TATUAnalyzer

} // namespace lksast

#endif
