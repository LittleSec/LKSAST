#include "ASTManager.h"

#include <llvm-c/Target.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/raw_ostream.h>

using namespace lksast;
using namespace clang;

#define USAGE "Usage: ./bin astList.txt\n"

int main(int argc, char *argv[]) {
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmParser();

  if (argc < 2) {
    llvm::errs() << USAGE;
    return 1;
  }

  std::string astFiletxt = argv[1];
  ASTManager manager(astFiletxt);
#if 0
  for (auto fd : manager.getFunctionDecls()) {
    llvm::errs() << fd->getNameInfo().getAsString() << ": ";
    SourceManager &sm = fd->getASTContext().getSourceManager();
    llvm::errs() << fd->getLocation().printToString(sm) << "\n\t";
    bool flag1 = fd->doesThisDeclarationHaveABody();
    bool flag2 = fd->isDefined();
    llvm::errs() << "doesThisDeclarationHaveABody: " << flag1
                 << ", isDefined: " << flag2 << "\n";
  }
#endif
  llvm::errs() << "direct function call: " << manager.getDirectCallCnt()
               << "\n";
  llvm::errs() << "var function ptr call: " << manager.getVarFPCnt() << "\n";
  llvm::errs() << "parm var function ptr call: " << manager.getParmVarFPCnt()
               << "\n";
  llvm::errs() << "field var function ptr call: " << manager.getFieldVarFPCnt()
               << "\n";

  llvm::errs() << "hello world\n";
  return 0;
}