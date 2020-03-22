#pragma once
#include "KaleidoscopeJIT.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>

using namespace llvm;
using namespace llvm::orc;


enum Token
{
	tok_eof = -1,

	// commands
	tok_def = -2,
	tok_extern = -3,

	// primary
	tok_identifier = -4,
	tok_number = -5,

	// control
	tok_if = -6,
	tok_then = -7,
	tok_else = -8,
	tok_for = -9,
	tok_in = -10,

	// operators
	tok_binary = -11,
	tok_unary = -12,

	//array
	tok_array = -13,
	tok_new = -14,
	tok_arrayCall = -15,

	//var definition
	tok_var = -16,

	//struct
	tok_struct = -17,

    //return
    tok_double = -18,
    tok_void = -19,
    tok_return = -20,
	// error
	lexerE = -21,
};
enum ASTType{
	variable = -1,
	callArray = -2,
	number = -3,
	call = -4,

	unary = -5,
	binary = -6,
	
	ifT = -7,
	forT = -8,
	var = -9,
	array = -10,
	function = -11,
	proto = -12,
	body = -13,

};
enum returnType{
    doubleR = 0,
    voidR = 1,
};


extern std::string fileStr;
extern int CurTok;

extern int fileIndex;
extern std::string IdentifierStr; // Filled in if tok_identifier
extern double NumVal;			  // Filled in if tok_number

int getNextToken();

void printwq(const char *Str);
void printint(const int i);

