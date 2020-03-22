#include "head.h"
#include "AST.h"
#include "Parser.h"
//===----------------------------------------------------------------------===//
// Parser
//===----------------------------------------------------------------------===//

/// CurTok/getNextToken - Provide a simple token buffer.  CurTok is the current
/// token the parser is looking at.  getNextToken reads another token from the
/// lexer and updates CurTok with its results.

static std::unique_ptr<ExprAST> ParseExpression();
static std::unique_ptr<ExprAST> ParseVarExpr();
static std::unique_ptr<ExprAST> ParsePrimary();
static std::unique_ptr<ExprAST> ParseArrayExpr();
static std::unique_ptr<ExprAST> ParseIdentifierExpr();
static std::unique_ptr<ExprAST> ParseIfExpr();
static std::unique_ptr<ExprAST> ParseForExpr();

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
static std::unique_ptr<ExprAST> ParseNumberExpr()
{
	auto Result = std::make_unique<NumberExprAST>(NumVal); 
	getNextToken();	// eat the number						   
	return std::move(Result);
}

/// parenexpr ::= '(' expression ')'
static std::unique_ptr<ExprAST> ParseParenExpr()
{
	getNextToken(); // eat (.
	auto V = ParseExpression();
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
///                    (',' identifier ('=' expression)?)* 'in' expression
static std::unique_ptr<ExprAST> ParseVarExpr()
{
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
		if (CurTok == '=')
		{
			getNextToken();
			Init = ParseExpression();
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
static std::unique_ptr<ExprAST> ParseArrayExpr()
{
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

		std::unique_ptr<ExprAST> size = ParseExpression();

		if(CurTok != ']'){
			return LogError("expected ]");
		}
		getNextToken();

		auto result = std::make_unique<DynamicArrayAST>(name, std::move(size));
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
				auto num = ParseNumberExpr();
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
			return std::move(result);
		}
		else
		{
			return LogError("unexpected symbol in array definition");
		}
	}
}


/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
///   ::= ifexpr
///   ::= forexpr
///   ::= varexpr
///   ::= arrayexpr
///   ::= { primary+ }
static std::unique_ptr<ExprAST> ParsePrimary()
{
	switch (CurTok)
	{
	default:
		return LogError("unknown token when expecting an expression");
	case tok_identifier:
		return ParseIdentifierExpr();
	case tok_number:
		return ParseNumberExpr();
	case '(':
		return ParseParenExpr();
	case tok_if:
		return ParseIfExpr();
	case tok_for:
		return ParseForExpr();
	case tok_var:
		return ParseVarExpr();
	case tok_array:
		return ParseArrayExpr();
	case '{':
		getNextToken(); // eat {
		std::vector<std::unique_ptr<ExprAST>> bodys;
		
		while(true){
			//auto B = ParsePrimary();
			auto B = ParseExpression();
			if(B){
				bodys.push_back(move(B));
			}else{
				return nullptr;
			}

			if(CurTok == tok_return){
				getNextToken(); //eat return
				std::unique_ptr<ExprAST> returnE = ParseExpression();
				if(!returnE) return nullptr;
				if(CurTok != '}')
					return LogError("expected }");
				getNextToken(); // eat }
				
				return std::make_unique<BodyAST>(std::move(returnE),std::move(bodys));
			}

			//no return
			if(CurTok == '}')
				break;
		}
		getNextToken(); //eat }
		std::unique_ptr<ExprAST> returnE;
		return move(std::make_unique<BodyAST>(move(returnE),std::move(bodys)));
	}
}

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
///   ::= identifier '[' e ']'
static std::unique_ptr<ExprAST> ParseIdentifierExpr()
{
	std::string IdName = IdentifierStr;

	getNextToken(); // eat identifier.

	if (CurTok != '(' && CurTok != '[' && CurTok != '.') // Simple variable ref.
		return std::make_unique<VariableExprAST>(IdName);
	if (CurTok == '(')
	{
		// Call.
		getNextToken(); // eat (
		std::vector<std::unique_ptr<ExprAST>> Args;
		if (CurTok != ')')
		{
			while (true)
			{
				if (auto Arg = ParseExpression())
					Args.push_back(std::move(Arg));
				else
					return nullptr;
				//һ������������Ҫô��)��Ҫô�� ,
				if (CurTok == ')')
					break;

				if (CurTok != ',')
					return LogError("Expected ')' or ',' in argument list");
				getNextToken();
			}
		}
		// Eat the ')'.
		getNextToken();
		return std::make_unique<CallExprAST>(IdName, std::move(Args));
	}
	else if (CurTok == '[')
	{
		// CallArray.
		getNextToken(); // eat [
		std::unique_ptr<ExprAST> index;
		if (CurTok != ']')
		{
			index = ParseExpression(); // Ҫ�иĽ�
		}
		else
		{
			return LogError("Expected index between [ ]");
		}
		// Eat the ']'.
		getNextToken();
		//if (CurTok != '=')
		return std::make_unique<CallArrayAST>(IdName, std::move(index));
		//else
		//	return ParseInitArrayExpr(std::make_unique<CallArrayAST>(IdName, std::move(index)));
	}
	
}

static std::unique_ptr<ExprAST> ParseBody(){
	fprintf(stderr,"ooooo3\n");
	auto bodyE = ParseExpression();
	fprintf(stderr,"ooooo4\n");
	//bodyE->codegen();
	//fprintf(stderr,"ooooo44\n");
	if(bodyE->getType()== ASTType::body){
		fprintf(stderr,"ooooo5\n");
		return move(bodyE);
	}else{
		fprintf(stderr,"ooooo6\n");
		std::vector<std::unique_ptr<ExprAST>> b;
		b.push_back(move(bodyE));
		fprintf(stderr,"ooooo7\n");
		std::unique_ptr<ExprAST> returnE;
		fprintf(stderr,"ooooo8\n");
		return std::make_unique<BodyAST>(move(returnE),move(b));
	}
}

/// ifexpr ::= 'if' expression 'then' expression 'else' expression
static std::unique_ptr<ExprAST> ParseIfExpr()
{
	getNextToken(); // eat the if.

	// condition.
	auto Cond = ParseExpression();
	if (!Cond)
		return nullptr;

	if (CurTok != tok_then){
		return LogError("expected then");
	}
	getNextToken(); // eat the then

	auto Then = ParseBody();
	if (!Then)
		return nullptr;

	if (CurTok != tok_else)
		return LogError("expected else");

	getNextToken();

	auto Else = ParseBody();
	if (!Else)
		return nullptr;
	return std::make_unique<IfExprAST>(std::move(Cond), std::move(Then), std::move(Else));
}

/// forexpr ::= 'for' identifier '=' expr ',' expr (',' expr)? 'in' expression
/// for i = 1, i < n, 1.0 in
///    putchard(42);
static std::unique_ptr<ExprAST> ParseForExpr()
{
	getNextToken(); // eat the for.

	if (CurTok != tok_identifier)
		return LogError("expected identifier after for");

	std::string IdName = IdentifierStr;
	getNextToken(); // eat identifier.

	if (CurTok != '=')
		return LogError("expected '=' after for");
	getNextToken(); // eat '='.

	auto Start = ParseExpression();
	if (!Start)
		return nullptr;
	if (CurTok != ',')
		return LogError("expected ',' after for start value");
	getNextToken(); //eat ','

	auto End = ParseExpression();
	if (!End)
		return nullptr;

	// The step value is optional.
	std::unique_ptr<ExprAST> Step;
	if (CurTok == ',')
	{
		getNextToken();
		Step = ParseExpression();
		if (!Step)
			return nullptr;
	}

	if (CurTok != tok_in)
		return LogError("expected 'in' after for");
	getNextToken(); // eat 'in'.

	auto Body = ParseBody();
	if (!Body)
		return nullptr;

	std::unique_ptr<ForExprAST> forE = std::make_unique<ForExprAST>(IdName, std::move(Start), std::move(End),
										std::move(Step), std::move(Body));
	return move(forE);
}

/// unary
///   ::= primary
///   ::= '!' unary
static std::unique_ptr<ExprAST> ParseUnary()
{

	// If the current token is not an operator, it must be a primary expr.
	if (!isascii(CurTok) || CurTok == '(' || CurTok == ',' || CurTok=='{') // ( �������ŵı���ʽ���� , ��ɶ��
	{	
		fprintf(stderr,"ppppp5\n");
		return ParsePrimary();
	}
	// If this is a unary operator, read it.
	int Opc = CurTok;
	getNextToken();
	if (auto Operand = ParseUnary()){
		fprintf(stderr,"ppppp5\n");
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

static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS)
{
	// If this is a binop, find its precedence.
	while (true)
	{
		int TokPrec = GetTokPrecedence();

		// If this is a binop that binds at least as tightly as the current binop,
		// consume it, otherwise we are done.
		if (TokPrec < ExprPrec)
			return std::move(LHS);

		// Okay, we know this is a binop.
		int BinOp = CurTok;
		getNextToken(); // eat binop

		// Parse the unary expression after the binary operator.
		auto RHS = ParseUnary();
		if (!RHS)
			return nullptr;

		// If BinOp binds less tightly with RHS than the operator after RHS, let
		// the pending operator take RHS as its LHS.
		int NextPrec = GetTokPrecedence();
		if (TokPrec < NextPrec)
		{
			RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
			if (!RHS)
				return nullptr;
		}

		// Merge LHS/RHS.
		LHS = std::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
	}
}

/// expression
///   ::= unary binoprhs
static std::unique_ptr<ExprAST> ParseExpression()
{
	auto LHS = ParseUnary();
	if (!LHS)
		return nullptr;

	return ParseBinOpRHS(0, std::move(LHS));
}

/// prototype
///   ::= id '(' id* ')'
///   ::= binary LETTER number? (id, id)
///   ::= unary LETTER (id)
std::unique_ptr<PrototypeAST> ParsePrototype(){ 
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
std::unique_ptr<FunctionAST> ParseDefinition()
{
	getNextToken(); // eat def.
	fprintf(stderr,"ooooo1\n");
    auto Proto = ParsePrototype();
	if (!Proto)
		return nullptr;
	fprintf(stderr,"ooooo2\n");
	auto bodyE = ParseBody();
	if(!bodyE){
		return nullptr;
	}
	fprintf(stderr,"ooooo3\n");
	return move(std::make_unique<FunctionAST>(move(Proto),move(bodyE)));
}

/// toplevelexpr ::= expression
std::unique_ptr<FunctionAST> ParseTopLevelExpr()
{
	fprintf(stderr,"ppppp3\n");
	if (auto E = ParseExpression())
	{
		fprintf(stderr,"ppppp4\n");
		// Make an anonymous proto.
		auto Proto = std::make_unique<PrototypeAST>("__anon_expr", std::vector<std::string>(),doubleR);
		fprintf(stderr,"ppppp5\n");
		std::vector<std::unique_ptr<ExprAST>> bodyE;
		fprintf(stderr,"ppppp6\n");
		bodyE.push_back(std::move(E));
		fprintf(stderr,"ppppp7\n");
		std::unique_ptr<ExprAST> returnE;
		fprintf(stderr,"ppppp8\n");
		std::unique_ptr<BodyAST> bodys = std::make_unique<BodyAST>(move(returnE),move(bodyE));
		fprintf(stderr,"ppppp9\n");
		return std::make_unique<FunctionAST>(std::move(Proto), std::move(bodys));
	}
	return nullptr;
}

/// external ::= 'extern' prototype
std::unique_ptr<PrototypeAST> ParseExtern()
{
	getNextToken(); // eat extern.
	return ParsePrototype();
}