#include "head.h"
#include "AST.h"
#include "Parser.h"

std::unique_ptr<Module> TheModule;
std::unique_ptr<legacy::FunctionPassManager> TheFPM;
std::unique_ptr<KaleidoscopeJIT> TheJIT;

int ExprAST::getType(){
	return type;
}

int GetTokPrecedence()
{
	//printwq("precedence\n");
	BinopPrecedence[assignment] = 2;  // =
	BinopPrecedence[lessThen] = 10;   // <
	BinopPrecedence[moreThen] = 10;   // >
	BinopPrecedence[lessEqual] = 10;  // <=
	BinopPrecedence[moreEqual] = 10;  // >=
	BinopPrecedence[notEqual] = 10;  // ==
	BinopPrecedence[equalSign] = 10;  // ==
	BinopPrecedence[plus] = 20;       // +
	BinopPrecedence[minus] = 20;	  // -
	BinopPrecedence[mult] = 40; 	  // *
	BinopPrecedence[division] = 40;   // /

	//if (!isascii(CurTok))
	//	return -1;

	// Make sure it's a declared binop.
	int TokPrec = BinopPrecedence[CurTok];
	//printint(TokPrec);
	if (TokPrec <= 0) 
		return -1;
	return TokPrec;
}

/// Error* - These are little helper functions for error handling.
std::unique_ptr<ExprAST> LogError(const char *Str)
{
	return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char *Str)
{
	LogError(Str);
	return nullptr;
}

std::unique_ptr<FunctionAST> LogErrorF(const char *Str)
{
	LogError(Str);
	return nullptr;
}

Value *LogErrorV(const char *Str)
{
	LogError(Str);
	return nullptr;
}
//===----------------------------------------------------------------------===//
// Code Generation
//===----------------------------------------------------------------------===//

Function *getFunction(std::string Name)
{
	// First, see if the function has already been added to the current module.

    // Function * getFunction (StringRef Name) const
 	// Look up the specified function in the module symbol table
	if (auto *F = TheModule->getFunction(Name))
		return F;

	// If not, check whether we can codegen the declaration from some existing prototype.
	auto FI = FunctionProtos.find(Name); 
	if (FI != FunctionProtos.end())
		return FI->second->codegen();

	// If no existing prototype exists, return null.
	return nullptr;
}

/// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
/// the function.  This is used for mutable variables etc.
static AllocaInst *CreateEntryBlockAlloca(Function *TheFunction, StringRef VarName, int varType)
{
	return Builder.CreateAlloca(Type::getDoubleTy(TheContext),nullptr, VarName);
	/*
	/// loading ....
	switch(varType){
	case tok_double:
		return Builder.CreateAlloca(Type::getDoubleTy(TheContext),nullptr, VarName);
	case tok_i1:
		return 
	}*/
}


Value *NumberExprAST::codegen()
{
//	printwq("codegen number\n");
	return ConstantFP::get(TheContext, APFloat(Val));
}

Value *VariableExprAST::codegen()
{
//	printwq("codegen variable\n");
	// Look this variable up in the function.
	AllocaInst *V = NamedValues[Name];
	if (!V){
		return LogErrorV("Unknown variable name");
	}
//	printwq("codegen variable end\n");
	return Builder.CreateLoad(V, Name.c_str());
}

Value *UnaryExprAST::codegen()
{
//	printwq("codegen unary\n");

	Value *OperandV = Operand->codegen();
	if (!OperandV)
		return nullptr;

	if(Opcode == notSign){
		return Builder.CreateNot(OperandV);
	}else if(Opcode == minus){
		return Builder.CreateFNeg(OperandV);
	}else if(Opcode == plus){
		return OperandV;
	}

	Function *F = getFunction(std::string("unary") + (char)Opcode);
	if (!F)
		return LogErrorV("Unknown unary operator");

//	printwq("codegen unary end\n");
	return Builder.CreateCall(F, OperandV, "unop");
}

Value *BinaryExprAST::codegen()
{

	if (Op == assignment)
	{
		if (LHS->getType() != variable && LHS->getType() != callArray){
            return LogErrorV("expected variable or array element");
        }
        Value* Variable;
		if(LHS->getType() == variable){

			VariableExprAST *LHSE = static_cast<VariableExprAST *>(LHS.get());
			Variable = NamedValues[LHSE->getName()];

		}else{

			CallArrayAST *LHSE = static_cast<CallArrayAST *>(LHS.get());
			Value *arrayPtr = NamedArray[LHSE->getArrayName()];
			if (!arrayPtr){

				arrayPtr = DynamicArray[LHSE->getArrayName()];
				if(!arrayPtr)
					return LogErrorV("undefined array");

				arrayPtr = Builder.CreateLoad(arrayPtr);

			}
			
			Value *index = LHSE->getIndex()->codegen();
			Variable = Builder.CreateGEP(cast<PointerType>(arrayPtr->getType()->getScalarType())->getElementType(),
												arrayPtr, {Builder.CreateFPToUI(index,Type::getInt32Ty(TheContext))});
		}
		
        if (!Variable){
            return LogErrorV("Unknown left value");
        }

        Value* Val = RHS->codegen(); //右值
        if (!Val)
            return nullptr;
		return Builder.CreateStore(Val, Variable);
	}

	Value *L = LHS->codegen();
	Value *R = RHS->codegen();
	if (!L || !R)
		return nullptr;

	switch (Op)
	{
	case plus:
		return Builder.CreateFAdd(L, R, "addtmp");
	case minus:
		return Builder.CreateFSub(L, R, "subtmp");
	case mult:
		return Builder.CreateFMul(L, R, "multmp");
	case division:
		return Builder.CreateFDiv(L, R, "divmp");
	case lessThen:
		return Builder.CreateFCmpULT(L, R, "cmptmp"); //cmp -> compare
	case moreThen:
		return Builder.CreateFCmpUGT(L, R, "cmptmp"); //cmp -> compare
	case lessEqual:
		return  Builder.CreateFCmpULE(L, R, "cmptmp"); //cmp -> compare
	case moreEqual:
		return Builder.CreateFCmpUGE(R, L, "cmptmp"); //cmp -> compare
	case equalSign:
		return Builder.CreateFCmpUEQ(R, L, "cmptmp"); //cmp -> compare
	case notEqual:
		return Builder.CreateFCmpUNE(R, L, "cmptmp"); //cmp -> compare
	default:
		break;
	}

	// If it wasn't a builtin binary operator, it must be a user defined one. Emit a call to it.
	Function *F = getFunction(std::string("binary") + (char)Op);
	if(!F){
		return LogErrorV("error binary op");
	}

	Value *Ops[] = {L, R};

//	printwq("codegen binary end\n");
	return Builder.CreateCall(F, Ops, "binop");
}

Value *CallExprAST::codegen()
{
//	printwq("codegen call\n");
	// Look up the name in the global module table.
	Function *CalleeF = getFunction(Callee);
	if (!CalleeF)
		return LogErrorV("Unknown function referenced");

	// If argument mismatch error.
	if (CalleeF->arg_size() != Args.size())
		return LogErrorV("Incorrect # arguments passed");

	std::vector<Value *> ArgsV;
	for (unsigned i = 0, e = Args.size(); i != e; ++i)
	{
		ArgsV.push_back(Args[i]->codegen());
		if (!ArgsV.back())
			return nullptr;
	}

//	printwq("codegen call end\n");
	return Builder.CreateCall(CalleeF, ArgsV, "calltmp");
}

Value *IfExprAST::codegen()
{
//	printwq("codegen if\n");
	bool needMerge = false;

	Value *CondV = Cond->codegen();
	if (!CondV)
		return nullptr;

	Function *TheFunction = Builder.GetInsertBlock()->getParent();

	// Create blocks for the then and else cases.  Insert the 'then' block at the
	// end of the function.
	BasicBlock *ThenBB = BasicBlock::Create(TheContext, "then", TheFunction);
	BasicBlock *ElseBB = BasicBlock::Create(TheContext, "else");
	BasicBlock *MergeBB = BasicBlock::Create(TheContext, "ifcont");

	Builder.CreateCondBr(CondV, ThenBB, ElseBB);

	// Emit then value.
	Builder.SetInsertPoint(ThenBB);

	Value *ThenV = Then->codegen();
	if (!ThenV)
		return nullptr;
	
	std::vector<std::unique_ptr<ExprAST>> ThenExprs = Then->getBodyExpr();
	if(ThenExprs[ThenExprs.size()-1]->getType()!=returnT){
		Builder.CreateBr(MergeBB);
		needMerge = true;
	}
	
	// Codegen of 'Then' can change the current block, update ThenBB for the PHI.
	ThenBB = Builder.GetInsertBlock();

	// Emit else block.
	TheFunction->getBasicBlockList().push_back(ElseBB);
	Builder.SetInsertPoint(ElseBB);

	Value *ElseV = Else->codegen();
	if (!ElseV)
		return nullptr;
	
	std::vector<std::unique_ptr<ExprAST>> ElseExprs = Else->getBodyExpr();
	if(ElseExprs[ElseExprs.size()-1]->getType()!=returnT){
		Builder.CreateBr(MergeBB);
		needMerge = true;
	}
	// Codegen of 'Else' can change the current block, update ElseBB for the PHI.
	ElseBB = Builder.GetInsertBlock();

	// Emit merge block.
	if(needMerge){
		TheFunction->getBasicBlockList().push_back(MergeBB);
		Builder.SetInsertPoint(MergeBB);
	}
	return Constant::getNullValue(Type::getDoubleTy(TheContext));
}

Value *ForExprAST::codegen()
{
//	printwq("codegen for\n");

	Function *TheFunction = Builder.GetInsertBlock()->getParent();
	// Create an alloca for the variable in the entry block.
	AllocaInst *ConAlloca = CreateEntryBlockAlloca(TheFunction, VarName, 1);
	//if there is variable with same name
	AllocaInst *OldVal = NamedValues[VarName];
	NamedValues[VarName] = ConAlloca;

	// Emit the start code first, without 'variable' in scope.
	Value *StartVal = Start->codegen();
	if (!StartVal)
		return nullptr;
	// Store the value into the alloca.
	Builder.CreateStore(StartVal, ConAlloca);

	BasicBlock *ConBB = BasicBlock::Create(TheContext,"conbr", TheFunction);
	BasicBlock *BodyBB = BasicBlock::Create(TheContext,"bodybr");
	BasicBlock *AfterBB = BasicBlock::Create(TheContext,"afterbr");

	Builder.CreateBr(ConBB);
	Builder.SetInsertPoint(ConBB);

	Value *EndCond = End->codegen();
	if (!EndCond)
		return nullptr;

	Builder.CreateCondBr(EndCond, BodyBB, AfterBB);

	ConBB = Builder.GetInsertBlock();

	// Emit body block.
	TheFunction->getBasicBlockList().push_back(BodyBB);
	Builder.SetInsertPoint(BodyBB);
	if (!Body->codegen())
		return nullptr;
	
	BodyBB = Builder.GetInsertBlock();
	// Emit the step value.
	Value *StepVal = nullptr;
	if (Step){
		StepVal = Step->codegen();
		if (!StepVal)
			return nullptr;
	}else
	{
		// If not specified, use 1.0.
		StepVal = ConstantFP::get(TheContext, APFloat(1.0));
	}
	Value *CurVar = Builder.CreateLoad(ConAlloca, VarName.c_str());
	Value *NextVar = Builder.CreateFAdd(CurVar, StepVal, "nextvar");
	Builder.CreateStore(NextVar, ConAlloca);

	Builder.CreateBr(ConBB);

	// Emit after block.
	TheFunction->getBasicBlockList().push_back(AfterBB);
	Builder.SetInsertPoint(AfterBB);

	if (OldVal)
		NamedValues[VarName] = OldVal;
	else
		NamedValues.erase(VarName);
	return Constant::getNullValue(Type::getDoubleTy(TheContext));
/////////////
/*	BasicBlock *LoopBB = BasicBlock::Create(TheContext,"bodybr", TheFunction);
	
	// Insert an explicit fall through from the current block to the LoopBB.
	Builder.CreateBr(LoopBB);

	// Start insertion in LoopBB.
	Builder.SetInsertPoint(LoopBB);

	// Within the loop, the variable is defined equal to the PHI node.  If it
	// shadows an existing variable, we have to restore it, so save it now.


	// Emit the body of the loop.  This, like any other expr, can change the
	// current BB.  Note that we ignore the value computed by the body, but don't
	// allow an error.
	if (!Body->codegen())
		return nullptr;

	// Emit the step value.
	Value *StepVal = nullptr;
	if (Step)
	{
		StepVal = Step->codegen();
		if (!StepVal)
			return nullptr;
	}
	else
	{
		// If not specified, use 1.0.
		StepVal = ConstantFP::get(TheContext, APFloat(1.0));
	}

	// Compute the end condition.
	Value *EndCond = End->codegen();
	if (!EndCond)
		return nullptr;

	// Reload, increment, and restore the alloca.  This handles the case where
	// the body of the loop mutates the variable.
	Value *CurVar = Builder.CreateLoad(Alloca, VarName.c_str());
	Value *NextVar = Builder.CreateFAdd(CurVar, StepVal, "nextvar");
	Builder.CreateStore(NextVar, Alloca);

	// Create the "after loop" block and insert it.
	BasicBlock *AfterBB = BasicBlock::Create(TheContext, "afterloop", TheFunction);
	
    //BasicBlock *AfterBB = BasicBlock::Create(TheContext, Builder.GetInsertBlock()->getParent());
	// Insert the conditional branch into the end of LoopEndBB.
	Builder.CreateCondBr(EndCond, LoopBB, AfterBB);

	// Any new code will be inserted in AfterBB.
	Builder.SetInsertPoint(AfterBB);

	// Restore the unshadowed variable.
	if (OldVal)
		NamedValues[VarName] = OldVal;
	else
		NamedValues.erase(VarName);

	// for expr always returns 0.0.
	return Constant::getNullValue(Type::getDoubleTy(TheContext));*/
}

Value *CallArrayAST::codegen()
{
//	printwq("codegen call array\n");

	Value *arrayPtr = NamedArray[arrayName];
	if (!arrayPtr)
	{
		arrayPtr = DynamicArray[arrayName];

		if(!arrayPtr)
			return LogErrorV("undefined array");
		arrayPtr = Builder.CreateLoad(arrayPtr);							
	}
	Value *index = arrayIndex->codegen();
    //int (*FP)() = (int (*)())(intptr_t)cantFail(index.getAddress());
	Value *eleptr = Builder.CreateGEP(cast<PointerType>(arrayPtr->getType()->getScalarType())->getElementType(),
									  arrayPtr, {Builder.CreateFPToUI(index,Type::getInt32Ty(TheContext))});
//    printwq("codegen call array end\n");
	return Builder.CreateLoad(eleptr);
    
}

Value *ArrayAST::codegen()
{
//	printwq("codegen array\n");

	Function *TheFunction = Builder.GetInsertBlock()->getParent();
	AllocaInst *Alloca =  Builder.CreateAlloca(Type::getDoubleTy(TheContext), ConstantInt::get(Type::getInt32Ty(TheContext), size), arrayName);
	/*CreateEntryBlockAlloca(TheFunction, arrayName, size);*/
    NamedArray[arrayName] = Alloca;

	if (length > 0)
	{
		//std::vector<std::unique_ptr<ExprAST>> elements;

		for (int i = 0; i < elements.size(); i++)
		{
			Value *eleptr = Builder.CreateGEP(cast<PointerType>(Alloca->getType()->getScalarType())->getElementType(),
											  Alloca, {ConstantInt::get(TheContext, APInt(32, i)) });
			Builder.CreateStore(elements[i]->codegen(), eleptr);
		}
		//Builder.CreateStore(InitVal, Alloca);
	}
    //return 0.0
	return Constant::getNullValue(Type::getDoubleTy(TheContext));
}

Value *DynamicArrayAST::codegen()
{

//	printwq("codegen dynamic array\n");

	AllocaInst *Alloca =  Builder.CreateAlloca(Type::getDoublePtrTy(TheContext),nullptr, arrayName);
    DynamicArray[arrayName] = Alloca;

	Value* Size = size->codegen();

	Size = Builder.CreateFPToSI(Size,Type::getInt64Ty(TheContext)); 

	Value * mallocSize = ConstantExpr::getSizeOf(Type::getDoubleTy(TheContext));

    mallocSize = ConstantExpr::getTruncOrBitCast(cast<Constant>(mallocSize),
            										Type::getInt64Ty(TheContext));

    Instruction * var_malloc= CallInst::CreateMalloc(Builder.GetInsertBlock(),
            Type::getInt64Ty(TheContext), Type::getDoubleTy(TheContext), mallocSize,Size,nullptr,"");

	Builder.Insert(var_malloc);
    Value *var = var_malloc;
	
	Builder.CreateStore(var,Alloca);
//	printwq("codegen dynamic array end\n");
	return Constant::getNullValue(Type::getDoubleTy(TheContext));
}

Value *VarExprAST::codegen()
{
//	printwq("codegen var\n");

	std::vector<AllocaInst *> OldBindings;
	Function *TheFunction = Builder.GetInsertBlock()->getParent();
   
	// Register all variables and emit their initializer.
	for (unsigned i = 0, e = VarNames.size(); i != e; ++i){
		const std::string &VarName = VarNames[i].first;
		ExprAST *Init = VarNames[i].second.get();
		// Emit the initializer before adding the variable to scope, this prevents
		// the initializer from referencing the variable itself, and permits stuff
		// like this:
		//  var a = 1 in
		//    var a = a in ...   # refers to outer 'a'.
		Value *InitVal;
		if (Init)
		{
			InitVal = Init->codegen();
			if (!InitVal)
				return nullptr;
		}
		else
		{ // If not specified, use 0.0.
			InitVal = ConstantFP::get(TheContext, APFloat(0.0));
		}
		AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, VarName, 1);
		auto* B = Builder.CreateStore(InitVal, Alloca);
		// Remember the old variable binding so that we can restore the binding when
		// we unrecurse.
		OldBindings.push_back(NamedValues[VarName]);
		// Remember this binding.
		NamedValues[VarName] = Alloca;
//		printwq("codegen var end1\n");
        return B;
	}

	return nullptr;
}

Function *PrototypeAST::codegen()
{
//	printwq("codegen prototype\n");

	std::vector<Type *> Doubles(Args.size(), Type::getDoubleTy(TheContext));

	FunctionType *FT;
    if(getReturnType()==doubleR){
        FT = FunctionType::get(Type::getDoubleTy(TheContext), Doubles, false);
    }else if(getReturnType()==voidR){
        FT = FunctionType::get(Type::getVoidTy(TheContext), Doubles, false);
    }
//	printf("==========================name: %s",(char*)Name.data());
	Function *F = Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());

	// Set names for all arguments.
	unsigned Idx = 0;
	for (auto &Arg : F->args())
		Arg.setName(Args[Idx++]);

//	printwq("codegen prototype end\n");

	return F;
}

Value* BodyAST::codegen()
{
//	printwq("codegen body\n");

	for(int i = 0;i<bodys.size();i++){
		bodys[i]->codegen();
	}
	//return lastExpr;
	return Constant::getNullValue(Type::getDoubleTy(TheContext));
	
}

Value *ReturnAST::codegen(){
	Value* returnValue = returnE->codegen();
	Builder.CreateRet(returnValue);
	// return returnValue;
	return Constant::getNullValue(Type::getDoubleTy(TheContext));
}

Function *FunctionAST::codegen()
{
//	printwq("codegen function\n");

	// Transfer ownership of the prototype to the FunctionProtos map, but keep a
	// reference to it for use below.
	auto &P = *Proto;
	FunctionProtos[Proto->getName()] = std::move(Proto);
	Function *TheFunction = getFunction(P.getName());
	if (!TheFunction)
		return nullptr;

	// If this is an operator, install it.
	if (P.isBinaryOp())
		BinopPrecedence[P.getOperatorName()] = P.getBinaryPrecedence();
	// Create a new basic block to start insertion into.
    //static BasicBlock * 	Create (LLVMContext &Context, const Twine &Name="", Function *Parent=nullptr, BasicBlock *InsertBefore=nullptr)
	BasicBlock *BB = BasicBlock::Create(TheContext, "entry", TheFunction);
	Builder.SetInsertPoint(BB);

	// Record the function arguments in the NamedValues map.
	NamedValues.clear();
	for (auto &Arg : TheFunction->args())
	{
		AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, Arg.getName(), 1);
		Builder.CreateStore(&Arg, Alloca);
		NamedValues[std::string(Arg.getName())] = Alloca;
	}

	// add a variable to record the return value
	// use "return" as its name to avoid variable name cash
	//Function *TheFunction = Builder.GetInsertBlock()->getParent();
	//AllocaInst *returnAlloca = CreateEntryBlockAlloca(Function *TheFunction,"return", 0); //0 is the variable Type in the future
	//NamedValues["return"] = returnAlloca;

	NamedArray.clear();   // clean the array in last function
	DynamicArray.clear();   //
	Body->codegen();
	
    // Validate the generated code, checking for consistency.
    verifyFunction(*TheFunction);
    // Run the optimizer on the function.  
	// TheFPM->run(*TheFunction);
    return TheFunction;

	// Error reading body, remove function.
	TheFunction->eraseFromParent();

	if (P.isBinaryOp())
		BinopPrecedence.erase(P.getOperatorName());

//	printwq("codegen function end\n");
	return nullptr;
}

