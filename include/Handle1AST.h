#ifndef LKSAST_HANDLE_ONE_AST_H
#define LKSAST_HANDLE_ONE_AST_H

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

/* use for searching */
#define CGNode_METHODS 1
#define ResourceAccessNode_METHODS 1
#define FunctionResult_METHODS 1
#define StmtLhsRhsAnalyzer_METHODS 1
#define FunctionAnalyzer_METHODS 1
#define TUAnalyzer_METHODS 1

/* Data Structure */
namespace lksast {

struct CGNode {
public:
  enum CallType {
    NULLCGNode,             // cast CGNode into bool
    DirectCall,             // cg.add("funname")
    StructMemberFunPtrCall, // cg.add("struct fs.open")
    ArrayFunPtrCall,        // cg.add("arrname")
    ParmFunPtrCall,         // cg.add("caller 0") // 0 parm is funptr
    OtherCall
  };
  CallType type;
  std::string name;
  std::string declloc;

  CGNode() : type(NULLCGNode){};
  CGNode(CallType t, const std::string &n, const std::string &dl);
  CGNode(const std::string &n, size_t parmidx, const std::string &dl);
  CGNode(const std::string &structname, const std::string &fieldname,
         const std::string &dl);
  bool operator<(const CGNode &ref) const;
  bool operator==(const CGNode &ref) const;
  operator bool() { return type != NULLCGNode; };
  bool operator!() { return type == NULLCGNode; };
  struct Hash {
    size_t operator()(const CGNode &n) const;
  };
};

struct ResourceAccessNode {
public:
  // 如果同时符合，则按照如下定义的优先级，其中结构体成员优先级最高
  enum ResourceType {
    NULLResourceNode, // cast ResourceAccessNode as bool
    StructMember,     // struct mm.area
    Structure,        // struct mm
    GlobalVal         // global val
  };
  enum AccessType { Read, Write, RW };
  ResourceType restype;
  AccessType accType;
  std::string name;

  ResourceAccessNode() : restype(NULLResourceNode){};
  ResourceAccessNode(ResourceType rt, AccessType at, std::string n);
  ResourceAccessNode(std::string structname, std::string fieldname,
                     AccessType at);
  void setReadAccessType();
  void setWriteAccessType();
  std::string printToString() const;
  bool operator<(const ResourceAccessNode &ref) const;
  bool operator==(const ResourceAccessNode &ref) const;
  operator bool() { return restype != NULLResourceNode; };
  bool operator!() { return restype == NULLResourceNode; };
  struct Hash {
    size_t operator()(const ResourceAccessNode &n) const;
  };
};

using CGType = std::unordered_set<CGNode, CGNode::Hash>;

using Ptr2InfoType =
    std::unordered_map<CGNode, std::unordered_set<CGNode, CGNode::Hash>,
                       CGNode::Hash>;

// FIXME: "val1, w" != "val1, r"
// but it can/should merge into "val1, rw"
using ResourcesType =
    std::unordered_set<ResourceAccessNode, ResourceAccessNode::Hash>;

class FunctionResult {
public:
  std::string funcname;
  std::string declloc;
  CGType _Callees;
  ResourcesType _Resources;

  FunctionResult(clang::FunctionDecl *FD);
  FunctionResult(const std::string &n) : funcname(n){};
  FunctionResult(const std::string &n, const std::string &l)
      : funcname(n), declloc(l){};
  bool operator<(const FunctionResult &ref) const;
  bool operator==(const FunctionResult &ref) const;
  struct Hash {
    size_t operator()(const FunctionResult &n) const;
  };
};

}; // namespace lksast

/* Helper Function */
namespace lksast {

CGNode GetPointeeWithILE(clang::Expr *initExpr, clang::FunctionDecl *scopeFD,
                         clang::SourceManager &_SM);
void AnalysisPtrInfoWithInitListExpr(clang::InitListExpr *ILE,
                                     clang::FunctionDecl *scopeFD,
                                     Ptr2InfoType &_Need2AnalysisPtrInfo,
                                     ConfigManager &_CfgMgr);
clang::ValueDecl *GetSimpleArrayDecl(clang::Expr *E, clang::SourceManager &_SM);

}; // namespace lksast

/* Core */
namespace lksast {

/*******************************************
 * 分析一条stmt的资源使用情况
 * 需要调用者区分stmt是否为二元操作符等影响资源读写情况的stmt
 * 通过参数isLhs区分，默认为rhs
 * 这是因为当前默认写资源的情况只有当资源位于赋值操作符的lhs才可能发生
 * 其余情况都可以按照读资源处理
 * 因此对于非赋值操作符的stmt设置isLhs为false即可通用
 *******************************************/
class StmtLhsRhsAnalyzer : public clang::StmtVisitor<StmtLhsRhsAnalyzer> {
private:
  clang::FunctionDecl *_ScopeFD;
  clang::SourceManager &_SM;
  ConfigManager &_CfgMgr;
  bool isLhs;
  bool hasAsm;
  ResourceAccessNode::AccessType _AccType;
  Ptr2InfoType &_Need2AnalysisPtrInfo;
  FunctionResult &_Result;

public:
  StmtLhsRhsAnalyzer(clang::FunctionDecl *fd, clang::SourceManager &sm,
                     ConfigManager &cfgmgr, Ptr2InfoType &ptrinfo,
                     FunctionResult &result)
      : _ScopeFD(fd), _SM(sm), _CfgMgr(cfgmgr), _Need2AnalysisPtrInfo(ptrinfo),
        _Result(result), isLhs(false), hasAsm(false) {
    _AccType = ResourceAccessNode::AccessType::Read;
  }
  StmtLhsRhsAnalyzer(clang::FunctionDecl *fd, clang::SourceManager &sm,
                     ConfigManager &cfgmgr, Ptr2InfoType &ptrinfo,
                     FunctionResult &result, bool lhs)
      : _ScopeFD(fd), _SM(sm), _CfgMgr(cfgmgr), _Need2AnalysisPtrInfo(ptrinfo),
        _Result(result), isLhs(lhs), hasAsm(false) {
    _AccType = isLhs ? ResourceAccessNode::AccessType::Write
                     : ResourceAccessNode::AccessType::Read;
  }

  void ResetStmt(bool lhs = false);
  void AnalysisResourceInVD(clang::VarDecl *VD);
  void AnalysisPtrInfoInVD(clang::VarDecl *VD);
  void VisitDeclStmt(clang::DeclStmt *DS);
  void VisitDeclRefExpr(clang::DeclRefExpr *DRE);
  void VisitMemberExpr(clang::MemberExpr *ME);
  /****************************
   * analysis call graph
   * analysis funptr point-to info
   ****************************/
  void AnalysisCallExprArg(clang::Expr *E, CGNode &pointer);
  void VisitCallExpr(clang::CallExpr *CE);
  // It seems that RecursiveASTVisitor don't have TraverseAsmStmt method.
  void VisitAsmStmt(clang::AsmStmt *AS);
  /***************************
   * Traverse and Dispatch
   ***************************/
  void Visit(clang::Stmt *S);
}; // class StmtLhsRhsAnalyzer

class FunctionAnalyzer {
private:
  clang::FunctionDecl *_FD;
  clang::SourceManager &_SM;
  ConfigManager &_CfgMgr;
  Ptr2InfoType &_Need2AnalysisPtrInfo;
  FunctionResult _Result;
  StmtLhsRhsAnalyzer _StmtAnalyzer;

public:
  FunctionAnalyzer(clang::FunctionDecl *fd, ConfigManager &cfgmgr,
                   Ptr2InfoType &ptrinfo)
      : _FD(fd), _CfgMgr(cfgmgr), _SM(_FD->getASTContext().getSourceManager()),
        _Need2AnalysisPtrInfo(ptrinfo), _Result(fd),
        _StmtAnalyzer(_FD, _SM, cfgmgr, ptrinfo, _Result){};

  void AnalysisParms();
  /*
   * 若rhs是FD，则分析lhs是否为member、array，
   *     是则加入指向信息lhs.insert(FD)，不是则忽略
   * 若rhs是PVD，则分析PVD是不是函数指针，
   *     是则加入指向信息lhs.insert("_FDname 0")，不是则忽略
   */
  CGNode getPointee(clang::Expr *rhs);
  CGNode getPointer(clang::Expr *lhs);
  void AnalysisPoint2InfoWithBOAssign(clang::Expr *lhs, clang::Expr *rhs);
  void AnalysisReadStmt(clang::Stmt *LHSorSR);
  void AnalysisWriteStmt(clang::Stmt *RHSorSW);
  void AnalysisWithCFG();
  void Analysis();
  FunctionResult &getFunctionResult() { return _Result; }
}; // class FunctionAnalyzer

class TUAnalyzer : public clang::ASTConsumer,
                   public clang::RecursiveASTVisitor<TUAnalyzer> {
private:
  clang::ASTContext &_ASTCtx;
  ConfigManager &_CfgMgr;
  std::unordered_set<FunctionResult, FunctionResult::Hash> TUResult;
  // FIXME, maybe _Need2AnalysisPtrInfo belong to all TUs, not juse this TU
  Ptr2InfoType _Need2AnalysisPtrInfo;

public:
  TUAnalyzer(const std::unique_ptr<clang::ASTUnit> &au, ConfigManager &cfgmgr)
      : _CfgMgr(cfgmgr), _ASTCtx(au->getASTContext()){};

  void check() { HandleTranslationUnit(_ASTCtx); }
  void HandleTranslationUnit(clang::ASTContext &Context) override;
  bool TraverseFunctionDecl(clang::FunctionDecl *FD); // Traverse
  bool VisitVarDecl(clang::VarDecl *VD);
  bool VisitInitListExpr(clang::InitListExpr *ILE);
  void dump(std::ofstream &of);
}; // class TUAnalyzer

}; // namespace lksast

#endif /* LKSAST_HANDLE_ONE_AST_H */
