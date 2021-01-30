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

bool hasDoneAnalysis(const CGsType &ptees) {
  for (auto &ptee : ptees) {
    if (ptee.type != ptee.DirectCall && ptee.type != ptee.NULLFunPtr) {
      return false;
    }
  }
  return true;
}

// src, not &src
void analysisPtrInfo(Ptr2InfoType src, Ptr2InfoType &tgr) {
  tgr.clear();
  unsigned int cnt = src.size(), lastcnt = 0, looptime = 0;
  bool isAllDirect;
  while (!src.empty()) {
    if (cnt == lastcnt) {
      break;
    }
    lastcnt = cnt;
    // insert tgr and remove src
    for (Ptr2InfoType::iterator info = src.begin(), ed = src.end();
         info != ed;) { // for (init; condition; post), post is empty
      if (hasDoneAnalysis(info->second)) {
        tgr.insert(*info);
        info = src.erase(info);
      } else {
        info++;
      }
    }
    llvm::errs() << "iter " << looptime++ << ", src.size() = " << src.size()
                 << "\n";
    // update src
    for (auto &info : src) {
      CGsType &curptees = info.second;
      CGsType shouldbeinsert;
      for (CGsType::iterator ptee = curptees.begin(), cged = curptees.end();
           ptee != cged;) { // for (init; condition; post), post is empty
        if (ptee->type != ptee->DirectCall && ptee->type != ptee->NULLFunPtr) {
          auto findptees = tgr.find(*ptee);
          if (findptees != tgr.end()) {
            for (auto &n : findptees->second) {
              /* Note:
               * may have Circular reference?
               */
              if (n == info.first) {
              } else {
                shouldbeinsert.insert(n);
              }
            }
            ptee = curptees.erase(ptee);
          } else {
            /* Note:
             * not find in tgr, and not find in src
             * means do not has this funptr(eg. not in compiledb.json)
             * so here remove it, too.
             */
            if (src.find(*ptee) == src.end()) {
              ptee = curptees.erase(ptee);
            } else if (*ptee == info.first) {
              ptee = curptees.erase(ptee);
            } else {
              ptee++;
            }
          }
        } else {
          ptee++;
        }
      }
      for (auto &insertptee : shouldbeinsert) {
        curptees.insert(insertptee);
      }
    }
    cnt = src.size();
  } // while (!src.empty())
  if (!src.empty()) {
    llvm::errs() << "[!] End iteration to analysis, these func ptrs have not "
                    "actually ptees:\n";
    for (auto &info : src) {
      llvm::errs() << info.first.name << " --> ";
      if (info.second.empty()) {
        llvm::errs() << "[ Empty pointees ] # ";
      } else {
        for (auto &ptee : info.second) {
          llvm::errs() << ptee.name << " # ";
        }
      }
      llvm::errs() << "\n";
    }
  } else {
    llvm::errs() << "[+] All pointers have clear pointees\n";
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
  Fun2JsonType fun2json_map;
  for (auto &au : manager.getAstUnits()) {
    llvm::errs() << "[!] Handling AST: " << au->getASTFileName() << "\n";
    TUAnalyzer analyzer(au, cfgmgr, Need2AnalysisPtrInfo);
    analyzer.check();
    std::string resultfilename = au->getASTFileName().str() + ".json";
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

  std::string ptrinfo_fn = cfgmgr.getFnNeed2AnalysisPtrInfo();
  if (ptrinfo_fn.substr(ptrinfo_fn.length() - 5) == ".json") {
    DumpPtrInfo2json(ptrinfo_fn, Need2AnalysisPtrInfo, JsonLogV::NORMAL);
  } else {
    DumpPtrInfo2txt(ptrinfo_fn, Need2AnalysisPtrInfo, true);
  }

  Ptr2InfoType PurePtrInfo;
  analysisPtrInfo(Need2AnalysisPtrInfo, PurePtrInfo);
  ptrinfo_fn = cfgmgr.getFnHasAnalysisPtrInfo();
  if (ptrinfo_fn.substr(ptrinfo_fn.length() - 5) == ".json") {
    DumpPtrInfo2json(ptrinfo_fn, PurePtrInfo, JsonLogV::FLAT_STRING);
  } else {
    DumpPtrInfo2txt(ptrinfo_fn, PurePtrInfo, true);
  }

  llvm::errs() << "hello world\n";
  return 0;
}