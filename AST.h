#ifndef AST_H
#define AST_H
#include "head.h"

/// BinopPrecedence - This holds the precedence for each binary operator that is
/// defined.
extern std::map<int, int> BinopPrecedence;

/// GetTokPrecedence - Get the precedence of the pending binary operator token.
int GetTokPrecedence();

//===----------------------------------------------------------------------===//
// Abstract Syntax Tree (aka Parse Tree)
//===----------------------------------------------------------------------===//


/// ExprAST - Base class for all expression nodes.
class ExprAST{
protected:
	int type;	
public:
	virtual ~ExprAST() = default;
	
	virtual Value *codegen() = 0;
	int getType();                         
};

/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST
{
	double Val;

public:
	NumberExprAST(double Val) : Val(Val) { type = ASTType::number; }
	double getVal() { return Val; }
	Value *codegen() override;
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST
{
	std::string Name;
	int varType;
public:
	VariableExprAST(const std::string &Name) : Name(Name) {
		type = variable;
	}
	const std::string &getName() const { return Name; }
	Value *codegen() override;
};

/// UnaryExprAST - Expression class for a unary operator.
class UnaryExprAST : public ExprAST
{
	int Opcode;					  
	std::unique_ptr<ExprAST> Operand; 

public:
	UnaryExprAST(int Opcode, std::unique_ptr<ExprAST> Operand)
		: Opcode(Opcode), Operand(std::move(Operand)) {type = ASTType::unary;}

	Value *codegen() override;
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST
{
	int Op;
	std::unique_ptr<ExprAST> LHS, RHS;

public:
	BinaryExprAST(int Op, std::unique_ptr<ExprAST> LHS,
				  std::unique_ptr<ExprAST> RHS)
		: Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {type = ASTType::binary;}

	Value *codegen() override;
};

/// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST
{
	std::string Callee;
	std::vector<std::unique_ptr<ExprAST>> Args;

public:
	CallExprAST(const std::string &Callee,
				std::vector<std::unique_ptr<ExprAST>> Args)
		: Callee(Callee), Args(std::move(Args)) {type = ASTType::call;}

	Value *codegen() override;
};

class BodyAST: public ExprAST{
	std::vector<std::unique_ptr<ExprAST>> bodys;
	//std::unique_ptr<ExprAST> returnE;

public:
	BodyAST(std::vector<std::unique_ptr<ExprAST>> bodys1){
		for(int i=0;i<bodys1.size();i++){
			bodys.push_back(move(bodys1[i]));
		}
		//returnE = move(returnE1);
		type = ASTType::body;
	}

	BodyAST(){}
	
	std::vector<std::unique_ptr<ExprAST>> getBodyExpr(){
		return move(bodys);
	}
	
	Value* codegen();
};

/// IfExprAST - Expression class for if/then/else.
class IfExprAST : public ExprAST
{
	std::unique_ptr<ExprAST> Cond;
	std::unique_ptr<BodyAST> Then, Else;

public:
	IfExprAST(std::unique_ptr<ExprAST> Cond, std::unique_ptr<BodyAST> Then,
			  std::unique_ptr<BodyAST> Else)
		: Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {
			type = ASTType::ifT;
		}

	Value *codegen() override;
};

/// ForExprAST - Expression class for for/in.
class ForExprAST : public ExprAST
{
	std::string VarName;
	std::unique_ptr<ExprAST> Start, End, Step;
	std::unique_ptr<ExprAST> Body;

public:
	ForExprAST(const std::string &VarName, std::unique_ptr<ExprAST> Start,
			   std::unique_ptr<ExprAST> End, std::unique_ptr<ExprAST> Step,
			   std::unique_ptr<ExprAST> Body)
		: VarName(VarName), Start(std::move(Start)), End(std::move(End)),
		  Step(std::move(Step)), Body(std::move(Body)) {
			  type = ASTType::forT;
		  }

	Value *codegen() override;
};

///VarExpr
class VarExprAST : public ExprAST
{
	std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames;
	int varType;
	//std::unique_ptr<ExprAST> Body;

public:
	VarExprAST(std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames, int varType)
		: VarNames(std::move(VarNames)), varType(varType) {
			 type = ASTType::var;
		}
	Value *codegen() override;
};

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes), as well as if it is an operator. 
class PrototypeAST
{
	std::string Name;
	std::vector<std::string> Args;
	bool IsOperator;
	unsigned Precedence; // Precedence if a binary op.
    int returnType;
	int type;

public:
	PrototypeAST(const std::string &Name, std::vector<std::string> Args, int returnType,
				 bool IsOperator = false, unsigned Prec = 0)
		: Name(Name), Args(std::move(Args)),returnType(returnType),IsOperator(IsOperator),
		  Precedence(Prec) {
			  type = ASTType::proto;
		  }
	int getType(){ return type; }
	Function *codegen();
	const std::string &getName() const { return Name; }

	bool isUnaryOp() const { return IsOperator && Args.size() == 1; }
	bool isBinaryOp() const { return IsOperator && Args.size() == 2; }

	char getOperatorName() const
	{
		assert(isUnaryOp() || isBinaryOp());
		return Name[Name.size() - 1]; 
    }

	unsigned getBinaryPrecedence() const { return Precedence; }
    const int &getReturnType() const{ return returnType;}
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST
{
	std::unique_ptr<PrototypeAST> Proto;
	std::unique_ptr<ExprAST> Body; 
	int type;
   // std::unique_ptr<ExprAST> returnE;

public:
	FunctionAST(std::unique_ptr<PrototypeAST> Proto1, std::unique_ptr<ExprAST> Body1/*, std::unique_ptr<ExprAST> returnE1*/)
	{
		Proto = std::move(Proto1);
		Body = std::move(Body1);
		type = ASTType::function;
        //returnE = move(returnE1);
	}
	int getType(){
		return type;
	}
	Function *codegen();
};

///arrayAST
class ArrayAST : public ExprAST
{
	std::string arrayName;
	int size;
	int length;
	std::vector<std::unique_ptr<ExprAST>> elements;

public:
	ArrayAST(std::string arrayName, std::vector<std::unique_ptr<ExprAST>> element) : arrayName(arrayName)
	{
		size = element.size();
		length = element.size();
		for (int i = 0; i < element.size(); i++)
		{
			elements.push_back(std::move(element[i]));
		}
		type = ASTType::array;
	}
	ArrayAST(std::string arrayName, int size) : arrayName(arrayName), size(size) { 
		length = 0; 
		type = ASTType::array;
	}
	const std::string getName() { return arrayName; }

	Value *codegen();
};

///dynamicArrayAST
class DynamicArrayAST:public ExprAST{
	std::string arrayName;
	std::unique_ptr<ExprAST> size;
public:
	DynamicArrayAST(std::string arrayName,std::unique_ptr<ExprAST> size):arrayName(arrayName),size(move(size)){}
	std::unique_ptr<ExprAST> getSize(){ return move(size); }

	Value* codegen();
};

///callArray
class CallArrayAST : public ExprAST
{
	std::string arrayName; //a[0]
	std::unique_ptr<ExprAST> arrayIndex;

public:
	CallArrayAST(std::string arrayName, std::unique_ptr<ExprAST> arrayIndex) 
	: arrayName(arrayName), arrayIndex(move(arrayIndex)) {
		type = callArray;
	}
	std::string getArrayName()
	{
		return arrayName;
	}
	std::unique_ptr<ExprAST> getIndex()
	{
		return move(arrayIndex);
	}
	Value *codegen();
};

///returnExpr
class ReturnAST : public ExprAST
{
	std::unique_ptr<ExprAST> returnE;
public:
	ReturnAST(std::unique_ptr<ExprAST> returnE) : returnE(move(returnE)) {
		type = returnT;
	}
	
	Value *codegen();
};
 // end anonymous namespace

extern std::unique_ptr<Module> TheModule;
extern std::unique_ptr<legacy::FunctionPassManager> TheFPM;
extern std::unique_ptr<KaleidoscopeJIT> TheJIT;

static LLVMContext TheContext;
static IRBuilder<> Builder(TheContext);
static std::map<std::string, AllocaInst *> NamedValues;
static std::map<std::string, AllocaInst *> NamedArray;
static std::map<std::string, Value *> DynamicArray;
static std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;

std::unique_ptr<ExprAST> LogError(const char *Str);
std::unique_ptr<PrototypeAST> LogErrorP(const char *Str);
std::unique_ptr<FunctionAST> LogErrorF(const char *Str);
Value *LogErrorV(const char *Str);
#endif