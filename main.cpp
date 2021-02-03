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

bool hasDoneAnalysis(const Ptr2InfoType &src) {
  for (auto &info : src) {
    if (false == hasDoneAnalysis(info.second)) {
      return false;
    }
  }
  return true;
}

void removeUnExistAndSelfLoopPtr(Ptr2InfoType &src) {
  for (auto &info : src) {
    CGsType &curptees = info.second;
    for (CGsType::iterator ptee = curptees.begin(); ptee != curptees.end();) {
      if (ptee->type != ptee->DirectCall && ptee->type != ptee->NULLFunPtr) {
        if (src.find(*ptee) == src.end() || *ptee == info.first) {
          ptee = curptees.erase(ptee);
        } else {
          ptee++;
        }
      } else {
        ptee++;
      }
    }
  }
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
    for (Ptr2InfoType::iterator info = src.begin(); info != src.end();) {
      // for (init; condition; post), post is empty
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
      for (CGsType::iterator ptee = curptees.begin(); ptee != curptees.end();) {
        // for (init; condition; post), post is empty
        if (ptee->type != ptee->DirectCall && ptee->type != ptee->NULLFunPtr) {
          auto findptees = tgr.find(*ptee);
          if (findptees != tgr.end()) {
            // all CGNodes in tgr are DirectCall/NULLFunPtr
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

// Maybe this algorithm is the correct fixed point algorithm
void analysisPtrInfo(Ptr2InfoType &src) {
  bool isAllDirect = false;
  for (unsigned int i = 0, looptime = src.size() >> 2; i < looptime; i++) {
    // Check if all pointers have clear pointees
    if (isAllDirect = hasDoneAnalysis(src)) {
      break;
    }
    // start the iteration
    llvm::errs() << "iter #" << i << "\n";
    for (auto &info : src) {
      CGsType &curptees = info.second;
      CGsType shouldbeinsert;
      // Get/Collect ptees to replace
      for (const CGNode &ptee : curptees) {
        if (ptee.type != ptee.DirectCall && ptee.type != ptee.NULLFunPtr) {
          auto findptees = src.find(ptee);
          if (findptees != src.end()) {
            for (const CGNode &n : findptees->second) {
              /* Note:
               * DO NOT curptees.erase(ptee) here,
               * because findptees and curptees may have same FunPtr,
               * it will make the Circular reference.
               * See this if conditions
               */
              if (n == info.first || curptees.find(n) != curptees.end()) {
              } else {
                shouldbeinsert.insert(n);
              }
            }
          }
        }
      }
      // Remove the replaced ptees
      for (CGsType::iterator ptee = curptees.begin(); ptee != curptees.end();) {
        /* Note:
         * for (init; condition; post), post is empty
         * DO NOT use (cged = curptees.end(); ptee != cged),
         * becaise curptees.end() may change, too.
         */
        if (ptee->type != ptee->DirectCall && ptee->type != ptee->NULLFunPtr) {
          ptee = curptees.erase(ptee);
        } else {
          ptee++;
        }
      }
      // Replace them
      for (auto &insertptee : shouldbeinsert) {
        curptees.insert(insertptee);
      }
    }
  } // main loop
  if (isAllDirect) {
    llvm::errs() << "[+] All pointers have clear pointees\n";
  } else {
    llvm::errs() << "[!] End iteration to analysis, these func ptrs have not "
                    "actually ptees:\n";
    for (auto &info : src) {
      if (false == hasDoneAnalysis(info.second)) {
        llvm::errs() << info.first.name << " --> ";
        for (auto &ptee : info.second) {
          if (ptee.type != ptee.DirectCall && ptee.type != ptee.NULLFunPtr) {
            llvm::errs() << ptee.name << " # ";
          }
        }
        llvm::errs() << "\n";
      } else {
        continue;
      }
    }
  } // if (isAllDirect)
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
  ASTManager manager(cfgmgr.getFnAstList(), true);
  Ptr2InfoType Need2AnalysisPtrInfo;
  Fun2JsonType fun2json_map;
  for (auto &af : manager.getAstFiles()) {
    std::unique_ptr<ASTUnit> au = manager.loadAUFromAF(af);
    if (au == nullptr) {
      continue;
    }
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

  removeUnExistAndSelfLoopPtr(Need2AnalysisPtrInfo);
  // Ptr2InfoType PurePtrInfo;
  // analysisPtrInfo(Need2AnalysisPtrInfo, PurePtrInfo);
  Ptr2InfoType PurePtrInfo = Need2AnalysisPtrInfo;
  analysisPtrInfo(PurePtrInfo);
  ptrinfo_fn = cfgmgr.getFnHasAnalysisPtrInfo();
  if (ptrinfo_fn.substr(ptrinfo_fn.length() - 5) == ".json") {
    DumpPtrInfo2json(ptrinfo_fn, PurePtrInfo, JsonLogV::FLAT_STRING);
  } else {
    DumpPtrInfo2txt(ptrinfo_fn, PurePtrInfo, true);
  }

  llvm::errs() << "hello world\n";
  return 0;
}
