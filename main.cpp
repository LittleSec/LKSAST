#define NORMAL_TEST_CODE 1
#define CMP_TEST_CODE 0

#include <vector>

#include "ASTManager.h"
#include "ConfigManager.h"
#if NORMAL_TEST_CODE
#include "DumpHelper.h"
#include "Handle1AST.h"
#endif
#if CMP_TEST_CODE
#include "TypeAnalysis.h"
#endif
#include <llvm-c/Target.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/raw_ostream.h>

using namespace lksast;
using namespace clang;

#define USAGE "Usage: ./bin config.json\n"

int main(int argc, char *argv[]) {
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmParser();

  if (argc != 2) {
    llvm::errs() << USAGE;
    llvm::errs() << "NOT: ";
    for (int i = 0; i < argc; i++) {
      llvm::errs() << argv[i] << " ";
    }
    llvm::errs() << "\n";
    return 1;
  }

  ConfigManager cfgmgr(argv[1]);
  // cfgmgr.dump();
  ASTManager manager(cfgmgr.getFnAstList(), true);

#if CMP_TEST_CODE
  TAPtr2InfoType FunPtrInfo;
  LangOptions LO;
  PrintingPolicy PP(LO);
  PP.PolishForDeclaration = true; // void (void) __attribute__((noreturn))
  for (auto &af : manager.getAstFiles()) {
    std::unique_ptr<ASTUnit> au = manager.loadAUFromAF(af);
    if (au == nullptr) {
      continue;
    }
    llvm::errs() << "[!] Handling AST: " << au->getASTFileName() << "\n";
    TATUAnalyzer analyzer(au, cfgmgr, FunPtrInfo, PP);
    analyzer.check();
    analyzer.dumpTree("read_write.txt");
  }
#endif /* CMP_TEST_CODE */

#if NORMAL_TEST_CODE
  Fun2JsonType fun2json_map;
  for (auto &af : manager.getAstFiles()) {
    std::unique_ptr<ASTUnit> au = manager.loadAUFromAF(af);
    if (au == nullptr) {
      continue;
    }
    llvm::errs() << "[!] Handling AST: " << au->getASTFileName() << "\n";
    TUAnalyzer analyzer(au, cfgmgr);
    analyzer.check();
    std::string resultfilename = au->getASTFileName().str() + ".moonshine.json";
    // analyzer.dumpTree(resultfilename);
    analyzer.dumpJSON(resultfilename, JsonLogV::NORMAL);
    shared_ptr<std::string> jsonfile =
        std::make_shared<std::string>(resultfilename);
    for (auto &funres : analyzer.getTUResult()) {
      fun2json_map[funres.funcname] = jsonfile;
    }
  }
  std::string fun2json_fn = cfgmgr.getFnFun2Json();
  if (fun2json_fn.substr(fun2json_fn.length() - 5) == ".json") {
    Dumpfun2json2json(fun2json_fn, fun2json_map);
  } else {
    Dumpfun2json2txt(fun2json_fn, fun2json_map);
  }
#endif /* NORMAL_TEST_CODE */

  llvm::errs() << "hello world\n";
  return 0;
}
