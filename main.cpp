#include "nlohmann/json.hpp"
#include <iomanip>
#include <iostream>
#include <vector>

#include "ASTManager.h"
#include "ConfigManager.h"
#include "Handle1AST.h"

#include <llvm-c/Target.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/raw_ostream.h>

using namespace lksast;
using namespace clang;
using json = nlohmann::json;

#define USAGE "Usage: ./bin astList.txt\n"

int testJson(std::vector<int> vec, const std::string &s) {
  // create an empty structure (null)
  json j;
  // add a number that is stored as double (note the implicit conversion of j to
  // an object)
  j["pi"] = 3.141;
  // add a Boolean that is stored as bool
  j["happy"] = true;
  // add a string that is stored as std::string
  j["name"] = "Niels";
  // add another null object by passing nullptr
  j["nothing"] = nullptr;
  // add an object inside the object
  j["answer"]["everything"] = 42;
  // add an array that is stored as std::vector (using an initializer list)
  j["list"] = {1, 0, 2};
  // add another object (using an initializer list of pairs)
  j["object"] = {{"currency", "USD"}, {"value", 42.99}};
  std::cout << std::setw(4) << j << std::endl;

  // instead, you could also write (which looks very similar to the JSON above)
  json j2 = {{"pi", 3.141},
             {"happy", true},
             {"name", s},
             {"nothing", vec},
             {"answer", {{"everything", 42}}},
             {"object", {{"currency", "USD"}, {"value", 42.99}}}};
  // j2["list"] = vec;
  std::cout << std::setw(4) << j2 << std::endl;

  return 0;
}

int main(int argc, char *argv[]) {
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmParser();

  if (argc < 2) {
    llvm::errs() << USAGE;
    std::string ts = "Join";
    return testJson({10, 30, 20}, ts);
    // return 1;
  } else if (argc == 2) {
    std::string astFiletxt = argv[1];
    ASTManager manager(astFiletxt);
    for (auto fd : manager.getFunctionDecls()) {
      llvm::errs() << fd->getNameInfo().getAsString() << ": ";
      SourceManager &sm = fd->getASTContext().getSourceManager();
      llvm::errs() << fd->getLocation().printToString(sm) << "\n\t";
      bool flag1 = fd->doesThisDeclarationHaveABody();
      bool flag2 = fd->isDefined();
      llvm::errs() << "doesThisDeclarationHaveABody: " << flag1
                   << ", isDefined: " << flag2 << "\n";
    }
    llvm::errs() << "direct function call: " << manager.getDirectCallCnt()
                 << "\n";
    llvm::errs() << "var function ptr call: " << manager.getVarFPCnt() << "\n";
    llvm::errs() << "parm var function ptr call: " << manager.getParmVarFPCnt()
                 << "\n";
    llvm::errs() << "field var function ptr call: "
                 << manager.getFieldVarFPCnt() << "\n";
  } else if (argc == 3) {
    ConfigManager cfgmgr(argv[1], argv[2]);
    cfgmgr.dump();
  } else if (argc == 4) {
    ASTManager manager(argv[1]);
    ConfigManager cfgmgr(argv[2], argv[3]);
    for (auto &au : manager.getAstUnits()) {
      llvm::errs() << "[!] Handling AST: " << au->getASTFileName() << "\n";
      TUAnalyzer analyzer(au, cfgmgr);
      analyzer.check();
      std::string resultfilename = au->getASTFileName().str() + ".txt";
      std::ofstream outfile(resultfilename);
      analyzer.dump(outfile);
      outfile.close();
    }
  } else {
    llvm::errs() << "TODO\n";
  }

  llvm::errs() << "hello world\n";
  return 0;
}