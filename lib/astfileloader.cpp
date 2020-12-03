#include "astfileloader.h"
#include "stringhelper.h"

#include <fstream>

#include <clang/Frontend/CompilerInstance.h>
#include <llvm/Support/raw_ostream.h>

using namespace clang;

namespace lksast {

std::vector<std::string> initialize(const std::string &astListxt) {
  std::vector<std::string> astFiles;
  std::ifstream fin(astListxt);
  std::string line;
  while (getline(fin, line)) {
    std::string fileName = Trim(line);
    if (fileName != "") {
      astFiles.push_back(fileName);
    }
  }
  fin.close();

  return astFiles;
}

std::unique_ptr<ASTUnit> loadFromASTFile(const std::string &astpath) {
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
