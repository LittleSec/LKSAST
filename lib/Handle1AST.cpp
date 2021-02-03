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

/* Helper Function */
namespace lksast {

std::pair<CGNode, CGNode>
FunPtrExtractor::FromExpr(Expr *E, FunctionDecl *scopeFD, bool shouldCheck) {
  std::pair<CGNode, CGNode> ret;
  ret.first = _nullcgnode;
  ret.second = _nullcgnode;
  if (E == nullptr) {
    return ret;
  }
  E = E->IgnoreParenCasts();
  /* Note:
   * CE->getCalleeDecl == CE->getCallee()->getReferencedDeclOfCallee()
   * clang/lib/AST/Expr.cpp#getReferencedDeclOfCallee()
   * getReferencedDeclOfCallee() think about both c cpp,
   * here just think about c,
   * so it does not need refer to the code of getReferencedDeclOfCallee()
   */
  // eg. call func1(&fun) ==> `&` is an UO
  if (UnaryOperator *UO = dyn_cast<UnaryOperator>(E)) {
    E = UO->getSubExpr()->IgnoreParenCasts();
  }
  if (DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(E)) {
    // local: parm, funcptr val, func
    // global: funcptr val, func
    ret.first = FromDeclRefExpr(DRE, scopeFD, shouldCheck);
  } else if (MemberExpr *ME = dyn_cast<MemberExpr>(E)) {
    ret.first = FromMemberExpr(ME, shouldCheck);
  } else if (ArraySubscriptExpr *ASE = dyn_cast<ArraySubscriptExpr>(E)) {
    ret.first = FromArraySubscriptExpr(ASE, scopeFD, shouldCheck);
  } else if (ConditionalOperator *CO = dyn_cast<ConditionalOperator>(E)) {
    ret.first = FromExpr(CO->getTrueExpr(), scopeFD, shouldCheck).first;
    ret.second = FromExpr(CO->getFalseExpr(), scopeFD, shouldCheck).first;
  } else if (BinaryConditionalOperator *BCO =
                 dyn_cast<BinaryConditionalOperator>(E)) {
    ret.first = FromExpr(BCO->getTrueExpr(), scopeFD, shouldCheck).first;
    ret.second = FromExpr(BCO->getFalseExpr(), scopeFD, shouldCheck).first;
    // } else if (IntegerLiteral *IL = dyn_cast<IntegerLiteral>(E)) {
  } else if (isa<ImplicitValueInitExpr>(E) ||
             E->isIntegerConstantExpr(_ASTCtx)) {
    if (!shouldCheck) {
      ret.first = CGNode(E->getExprLoc().printToString(_SM));
    }
  } else {
    // TODO: many Expr class will has FunPtr, eg. ILE, caller should filter
    // llvm::errs() << "[unknown][FunPtrExtractor] StmtClassName: "
    //              << E->getStmtClassName() << "\n";
    // E->getExprLoc().dump(_SM);
  }
  return ret;
}

// FIXME: Unused
std::string getUnionRDName(MemberExpr *ME, SourceManager &_SM) {
  Expr *subE = nullptr;
  std::string ret = "";
  while (true) {
    subE = ME->getBase()->IgnoreParenCasts();
    if (MemberExpr *subME = dyn_cast<MemberExpr>(subE)) {
      ValueDecl *memdecl = ME->getMemberDecl();
      FieldDecl *fd = dyn_cast<FieldDecl>(memdecl);
      assert(fd != nullptr); // assert(Decl::Field == memdecl->getKind());
      RecordDecl *rd = fd->getParent();
      if (rd->isAnonymousStructOrUnion()) {
        rd = rd->getOuterLexicalRecordContext();
      }
      if (rd->isStruct()) {
        ret = rd->getName().str() + "::" + ret;
        break;
      } else if (rd->isUnion()) {
        ret = rd->getName().str() + "::" + ret;
      } else {
        // ?
      }
    } else if (DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(subE)) {
      ValueDecl *valdecl = DRE->getDecl();
      assert(valdecl->getKind() == Decl::Record);
      if (ret == "") {
        ret = valdecl->getName().str() + "::" + ret;
      } else {
        ret = valdecl->getName().str();
      }
      break;
    } else {
      llvm::errs() << "[Err] ME->getBase() is not MemberExpr\n";
      subE->getExprLoc().dump(_SM);
      subE->dump();
      break;
    }
  }
}

CGNode FunPtrExtractor::FromMemberExpr(MemberExpr *ME, bool shouldCheck) {
  if (ME == nullptr) {
    return _nullcgnode;
  }
  ValueDecl *memdecl = ME->getMemberDecl();
  // TODO: anonymous Struct or Union, still has some problems
  if (Decl::Field == memdecl->getKind()) {
    FieldDecl *fd = dyn_cast<FieldDecl>(memdecl);
    assert(fd != nullptr);
    RecordDecl *rd = fd->getParent();
    if (!_CfgMgr.isNeedToAnalysis(rd)) {
      return _nullcgnode;
    }
    CGNode::CallType Ct;
    if (rd->isAnonymousStructOrUnion()) {
      rd = rd->getOuterLexicalRecordContext();
    }
    if (rd->isStruct()) {
      Ct = CGNode::CallType::StructMemberFunPtrCall;
    } else if (rd->isUnion()) {
      Ct = CGNode::CallType::UnionMemberFunPtrCall;
    } else {
      // llvm::errs() << "[Unknown][MemberExpr] RecordDecl is not a
      // Structure\n"; rd->getLocation().dump(_SM);
      return _nullcgnode;
    }
    QualType Ty = fd->getType();
    /* Note: 2 */
    CGNode tmp(Ct, rd->getName().str(), fd->getName().str(),
               fd->getLocation().printToString(_SM));
    if (shouldCheck) {
      if (Ty->isFunctionPointerType()) {
        return tmp;
      }
    } else {
      return tmp;
    }
  } else {
    llvm::errs() << "[Unknown] ME->getMemberDecl() is not a FieldDecl?\n";
    memdecl->getLocation().dump(_SM);
  }
  return _nullcgnode;
} // namespace lksast

CGNode FunPtrExtractor::FromArraySubscriptExpr(ArraySubscriptExpr *ASE,
                                               clang::FunctionDecl *scopeFD,
                                               bool shouldCheck) {
  if (ASE == nullptr) {
    return _nullcgnode;
  }
  Expr *arrBaseExpr = ASE->getBase()->IgnoreParenCasts();
  /* Note:
   * Maybe we can recursive call FromExpr(),
   * but here we want only handle a few situation:
   * 1. DeclRefExpr->VarDecl
   * 2. MemberExpr
   * By the way, Here VD usually is a ArrayFunPtrCall
   */
  if (DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(arrBaseExpr)) {
    return FromDeclRefExpr(DRE, scopeFD, shouldCheck);
  } else if (MemberExpr *ME = dyn_cast<MemberExpr>(arrBaseExpr)) {
    return FromMemberExpr(ME, shouldCheck);
  } else {
  }
  return _nullcgnode;
}

CGNode FunPtrExtractor::EmitCGNodeFromVD(VarDecl *VD, FunctionDecl *scopeFD) {
  CGNode tmp;
  if (VD->isLocalVarDeclOrParm()) {
    assert(scopeFD != nullptr);
    if (ParmVarDecl *PVD = dyn_cast<ParmVarDecl>(VD)) {
      tmp = CGNode(scopeFD->getName().str(), PVD->getFunctionScopeIndex(),
                   PVD->getLocation().printToString(_SM));
    } else {
      tmp =
          CGNode(CGNode::CallType::LocalVarFunPtrCall, scopeFD->getName().str(),
                 VD->getName().str(), VD->getLocation().printToString(_SM));
    }
  } else {
    tmp = CGNode(CGNode::CallType::GlobalVarFunPtrCall, VD->getName().str(),
                 VD->getLocation().printToString(_SM));
  }
  return tmp;
}

CGNode FunPtrExtractor::FromDeclRefExpr(DeclRefExpr *DRE, FunctionDecl *scopeFD,
                                        bool shouldCheck) {
  if (DRE == nullptr) {
    return _nullcgnode;
  }
  ValueDecl *valuedecl = DRE->getDecl();
  if (valuedecl == nullptr) {
    llvm::errs() << "[nullptr][DRE] DRE->getDecl()\n";
    DRE->getLocation().dump(_SM);
  } else if (VarDecl *VD = dyn_cast<VarDecl>(valuedecl)) {
    QualType Ty = VD->getType();
    /* Note: 2 */
    if (shouldCheck) {
      if (Ty->isFunctionPointerType()) {
        return EmitCGNodeFromVD(VD, scopeFD);
      }
    } else {
      /* shouldCheck==false
       * Note:
       * caller should ensure that when shouldCheck==false
       * the DRE is actually a function pointer,
       * eg. a void* ptr from parm
       */
      return EmitCGNodeFromVD(VD, scopeFD);
    } // if (shouldCheck)
  } else if (FunctionDecl *FD = dyn_cast<FunctionDecl>(valuedecl)) {
    CGNode tmp(CGNode::CallType::DirectCall, FD->getName().str(),
               FD->getLocation().printToString(_SM));
    return tmp;
  } else {
    if (!shouldCheck) {
      llvm::errs() << "[Unknown][DRE] DRE->getDecl() kind\n";
      valuedecl->getLocation().dump(_SM);
    } else {
      // ignore logging
    }
  }
  return _nullcgnode;
}

void FunPtrExtractor::FromStructureInitListExpr(
    InitListExpr *ILE, FunctionDecl *scopeFD,
    Ptr2InfoType &_Need2AnalysisPtrInfo) {
  if (ILE->isSyntacticForm()) {
    InitListExpr *tmpILE = ILE->getSemanticForm();
    if (tmpILE != nullptr) {
      ILE = tmpILE;
    }
  }
  QualType Ty = ILE->getType();
  CGNode::CallType Ct;
  RecordDecl *rd = nullptr;
  // if (rd->isAnonymousStructOrUnion()) {
  //   return _nullcgnode;
  // }
  if (Ty->isStructureType()) {
    Ct = CGNode::CallType::StructMemberFunPtrCall;
    rd = Ty->getAsStructureType()->getDecl();
  } else if (Ty->isUnionType()) {
    Ct = CGNode::CallType::UnionMemberFunPtrCall;
    rd = Ty->getAsUnionType()->getDecl();
  } else {
    return;
  }

  if (!_CfgMgr.isNeedToAnalysis(rd)) {
    return;
  }
  RecordDecl::field_iterator fdit = rd->field_begin(), fded = rd->field_end();
  // union has only one init
  for (unsigned int i = 0; i < ILE->getNumInits() && fdit != fded;
       i++, fdit++) {
    Expr *IE = ILE->getInit(i);
    if (isa<ImplicitValueInitExpr>(IE)) {
      continue;
    }
    std::pair<CGNode, CGNode> cgnodes = FromExpr(IE, scopeFD, true);
    CGNode ptee1 = cgnodes.first;
    CGNode ptee2 = cgnodes.second;
    if (ptee1 || ptee2) {
      /* Note: 1 */
      CGNode pter = CGNode(Ct, rd->getName().str(), fdit->getName().str(),
                           fdit->getLocation().printToString(_SM));
      if (ptee1) {
        _Need2AnalysisPtrInfo[pter].insert(ptee1);
      }
      if (ptee2) {
        _Need2AnalysisPtrInfo[pter].insert(ptee2);
      }
    }
  }
}

void FunPtrExtractor::FromVDNotStructure(VarDecl *VD, FunctionDecl *scopeFD,
                                         Ptr2InfoType &_Need2AnalysisPtrInfo) {
  CGNode pter;
  std::pair<CGNode, CGNode> cgnodes;
  QualType Ty = VD->getType();
  /*
   * 1. Global Var: GlobalVarFunPtrCall(scopeFD == nullptr)
   * 2. Local Var: LocalVarFunPtrCall(scopeFD != nullptr)
   */
  if (Ty->isFunctionPointerType()) {
    pter = EmitCGNodeFromVD(VD, scopeFD);
    if (Expr *E = VD->getInit()) {
      // VD may init with nullptr,
      cgnodes = FromExpr(E, scopeFD, false);
      CGNode ptee1 = cgnodes.first;
      CGNode ptee2 = cgnodes.second;
      if (ptee1 || ptee2) {
        if (ptee1) {
          _Need2AnalysisPtrInfo[pter].insert(ptee1);
        }
        if (ptee2) {
          _Need2AnalysisPtrInfo[pter].insert(ptee2);
        }
      }
    }
    return;
  }
  /*
   * 3. Array Var: ArrayFunPtrCall(InitListExpr)
   */
  if (!(Ty->isArrayType())) {
    return;
  }
  if (!(_ASTCtx.getAsArrayType(Ty)
            ->getElementType()
            ->isFunctionPointerType())) {
    return;
  }
  if (!(VD->hasInit())) {
    return;
  }
  // pter = CGNode(CGNode::CallType::ArrayFunPtrCall, VD->getName().str(),
  //               VD->getLocation().printToString(_SM));
  pter = EmitCGNodeFromVD(VD, scopeFD);
  Expr *E = VD->getInit()->IgnoreParenCasts();
  if (InitListExpr *ILE = dyn_cast<InitListExpr>(E)) {
    for (unsigned int i = 0; i < ILE->getNumInits(); i++) {
      Expr *IE = ILE->getInit(i);
      cgnodes = FromExpr(IE, scopeFD, false);
      CGNode ptee1 = cgnodes.first;
      CGNode ptee2 = cgnodes.second;
      if (ptee1 || ptee2) {
        if (ptee1) {
          _Need2AnalysisPtrInfo[pter].insert(ptee1);
        }
        if (ptee2) {
          _Need2AnalysisPtrInfo[pter].insert(ptee2);
        }
      }
    }
  } else {
    llvm::errs() << "[May Err] VD isArrayType and hasInit, but Not have a "
                    "InitListExpr\n";
    E->getExprLoc().dump(_SM);
  }
  return;
}
}; // namespace lksast

/* Core */
namespace lksast {
/**********************************
 * StmtLhsRhsAnalyzer Methods
 **********************************/
#if StmtLhsRhsAnalyzer_METHODS

void StmtLhsRhsAnalyzer::ResetStmt(bool lhs) {
  // FIXME? not need to reset hasAsm flag
  isLhs = lhs;
  _AccType = isLhs ? ResourceAccessNode::AccessType::Write
                   : ResourceAccessNode::AccessType::Read;
}

void StmtLhsRhsAnalyzer::AnalysisResourceInVD(VarDecl *VD) {
  QualType Ty = VD->getType();
  if (auto convertype = Ty->getPointeeOrArrayElementType()) {
    Ty = convertype->getCanonicalTypeInternal();
  }
  if (Ty->isStructureType()) {
    if (VD->hasInit()) {
      ResourceAccessNode tmp(ResourceAccessNode::ResourceType::Structure,
                             ResourceAccessNode::AccessType::Write,
                             VD->getName().str());
      _Result._Resources.insert(tmp);
      return;
    }
  }
  /*
    Note:
    It does not need to check if VD hasGlobalStorage,
    because we not need to think about StaticLocal Variable,
    and other GlobalStorage VD init does not belong to any functions.
    if (VD->hasGlobalStorage() && !VD->isStaticLocal())
   */
}

void StmtLhsRhsAnalyzer::AnalysisPtrInfoInVD(VarDecl *VD) {
  _FptrExtractor.FromVDNotStructure(VD, _ScopeFD, _Need2AnalysisPtrInfo);
}

void StmtLhsRhsAnalyzer::VisitInitListExpr(InitListExpr *ILE) {
  _FptrExtractor.FromStructureInitListExpr(ILE, _ScopeFD,
                                           _Need2AnalysisPtrInfo);
}

void StmtLhsRhsAnalyzer::VisitDeclStmt(DeclStmt *DS) {
  for (Decl *DI : DS->decls()) {
    if (VarDecl *VD = dyn_cast<VarDecl>(DI)) {
      AnalysisResourceInVD(VD);
      AnalysisPtrInfoInVD(VD);
    }
  }
}

void StmtLhsRhsAnalyzer::VisitDeclRefExpr(DeclRefExpr *DRE) {
  ValueDecl *valuedecl = DRE->getDecl();
  if (const VarDecl *VD = dyn_cast<VarDecl>(valuedecl)) {
    QualType Ty = VD->getType();
    if (auto convertype = Ty->getPointeeOrArrayElementType()) {
      Ty = convertype->getCanonicalTypeInternal();
    }
    if (Ty->isStructureType()) {
      if (!_CfgMgr.isNeedToAnalysis(Ty->getAsRecordDecl())) {
        return;
      }
      if (VD->hasInit()) {
        ResourceAccessNode tmp(ResourceAccessNode::ResourceType::Structure,
                               _AccType, VD->getName().str());
        _Result._Resources.insert(tmp);
        return;
      }
    }
    if (VD->hasGlobalStorage() && !VD->isStaticLocal()) {
      if (VD->hasInit()) {
        ResourceAccessNode tmp(ResourceAccessNode::ResourceType::GlobalVal,
                               _AccType, VD->getName().str());
        _Result._Resources.insert(tmp);
        return;
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

void StmtLhsRhsAnalyzer::VisitMemberExpr(MemberExpr *ME) {
  // FIXME: should be deduplicated the funcptr field, or think about:
  // struct fs->struct fsop->...(all of fields in fsop is funcptr)
  ValueDecl *memdecl = ME->getMemberDecl();
  if (FieldDecl *fd = dyn_cast<FieldDecl>(memdecl)) {
    RecordDecl *rd = fd->getParent();
    if (!rd->isStruct()) { // maybe a union
      return;
    }
    if (!_CfgMgr.isNeedToAnalysis(rd)) {
      return;
    }
    ResourceAccessNode tmp(rd->getName().str(), fd->getName().str(), _AccType);
    _Result._Resources.insert(tmp);
  } else {
    llvm::errs() << "[Unknown] ME->getMemberDecl() is not a FieldDecl?\n";
    memdecl->getLocation().dump(_SM);
  }
}

void StmtLhsRhsAnalyzer::AnalysisCallExprArg(Expr *E, CGNode &pointer) {
  E = E->IgnoreParenCasts(); // switch-case will depend this
  std::pair<CGNode, CGNode> cgnodes =
      _FptrExtractor.FromExpr(E, _ScopeFD, true);
  CGNode ptee1 = cgnodes.first;
  CGNode ptee2 = cgnodes.second;
  if (ptee1 || ptee2) {
    if (ptee1) {
      _Need2AnalysisPtrInfo[pointer].insert(ptee1);
    }
    if (ptee2) {
      _Need2AnalysisPtrInfo[pointer].insert(ptee2);
    }
  } else {
    // follow code just for debugging
    switch (E->getStmtClass()) {
    case Stmt::UnaryExprOrTypeTraitExprClass:  // sizeof
    case Stmt::BinaryOperatorClass:            // a+b
    case Stmt::UnaryOperatorClass:             // !b
    case Stmt::CompoundAssignOperatorClass:    // a+=1
    case Stmt::IntegerLiteralClass:            // 1
    case Stmt::CharacterLiteralClass:          // 'a'
    case Stmt::StringLiteralClass:             // "str"
    case Stmt::CompoundLiteralExprClass:       // [c99] struct, (a_t){1}
    case Stmt::ArraySubscriptExprClass:        // a[1]
    case Stmt::CallExprClass:                  //
    case Stmt::MemberExprClass:                // task->pid
    case Stmt::ConditionalOperatorClass:       // a ? 1 : 2
    case Stmt::BinaryConditionalOperatorClass: // [gun c ext] x ?: y
    case Stmt::PredefinedExprClass:            // [c99] __func__
    case Stmt::OffsetOfExprClass:              // [c99] offsetof(record, member)
    case Stmt::VAArgExprClass:                 // __va_list_tag
    case Stmt::StmtExprClass:                  // too complex...
      // llvm::errs() << "[CallExpr Arg] StmtClassName: [ "
      //              << E->getStmtClassName() << " ]\n";
      // E->getExprLoc().dump(_SM);
      break;
    case Stmt::DeclRefExprClass:
      if (DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(E)) {
        if (ValueDecl *valuedecl = DRE->getDecl()) {
          switch (valuedecl->getKind()) {
          case Decl::Var:
          case Decl::ParmVar:
          case Decl::EnumConstant:
            break;
          default:
            llvm::errs()
                << "[Unknown] CallExprArg->getDecl()->getDeclKindName(): "
                << valuedecl->getDeclKindName() << "\n";
            E->getExprLoc().dump(_SM);
            E->dump();
            exit(1);
            break;
          }
        }
      }
      break;
    default:
      llvm::errs() << "[Unknown] CallExpr Arg StmtClass\n";
      E->getExprLoc().dump(_SM);
      E->dump();
      exit(1);
      break;
    }
  }
}

void StmtLhsRhsAnalyzer::VisitCallExpr(CallExpr *CE) {
  if (FunctionDecl *FD = CE->getDirectCallee()) {
    // if (calleeD->getKind() == Decl::Function) {
    string srcloc = FD->getLocation().printToString(_SM);
    string funname = FD->getName().str();
    CGNode tmp(CGNode::CallType::DirectCall, funname, srcloc);
    _Result._Callees.insert(tmp);
    // analysis args
    if (_CfgMgr.ignoredFunc_set.find(funname) !=
        _CfgMgr.ignoredFunc_set.end()) {
      return;
    }
    unsigned int numOfArgs = CE->getNumArgs();
    for (unsigned int i = 0; i < numOfArgs; i++) {
      CGNode tmppointer(FD->getName().str(), i, srcloc); // ParmFunPtrCall
      AnalysisCallExprArg(CE->getArg(i), tmppointer);
    }
  } else { // not DirectCallee
    /* Note:
     * May comes from Parm and it is also NOT a FunctionPointerType,
     * because C lang can cast a pointer type,
     * normally, in this situation, parm maybe a (void*)
     * getCallee() =>
     * |-ParenExpr
     * | `-CStyleCastExpr
     * |   `-ImplicitCastExpr
     * |     `-DeclRefExpr => getDecl()==ParmVar
     */
    CGNode tmp =
        _FptrExtractor.FromExpr(CE->getCallee(), _ScopeFD, false).first;
    if (tmp) {
      _Result._Callees.insert(tmp);
    } else {
      llvm::errs()
          << "[Err][CallExpr] can not extract func ptr from CallExpr\n";
      CE->getExprLoc().dump(_SM);
      CE->dump();
    }
  } // is DirectCallee?
}

#define ASM_DEBUG 0

void StmtLhsRhsAnalyzer::Visit(Stmt *S) {
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

void StmtLhsRhsAnalyzer::VisitAsmStmt(AsmStmt *AS) {
  if (!hasAsm) {
    hasAsm = true;
#if ASM_DEBUG
    _CfgMgr.ignoredFunc_set.insert(_ScopeFD->getName().str());
#endif
    llvm::errs() << "[!] Warning, function body [ " << _Result.funcname
                 << " ] has ASM Code!\n";
    llvm::errs() << "  `-[!] " << AS->getAsmLoc().printToString(_SM) << "\n";
  }
}

#endif /* StmtLhsRhsAnalyzer_METHODS */

/**********************************
 * FunctionAnalyzer Methods
 **********************************/
#if FunctionAnalyzer_METHODS

// Note: just logging
void FunctionAnalyzer::AnalysisParms() {
  unsigned int numOfParams = _FD->getNumParams();
  for (unsigned int i = 0; i < numOfParams; i++) {
    ParmVarDecl *PVD = _FD->getParamDecl(i);
    if (PVD->getType()->isFunctionPointerType()) {
      llvm::errs() << "[PVD: '" << _FD->getName() << " " << i
                   << "'] isFunctionPointerType: "
                   << PVD->getLocation().printToString(_SM) << "\n";
    }
  }
}

void FunctionAnalyzer::AnalysisPoint2InfoWithBOAssign(Expr *lhs, Expr *rhs) {
  /* Note:
   * eg. struct fsop1->open = struct fsop2->open.
   * will make analysis in a dead loop?
   * analysisPtrInfo() fixed-point algorithm will handle this situation.
   */
  bool checkAgain = true;
  CGNode pter;
  std::pair<CGNode, CGNode> cgnodes = _FptrExtractor.FromExpr(rhs, _FD, true);
  CGNode ptee1 = cgnodes.first;
  CGNode ptee2 = cgnodes.second;
  if (ptee1 || ptee2) {
    pter = _FptrExtractor.FromExpr(lhs, _FD, false).first;
    if (pter) {
      if (ptee1) {
        _Need2AnalysisPtrInfo[pter].insert(ptee1);
      }
      if (ptee2) {
        _Need2AnalysisPtrInfo[pter].insert(ptee2);
      }
      checkAgain = false;
    } else {
      llvm::errs() << "[Err] rhs is ptr, but lhs not\n";
      lhs->getExprLoc().dump(_SM);
      lhs->dump();
    }
  }
  if (checkAgain) {
    pter = _FptrExtractor.FromExpr(lhs, _FD, true).first;
    if (pter) {
      cgnodes = _FptrExtractor.FromExpr(rhs, _FD, false);
      ptee1 = cgnodes.first;
      ptee2 = cgnodes.second;
      if (ptee1 || ptee2) {
        if (ptee1) {
          _Need2AnalysisPtrInfo[pter].insert(ptee1);
        }
        if (ptee2) {
          _Need2AnalysisPtrInfo[pter].insert(ptee2);
        }
      } else {
        llvm::errs() << "[Err] lhs is ptr, but rhs not\n";
        rhs->getExprLoc().dump(_SM);
        rhs->dump();
      }
    }
  }
}

void FunctionAnalyzer::AnalysisReadStmt(Stmt *LHSorSR) {
  _StmtAnalyzer.ResetStmt(false);
  _StmtAnalyzer.Visit(LHSorSR);
}

void FunctionAnalyzer::AnalysisWriteStmt(Stmt *RHSorSW) {
  _StmtAnalyzer.ResetStmt(true);
  _StmtAnalyzer.Visit(RHSorSW);
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
    for (CFGBlock::const_iterator bi = block->begin(), be = block->end();
         bi != be; ++bi) {
      if (llvm::Optional<CFGStmt> CS = bi->getAs<CFGStmt>()) {
        const Stmt *stmt = CS->getStmt();
        // if (const AsmStmt *AS = dyn_cast<AsmStmt>(stmt)) {
        //   break;
        // } else
        if (const BinaryOperator *BO = dyn_cast<BinaryOperator>(stmt)) {
          Expr *lhs = BO->getLHS();
          Expr *rhs = BO->getRHS();
          if (BO->getOpcode() == BO_Assign) {
            /* Point to info
             * if rhs is FD, then analysis lhs,
             * think about if lhs is memberexpr/array/parm,
             * and do not handle other situation.
             */
            AnalysisPoint2InfoWithBOAssign(lhs, rhs);
          }
          if (BO->isAssignmentOp()) {
            /* Resource access
             * DeclRefExpr in lhs(maybe nest) is write, other is read,
             */
            AnalysisWriteStmt(lhs);
            AnalysisReadStmt(rhs);
          } else {
            AnalysisReadStmt(const_cast<Stmt *>(stmt));
          }
        } else if (const UnaryOperator *UO = dyn_cast<UnaryOperator>(stmt)) {
          /* Here we assume that:
           * All UnOp will write accsee Opcode, not only
           * isIncrementDecrementOp()
           */
          AnalysisWriteStmt(UO->getSubExpr());
        } else {
          /*
          llvm::errs() << "Stmt is: [ " << stmt->getStmtClassName()
                       << " ] In " << stmt->getBeginLoc().printToString(_SM)
                       << "\n";
          */
          AnalysisReadStmt(const_cast<Stmt *>(stmt));
        } // if stmt is a BinOp or UnOp or Other?
      }
    } // for stmt in bb
  }   // for bb in cfg
}

void FunctionAnalyzer::Analysis() {
  if (_FD->hasBody()) {
    std::string funcname = _FD->getName().str();
    _CfgMgr.hasAnalysisFunc_set.insert(funcname);
    AnalysisParms();
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
      FunctionAnalyzer funcAnalyzer(FD, _CfgMgr, _Need2AnalysisPtrInfo,
                                    _FptrExtractor);
      funcAnalyzer.Analysis();
      FunctionResult &curFuncRes = funcAnalyzer.getFunctionResult();
      TUResult.insert(curFuncRes);
    }
  }
  // return RecursiveASTVisitor<TUAnalyzer>::TraverseFunctionDecl(FD);
  return true;
}

bool TUAnalyzer::VisitVarDecl(VarDecl *VD) {
  if (VD->getKind() == Decl::Var) {
    _FptrExtractor.FromVDNotStructure(VD, nullptr, _Need2AnalysisPtrInfo);
  }
  return true;
}

bool TUAnalyzer::VisitInitListExpr(InitListExpr *ILE) {
  _FptrExtractor.FromStructureInitListExpr(ILE, nullptr, _Need2AnalysisPtrInfo);
  return true;
}

#endif /* TUAnalyzer_METHODS */

}; // namespace lksast
