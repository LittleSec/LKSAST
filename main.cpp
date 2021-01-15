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

#define USAGE "Usage: ./bin config.json\n"

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
        if (ptee.type != ptee.DirectCall && ptee.type != ptee.NULLFunPtr) {
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
        if (ptee->type != ptee->DirectCall && ptee->type != ptee->NULLFunPtr) {
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
  ASTManager manager(cfgmgr.getFnAstList());
  Ptr2InfoType Need2AnalysisPtrInfo;
  std::ofstream fun2jsonfile(cfgmgr.getFnFun2Json());
  for (auto &au : manager.getAstUnits()) {
    llvm::errs() << "[!] Handling AST: " << au->getASTFileName() << "\n";
    TUAnalyzer analyzer(au, cfgmgr, Need2AnalysisPtrInfo);
    analyzer.check();
    std::string resultfilename = au->getASTFileName().str() + ".json";
    std::ofstream outfile(resultfilename);
    analyzer.dumpJSON(outfile);
    outfile.close();
    for (auto &funres : analyzer.getTUResult()) {
      fun2jsonfile << funres.funcname << " " << resultfilename << "\n";
    }
  }
  fun2jsonfile.close();

  std::ofstream outfile(cfgmgr.getFnNeed2AnalysisPtrInfo());
  DumpPtrInfo2txt(outfile, Need2AnalysisPtrInfo, true);
  outfile.close();

  Ptr2InfoType PurePtrInfo;
  analysisPtrInfo(Need2AnalysisPtrInfo, PurePtrInfo);
  DumpPtrInfo2txt(cfgmgr.getFnHasAnalysisPtrInfo(), PurePtrInfo, true);

  llvm::errs() << "hello world\n";
  return 0;
}