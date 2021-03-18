#include "Handle1AST.h"
#include "Common.h"
#include "ConfigManager.h"

#include <clang/AST/ASTContext.h>
#include <clang/Analysis/CFG.h>
#include <llvm/Support/raw_ostream.h>

using namespace clang;

/* Data Structure */
namespace lksast {

/**********************************
 * CGNode Methods
 **********************************/
#if CGNode_METHODS

CGNode::CGNode(CallType t, const std::string &n, const std::string &dl)
    : type(t), declloc(dl) {
  switch (t) {
  case ArrayFunPtrCall:
    name = "Array " + n;
    break;
  case GlobalVarFunPtrCall:
    name = "GlobalVar " + n;
    break;
  default:
    name = n; // DirectCall
    break;
  }
}

CGNode::CGNode(const std::string &n, size_t parmidx, const std::string &dl)
    : declloc(dl) {
  type = ParmFunPtrCall;
  name = n + " " + std::to_string(parmidx);
}

CGNode::CGNode(CallType t, const std::string &n1, const std::string &n2,
               const std::string &dl)
    : type(t), declloc(dl) {
  switch (t) {
  case StructMemberFunPtrCall:
    name = "struct " + n1 + "." + n2;
    break;
  case UnionMemberFunPtrCall:
    name = "union " + n1 + "." + n2;
    break;
  case LocalVarFunPtrCall:
    name = n1 + " " + n2;
    break;
  default:
    llvm::errs() << "[Err] Call CGNode() Err!\n";
    break;
  }
}

const char *CGNode::CallType2String() const {
  switch (type) {
  case NULLCGNode:
    return "NULLCGNode";
  case DirectCall:
    return "DirectCall";
  case StructMemberFunPtrCall:
    return "StructMemberFunPtrCall";
  case UnionMemberFunPtrCall:
    return "UnionMemberFunPtrCall";
  case ArrayFunPtrCall:
    return "ArrayFunPtrCall";
  case ParmFunPtrCall:
    return "ParmFunPtrCall";
  case GlobalVarFunPtrCall:
    return "GlobalVarFunPtrCall";
  case LocalVarFunPtrCall:
    return "LocalVarFunPtrCall";
  case NULLFunPtr:
    return "NULLFunPtr";
  // case OtherCall:
  default:
    return "OtherCall(for debug)";
  }
}

bool CGNode::operator<(const CGNode &ref) const {
  if (type < ref.type) {
    return true;
  } else {
    if (name < ref.name) {
      return true;
    } else {
      // if (declloc < ref.declloc) {
      //   return true;
      // }
      return false;
    }
  }
}
bool CGNode::operator==(const CGNode &ref) const {
  return type == ref.type && name == ref.name;
  // return type == ref.type && name == ref.name && declloc == ref.declloc;
}

size_t CGNode::Hash::operator()(const CGNode &n) const {
  size_t typeHash = static_cast<std::size_t>(n.type);
  size_t nameHash = std::hash<string>{}(n.name) << 1;
  // size_t declHash = std::hash<string>{}(n.declloc) << 1;
  return typeHash ^ nameHash;
  // return ((typeHash ^ nameHash) >> 1) ^ declHash;
}

#endif /* CGNode_METHODS */

/**********************************
 * ResourceAccessNode Methods
 **********************************/
#if ResourceAccessNode_METHODS

ResourceAccessNode::ResourceAccessNode(ResourceType rt, AccessType at,
                                       std::string n)
    : restype(rt), accType(at) {
  if (rt == Structure) {
    name = "struct " + n;
  } else {
    name = "global " + n;
  }
}

ResourceAccessNode::ResourceAccessNode(std::string structname,
                                       std::string fieldname, AccessType at)
    : accType(at) {
  restype = StructMember;
  name = "struct " + structname + "." + fieldname;
}

void ResourceAccessNode::setReadAccessType() {
  if (accType == Write) {
    accType = RW;
  }
}

void ResourceAccessNode::setWriteAccessType() {
  if (accType == Read) {
    accType = RW;
  }
}

std::string ResourceAccessNode::printToString() const {
  if (restype == NULLResourceNode) {
    return "<NULL>";
  } else {
    std::string accstr;
    switch (accType) {
    case Read:
      accstr = "[R] ";
      break;
    case Write:
      accstr = "[W] ";
      break;
    case RW:
      accstr = "[R/W] ";
      break;
    }
    return accstr + name;
  }
}

const char *ResourceAccessNode::ResourceType2String() const {
  switch (restype) {
  case NULLResourceNode:
    return "NULLResourceNode";
  case StructMember:
    return "StructMember";
  case GlobalVal:
    return "GlobalVal";
  case Structure:
    return "Structure";
  default:
    return "[Err]ResourceNode";
  }
}

const char *ResourceAccessNode::AccessType2String() const {
  switch (accType) {
  case Read:
    return "R";
  case Write:
    return "W";
  case RW:
    return "R/W";
  }
}

bool ResourceAccessNode::operator<(const ResourceAccessNode &ref) const {
  if (name < ref.name) {
    return true;
  } else {
    if (restype < ref.restype) {
      return false; // 注意这里是逆序
    } else {
      return true;
    }
  }
}

bool ResourceAccessNode::operator==(const ResourceAccessNode &ref) const {
  return restype == ref.restype && name == ref.name && accType == ref.accType;
  // FIXME: should be merge: "val1, w" and "val1, r"
  // return restype == ref.restype && name == ref.name;
}

size_t ResourceAccessNode::Hash::operator()(const ResourceAccessNode &n) const {
  size_t restypeHash = static_cast<std::size_t>(n.restype);
  size_t acctypeHash = static_cast<std::size_t>(n.accType) << 1;
  size_t nameHash = std::hash<string>{}(n.name) << 1;
  return ((restypeHash ^ acctypeHash) >> 1) ^ nameHash;
}

#endif /* ResourceAccessNode_METHODS */

/**********************************
 * FunctionResult Methods
 **********************************/
#if FunctionResult_METHODS

FunctionResult::FunctionResult(FunctionDecl *FD) {
  funcname = FD->getName().str();
  SourceManager &sm = FD->getASTContext().getSourceManager();
  declloc = FD->getLocation().printToString(sm);
}

bool FunctionResult::operator<(const FunctionResult &ref) const {
  if (funcname < ref.funcname) {
    return true;
  } else {
    // if (declloc < ref.declloc) {
    //   return true;
    // }
    return false;
  }
}

bool FunctionResult::operator==(const FunctionResult &ref) const {
  return funcname == ref.funcname;
  // return funcname == ref.funcname && declloc == ref.declloc;
}

size_t FunctionResult::Hash::operator()(const FunctionResult &n) const {
  size_t nameHash = std::hash<string>{}(n.funcname);
  // size_t declHash = std::hash<string>{}(n.declloc) << 1;
  return nameHash;
  // return nameHash ^ declHash;
}

#endif /* FunctionResult_METHODS */

} // namespace lksast

/* Core */
namespace lksast {
/**********************************
 * StmtLhsRhsAnalyzer Methods
 **********************************/
#if StmtLhsRhsAnalyzer_METHODS

void StmtLhsRhsAnalyzer::ResetStmt(bool lhs, ResAnaMode_t mode) {
  // FIXME? not need to reset hasAsm flag
  _ResourceMode = mode;
  isLhs = lhs;
  _AccType = isLhs ? ResourceAccessNode::AccessType::Write
                   : ResourceAccessNode::AccessType::Read;
}

void StmtLhsRhsAnalyzer::VisitMemberExpr(MemberExpr *ME) {
  if (_ResourceMode == NONE_MODE) {
    return;
  }
  // FIXME: should be deduplicated the funcptr field, or think about:
  // struct fs->struct fsop->...(all of fields in fsop is funcptr)
  ValueDecl *memdecl = ME->getMemberDecl();
  if (FieldDecl *fd = dyn_cast<FieldDecl>(memdecl)) {
    if (!_CfgMgr.isNeedToAnalysis(fd)) {
      return;
    }
    RecordDecl *rd = fd->getParent();
    /* Note: 3 */
    if (!rd->isStruct()) {
      return;
    }
    if (!_CfgMgr.isNeedToAnalysis(rd)) {
      return;
    }
    ResourceAccessNode tmp(rd->getName().str(), fd->getName().str(), _AccType);
    /* Note:
     * VisitDeclRefExpr() will insert these resource into _Resources
     * if they are global
     */
    _tmpResources.insert(tmp);
  } else {
    llvm::errs() << "[Unknown] ME->getMemberDecl() is not a FieldDecl?\n";
    memdecl->getLocation().dump(_SM);
  }
}

void StmtLhsRhsAnalyzer::VisitDeclRefExpr(DeclRefExpr *DRE) {
  if (_ResourceMode == NONE_MODE) {
    return;
  }
  ValueDecl *valuedecl = DRE->getDecl();
  if (const VarDecl *VD = dyn_cast<VarDecl>(valuedecl)) {
    QualType Ty = VD->getType();
    if (auto convertype = Ty->getPointeeOrArrayElementType()) {
      Ty = convertype->getCanonicalTypeInternal();
    }
    if (VD->hasGlobalStorage() && !VD->isStaticLocal()) {
      /* Note:
       * It seems that MoonShine Smatch do not handle the global var which is
       * not structure
       */
      if (Ty->isStructureType()) {
        for (auto &t : _tmpResources) {
          _Result._Resources.insert(t);
        }
        _tmpResources.clear();
        return;
      } else {
        // ResourceAccessNode tmp(ResourceAccessNode::ResourceType::GlobalVal,
        //                        _AccType, VD->getName().str());
        // _Result._Resources.insert(tmp);
      }
    }
  } else {
    if (valuedecl) {
      /* Note: 3 */
      switch (valuedecl->getKind()) {
      case Decl::Function:
      case Decl::EnumConstant:
        break;
      default:
        llvm::errs() << "[Unknown] DeclRefExpr->getDecl()->getDeclKindName(): "
                     << valuedecl->getDeclKindName() << "\n";
        break;
      }
    } else {
      llvm::errs() << "[nullptr] DRE->getDecl()\n";
      DRE->getLocation().dump(_SM);
      DRE->dump();
    }
  }
}

void StmtLhsRhsAnalyzer::VisitCallExpr(CallExpr *CE) {
  if (_ResourceMode != NONE_MODE) {
    return;
  }
  if (FunctionDecl *FD = CE->getDirectCallee()) {
    // if (calleeD->getKind() == Decl::Function) {
    string srcloc = FD->getLocation().printToString(_SM);
    string funname = FD->getName().str();
    CGNode tmp(CGNode::CallType::DirectCall, funname, srcloc);
    _Result._Callees.insert(tmp);
  }
}

#define ASM_DEBUG 0

void StmtLhsRhsAnalyzer::Visit(Stmt *S) {
  if (S == nullptr) {
    return;
  }
#if ASM_DEBUG
  if (hasAsm) {
    return;
  }
#endif
  StmtVisitor<StmtLhsRhsAnalyzer>::Visit(S); // visit father first
  for (auto *Child : S->children()) {
    if (Child) {
      Visit(Child);
    }
  }
  // StmtVisitor<StmtLhsRhsAnalyzer>::Visit(S); // visit children first
}

#endif /* StmtLhsRhsAnalyzer_METHODS */

/**********************************
 * FunctionAnalyzer Methods
 **********************************/
#if FunctionAnalyzer_METHODS

void FunctionAnalyzer::AnalysisReadStmt(Stmt *LHSorSR) {
  _StmtAnalyzer.ResetStmt(false, StmtLhsRhsAnalyzer::ResAnaMode_t::ALL_MODE);
  _StmtAnalyzer.Visit(LHSorSR);
}

void FunctionAnalyzer::AnalysisWriteStmt(Stmt *RHSorSW) {
  _StmtAnalyzer.ResetStmt(true, StmtLhsRhsAnalyzer::ResAnaMode_t::ALL_MODE);
  _StmtAnalyzer.Visit(RHSorSW);
}

void FunctionAnalyzer::AnalysisCallee(Stmt *s) {
  _StmtAnalyzer.ResetStmt(false, StmtLhsRhsAnalyzer::ResAnaMode_t::NONE_MODE);
  _StmtAnalyzer.Visit(s);
}

void FunctionAnalyzer::AnalysisWithCFG() {
  std::unique_ptr<CFG> cfg = CFG::buildCFG(
      _FD, _FD->getBody(), &_FD->getASTContext(), CFG::BuildOptions());
  if (cfg == nullptr) {
    return;
  }
  for (CFG::const_reverse_iterator it = cfg->rbegin(), ei = cfg->rend();
       it != ei; ++it) {
    const CFGBlock *block = *it;

    if (Stmt *TS = const_cast<Stmt *>(block->getTerminatorStmt())) {
      switch (TS->getStmtClass()) {
      case Stmt::IfStmtClass:     /* if, else, else if */
      case Stmt::SwitchStmtClass: /* switch case */
      case Stmt::DoStmtClass:
      case Stmt::WhileStmtClass:
      case Stmt::ForStmtClass:
        /* loop */
        // just handle condition
        // condition maybe null, eg, for ( ; ; )
        if (const Stmt *TC = block->getTerminatorCondition()) {
          AnalysisReadStmt(const_cast<Stmt *>(TC));
        }
        break;
      case Stmt::ConditionalOperatorClass:
      case Stmt::BinaryConditionalOperatorClass:
        /* ? : */
      case Stmt::BinaryOperatorClass: /* &&, ||, ! */
      case Stmt::BreakStmtClass:
      case Stmt::ContinueStmtClass:
      case Stmt::GotoStmtClass:
      case Stmt::IndirectGotoStmtClass:
        /* goto */
      case Stmt::GCCAsmStmtClass:
      case Stmt::MSAsmStmtClass:
        /* asm code */
        // not need to handle
        break;
      default:
        llvm::errs() << "[!] Warning: Unknown Terminator Stmt Class\n";
        llvm::errs() << "  |- [ " << TS->getStmtClassName() << " ]: ";
        block->printTerminator(llvm::errs(), LangOptions());
        llvm::errs() << "\n";
        llvm::errs() << "  `- in ";
        TS->getBeginLoc().print(llvm::errs(), _SM);
        llvm::errs() << "\n";
        break;
      }
    }

    for (CFGBlock::const_iterator bi = block->begin(), be = block->end();
         bi != be; ++bi) {
      if (llvm::Optional<CFGStmt> CS = bi->getAs<CFGStmt>()) {
        const Stmt *stmt = CS->getStmt();
        AnalysisCallee(const_cast<Stmt *>(stmt));
        if (const BinaryOperator *BO = dyn_cast<BinaryOperator>(stmt)) {
          Expr *lhs = BO->getLHS();
          if (BO->isAssignmentOp()) {
            AnalysisWriteStmt(lhs);
          }
        } else if (const UnaryOperator *UO = dyn_cast<UnaryOperator>(stmt)) {
          /* Here we assume that:
           * All UnOp will write accsee Opcode, not only
           * isIncrementDecrementOp()
           */
          AnalysisWriteStmt(UO->getSubExpr());
        } // if stmt is a BinOp or UnOp ?
      }
    } // for stmt in bb
  }   // for bb in cfg
}

void FunctionAnalyzer::Analysis() {
  if (_FD->hasBody()) {
    std::string funcname = _FD->getName().str();
    _CfgMgr.hasAnalysisFunc_set.insert(funcname);
    AnalysisWithCFG();
  } else {
    llvm::errs() << "[-] Err! FunctionDecl has no body!\n";
    llvm::errs() << "  `- [-] " << _FD->getLocation().printToString(_SM);
  }
}

#endif /* FunctionAnalyzer_METHODS */

/**********************************
 * TUAnalyzer Methods
 **********************************/
#if TUAnalyzer_METHODS

void TUAnalyzer::HandleTranslationUnit(ASTContext &Context) {
  TranslationUnitDecl *TUD = _ASTCtx.getTranslationUnitDecl();
  TraverseDecl(TUD);
}

// bool VisitFunctionDecl(FunctionDecl *FD) {
bool TUAnalyzer::TraverseFunctionDecl(FunctionDecl *FD) {
  if (FD->hasBody()) {
    if (_CfgMgr.isNeedToAnalysis(FD)) {
      llvm::errs() << "[+] Analysis Function: " << FD->getName() << "\n";
      FunctionAnalyzer funcAnalyzer(FD, _CfgMgr);
      funcAnalyzer.Analysis();
      FunctionResult &curFuncRes = funcAnalyzer.getFunctionResult();
      TUResult.insert(curFuncRes);
    }
  }
  // return RecursiveASTVisitor<TUAnalyzer>::TraverseFunctionDecl(FD);
  return true;
}

#endif /* TUAnalyzer_METHODS */

}; // namespace lksast
