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
	if (!isascii(CurTok))
		return -1;

	// Make sure it's a declared binop.
	int TokPrec = BinopPrecedence[CurTok];
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
static AllocaInst *CreateEntryBlockAlloca(Function *TheFunction, StringRef VarName, int num)
{

	IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
	return TmpB.CreateAlloca(Type::getDoubleTy(TheContext), ConstantInt::get(Type::getInt32Ty(TheContext), num), VarName);
}


Value *NumberExprAST::codegen()
{
	return ConstantFP::get(TheContext, APFloat(Val));
}



Value *VariableExprAST::codegen()
{
	// Look this variable up in the function.
	AllocaInst *V = NamedValues[Name];
	if (!V){
		return LogErrorV("Unknown variable name");
	}
	return Builder.CreateLoad(V, Name.c_str());
}

Value *UnaryExprAST::codegen()
{
	Value *OperandV = Operand->codegen();
	if (!OperandV)
		return nullptr;

	Function *F = getFunction(std::string("unary") + Opcode);
	if (!F)
		return LogErrorV("Unknown unary operator");

	return Builder.CreateCall(F, OperandV, "unop");
}

Value *BinaryExprAST::codegen()
{

	if (Op == '=')
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

				/*Value *index = LHSE->getIndex()->codegen();
				
				Variable = Builder.CreateGEP(cast<PointerType>(realArrayPtr->getType()->getScalarType())->getElementType(),
											realArrayPtr, {Builder.CreateFPToUI(index,Type::getInt32Ty(TheContext))});
				*/
			}
			//else{
				Value *index = LHSE->getIndex()->codegen();
				Variable = Builder.CreateGEP(cast<PointerType>(arrayPtr->getType()->getScalarType())->getElementType(),
												arrayPtr, {Builder.CreateFPToUI(index,Type::getInt32Ty(TheContext))});
			//}
		}
		
        if (!Variable){
            return LogErrorV("Unknown left value");
        }

        Value* Val = RHS->codegen(); //右值
        if (!Val)
            return nullptr;
		Value* B = Builder.CreateStore(Val, Variable);
        return B;
	}

	Value *L = LHS->codegen();
	Value *R = RHS->codegen();
	if (!L || !R)
		return nullptr;

	switch (Op)
	{
	case '+':
		return Builder.CreateFAdd(L, R, "addtmp");
	case '-':
		return Builder.CreateFSub(L, R, "subtmp");
	case '*':
		return Builder.CreateFMul(L, R, "multmp");
	case '<':
		L = Builder.CreateFCmpULT(L, R, "cmptmp"); //cmp -> compare
		// Convert bool 0/1 to double 0.0 or 1.0
		return Builder.CreateUIToFP(L, Type::getDoubleTy(TheContext), "booltmp");
	default:
		break;
	}

	// If it wasn't a builtin binary operator, it must be a user defined one. Emit a call to it.
	Function *F = getFunction(std::string("binary") + Op);
	assert(F && "binary operator not found!");

	Value *Ops[] = {L, R};
	return Builder.CreateCall(F, Ops, "binop");
}

Value *CallExprAST::codegen()
{
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

	return Builder.CreateCall(CalleeF, ArgsV, "calltmp");
}

Value *IfExprAST::codegen()
{
	Value *CondV = Cond->codegen();
	if (!CondV)
		return nullptr;

	// Convert condition to a bool by comparing non-equal to 0.0.
	CondV = Builder.CreateFCmpONE(
		CondV, ConstantFP::get(TheContext, APFloat(0.0)), "ifcond");

	Function *TheFunction = Builder.GetInsertBlock()->getParent();

	// Create blocks for the then and else cases.  Insert the 'then' block at the
	//end of the function.
	BasicBlock *ThenBB = BasicBlock::Create(TheContext, "then", TheFunction);
	BasicBlock *ElseBB = BasicBlock::Create(TheContext, "else");
	BasicBlock *MergeBB = BasicBlock::Create(TheContext, "ifcont");

	Builder.CreateCondBr(CondV, ThenBB, ElseBB);

	// Emit then value.
	Builder.SetInsertPoint(ThenBB);

	Value *ThenV = Then->codegen();
	if (!ThenV)
		return nullptr;

	Builder.CreateBr(MergeBB);
	// Codegen of 'Then' can change the current block, update ThenBB for the PHI.
	ThenBB = Builder.GetInsertBlock();

	// Emit else block.
	TheFunction->getBasicBlockList().push_back(ElseBB);
	Builder.SetInsertPoint(ElseBB);

	Value *ElseV = Else->codegen();
	if (!ElseV)
		return nullptr;

	Builder.CreateBr(MergeBB);
	// Codegen of 'Else' can change the current block, update ElseBB for the PHI.
	ElseBB = Builder.GetInsertBlock();

	// Emit merge block.
	TheFunction->getBasicBlockList().push_back(MergeBB);
	Builder.SetInsertPoint(MergeBB);
	PHINode *PN = Builder.CreatePHI(Type::getDoubleTy(TheContext), 2, "iftmp");

	PN->addIncoming(ThenV, ThenBB);
	PN->addIncoming(ElseV, ElseBB);
	return PN;
}

Value *ForExprAST::codegen()
{
	Function *TheFunction = Builder.GetInsertBlock()->getParent();
	// Create an alloca for the variable in the entry block.
	AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, VarName, 1);
	
	// Emit the start code first, without 'variable' in scope.
	Value *StartVal = Start->codegen();
	if (!StartVal)
		return nullptr;

	// Store the value into the alloca.
	Builder.CreateStore(StartVal, Alloca);
	// Make the new basic block for the loop header, inserting after current
	// block.
	BasicBlock *LoopBB = BasicBlock::Create(TheContext,"loopbr", TheFunction);
	
	// Insert an explicit fall through from the current block to the LoopBB.
	Builder.CreateBr(LoopBB);

	// Start insertion in LoopBB.
	Builder.SetInsertPoint(LoopBB);

	// Within the loop, the variable is defined equal to the PHI node.  If it
	// shadows an existing variable, we have to restore it, so save it now.
	AllocaInst *OldVal = NamedValues[VarName];
	NamedValues[VarName] = Alloca;

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

	// Convert condition to a bool by comparing non-equal to 0.0.
	EndCond = Builder.CreateFCmpONE(
		EndCond, ConstantFP::get(TheContext, APFloat(0.0)), "loopcond");

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
	return Constant::getNullValue(Type::getDoubleTy(TheContext));
}

Value *CallArrayAST::codegen()
{
	Value *arrayPtr = NamedArray[arrayName];
	if (!arrayPtr)
	{
		arrayPtr = DynamicArray[arrayName];

		if(!arrayPtr)
			return LogErrorV("undefined array");
		arrayPtr = Builder.CreateLoad(arrayPtr);
		/*Value *index = arrayIndex->codegen();
		Value *eleptr = Builder.CreateGEP(cast<PointerType>(ptr->getType()->getScalarType())->getElementType(),
									  ptr, {Builder.CreateFPToUI(index,Type::getInt32Ty(TheContext))});
		return Builder.CreateLoad(eleptr);*/								
	}
	Value *index = arrayIndex->codegen();
    //int (*FP)() = (int (*)())(intptr_t)cantFail(index.getAddress());
	Value *eleptr = Builder.CreateGEP(cast<PointerType>(arrayPtr->getType()->getScalarType())->getElementType(),
									  arrayPtr, {Builder.CreateFPToUI(index,Type::getInt32Ty(TheContext))});
    return Builder.CreateLoad(eleptr);
    
}

Value *ArrayAST::codegen()
{
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

Value *DynamicArrayAST::codegen(){
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

	return Constant::getNullValue(Type::getDoubleTy(TheContext));
}

Value *VarExprAST::codegen()
{
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
        return B;
	}

	/*
    // Codegen the body, now that all vars are in scope.
	Value *BodyVal = Body->codegen();
	if (!BodyVal)
		return nullptr;

	// Pop all our variables from scope.
	for (unsigned i = 0, e = VarNames.size(); i != e; ++i)
		NamedValues[VarNames[i].first] = OldBindings[i];

	// Return the body computation.
    */
	return nullptr;
}

Function *PrototypeAST::codegen()
{
	
	std::vector<Type *> Doubles(Args.size(), Type::getDoubleTy(TheContext));

	FunctionType *FT;
    if(getReturnType()==doubleR){
        FT = FunctionType::get(Type::getDoubleTy(TheContext), Doubles, false);
    }else if(getReturnType()==voidR){
        FT = FunctionType::get(Type::getVoidTy(TheContext), Doubles, false);
    }
	printf("==========================name: %s",(char*)Name.data());
	Function *F = Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());

	// Set names for all arguments.
	unsigned Idx = 0;
	for (auto &Arg : F->args())
		Arg.setName(Args[Idx++]);

	return F;
}

Value *BodyAST::codegen(){
	for(int i = 0;i<bodys.size();i++){
		(bodys[i])->codegen();
	}
	if(!returnE){
        return Constant::getNullValue(Type::getDoubleTy(TheContext));
    }else{
		Value* returnV = returnE->codegen();
        return std::move(returnV);
    }
}

Function *FunctionAST::codegen()
{
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
	/*for (int i = 0; i < Body.size(); i++)
	{
		std::unique_ptr<ExprAST> expr = move(Body[i]);
		Value *Val = expr->codegen();
	}*/

	Value* BodyV = Body->codegen();
	Builder.CreateRet(BodyV);
    // Finish off the function.
	//int i = P.getReturnType();
	//printint(i);
  /*  if(P.getReturnType() == voidR){
        Builder.CreateRetVoid();
    }else{
        Builder.CreateRet(returnE->codegen());
    }*/

    // Validate the generated code, checking for consistency.
    verifyFunction(*TheFunction);
    // Run the optimizer on the function.  // TheFPM->run(*TheFunction);
    return TheFunction;

	// Error reading body, remove function.
	TheFunction->eraseFromParent();

	if (P.isBinaryOp())
		BinopPrecedence.erase(P.getOperatorName());
	return nullptr;
}

