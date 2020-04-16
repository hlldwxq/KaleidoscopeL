#include "head.h"
#include "AST.h"
#include "Parser.h"

//===----------------------------------------------------------------------===//
// Lexer
//===----------------------------------------------------------------------===//

// The lexer returns tokens [0-255] if it is an unknown character, otherwise one
// of these for known things.

int fileIndex=0;
std::string IdentifierStr; // Filled in if tok_identifier
double NumVal;			  // Filled in if tok_number

int getChar(){

//	printint(fileIndex);
//	printwq("   size\n");
    if(fileIndex < fileStr.size()){
		
        int c = fileStr[fileIndex++];
        return c;
    }else{
        return EOF;
    }
}
/// gettok - Return the next token from standard input.
static int gettok()
{
	/*fprintf(stderr,"gettok\n");
	fprintf(stderr,"curtoken: %d \n",CurTok);*/
	static int LastChar = ' ';

	// Skip any whitespace.
	while (isspace(LastChar))
		LastChar = getChar(); // c language function

	// identifier: [a-zA-Z][a-zA-Z0-9]*
	if (isalpha(LastChar)){ //[a-zA-Z]
		IdentifierStr = LastChar;
		while (isalnum((LastChar = getChar()))) //isalnum [a-zA-Z0-9]
			IdentifierStr += LastChar;

		if (IdentifierStr == "def")
			return tok_def;
		if (IdentifierStr == "extern")
			return tok_extern;
		if (IdentifierStr == "if")
			return tok_if;
		if (IdentifierStr == "then")
			return tok_then;
		if (IdentifierStr == "else")
			return tok_else;
		if (IdentifierStr == "for")
			return tok_for;
		if (IdentifierStr == "in")
			return tok_in;
		if (IdentifierStr == "binary")
			return tok_binary;
		if (IdentifierStr == "unary")
			return tok_unary;
		if (IdentifierStr == "array")
			return tok_array;
		if (IdentifierStr == "var")
			return tok_var;
		if (IdentifierStr == "struct")
			return tok_struct;
        if(IdentifierStr == "double")
            return tok_double;
        if(IdentifierStr == "void")
            return tok_void;
        if(IdentifierStr == "return")
            return tok_return;
		if(IdentifierStr == "new")
			return tok_new;
		return tok_identifier;
	}

	if (isdigit(LastChar) || LastChar == '.')
	{ // Number: [0-9.]+
		int pointNum = 0;
		std::string NumStr;
		do
		{
			if (LastChar == '.' && pointNum == 0)
			{
				pointNum += 1;
			}
			else if (LastChar == '.' && pointNum > 0)
			{
				return lexerE;
			}
			NumStr += LastChar;
			LastChar = getChar();
		} while (isdigit(LastChar) || LastChar == '.');

		NumVal = strtod(NumStr.c_str(), nullptr); //cover string to number
		return Token::tok_number;
	}

	if (LastChar == '#')
	{
		// Comment until end of line.
		do
			LastChar = getChar();
		while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

		if (LastChar != EOF)
			return gettok();
	}

	// Check for end of file.  Do not eat the EOF.
	if (LastChar == EOF)
		return tok_eof;

	// Otherwise, just return the character as its ascii value.
	int ThisChar = LastChar;

	LastChar = getChar();
	return ThisChar;
}

int getNextToken() {   return CurTok = gettok(); }