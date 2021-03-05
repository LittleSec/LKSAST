#include "TypeAnalysis.h"
#include "ConfigManager.h"
#include "stringhelper.h"

#include <clang/AST/ASTContext.h>
#include <llvm/Support/raw_ostream.h>

using namespace clang;

namespace lksast {

std::string TrimAttr(const std::string &s) {
  std::vector<std::string> res;
  SplitStr2Vec(s, res, " ");
  for (std::vector<std::string>::iterator it = res.begin(), ed = res.end();
       it != ed;) {
    if (it->find("__attribute__") == 0) {
      it = res.erase(it);
    } else {
      it++;
    }
  }
  return Join(res, " ");
}

const char *TACGNode::CallType2String() const {
  switch (type) {
  case NULLCGNode:
    return "NULLCGNode";
  case DirectCall:
    return "DirectCall";
  case InDirectCall:
    return "InDirectCall";
  default:
    return "OtherCall(for debug)";
  }
}

bool TACGNode::operator<(const TACGNode &ref) const {
  if (type < ref.type) {
    return true;
  } else {
    if (identifier < ref.identifier) {
      return true;
    } else {
      // if (declloc < ref.declloc) {
      //   return true;
      // }
      return false;
    }
  }
}

bool TACGNode::operator==(const TACGNode &ref) const {
  return type == ref.type && identifier == ref.identifier;
  // return type == ref.type && identifier == ref.identifier && declloc ==
  // ref.declloc;
}

size_t TACGNode::Hash::operator()(const TACGNode &n) const {
  return std::hash<std::string>{}(n.identifier);
}

void TAFunctionAnalyzer::VisitCallExpr(CallExpr *CE) {
  if (FunctionDecl *FD = CE->getDirectCallee()) {
    // llvm::errs() << "VisitCallExpr - D: "
    //              << TrimAttr(FD->getType().getAsString(_PP)) << "\n";
    _Callees.insert(TACGNode(FD));
  } else {
    QualType funtype =
        CE->getCallee()->IgnoreParens()->getType(); // not ignore cast
    if (funtype->isFunctionPointerType()) {
      funtype = funtype->getPointeeType();
    }
    std::string funtypestr = TrimAttr(funtype.getAsString(_PP));
    // llvm::errs() << "VisitCallExpr - U: " << funtypestr << "\n";
    _Callees.insert(TACGNode(TACGNode::CallType::InDirectCall, funtypestr));
  }
}

void TAFunctionAnalyzer::Visit(Stmt *S) {
  StmtVisitor<TAFunctionAnalyzer>::Visit(S); // visit father first
  for (auto *Child : S->children()) {
    if (Child) {
      Visit(Child);
    }
  }
  // StmtVisitor<TAFunctionAnalyzer>::Visit(S); // visit children first
}

void TATUAnalyzer::HandleTranslationUnit(ASTContext &Context) {
  TranslationUnitDecl *TUD = _ASTCtx.getTranslationUnitDecl();
  TraverseDecl(TUD);
}

bool TATUAnalyzer::TraverseFunctionDecl(FunctionDecl *FD) {
  if (_CfgMgr.isNeedToAnalysis(FD)) {
    std::string funcname = FD->getName().str();
    llvm::errs() << "TraverseFunctionDecl " << funcname << "\n";
    TAFunctionAnalyzer funcAnalyzer(FD, _CfgMgr, _PP);
    funcAnalyzer.Visit(FD->getBody());
    std::string funtypestr = TrimAttr(FD->getType().getAsString(_PP));
    _PtrInfo[funtypestr].insert(funcname);
    _TUResult[funcname] = funcAnalyzer.getCGs();
    _CfgMgr.hasAnalysisFunc_set.insert(funcname);
  }
  return true;
}

void TATUAnalyzer::dumpTree(const std::string &filename) {
  std::ofstream outfile(filename);
  dumpTree(outfile);
  outfile.close();
}

void TATUAnalyzer::dumpTree(std::ofstream &of) {
  of << "Function Info:\n";
  for (auto &tures : _TUResult) {
    of << "  |- " << tures.first << ":\n";
    of << "  |    |- callgraph:\n";
    for (auto &cgn : tures.second) {
      of << "  |    |    |- " << cgn.identifier << "\n";
    }
    of << "  |    |    `-<End of callgraph>\n";
    of << "  |    `-<End of Function: " << tures.first << ">\n";
  }
  of << "  `-<EOF>\n";
  of << "\n";
}

void TATUAnalyzer::dumpJSON(const std::string &filename) {
  std::ofstream outfile(filename);
  dumpJSON(outfile);
  outfile.close();
}

void TATUAnalyzer::dumpJSON(std::ofstream &of) {}

} // namespace lksast
