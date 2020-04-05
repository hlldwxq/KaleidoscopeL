#pragma once
#include "AST.h"

std::unique_ptr<PrototypeAST> ParsePrototype(int level);
std::unique_ptr<FunctionAST> ParseDefinition(int level);
std::unique_ptr<FunctionAST> ParseTopLevelExpr(int level);
std::unique_ptr<PrototypeAST> ParseExtern(int level);