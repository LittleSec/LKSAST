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

namespace lksast {

ASTManager::ASTManager(const std::string &astListxt, bool isLimitMem) {
  initAFsFromASTListxt(astListxt);
  if (false == isLimitMem) {
    initAUsFromAFs();
  }
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

std::vector<std::unique_ptr<clang::ASTUnit>> &ASTManager::getAstUnits() {
  if (_AUs.empty()) {
    initAUsFromAFs();
  }
  return _AUs;
}

void ASTManager::initAUsFromAFs() {
  for (auto ast : _AFs) {
    std::unique_ptr<ASTUnit> AU = loadAUFromAF(ast);
    if (AU) {
      _AUs.push_back(std::move(AU));
    }
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

} // namespace lksast
