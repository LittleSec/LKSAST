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