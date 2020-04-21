#include "head.h"
#include "AST.h"
#include "Parser.h"
//===----------------------------------------------------------------------===//
// Parser
//===----------------------------------------------------------------------===//

/// CurTok/getNextToken - Provide a simple token buffer.  CurTok is the current
/// token the parser is looking at.  getNextToken reads another token from the
/// lexer and updates CurTok with its results.

static std::unique_ptr<ExprAST> ParseExpression(int level);
static std::unique_ptr<ExprAST> ParseVarExpr(int level);
static std::unique_ptr<ExprAST> ParsePrimary(int level);
static std::unique_ptr<ExprAST> ParseArrayExpr(int level);
static std::unique_ptr<ExprAST> ParseIdentifierExpr(int level);
static std::unique_ptr<ExprAST> ParseIfExpr(int level);
static std::unique_ptr<ExprAST> ParseForExpr(int level);

std::string double2Str(double d) {
    std::stringstream ss;
    ss << d;
    return ss.str();
}

std::string int2Str(int i) {
    std::stringstream ss;
    ss << i;
    return ss.str();
}
/// numberexpr ::= number
static std::unique_ptr<ExprAST> ParseNumberExpr(int level)
{
	/*for(int i=0;i<level;i++){
		printwq(" ");
	}
	printwq("parse number\n");*/
	auto Result = std::make_unique<NumberExprAST>(NumVal); 
	getNextToken();	// eat the number	
					   
	return std::move(Result);
}

/// parenexpr ::= '(' expression ')'
static std::unique_ptr<ExprAST> ParseParenExpr(int level)
{
/*	for(int i=0;i<level;i++){
		printwq(" ");
	}
	printwq("parse paren\n");*/
	getNextToken(); // eat (.
	auto V = ParseExpression(level+1);
	if (!V)
	{
		return nullptr;
	}
	if (CurTok != ')')
		return LogError("expected ')'");
	getNextToken(); // eat ).

	return V;	
}

/// varexpr ::= 'var' identifier ('=' expression)?
///                    (',' identifier ('=' expression)?)*
static std::unique_ptr<ExprAST> ParseVarExpr(int level)
{
	for(int i=0;i<level;i++){
		printwq(" ");
	}
	printwq("parse var\n");

	getNextToken(); //eat var
	std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames;
	//At least one variable name is required
	if (CurTok != tok_identifier)
		return LogError("expected identifier after var");

	while (true)
	{
		std::string Name = IdentifierStr;
		getNextToken();

		//read th optional initializer
		std::unique_ptr<ExprAST> Init = nullptr;
		if (CurTok == assignment)
		{
			printwq("var assigment \n");
			getNextToken();
			Init = ParseExpression(level+1);
			if (!Init)
				return nullptr;
		}
		VarNames.push_back(std::make_pair(Name, std::move(Init)));

		// end of var list, exit loop
		if (CurTok != ',')
			break;
		getNextToken();
		if (CurTok != tok_identifier)
			return LogError("expected identifier list after var");
	}

	return std::make_unique<VarExprAST>(std::move(VarNames));
}

///arrayexpr ::= array a = { 1.1, 2.2 }
///          ::= array a = [3]
///          ::= array *a = new [n]
static std::unique_ptr<ExprAST> ParseArrayExpr(int level)
{
/*	for(int i=0;i<level;i++){
		printwq(" ");
	}
	printwq("parse array\n");*/

	getNextToken(); // eat array

	if(CurTok=='*'){
		getNextToken(); // eat *
		
		if(CurTok != tok_identifier){
			return LogError("expected array name");
		}
		std::string name = IdentifierStr;
		getNextToken(); //eat name
		
		if(CurTok != '='){
			return LogError("expected =");
		}
		getNextToken();

		if(CurTok != tok_new){
			return LogError("expected new keyword");
		}
		getNextToken();
		
		if(CurTok != '['){
			return LogError("expected [");
		}
		getNextToken();

		std::unique_ptr<ExprAST> size = ParseExpression(level+1);

		if(CurTok != ']'){
			return LogError("expected ]");
		}
		getNextToken();

		auto result = std::make_unique<DynamicArrayAST>(name, std::move(size));
/*		for(int i=0;i<level;i++){
			printwq(" ");
		}
		printwq("end parse array 1\n");*/
		return std::move(result);

	}else if(CurTok == tok_identifier){

		std::string name = IdentifierStr;
		getNextToken(); //eat identifier
		if (CurTok != '=')
		{
			return LogError("expected '=' in array definition");
		}
		getNextToken(); //eat =
		if (CurTok == '{')
		{
			getNextToken(); //eat {
			std::vector<std::unique_ptr<ExprAST>> content;
			while (CurTok != '}')
			{
				auto num = ParseNumberExpr(level+1);
				if (!num)
				{
					return nullptr;
				}
				else
				{
					content.push_back(std::move(num));
				}

				if (CurTok != ',' && CurTok != '}')
				{
					LogError((char*)(int2Str(CurTok)).data());
					return LogError("unexpected symbol in array definition");
				}
				if(CurTok == ','){
					getNextToken();
				}
			}
			getNextToken(); //eat }
			auto result = std::make_unique<ArrayAST>(name, std::move(content));
		/*	for(int i=0;i<level;i++){
				printwq(" ");
			}
			printwq("end parse array 1\n");*/
			return std::move(result);
		}
		else if (CurTok == '[')
		{
			getNextToken(); //eat '['
			if (CurTok != tok_number)
			{
				return LogError("expected a number as the size of array");
			}
			int num = NumVal;
			if (num <= 0)
			{
				return LogError("the size of array should be positive");
			}
			getNextToken(); //eat number
			if (CurTok != ']')
			{
				return LogError("epected ']'");
			}
			else
			{
				getNextToken();
			}
			auto result = std::make_unique<ArrayAST>(name, num);
		/*	for(int i=0;i<level;i++){
				printwq(" ");
			}
			printwq("end parse array 2\n");*/
			return std::move(result);
		}
		else
		{
			return LogError("unexpected symbol in array definition");
		}
	}
}

static std::unique_ptr<ExprAST> ParseReturnExpr(int level)
{
	/*	for(int i=0;i<level;i++){
		printwq(" ");
	}
	printwq("parse array\n");*/

	getNextToken(); //eat return
	std::unique_ptr<ExprAST> returnE = ParseExpression(level+1);
	
	return std::make_unique<ReturnAST>(std::move(returnE));
}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
///   ::= ifexpr
///   ::= forexpr
///   ::= varexpr
///   ::= arrayexpr
///   ::= returnexpr
///   ::= { primary+ }
static std::unique_ptr<ExprAST> ParsePrimary(int level)
{                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      
	/*for(int i=0;i<level;i++){
		printwq(" ");
	}
	printwq("parse primary\n");*/

	switch (CurTok)
	{
	default:
		return LogError("unknown token when expecting an expression");
	case tok_identifier:
		/*for(int i=0;i<level;i++){
			printwq(" ");
		}
		printwq("end parse primary 1\n");*/
		return ParseIdentifierExpr(level+1);
	case tok_number:
		/*for(int i=0;i<level;i++){
			printwq(" ");
		}
		printwq("end parse primary 2\n");*/
		return ParseNumberExpr(level+1);
	case '(':
		/*for(int i=0;i<level;i++){
			printwq(" ");
		}
		printwq("end parse primary 3\n");*/
		return ParseParenExpr(level+1);
	case tok_if:
		/*for(int i=0;i<level;i++){
			printwq(" ");
		}
		printwq("end parse primary 4\n");*/
		return ParseIfExpr(level+1);
	case tok_for:
		/*for(int i=0;i<level;i++){
			printwq(" ");
		}
		printwq("end parse primary 5\n");*/
		return ParseForExpr(level+1);
	case tok_var:
		/*for(int i=0;i<level;i++){
			printwq(" ");
		}
		printwq("end parse primary 6\n");*/
		return ParseVarExpr(level+1);
	case tok_array:
		/*for(int i=0;i<level;i++){
			printwq(" ");
		}
		printwq("end parse primary 7\n");*/
		return ParseArrayExpr(level+1);
	case  tok_return:
		for(int i=0;i<level;i++){
			printwq(" ");
		}
		printwq("end parse return\n");
		return ParseReturnExpr(level+1);
	case '{':
		for(int i=0;i<level;i++){
			printwq(" ");
		}
		printwq("end parse {\n");
		getNextToken(); // eat {
		std::vector<std::unique_ptr<ExprAST>> bodys;
		
		while(true){

			auto B = ParseExpression(level+1);
			if(B){
				if (B->getType() == returnT){
					bodys.push_back(move(B));
					break;
				}
				else{
					bodys.push_back(move(B));
				}
			}else{
				return nullptr;
			}
			
			//no return
			if(CurTok == '}')
				break;
		}
		if(CurTok != '}'){
			while(true){
				// omit the expression after return
				auto B = ParseExpression(level+1);
				if(CurTok == '}')
					break;
			}
		}
		getNextToken(); //eat }

		return move(std::make_unique<BodyAST>(std::move(bodys)));
			
	}
		
}


/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
///   ::= identifier '[' e ']'
static std::unique_ptr<ExprAST> ParseIdentifierExpr(int level)
{
	for(int i=0;i<level;i++){
		printwq(" ");
	}
	printwq("parse identifier\n");
	std::string IdName = IdentifierStr;

	getNextToken(); // eat identifier.

	if (CurTok != '(' && CurTok != '[' && CurTok != '.') // Simple variable ref.
	{	
	/*	for(int i=0;i<level;i++){
			printwq(" ");
		}
		printwq("end identifier end 1\n");*/
		return std::make_unique<VariableExprAST>(IdName);
	}
	if (CurTok == '(')
	{
		// Call.
		getNextToken(); // eat (
		std::vector<std::unique_ptr<ExprAST>> Args;
		if (CurTok != ')')
		{
			while (true)
			{
				if (auto Arg = ParseExpression(level+1))
					Args.push_back(std::move(Arg));
				else
					return nullptr;

				if (CurTok == ')')
					break;

				if (CurTok != ',')
					return LogError("Expected ')' or ',' in argument list");
				getNextToken();
			}
		}
		// Eat the ')'.
		getNextToken();
	/*	for(int i=0;i<level;i++){
			printwq(" ");
		}
		printwq("end parse identifier call\n");*/
		return std::make_unique<CallExprAST>(IdName, std::move(Args));
	}
	else if (CurTok == '[')
	{
		// CallArray.
		getNextToken(); // eat [
		std::unique_ptr<ExprAST> index;
		if (CurTok != ']')
		{
			index = ParseExpression(level+1); // Ҫ�иĽ�
		}
		else
		{
			return LogError("Expected index between [ ]");
		}
		// Eat the ']'.
		getNextToken();
		//if (CurTok != '=')
	/*	for(int i=0;i<level;i++){
			printwq(" ");
		}
		printwq("end parse identifier array\n");*/
		return std::make_unique<CallArrayAST>(IdName, std::move(index));
		//else
		//	return ParseInitArrayExpr(std::make_unique<CallArrayAST>(IdName, std::move(index)));
	}
	
}

static std::unique_ptr<ExprAST> ParseBody(int level){
	/*for(int i=0;i<level;i++){
		printwq(" ");
	}
	printwq("parse body");*/
	auto bodyE = ParseExpression(level+1);

	if(bodyE->getType()== ASTType::body){
		/*for(int i=0;i<level;i++){
			printwq(" ");
		}
		printwq("end parse body return\n");*/
		return move(bodyE);
	}else{
		std::vector<std::unique_ptr<ExprAST>> realBody;
		realBody.push_back(move(bodyE));
		/*for(int i=0;i<level;i++){
			printwq(" ");
		}
		printwq("end parse body ret\n");*/
		return std::make_unique<BodyAST>(move(realBody));
	}
}

/// ifexpr ::= 'if' expression 'then' expression 'else' expression
static std::unique_ptr<ExprAST> ParseIfExpr(int level){

	/*for(int i=0;i<level;i++){
		printwq(" ");
	}
	printwq("parse if\n");*/
	getNextToken(); // eat the if.

	// condition.
	auto Cond = ParseExpression(level+1);
	if (!Cond)
		return LogError("wrong condition");

	if (CurTok != tok_then){
		return LogError("expected then");
	}
	getNextToken(); // eat the then

	auto Then = ParseBody(level+1);
	if (!Then)
		return LogError("wrong then body");

	printwq("parse else \n");
	if (CurTok != tok_else)
		return LogError("expected else");
	printwq("end parse else\n");
	getNextToken(); //eat else

	auto Else = ParseBody(level+1);
	if (!Else)
		return LogError("wrong else body");

	return std::make_unique<IfExprAST>(std::move(Cond), std::move(Then), std::move(Else));
}

/// forexpr ::= 'for' identifier '=' expr ',' expr (',' expr)? 'in' expression
/// for i = 1, i < n, 1.0 in
///    putchard(42);
static std::unique_ptr<ExprAST> ParseForExpr(int level)
{
	for(int i=0;i<level;i++){
		printwq(" ");
	}
	printwq("parse for\n");
	getNextToken(); // eat the for.

	if (CurTok != tok_identifier)
		return LogError("expected identifier after for");

	std::string IdName = IdentifierStr;
	getNextToken(); // eat identifier.

	if (CurTok != assignment)
		return LogError("expected '=' after for");
	getNextToken(); // eat '='.

	auto Start = ParseExpression(level+1);
	if (!Start)
		return nullptr;
	if (CurTok != ',')
		return LogError("expected ',' after for start value");
	getNextToken(); //eat ','

	auto End = ParseExpression(level+1);
	if (!End)
		return nullptr;

	// The step value is optional.
	std::unique_ptr<ExprAST> Step;
	if (CurTok == ',')
	{
		getNextToken();
		Step = ParseExpression(level+1);
		if (!Step)
			return nullptr;
	}

	if (CurTok != tok_in)
		return LogError("expected 'in' after for");
	getNextToken(); // eat 'in'.

	auto Body = ParseBody(level+1);
	if (!Body)
		return nullptr;

	std::unique_ptr<ForExprAST> forE = std::make_unique<ForExprAST>(IdName, std::move(Start), std::move(End),
										std::move(Step), std::move(Body));
	return move(forE);
}

/// unary
///   ::= primary
///   ::= '!' unary
static std::unique_ptr<ExprAST> ParseUnary(int level)
{
	for(int i=0;i<level;i++){
		printwq(" ");
	}
	printwq("parse unary\n");
	// If the current token is not an operator, it must be a primary expr.
	if (CurTok < 0 || CurTok == '(' || CurTok == ',' || CurTok=='{') // ( �������ŵı���ʽ���� , ��ɶ��
	{	
		/*for(int i=0;i<level;i++){
			printwq(" ");
		}
		printwq("end parse primary 1\n");*/
		if(CurTok != minus || CurTok != plus || CurTok != notSign){
			return ParsePrimary(level+1);
		}
		
	}
	// If this is a unary operator, read it.
	int Opc = CurTok;
	getNextToken();
	if (auto Operand = ParseUnary(level+1)){
		/*for(int i=0;i<level;i++){
			printwq(" ");
		}
		printwq("end parse unary 2\n");*/
		return std::make_unique<UnaryExprAST>(Opc, std::move(Operand));
	}
	return nullptr;
}

///# Logical unary not.
///  def unary!(v)
///    if v then
///      0
///    else
///      1;

///# Define > with the same precedence as < .
///	def binary> 10 (LHS RHS)
///	  RHS < LHS;

/// binoprhs
///   ::= ('+' unary)*

static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS, int level)
{
	for(int i=0;i<level;i++){
		printwq(" ");
	}
	printwq("parse binopRHS\n");
	// If this is a binop, find its precedence.
	while (true)
	{
		int TokPrec = GetTokPrecedence();
		//printint(TokPrec);
		// If this is a binop that binds at least as tightly as the current binop,
		// consume it, otherwise we are done.
		if (TokPrec < ExprPrec)
			return std::move(LHS);

		// Okay, we know this is a binop.
		int BinOp = CurTok;
		getNextToken(); // eat binop

		// Parse the unary expression after the binary operator.
		auto RHS = ParseUnary(level+1);
		if (!RHS)
			return nullptr;

		// If BinOp binds less tightly with RHS than the operator after RHS, let
		// the pending operator take RHS as its LHS.
		int NextPrec = GetTokPrecedence();
		//printint(NextPrec);
		if (TokPrec < NextPrec)
		{
			RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS), level+1);
			if (!RHS)
				return nullptr;
		}

		// Merge LHS/RHS.
		//printwq("binary AST \n");
		LHS = std::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
	}
}

/// expression
///   ::= unary binoprhs
static std::unique_ptr<ExprAST> ParseExpression(int level)
{
	for(int i=0;i<level;i++){
		printwq(" ");
	}
	printwq("parse expression\n");

	auto LHS = ParseUnary(level+1);
	if (!LHS)
		return nullptr;

	return ParseBinOpRHS(0, std::move(LHS),level+1);
}

/// prototype
///   ::= id '(' id* ')'
///   ::= binary LETTER number? (id, id)
///   ::= unary LETTER (id)
std::unique_ptr<PrototypeAST> ParsePrototype(int level){ 
	for(int i=0;i<level;i++){
		printwq(" ");
	}
	printwq("parse proto type\n");
	std::string FnName;
    int returnType;
	unsigned Kind = 0; // 0 = identifier, 1 = unary, 2 = binary.
	unsigned BinaryPrecedence = 30;

	switch (CurTok)
	{
	default:
		return LogErrorP("Expected function name in prototype");
	case tok_double:
        returnType = doubleR;
        getNextToken();
        break;
    case tok_void:
        returnType = voidR;
        getNextToken();
        break;
	case tok_unary:
		getNextToken();
		if (!isascii(CurTok))
			return LogErrorP("Expected unary operator");
		FnName = "unary";
		FnName += (char)CurTok; //һԪ��
		Kind = 1;
		getNextToken();
		break;
	case tok_binary:
		getNextToken();
		if (!isascii(CurTok))
			return LogErrorP("Expected binary operator");
		FnName = "binary";
		FnName += (char)CurTok;
		Kind = 2;
		getNextToken();

		// Read the precedence if present.
		if (CurTok == tok_number)
		{
			if (NumVal < 1 || NumVal > 100)
				return LogErrorP("Invalid precedence: must be 1..100");
			BinaryPrecedence = (unsigned)NumVal;
			getNextToken();
		}
		break;
	}
    if (CurTok != tok_identifier)
        return LogErrorP("Expected function name");

    FnName = IdentifierStr;
    getNextToken();

	if (CurTok != '(')
		return LogErrorP("Expected '(' in prototype");

	std::vector<std::string> ArgNames;
	while (getNextToken() == tok_identifier)
	{
		ArgNames.push_back(IdentifierStr);
	}
	if (CurTok != ')')
		return LogErrorP("Expected ')' in prototype");

	// success.
	getNextToken(); // eat ')'.

	// Verify right number of names for operator.
	if (Kind && ArgNames.size() != Kind)
		return LogErrorP("Invalid number of operands for operator");
	return std::make_unique<PrototypeAST>(FnName, ArgNames,returnType, Kind != 0, BinaryPrecedence);
}


/// definition ::= 'def' prototype expression
std::unique_ptr<FunctionAST> ParseDefinition(int level)
{
	for(int i=0;i<level;i++){
		printwq(" ");
	}
	printwq("parse definition\n");

	getNextToken(); // eat def.
    auto Proto = ParsePrototype(level+1);
	if (!Proto)
		return nullptr;
	auto bodyE = ParseBody(level+1);
	if(!bodyE){
		return nullptr;
	}
	return move(std::make_unique<FunctionAST>(move(Proto),move(bodyE)));
}

/// toplevelexpr ::= expression
std::unique_ptr<FunctionAST> ParseTopLevelExpr(int level)
{
/*	for(int i=0;i<level;i++){
		printwq(" ");
	}
	printwq("parse top level\n");*/

	if (auto E = ParseExpression(level+1))
	{
		// Make an anonymous proto.
		auto Proto = std::make_unique<PrototypeAST>("__anon_expr", std::vector<std::string>(),doubleR);
		std::vector<std::unique_ptr<ExprAST>> bodyE;
		bodyE.push_back(std::move(E));
		//std::unique_ptr<ExprAST> returnE;
		std::unique_ptr<BodyAST> bodys = std::make_unique<BodyAST>(/*move(returnE),*/move(bodyE));
		return std::make_unique<FunctionAST>(std::move(Proto), std::move(bodys));
	}
	return nullptr;
}

/// external ::= 'extern' prototype
std::unique_ptr<PrototypeAST> ParseExtern(int level)
{
/*	for(int i=0;i<level;i++){
		printwq(" ");
	}
	printwq("parse extern\n");*/
	getNextToken(); // eat extern.
	return ParsePrototype(level+1);
}