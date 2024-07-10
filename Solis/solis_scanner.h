

#ifndef SOLIS_SCANNER_H
#define SOLIS_SCANNER_H

#include "solis_common.h"

typedef enum SolisTokenType
{
	TOKEN_IDENTIFIER, 

	TOKEN_LEFT_PAREN,
	TOKEN_RIGHT_PAREN, 

	TOKEN_LEFT_BRACKET, 
	TOKEN_RIGHT_BRACKET, 

	TOKEN_LEFT_BRACE,
	TOKEN_RIGHT_BRACE, 

	TOKEN_PLUS,
	TOKEN_MINUS,

	TOKEN_STAR,
	TOKEN_STAR_STAR,

	TOKEN_SLASH,
	TOKEN_SLASH_SLASH, 

	TOKEN_DOT,
	TOKEN_DOT_DOT,

	TOKEN_COMMA, 

	TOKEN_EQ,
	TOKEN_EQEQ, 
	TOKEN_LT,
	TOKEN_GT,
	TOKEN_LTEQ,
	TOKEN_GTEQ, 

	TOKEN_BANG,
	TOKEN_BANGEQ, 

	TOKEN_SEMICOLON, 

	TOKEN_VAR,
	TOKEN_CONST, 
	TOKEN_FUNCTION,

	TOKEN_DO,
	TOKEN_THEN,
	TOKEN_END, 
	TOKEN_BREAK, 

	// Control flow statements
	TOKEN_FOR,
	TOKEN_WHILE,
	TOKEN_IF,
	TOKEN_ELSE,

	// Comparison
	TOKEN_IN,
	TOKEN_AND,
	TOKEN_OR,

	TOKEN_IS,
	TOKEN_AS, 

	TOKEN_NULL,
	TOKEN_TRUE,
	TOKEN_FALSE,

	TOKEN_ENUM, 
	TOKEN_CLASS,
	TOKEN_INHERITS, 
	TOKEN_STATIC, 
	TOKEN_SELF,

	TOKEN_STRING,

	TOKEN_NUMBER,

	TOKEN_RETURN,

	TOKEN_IMPORT, 

	TOKEN_LINE, 
	TOKEN_EOF,

	TOKEN_ERROR, 
} SolisTokenType;

typedef struct Token
{
	SolisTokenType type;

	const char* start;
	int length;

	// The line where the token is in the source string
	int line;


} Token;

/*
	Initialises the scanner to be ready for scanning
*/
void solisInitScanner(const char* sourceCode);

/*
	Grab the next token in the source.
	Do not call if you have used solisScanSource
*/
Token solisScanToken();

typedef struct
{

	Token* tokens;
	size_t count;

} TokenList;

/*
	Returns the fill list of tokens from a source.

	Must call solisFreeTokenList when done with the token list. 
*/
TokenList solisScanSource(VM* vm, const char* source);

/*
	Free a token list allocation
*/
static inline void solisFreeTokenList(TokenList* list)
{
	SOLIS_FREE_FUNC(list->tokens);
	list->count = 0;
}


void solisPrintTokenList(TokenList* list);


#endif // SOLIS_SCANNER_H