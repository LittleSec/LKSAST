#ifndef LKSAST_AST_FILE_LOADER_H
#define LKSAST_AST_FILE_LOADER_H

#include <memory>
#include <string>
#include <vector>

#include <clang/AST/Decl.h>
#include <clang/Frontend/ASTUnit.h>

namespace lksast {

std::vector<std::string> initialize(const std::string &astListxt);

std::unique_ptr<clang::ASTUnit> loadFromASTFile(const std::string &astpath);

} // namespace lksast

#endif
