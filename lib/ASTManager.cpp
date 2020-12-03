#include <fstream>

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/CompilerInstance.h>
#include <llvm/Support/raw_ostream.h>

#include "ASTManager.h"
#include "stringhelper.h"

using namespace clang;
using namespace lksast;

namespace {

class FunctionDeclLoader : public ASTConsumer,
                           public RecursiveASTVisitor<FunctionDeclLoader> {
private:
  ASTContext *_ASTCtx;
  std::vector<FunctionDecl *> &_FDs;
  uint64_t _DirectCallCnt;
  uint64_t _VarFPCnt;
  uint64_t _ParmVarFPCnt;
  uint64_t _FieldVarFPCnt;

public: /* getter/setter */
  PRIVATE_MEMBER_GETTER(uint64_t, DirectCallCnt);
  PRIVATE_MEMBER_GETTER(uint64_t, VarFPCnt);
  PRIVATE_MEMBER_GETTER(uint64_t, ParmVarFPCnt);
  PRIVATE_MEMBER_GETTER(uint64_t, FieldVarFPCnt);

public:
  FunctionDeclLoader(const std::unique_ptr<clang::ASTUnit> &au,
                     std::vector<FunctionDecl *> &fds)
      : _FDs(fds), _DirectCallCnt(0), _VarFPCnt(0), _ParmVarFPCnt(0),
        _FieldVarFPCnt(0) {
    ASTContext &ctx = au->getASTContext();
    _ASTCtx = &ctx;
  }

  ~FunctionDeclLoader() { llvm::errs() << "[ ok ]\n"; }

  void check() { HandleTranslationUnit(*_ASTCtx); }

  std::vector<FunctionDecl *> &getFunctionDecls() { return _FDs; };

  void HandleTranslationUnit(ASTContext &Context) override {
    TranslationUnitDecl *TUD = _ASTCtx->getTranslationUnitDecl();
    TraverseDecl(TUD);
  }

  bool isNeedToAnalysis(SourceManager &SM, SourceLocation SL) {
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
    if (locstr.find("/usr/include") == 0) {
      return false;
    }
    return true;
  }

  // bool TraverseFunctionDecl(FunctionDecl *FD) {
  bool VisitFunctionDecl(FunctionDecl *FD) {
    SourceManager &sm = _ASTCtx->getSourceManager();
    if (isNeedToAnalysis(sm, FD->getLocation())) {
      // FD->dump();
      // _FDs.push_back(FD);
      _DirectCallCnt++;
    }
    return true;
  }

  ValueDecl *analysisArrayFunPtr(Expr *E) {
    if (ArraySubscriptExpr *ASE = dyn_cast<ArraySubscriptExpr>(E)) {
      if (Expr *arrBaseExpr = ASE->getBase()->IgnoreImpCasts()) {
        if (DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(arrBaseExpr)) {
          // DRE->getDecl()->dump();
          return DRE->getDecl();
        } else {
          llvm::errs() << "dyn_cast<DeclRefExpr>(arrBaseExpr)\n";
        }
      } else {
        llvm::errs() << "ASE->getBase()->IgnoreImpCasts()\n";
      }
    } else {
      llvm::errs() << "dyn_cast<ArraySubscriptExpr>(E)\n";
    }
    return nullptr;
  }
#if 0
  bool VisitCallExpr(CallExpr *CE) {
    Decl *calleeD = CE->getCalleeDecl();
    if (FunctionDecl *FD = CE->getDirectCallee()) {
      // if (calleeD->getKind() == Decl::Function) {
      _DirectCallCnt++;
    } else if (calleeD == nullptr) {
      llvm::errs() << "### getCalleeDecl nullptr ###\n";
      llvm::errs() << CE->getBeginLoc().printToString(
                          _ASTCtx->getSourceManager())
                   << "\n";
      CE->dump();
      if (Expr *E = CE->getCallee()->IgnoreImpCasts()) {
        if (auto *valuedecl = analysisArrayFunPtr(E)) {
          if (valuedecl->getKind() == Decl::Var) {
            _VarFPCnt++;
          } else {
            valuedecl->dump();
          }
        }
      } else {
        llvm::errs() << "### getCallee nullptr ###\n";
      }
      llvm::errs() << "### end nullptr ###\n";
    } else {
      switch (calleeD->getKind()) {
      case Decl::Var:
        _VarFPCnt++;
        llvm::errs() << CE->getBeginLoc().printToString(
                            _ASTCtx->getSourceManager())
                     << "\n";
        CE->dump();
        break;
      case Decl::ParmVar:
        _ParmVarFPCnt++;
        break;
      case Decl::Field:
        _FieldVarFPCnt++;
        break;
      default:
        llvm::errs() << "[!] should be consider!\n";
        calleeD->dump();
        break;
      }
    }
    return true;
  }
#endif
};

} // namespace

ASTManager::ASTManager(const std::string &astListxt)
    : _DirectCallCnt(0), _VarFPCnt(0), _ParmVarFPCnt(0), _FieldVarFPCnt(0) {
  initAFsFromASTListxt(astListxt);
  initAUsFromAFs();
  initFDsFromAUs();
}

void ASTManager::initAFsFromASTListxt(const std::string &astListxt) {
  std::ifstream fin(astListxt);
  std::string line;
  while (getline(fin, line)) {
    std::string fileName = Trim(line);
    if (fileName != "") {
      _AFs.push_back(fileName);
    }
  }
  fin.close();
}

void ASTManager::initAUsFromAFs() {
  for (auto ast : _AFs) {
    std::unique_ptr<ASTUnit> AU = loadAUFromAF(ast);
    if (AU) {
      _AUs.push_back(std::move(AU));
    }
  }
}

void ASTManager::initFDsFromAUs() {
  uint64_t processingcnt = 0;
  for (auto &au : _AUs) {
    processingcnt++;
    llvm::errs() << "[! " << processingcnt << "] processing "
                 << au->getASTFileName() << "\n";
    loadFDFromAU(au);
    // if (!fds.empty()) {
    //   _FDs.insert(_FDs.end(), fds.begin(), fds.end());
    // }
  }
}

std::unique_ptr<ASTUnit> ASTManager::loadAUFromAF(const std::string &astpath) {
  FileSystemOptions FileSystemOpts;
  IntrusiveRefCntPtr<DiagnosticsEngine> Diags =
      CompilerInstance::createDiagnostics(new DiagnosticOptions());
  std::shared_ptr<PCHContainerOperations> PCHContainerOps;
  PCHContainerOps = std::make_shared<PCHContainerOperations>();
  return std::unique_ptr<ASTUnit>(
      ASTUnit::LoadFromASTFile(astpath, PCHContainerOps->getRawReader(),
                               ASTUnit::LoadEverything, Diags, FileSystemOpts));
}

void ASTManager::loadFDFromAU(const std::unique_ptr<ASTUnit> &au) {
  FunctionDeclLoader fdLoader(au, _FDs);
  fdLoader.check();
  _DirectCallCnt += fdLoader.getDirectCallCnt();
  _VarFPCnt += fdLoader.getVarFPCnt();
  _ParmVarFPCnt += fdLoader.getParmVarFPCnt();
  _FieldVarFPCnt += fdLoader.getFieldVarFPCnt();
}
