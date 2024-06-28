

#include "solis_scanner.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

typedef struct
{
	const char* start;
	const char* current;

	int line;

} Scanner;

Scanner scanner;

typedef struct
{
	const char* iden; 
	size_t length;

	TokenType type;

} Keyword;

// List of the language keywords
static Keyword keywords[] =
{
	{ "var", 3, TOKEN_VAR },
	{ "const", 5, TOKEN_CONST },

	{ "for", 3, TOKEN_FOR },
	{ "while", 5, TOKEN_WHILE },
	{ "if", 2, TOKEN_IF },
	{ "else", 4, TOKEN_ELSE },

	{ "do", 2, TOKEN_DO },
	{ "then", 4, TOKEN_THEN },
	{ "end", 3, TOKEN_END },
	{ "break", 5, TOKEN_BREAK },

	{ "in", 2, TOKEN_IN },
	{ "and", 3, TOKEN_AND },
	{ "or", 2, TOKEN_OR },
	{ "is", 2, TOKEN_IS },

	{ "null", 4, TOKEN_NULL },
	{ "true", 4, TOKEN_TRUE }, 
	{ "false", 5, TOKEN_FALSE },

	{ "return", 6, TOKEN_FALSE },

	{ "function", 8, TOKEN_FUNCTION }, 

	{ NULL, 0, TOKEN_EOF }
};


static bool isName(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool isDigit(char c)
{
	return c >= '0' && c <= '9';
}

static bool isAtEnd() 
{
	return *scanner.current == '\0';
}

static Token makeToken(TokenType type)
{
	Token tk;
	tk.type = type;
	tk.start = scanner.start;
	tk.length = (int)(scanner.current - scanner.start);
	tk.line = scanner.line;
	return tk;
}

static Token errorToken(const char* message) 
{
	Token tk;
	tk.type = TOKEN_ERROR;
	tk.start = message;
	tk.length = (int)strlen(message);
	tk.line = scanner.line;
	return tk;
}

// Some utility functions

// Advance to the next char in the source code
static char advance() 
{
	scanner.current++;
	return scanner.current[-1];
}

static bool match(char expected) {
	if (isAtEnd()) return false;
	if (*scanner.current != expected) return false;
	scanner.current++;
	return true;
}

static char peek() {
	return *scanner.current;
}

static void skipWhitespace() {
	for (;;) {
		char c = peek();
		/*switch (c) {
		case ' ':
		case '\r':
		case '\t':
			advance();
			break;
		case '\n':
			scanner.line++;
			advance();
		default:
			return;
		}*/

		if (c == ' ' || c == '\r' || c == '\t')
		{
			advance();
		}
		else if (c == '\n')
		{
			scanner.line++;
			advance();
		}
		else
		{
			return;
		}
	}
}


static char peekNext() {
	if (isAtEnd()) return '\0';
	return scanner.current[1];
}

static Token string() 
{
	while (peek() != '"' && !isAtEnd()) {
		if (peek() == '\n') scanner.line++;
		advance();
	}

	if (isAtEnd()) return errorToken("Unterminated string.");

	// The closing quote.
	advance();
	return makeToken(TOKEN_STRING);
}

static Token number()
{
	while (isDigit(peek())) advance();

	if (peek() == '.' && isDigit(peekNext()))
	{
		advance();
		while (isDigit(peek())) advance();
	}
	
	return makeToken(TOKEN_NUMBER);
}



static TokenType identifierType() {

	size_t length = scanner.current - scanner.start;

	// Match the keyword against the keywords list
	for (int i = 0; keywords[i].iden != NULL; i++)
		if (length == keywords[i].length && memcmp(scanner.start, keywords[i].iden, length) == 0)
			return keywords[i].type;

	return TOKEN_IDENTIFIER;
}

static Token identifier()
{
	while(isName(peek()) || isDigit(peek())) advance();
	return makeToken(identifierType());
}

// ----

void solisInitScanner(const char* sourceCode)
{
	scanner.start = sourceCode;
	scanner.current = sourceCode;
	scanner.line = 1;
}

Token solisScanToken()
{
	skipWhitespace();

	scanner.start = scanner.current;

	if (isAtEnd())
		return makeToken(TOKEN_EOF);

	char c = advance();


	// Test if its a number
	if (isDigit(c)) 
		return number();

	if (isName(c))
		return identifier();

	switch (c)
	{
	case '(': return makeToken(TOKEN_LEFT_PAREN);
	case ')': return makeToken(TOKEN_RIGHT_PAREN);
	case '[': return makeToken(TOKEN_LEFT_BRACKET);
	case ']': return makeToken(TOKEN_RIGHT_BRACKET);
	case '{': return makeToken(TOKEN_LEFT_BRACE);
	case '}': return makeToken(TOKEN_RIGHT_BRACE);
	case '+': return makeToken(TOKEN_PLUS);
	case '-':
	{
		// -- is a comment
		if (peekNext() == '-')
		{
			while (peek() != '\n' && !isAtEnd()) advance();
		}
		else
			return makeToken(TOKEN_MINUS);
	}
	case '/': return makeToken(TOKEN_SLASH);
	case '*': return makeToken(match('*') ? TOKEN_STAR_STAR : TOKEN_STAR);
	case '.': return makeToken(match('.') ? TOKEN_DOT_DOT : TOKEN_DOT);
	case ',': return makeToken(TOKEN_COMMA);
	case '!': return makeToken(match('=') ? TOKEN_BANGEQ : TOKEN_BANG);
	case '=': return makeToken(match('=') ? TOKEN_EQEQ : TOKEN_EQ);
	case ';': return makeToken(TOKEN_SEMICOLON);
	case '<': return makeToken(match('=') ? TOKEN_LTEQ : TOKEN_LT);
	case '>': return makeToken(match('=') ? TOKEN_GTEQ : TOKEN_GT);
	case '"': return string();
	}

	// Unexpected character. Let the compiler handle it
	return errorToken("Unexpected character.");
}


TokenList solisScanSource(const char* source)
{
	solisInitScanner(source);

	TokenList list;
	memset(&list, 0, sizeof(TokenList));

	// Just loop until the EOF and combine the tokens into a list
	for (;;)
	{
		Token tk = solisScanToken();
		
		list.count++;

		list.tokens = (Token*)solisReallocate(list.tokens, sizeof(Token) * list.count - 1, sizeof(Token) * list.count);

		// Check if its been allocated 
		SOLIS_ASSERT(list.tokens);

		list.tokens[list.count - 1] = tk;

		if (tk.type == TOKEN_EOF)
			break;
	}

	return list;
}

void solisPrintTokenList(TokenList* list)
{
	int line = -1;
	for (int i = 0; i < list->count; i++)
	{
		Token token = list->tokens[i];

		if (token.line != line) {
			printf("%4d ", token.line);
			line = token.line;
		}
		else {
			printf("   | ");
		}

		printf("%2d '%.*s'\n", token.type, token.length, token.start);

		if (token.type == TOKEN_EOF) break;
	}
}