#include "head.h"
#include "AST.h"
#include "Parser.h"

std::string fileStr;
std::map<char, int> BinopPrecedence;
int CurTok;
//===----------------------------------------------------------------------===//
// Top-Level parsing and JIT Driver
//===----------------------------------------------------------------------===//

static void InitializeModuleAndPassManager(std::string fileName);
static void HandleDefinition();
static void HandleExtern();
static void HandleTopLevelExpression();
/// top ::= definition | external | expression | ';'
static void HandleTopLoop();
void printwq(const char *Str){
	fprintf(stderr, "%s", Str);
}
void printint(const int i){
	fprintf(stderr, "info: %d", i);
}

//===----------------------------------------------------------------------===//
// Main driver code.
//===----------------------------------------------------------------------===//

int main() {

	//fprintf(stderr,"first");
	std::string fileName = "file.k";
	
	//std::cin >> fileName;
	std::ifstream in(fileName);
    std::ostringstream tmp;
    tmp << in.rdbuf();
    fileStr = tmp.str();

	//fprintf(stderr,"%s \n",(char*)fileStr.data());

    InitializeNativeTarget();
	InitializeNativeTargetAsmPrinter();
	InitializeNativeTargetAsmParser();

	//fprintf(stderr,"third\n");
	// Install standard binary operators.
	// 1 is lowest precedence.
	BinopPrecedence['='] = 2;
	BinopPrecedence['<'] = 10;
	BinopPrecedence['+'] = 20;
	BinopPrecedence['-'] = 20;
	BinopPrecedence['*'] = 40; // highest.

	// Prime the first token.
	// fprintf(stderr, "ready> ");
	getNextToken();

	//TheJIT = std::make_unique<KaleidoscopeJIT>();
	InitializeModuleAndPassManager(fileName);
	// Run the main "interpreter loop" now.
	HandleTopLoop();
	TheModule->dump();
	return 0;
}

static void InitializeModuleAndPassManager(std::string fileName) {

	// Open a new module.
	TheModule = std::make_unique<Module>(fileName, TheContext);
	//TheModule->setDataLayout(TheJIT->getTargetMachine().createDataLayout());

	// Create a new pass manager attached to it.
	TheFPM = std::make_unique<legacy::FunctionPassManager>(TheModule.get());

	// Do simple "peephole" optimizations and bit-twiddling optzns.
	TheFPM->add(createInstructionCombiningPass());
	// Reassociate expressions.
	TheFPM->add(createReassociatePass());
	// Eliminate Common SubExpressions.
	TheFPM->add(createGVNPass());
	// Simplify the control flow graph (deleting unreachable blocks, etc).
	TheFPM->add(createCFGSimplificationPass());

	TheFPM->doInitialization();
}

static void HandleDefinition() {
	if (auto FnAST = ParseDefinition(0))
	{	
		if (auto FnIR = FnAST->codegen())
		{
			//loading...
		}
	}
	else
	{
		// Skip token for error recovery.
		getNextToken();
	}
}

static void HandleExtern() {

	if (auto ProtoAST = ParseExtern(0))
	{
		if (auto *FnIR = ProtoAST->codegen())
		{
			FunctionProtos[ProtoAST->getName()] = std::move(ProtoAST);
		}
	}
	else
	{
		// Skip token for error recovery.
		getNextToken();
	}
}

static void HandleTopLevelExpression() {

	// Evaluate a top-level expression into an anonymous function.
	if (auto FnAST = ParseTopLevelExpr(0))
	{
		if (auto* FnIR = FnAST->codegen())
		{
			// JIT the module containing the anonymous expression, keeping a handle so
			// we can free it later.
			//fprintf(stderr, "top level expression: ");

            // FnIR->print(errs()); //

            // auto H = TheJIT->addModule(std::move(TheModule));
			//InitializeModuleAndPassManager();
			// Search the JIT for the __anon_expr symbol.
			//auto ExprSymbol = TheJIT->findSymbol("__anon_expr");

			//assert(ExprSymbol && "Function not found");

			// Get the symbol's address and cast it to the right type (takes no
			// arguments, returns a double) so we can call it as a native function.
			//double (*FP)() = (double (*)())(intptr_t)cantFail(ExprSymbol.getAddress());
			//fprintf(stderr, "Evaluated to %f\n", FP());

			// Delete the anonymous expression module from the JIT.
			//TheJIT->removeModule(H);
		}
	}
	else
	{
		// Skip token for error recovery.
		getNextToken();
	}
}

/// top ::= definition | external | expression | ';'
static void HandleTopLoop() {

	while(true){
		switch (CurTok)
		{
			case tok_eof:
				return;
			case ';': // ignore top-level semicolons.
				getNextToken();
				break;
			case tok_def:
				HandleDefinition();
				break;
			case tok_extern:
				HandleExtern();
				break;
			default:
				HandleTopLevelExpression();
				break;
		}
	}
}

//===----------------------------------------------------------------------===//
// "Library" functions that can be "extern'd" from user code.
//===----------------------------------------------------------------------===//

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

/// putchard - putchar that takes a double and returns 0.
extern "C" DLLEXPORT double putchard(double X)
{
	fputc((char)X, stderr);
	return 0;
}

/// printd - printf that takes a double prints it as "%f\n", returning 0.
extern "C" DLLEXPORT double printd(double X)
{
	fprintf(stderr, "%f\n", X);
	return 0;
}