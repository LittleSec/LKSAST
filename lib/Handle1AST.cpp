#include <iomanip>

#include "Common.h"
#include "ConfigManager.h"
#include "Handle1AST.h"
#include "nlohmann/json.hpp"

#include <clang/AST/ASTContext.h>
#include <clang/Analysis/CFG.h>
#include <llvm/Support/raw_ostream.h>

using namespace clang;
using json = nlohmann::json;

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
    name = n;
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
  case ArrayFunPtrCall:
    return "ArrayFunPtrCall";
  case ParmFunPtrCall:
    return "ParmFunPtrCall";
  case GlobalVarFunPtrCall:
    return "GlobalVarFunPtrCall";
  case LocalVarFunPtrCall:
    return "LocalVarFunPtrCall";
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

CGNode GetPointeeWithILE(Expr *initExpr, FunctionDecl *scopeFD,
                         SourceManager &_SM) {
  initExpr = initExpr->IgnoreParenCasts();
  CGNode nullcgnode;
  if (DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(initExpr)) {
    if (ValueDecl *ValueD = DRE->getDecl()) {
      if (VarDecl *VD = dyn_cast<VarDecl>(ValueD)) {
        if (VD->getType()->isFunctionPointerType()) {
          if (ParmVarDecl *PVD = dyn_cast<ParmVarDecl>(ValueD)) {
            assert(scopeFD != nullptr);
            CGNode tmp(scopeFD->getName().str(), PVD->getFunctionScopeIndex(),
                       PVD->getLocation().printToString(_SM));
            return tmp;
          } else {
            llvm::errs() << "[Unknown] initExpr is a FunctionPointerType "
                            "but not from ParmVar\n";
            initExpr->getExprLoc().dump(_SM);
          } // if VD is ParmVarDecl
        } else {
          /* Note: 2 */
          ;
        } // end analysis VarDecl
      } else if (FunctionDecl *FD = dyn_cast<FunctionDecl>(ValueD)) {
        CGNode tmp(CGNode::CallType::DirectCall, FD->getName().str(),
                   FD->getLocation().printToString(_SM));
        return tmp;
      } else {
        /* Note: 3 */
        // initExpr is not a function
      } // cast ValueDecl into detail Decl
    } else {
      llvm::errs() << "[nullptr] initExpr DRE->getDecl()\n";
      DRE->getExprLoc().dump(_SM);
    } // if (VarDecl *VD = dyn_cast<VarDecl>(ValueD))
  }
  return nullcgnode;
}

void AnalysisPtrInfoWithInitListExpr(InitListExpr *ILE, FunctionDecl *scopeFD,
                                     Ptr2InfoType &_Need2AnalysisPtrInfo,
                                     ConfigManager &_CfgMgr) {
  if (ILE->isSyntacticForm()) {
    InitListExpr *tmpILE = ILE->getSemanticForm();
    if (tmpILE != nullptr) {
      ILE = tmpILE;
    }
  }
  QualType Ty = ILE->getType();
  if (Ty->isStructureType()) {
    RecordDecl *rd = Ty->getAsStructureType()->getDecl();
    if (!_CfgMgr.isNeedToAnalysis(rd)) {
      return;
    }
    SourceManager &sm = rd->getASTContext().getSourceManager();
    RecordDecl::field_iterator fdit = rd->field_begin(), fded = rd->field_end();
    for (unsigned int i = 0; i < ILE->getNumInits() && fdit != fded;
         i++, fdit++) {
      Expr *IE = ILE->getInit(i);
      if (!isa<ImplicitValueInitExpr>(IE)) {
        CGNode ptee = GetPointeeWithILE(IE, scopeFD, sm);
        if (ptee) {
          /* Note: 1 */
          // assert(fdit->getType()->isFunctionPointerType());
          CGNode pter = CGNode(CGNode::CallType::StructMemberFunPtrCall,
                               rd->getName().str(), fdit->getName().str(),
                               fdit->getLocation().printToString(sm));
          _Need2AnalysisPtrInfo[pter].insert(ptee);
        }
      }
    }
  }
}

ValueDecl *GetSimpleArrayDecl(Expr *E, SourceManager &_SM) {
  if (ArraySubscriptExpr *ASE = dyn_cast<ArraySubscriptExpr>(E)) {
    if (Expr *arrBaseExpr = ASE->getBase()->IgnoreParenCasts()) {
      if (DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(arrBaseExpr)) {
        // DRE->getDecl()->dump();
        return DRE->getDecl();
      } else {
        llvm::errs() << "[Not Support] Array base is NOT a DeclRefExpr\n";
        arrBaseExpr->getExprLoc().dump(_SM);
      }
    } else {
      llvm::errs() << "[nullptr] ASE->getBase()->IgnoreParenCasts()\n";
      ASE->getExprLoc().dump(_SM);
    }
  } else {
    llvm::errs() << "[nullptr] Expr is NOT ArraySubscriptExpr\n";
    E->getExprLoc().dump(_SM);
  }
  return nullptr;
};

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

// TODO: check again
void StmtLhsRhsAnalyzer::AnalysisPtrInfoInVD(VarDecl *VD) {
  if (Expr *E = VD->getInit()) {
    if (InitListExpr *ILE = dyn_cast<InitListExpr>(E->IgnoreParenCasts())) {
      AnalysisPtrInfoWithInitListExpr(ILE, _ScopeFD, _Need2AnalysisPtrInfo,
                                      _CfgMgr);
    }
  }
}

void StmtLhsRhsAnalyzer::VisitDeclStmt(DeclStmt *DS) {
  for (Decl *DI : DS->decls()) {
    if (VarDecl *VD = dyn_cast<VarDecl>(DI)) {
      AnalysisResourceInVD(VD);
      AnalysisPtrInfoInVD(VD);
    }
  }
}

// TODO: support LocalVarFunPtrCall
void StmtLhsRhsAnalyzer::VisitDeclRefExpr(DeclRefExpr *DRE) {
  ValueDecl *valuedecl = DRE->getDecl();
  if (const VarDecl *VD = dyn_cast<VarDecl>(valuedecl)) {
    QualType Ty = VD->getType();
    if (auto convertype = Ty->getPointeeOrArrayElementType()) {
      Ty = convertype->getCanonicalTypeInternal();
    }
    if (Ty->isStructureType()) {
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
  if (Decl::Field == memdecl->getKind()) {
    FieldDecl *fd = dyn_cast<FieldDecl>(memdecl);
    assert(fd != nullptr);
    RecordDecl *rd = fd->getParent();
    if (!rd->isStruct()) { // maybe a union
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
  if (E == nullptr) {
    return;
  }
  E = E->IgnoreParenCasts();
  // eg. call func1(&fun) ==> `&` is an UO
  if (UnaryOperator *UO = dyn_cast<UnaryOperator>(E)) {
    E = UO->getSubExpr();
  }
  // -i ==> i ==> shit...
  E = E->IgnoreParenCasts();
  if (DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(E)) {
    if (ValueDecl *ValueD = DRE->getDecl()) {
      if (VarDecl *VD = dyn_cast<VarDecl>(ValueD)) {
        if (VD->getType()->isFunctionPointerType()) {
          if (ParmVarDecl *PVD = dyn_cast<ParmVarDecl>(ValueD)) {
            CGNode pointee(_Result.funcname, PVD->getFunctionScopeIndex(),
                           PVD->getLocation().printToString(
                               _SM)); // FIXME: record which loc?
            _Need2AnalysisPtrInfo[pointer].insert(pointee);
          } else {
            llvm::errs() << "[Not Support] CallExpr Arg is a "
                            "FunctionPointerType but not from ParmVar\n";
            E->getExprLoc().dump(_SM);
          }
        } else {
          /* Note: 2 */
          ;
        }
      } else if (FunctionDecl *FD = dyn_cast<FunctionDecl>(ValueD)) {
        CGNode pointee(CGNode::CallType::DirectCall, FD->getName().str(),
                       FD->getLocation().printToString(_SM));
        _Need2AnalysisPtrInfo[pointer].insert(pointee);
      } else {
        /* Note: 3 */
        switch (ValueD->getKind()) {
        case Decl::Function:
        case Decl::EnumConstant:
          break;
        default:
          llvm::errs()
              << "[Unknown] CallExprArg->getDecl()->getDeclKindName(): "
              << ValueD->getDeclKindName() << "\n";
          E->getExprLoc().dump(_SM);
          break;
        }
      } // cast ValueDecl into detail Decl
    } else {
      llvm::errs() << "[nullptr] DRE->getDecl()\n";
      DRE->getExprLoc().dump(_SM);
      DRE->dump();
    } // if (VarDecl *VD = dyn_cast<VarDecl>(ValueD))
  } else {
    switch (E->getStmtClass()) {
    case Stmt::UnaryExprOrTypeTraitExprClass:  // sizeof
    case Stmt::BinaryOperatorClass:            // a+b
    case Stmt::UnaryOperatorClass:             // !b
    case Stmt::CompoundAssignOperatorClass:    // a+=1
    case Stmt::IntegerLiteralClass:            // 1
    case Stmt::CharacterLiteralClass:          // 'a'
    case Stmt::StringLiteralClass:             // "str"
    case Stmt::CompoundLiteralExprClass:       // [c99] struct, (a_t){1}
    case Stmt::ArraySubscriptExprClass:        // TODO: a[1]
    case Stmt::CallExprClass:                  //
    case Stmt::MemberExprClass:                // TODO: task->pid
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
    default:
      llvm::errs() << "[Unknown] CallExpr Arg StmtClass\n";
      E->getExprLoc().dump(_SM);
      E->dump();
      exit(1);
      break;
    }
  }
}

/*
  一 如果CE里实参里有函数指针，应当分析并记录其指向关系
  ("calleename 0 ptr2funcname")，其中：
    1 ptr2funcname要么是一个确定的函数名或者是下面二中所述情况之一
    2 例如ptr2funcname是函数来自参数则("calleename 0 callername 0")
    3 TODO: calleename只能是直接调用，暂不支持函数指针
  二 直接调用则记录到CG中，
  非直接调用都按照函数指针处理，判断fp的来源共三种情况
    1 是fp是结构体某个域，则记录该结构体的类型和域名信息("struct fop.read")
    2 是fp是fp数组里某个值，则记录该数组名("array funptr")
    3 是直接fp()，这种情况如果是从parm来则记录当前_FD名和parmidx("_FDname
    0")
  说明:

  其他情况均不考虑，如：
*/
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
    Decl *calleeD = CE->getCalleeDecl();
    if (calleeD == nullptr) { // fun ptr arr
      // eg. call mycal[1](a, b); which mycal is fun ptr array:
      // int (*mycal[])(int, int) = {mysub, mymod, mydivi};
      if (Expr *E = CE->getCallee()->IgnoreParenCasts()) {
        if (ValueDecl *valuedecl = GetSimpleArrayDecl(E, _SM)) {
          if (valuedecl->getKind() == Decl::Var) {
            QualType Ty = valuedecl->getType();
            const clang::Type *EleTy = Ty->getPointeeOrArrayElementType();
            if (Ty->isFunctionPointerType() ||
                (EleTy != nullptr && EleTy->isFunctionPointerType())) {
              CGNode tmp(CGNode::CallType::ArrayFunPtrCall,
                         valuedecl->getName().str(),
                         valuedecl->getLocation().printToString(_SM));
              _Result._Callees.insert(tmp);
            } else {
              llvm::errs() << "[Err] Callee not a FunctionPointerType: "
                           << E->getExprLoc().printToString(_SM) << "\n";
              E->dump();
            }
          } else {
            llvm::errs() << "[Unknown] GetSimpleArrayDecl not a VD: [ "
                         << valuedecl->getDeclKindName() << " ]\n";
          }
        } else {
          // TODO
          /* Note:
           * May comes from Parm and it is also NOT a FunctionPointerType,
           * because C lang can cast a pointer type,
           * normally, in this situation, parm maybe a (void*)
           * getCallee() =>
           * |-ParenExpr
           * | `-CStyleCastExpr
           * |   `-ImplicitCastExpr
           * |     `-DeclRefExpr => getDecl()==ParmVar
           * */
          llvm::errs() << "[!] Unknown getCallee\n";
        }
      } else {
        llvm::errs() << "[!] getCallee is nullptr\n";
      }
    } else { // has CalleeDecl
      if (ParmVarDecl *PVD = dyn_cast<ParmVarDecl>(calleeD)) {
        CGNode tmp(
            _Result.funcname, PVD->getFunctionScopeIndex(),
            PVD->getLocation().printToString(_SM)); // FIXME: record which loc?
        _Result._Callees.insert(tmp);
      } else if (FieldDecl *FieldD = dyn_cast<FieldDecl>(calleeD)) {
        RecordDecl *RD = FieldD->getParent();
        if (RD->isStruct()) {
          string srcloc = FieldD->getLocation().printToString(_SM);
          CGNode tmp(CGNode::CallType::StructMemberFunPtrCall,
                     RD->getName().str(), FieldD->getName().str(), srcloc);
          _Result._Callees.insert(tmp);
        } else {
          llvm::errs() << "[!] FieldDecl is not a Structure\n";
        }
      } else if (VarDecl *VD = dyn_cast<VarDecl>(calleeD)) {
        llvm::errs() << "[!] CalleeDecl is unknown VarDecl!\n";
        VD->dump();
      } else {
        llvm::errs() << "[!] CalleeDecl is unknown Decl!\n";
        calleeD->dump();
      }
    } // CE->getCalleeDecl() == nullptr?
  }   // is DirectCallee?
}

void StmtLhsRhsAnalyzer::Visit(Stmt *S) {
  if (hasAsm) {
    return;
  }
  StmtVisitor<StmtLhsRhsAnalyzer>::Visit(S); // visit father first
  for (auto *Child : S->children()) {
    if (Child) {
      Visit(Child);
    }
  }
  // StmtVisitor<StmtLhsRhsAnalyzer>::Visit(S); // visit children first
}

void StmtLhsRhsAnalyzer::VisitAsmStmt(AsmStmt *AS) {
  hasAsm = true;
  _CfgMgr.ignoredFunc_set.insert(_ScopeFD->getName().str());
  llvm::errs() << "[!] Warning, function body [ " << _Result.funcname
               << " ] has ASM Code!\n";
  llvm::errs() << "  `-[!] " << AS->getAsmLoc().printToString(_SM) << "\n";
}

#endif /* StmtLhsRhsAnalyzer_METHODS */

/**********************************
 * FunctionAnalyzer Methods
 **********************************/
#if FunctionAnalyzer_METHODS

// TODO
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

/*
 * 若rhs是FD，则分析lhs是否为member、array，
 *     是则加入指向信息lhs.insert(FD)，不是则忽略
 * 若rhs是PVD，则分析PVD是不是函数指针，
 *     是则加入指向信息lhs.insert("_FDname 0")，不是则忽略
 * // TODO
 * 其实还有一种可以处理的情况，rhs是个普通的指针被强转成函数指针后，
 * 赋值给lhs，如果lhs是函数指针的情况下，这种情况应该实现识别
 */
CGNode FunctionAnalyzer::getPointee(Expr *rhs) {
  rhs = rhs->IgnoreParenCasts();
  CGNode nullcgnode;
  if (DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(rhs)) {
    if (ValueDecl *ValueD = DRE->getDecl()) {
      if (VarDecl *VD = dyn_cast<VarDecl>(ValueD)) {
        if (VD->getType()->isFunctionPointerType()) {
          if (ParmVarDecl *PVD = dyn_cast<ParmVarDecl>(ValueD)) {
            CGNode tmp(_FD->getName().str(), PVD->getFunctionScopeIndex(),
                       PVD->getLocation().printToString(_SM));
            return tmp;
          } else {
            llvm::errs() << "[Unknown] rhs is a FunctionPointerType "
                            "but not from ParmVar\n";
            rhs->getExprLoc().dump(_SM);
          } // if VD is ParmVarDecl
        } else {
          /* Note: 2 */
          ;
        } // end analysis VarDecl
      } else if (FunctionDecl *FD = dyn_cast<FunctionDecl>(ValueD)) {
        CGNode tmp(CGNode::CallType::DirectCall, FD->getName().str(),
                   FD->getLocation().printToString(_SM));
        return tmp;
      } else {
        /* Note: 3 */
        // rhs is not a function
      } // cast ValueDecl into detail Decl
    } else {
      llvm::errs() << "[nullptr] rhs DRE->getDecl()\n";
      DRE->getExprLoc().dump(_SM);
    } // if (VarDecl *VD = dyn_cast<VarDecl>(ValueD))
  }
  /*
   Note:
   Is it possible rhs is not DeclRefExpr, but it a funcptr too,
   eg. struct fsop1->open = struct fsop2->open.
   We not need to handle this because it do not change any point-to info.
  */
  return nullcgnode;
}

CGNode FunctionAnalyzer::getPointer(Expr *lhs) {
  CGNode nullcgnode;
  lhs = lhs->IgnoreParenCasts();
  if (MemberExpr *ME = dyn_cast<MemberExpr>(lhs)) {
    ValueDecl *memdecl = ME->getMemberDecl();
    if (Decl::Field == memdecl->getKind()) {
      FieldDecl *fd = dyn_cast<FieldDecl>(memdecl);
      assert(fd != nullptr);
      RecordDecl *rd = fd->getParent();
      if (!rd->isStruct()) {
        llvm::errs() << "[Unknown] RecordDecl is not a Structure?\n";
        rd->getLocation().dump(_SM);
        return nullcgnode;
      }
      /* Note: 1 */
      // assert(fd->getType()->isFunctionPointerType());
      CGNode tmp(CGNode::CallType::StructMemberFunPtrCall, rd->getName().str(),
                 fd->getName().str(), fd->getLocation().printToString(_SM));
      return tmp;
    } else {
      llvm::errs() << "[Unknown] ME->getMemberDecl() is not a FieldDecl?\n";
      memdecl->getLocation().dump(_SM);
    }
  } else if (ArraySubscriptExpr *ASE = dyn_cast<ArraySubscriptExpr>(lhs)) {
    if (ValueDecl *valuedecl = GetSimpleArrayDecl(lhs, _SM)) {
      if (valuedecl->getKind() == Decl::Var) {
        QualType Ty = valuedecl->getType();
        const clang::Type *EleTy = Ty->getPointeeOrArrayElementType();
        if (Ty->isFunctionPointerType() ||
            (EleTy != nullptr && EleTy->isFunctionPointerType())) {
          CGNode tmp(CGNode::CallType::ArrayFunPtrCall,
                     valuedecl->getName().str(),
                     valuedecl->getLocation().printToString(_SM));
          return tmp;
        } else {
          llvm::errs() << "[Err] not a array pointer: "
                       << ASE->getExprLoc().printToString(_SM) << "\n";
          ASE->dump();
          valuedecl->dump();
          assert(0);
        }
      } else {
        llvm::errs() << "[Unknown] Array base Decl\n";
        valuedecl->getLocation().dump(_SM);
      }
    } else {
      llvm::errs() << "[Not Support] Array base is NOT simple\n";
      ASE->getExprLoc().dump(_SM);
    }
  } else {
    llvm::errs() << "[Unknown] lhs not a struct member or array\n";
    lhs->getExprLoc().dump(_SM);
  }
  return nullcgnode;
}

void FunctionAnalyzer::AnalysisPoint2InfoWithBOAssign(Expr *lhs, Expr *rhs) {
  CGNode ptee = getPointee(rhs);
  if (ptee) {
    CGNode pter = getPointer(lhs);
    if (pter) {
      _Need2AnalysisPtrInfo[pter].insert(ptee);
    }
  }
  return;
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
        if (const AsmStmt *AS = dyn_cast<AsmStmt>(stmt)) {
          break;
        } else if (const BinaryOperator *BO = dyn_cast<BinaryOperator>(stmt)) {
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
      // TODO: check again
      llvm::errs() << "[+] Analysis Function: " << FD->getName() << "\n";
      FunctionAnalyzer funcAnalyzer(FD, _CfgMgr, _Need2AnalysisPtrInfo);
      funcAnalyzer.Analysis();
      FunctionResult &curFuncRes = funcAnalyzer.getFunctionResult();
      TUResult.insert(curFuncRes);
    }
  }
  return true;
}

// TODO: support GlobalVarFunPtrCall
bool TUAnalyzer::VisitVarDecl(VarDecl *VD) {
  SourceManager &sm = VD->getASTContext().getSourceManager();
  /* handle func ptr array InitListExpr */
  QualType Ty = VD->getType();
  if (Ty->isFunctionPointerType()) {
    if (Expr *E = VD->getInit()) {
      // TODO: cast?
    }
  }
  if (!(Ty->isArrayType())) {
    return true;
  }
  if (!(_ASTCtx.getAsArrayType(Ty)
            ->getElementType()
            ->isFunctionPointerType())) {
    return true;
  }
  if (!(VD->hasInit())) {
    return true;
  }
  CGNode pter = CGNode(CGNode::CallType::ArrayFunPtrCall, VD->getName().str(),
                       VD->getLocation().printToString(sm));
  Expr *E = VD->getInit()->IgnoreParenCasts();
  if (InitListExpr *ILE = dyn_cast<InitListExpr>(E)) {
    for (unsigned int i = 0; i < ILE->getNumInits(); i++) {
      Expr *IE = ILE->getInit(i);
      if (!isa<ImplicitValueInitExpr>(IE)) {
        CGNode ptee = GetPointeeWithILE(IE, nullptr, sm);
        if (ptee) {
          _Need2AnalysisPtrInfo[pter].insert(ptee);
        }
      }
    }
  } else {
    llvm::errs() << "[May Err] VD isArrayType and hasInit, but Not have a "
                    "InitListExpr\n";
    E->getExprLoc().dump(sm);
    return false;
  }
  return true;
}

bool TUAnalyzer::VisitInitListExpr(InitListExpr *ILE) {
  AnalysisPtrInfoWithInitListExpr(ILE, nullptr, _Need2AnalysisPtrInfo, _CfgMgr);
  return true;
}

/*
Need to analysis function pointer:
  |- Array gifconf_list --> [register_gifconf 1, ]
  |- struct pernet_operations.init --> [netdev_init, ]
  `-<EOF>
Function Info:
  |- function name 1:
  |    |- callgraph:
  |    |    |- struct net_device_ops.ndo_do_ioctl
  |    |    |- memcpy
  |    |    |- ...
  |    |    `-<End of callgraph>
  |    |- resource:
  |    |    |- struct net_device.netdev_ops
  |    |    |- ...
  |    |    `-<End of resource>
  |    `-<End of Function: function name 1>
  |- ...
  |- ...
  `-<EOF>
*/
void TUAnalyzer::dump(std::ofstream &of) {
  of << "Function Info:\n";
  for (auto &tures : TUResult) {
    of << "  |- " << tures.funcname << ":\n";
    of << "  |    |- callgraph:\n";
    for (auto &cgn : tures._Callees) {
      of << "  |    |    |- " << cgn.name << "\n";
    }
    of << "  |    |    `-<End of callgraph>\n";
    of << "  |    |- resource:\n";
    for (auto &reso : tures._Resources) {
      of << "  |    |    |- " << reso.name << "\n";
    }
    of << "  |    |    `-<End of resource>\n";
    of << "  |    `-<End of Function: " << tures.funcname << ">\n";
  }
  of << "  `-<EOF>\n";
  of << "\n";
}

void TUAnalyzer::dumpJSON(std::ofstream &of, bool ismin) {
  json topj;
  for (auto &tures : TUResult) {
    json funj;
    if (!ismin) {
      funj["declloc"] = tures.declloc;
    }
    funj["callgraph"] = json::array();
    for (auto &cgn : tures._Callees) {
      json cgnodej;
      cgnodej["CallType"] = cgn.CallType2String();
      cgnodej["name"] = cgn.name;
      if (!ismin) {
        cgnodej["declloc"] = cgn.declloc;
      }
      funj["callgraph"].push_back(cgnodej);
    }
    funj["resource access"] = json::array();
    for (auto &reso : tures._Resources) {
      json resj;
      resj["name"] = reso.name;
      resj["ResourceType"] = reso.ResourceType2String();
      resj["AccessType"] = reso.AccessType2String();
      funj["resource access"].push_back(resj);
    }
    topj[tures.funcname] = funj;
  }
  of << std::setw(2) << topj;
}

#endif /* TUAnalyzer_METHODS */

}; // namespace lksast
