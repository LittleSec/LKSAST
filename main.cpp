#include "nlohmann/json.hpp"
#include <iomanip>
#include <iostream>
#include <vector>

#include "ASTManager.h"
#include "ConfigManager.h"
#include "DumpHelper.h"
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

// src, not &src
void analysisPtrInfo(Ptr2InfoType src, Ptr2InfoType &tgr) {
  tgr.clear();
  unsigned int cnt = 0;
  bool isAllDirect;
  while (!src.empty()) {
    if (cnt > 20) {
      break;
    }
    // insert tgr and remove src
    for (Ptr2InfoType::iterator info = src.begin(), ed = src.end();
         info != ed;) {
      CGsType &curptees = info->second;
      isAllDirect = true;
      for (auto &ptee : curptees) {
        if (ptee.type != ptee.DirectCall) {
          isAllDirect = false;
          break;
        }
      }
      if (isAllDirect) {
        tgr.insert(*info);
        info = src.erase(info);
      } else {
        info++;
      }
    }
    llvm::errs() << "iter " << cnt++ << ", src.size() = " << src.size() << "\n";
    // update src
    for (auto &info : src) {
      CGsType &curptees = info.second;
      CGsType shouldbeinsert;
      for (CGsType::iterator ptee = curptees.begin(), cged = curptees.end();
           ptee != cged;) {
        if (ptee->type != ptee->DirectCall) {
          auto findptees = tgr.find(*ptee);
          if (findptees != tgr.end()) {
            for (auto &n : findptees->second) {
              shouldbeinsert.insert(n);
            }
            ptee = curptees.erase(ptee);
          } else {
            ptee++;
          }
        } else {
          ptee++;
        }
      }
      for (auto &insertptee : shouldbeinsert) {
        curptees.insert(insertptee);
      }
    }
  } // while (!src.empty())
  if (!src.empty()) {
    llvm::errs() << "[!] End iteration to analysis, these func ptrs have not "
                    "actually ptees:\n";
    for (auto &info : src) {
      llvm::errs() << info.first.name << " --> ";
      for (auto &ptee : info.second) {
        llvm::errs() << ptee.name << " # ";
      }
      llvm::errs() << "\n";
    }
  }
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
    Ptr2InfoType Need2AnalysisPtrInfo;
    for (auto &au : manager.getAstUnits()) {
      llvm::errs() << "[!] Handling AST: " << au->getASTFileName() << "\n";
      TUAnalyzer analyzer(au, cfgmgr, Need2AnalysisPtrInfo);
      analyzer.check();
      std::string resultfilename = au->getASTFileName().str() + ".txt";
      std::ofstream outfile(resultfilename);
      analyzer.dump(outfile);
      outfile.close();
    }
    std::ofstream outfile("Need2AnalysisPtrInfo.txt");
    DumpPtrInfo2txt(outfile, Need2AnalysisPtrInfo, true);
    outfile.close();

    Ptr2InfoType PurePtrInfo;
    analysisPtrInfo(Need2AnalysisPtrInfo, PurePtrInfo);
    DumpPtrInfo2txt("PtrInfo.txt", PurePtrInfo, true);
  } else {
    llvm::errs() << "TODO\n";
  }

  llvm::errs() << "hello world\n";
  return 0;
}