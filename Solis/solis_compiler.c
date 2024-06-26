
#include "solis_compiler.h"

#include "solis_scanner.h"
#include <stdio.h>
#include <stdint.h>


// Forward declare some functions



typedef enum {
	PREC_NONE,
	PREC_ASSIGNMENT,  // =
	PREC_OR,          // or
	PREC_AND,         // and
	PREC_EQUALITY,    // == !=
	PREC_COMPARISON,  // < > <= >=
	PREC_TERM,        // + -
	PREC_FACTOR,      // * /
	PREC_POWER,		  // ** 
	PREC_UNARY,       // ! -
	PREC_CALL,        // . ()
	PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)();

typedef struct {
	ParseFn prefix;
	ParseFn infix;
	Precedence precedence;
} ParseRule;



static void grouping();
static void number();
static void expression();
static void unary();
static void parsePrecedence(Precedence precedence);
static void binary();
static void literal();
static void string();
static ParseRule* getRule(TokenType type);

ParseRule rules[] = {
  [TOKEN_LEFT_PAREN] = {grouping, NULL,   PREC_NONE},
  [TOKEN_RIGHT_PAREN] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LEFT_BRACE] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RIGHT_BRACE] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_COMMA] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_DOT] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_MINUS] = {unary,    binary, PREC_TERM},
  [TOKEN_PLUS] = {NULL,     binary, PREC_TERM},
  [TOKEN_SEMICOLON] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SLASH] = {NULL,     binary, PREC_FACTOR},
  [TOKEN_STAR] = {NULL,     binary, PREC_FACTOR},
  [TOKEN_STAR_STAR] = {NULL,     binary, PREC_POWER},
  [TOKEN_BANG] = {unary,     NULL,   PREC_NONE},
  [TOKEN_BANGEQ] = {NULL,     binary,   PREC_EQUALITY},
  [TOKEN_EQ] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EQEQ] = {NULL,     binary,   PREC_EQUALITY},
  [TOKEN_GT] = {NULL,     binary,   PREC_COMPARISON},
  [TOKEN_GTEQ] = {NULL,     binary,   PREC_COMPARISON},
  [TOKEN_LT] = {NULL,     binary,   PREC_COMPARISON},
  [TOKEN_LTEQ] = {NULL,     binary,   PREC_COMPARISON},
  [TOKEN_IDENTIFIER] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_STRING] = {string,     NULL,   PREC_NONE},
  [TOKEN_NUMBER] = {number,   NULL,   PREC_NONE},
  [TOKEN_AND] = {NULL,     NULL,   PREC_NONE},
  // [TOKEN_CLASS] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE] = {literal,     NULL,   PREC_NONE},
  [TOKEN_FOR] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUNCTION] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NULL] = {literal,     NULL,   PREC_NONE},
  [TOKEN_OR] = {NULL,     NULL,   PREC_NONE},
  // [TOKEN_PRINT] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RETURN] = {NULL,     NULL,   PREC_NONE},
  // [TOKEN_SUPER] = {NULL,     NULL,   PREC_NONE},
  //[TOKEN_THIS] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_TRUE] = {literal,     NULL,   PREC_NONE},
  [TOKEN_LET] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_WHILE] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ERROR] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EOF] = {NULL,     NULL,   PREC_NONE},
};

typedef struct
{
	// Store the list of tokens here
	TokenList tokenList;
	
	// Offset into token list we are currently at
	size_t tokenOffset;

	Token previous;
	Token current;

	bool hadError;
	bool panicMode;

} Parser;

Parser parser;

static void errorAt(Token* token, const char* message) 
{

	if (parser.panicMode) return;

	parser.panicMode = true;
	fprintf(stderr, "[line %d] Error", token->line);

	if (token->type == TOKEN_EOF) {
		fprintf(stderr, " at end");
	}
	else if (token->type == TOKEN_ERROR) {
		// Nothing.
	}
	else {
		fprintf(stderr, " at '%.*s'", token->length, token->start);
	}

	fprintf(stderr, ": %s\n", message);
	parser.hadError = true;
}

static void errorAtCurrent(const char* message) 
{
	errorAt(&parser.current, message);
}

static void error(const char* message) 
{
	errorAt(&parser.previous, message);
}

static void advance() 
{
	parser.previous = parser.current;

	for (;;) {
		if (parser.tokenOffset >= parser.tokenList.count)
		{
			// We shouldn't reach here
		}

		// Grab the next token
		parser.current = parser.tokenList.tokens[parser.tokenOffset++];

		if (parser.current.type != TOKEN_ERROR) break;
		
		// We have an error emitted from the scanner
		errorAtCurrent(parser.current.start);
		

	}
}

static void consume(TokenType type, const char* message) 
{
	if (parser.current.type == type) {
		advance();
		return;
	}

	errorAtCurrent(message);
}



struct sCompiler 
{
	struct sCompiler* parent;

	// this is the VM calling the compiler
	// This is needed for allocations
	VM* vm;

	Chunk* chunk;

	int scopeDepth;


};

Compiler compiler;



static Chunk* currentChunk()
{
	return compiler.chunk;
}

static void emitByte(uint8_t byte)
{
	solisWriteChunk(currentChunk(), byte);
}

static void emitBytes(uint8_t byte, uint8_t byte2)
{
	solisWriteChunk(currentChunk(), byte);
	solisWriteChunk(currentChunk(), byte2);
}

static void emitShort(uint16_t s)
{
	emitBytes((s >> 8) & 0xFF, s & 0xFF);
}

static void emitReturn() {
	emitByte(OP_RETURN);
}

static void initCompiler(VM* vm, Chunk* chunk)
{
	compiler.chunk = chunk;

	compiler.vm = vm;
}

static void endCompiler()
{
	emitReturn();
}




// Constant functions
static uint16_t makeConstant(Value value) {
	int constant = solisAddConstant(currentChunk(), value);

	return (uint16_t)constant;
}

static void emitConstant(Value value) {

	uint16_t constant = makeConstant(value);

	if (constant > 0xff)
	{
		emitByte(OP_CONSTANT_LONG);
		emitShort(constant);
		return;
	}
	emitBytes(OP_CONSTANT, (uint8_t)constant);
}


static void grouping() {
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number() {
	double value = strtod(parser.previous.start, NULL);
	emitConstant(SOLIS_NUMERIC_VALUE(value));
}

static void unary() {
	TokenType operatorType = parser.previous.type;

	// Compile the operand.
	parsePrecedence(PREC_UNARY);

	// Emit the operator instruction.
	switch (operatorType) {
	case TOKEN_MINUS: emitByte(OP_NEGATE); break;
	case TOKEN_BANG: emitByte(OP_NOT); break;
	default: return; 
	}
}

static void binary() 
{

	TokenType operatorType = parser.previous.type;
	ParseRule* rule = getRule(operatorType);

	parsePrecedence((Precedence)(rule->precedence + 1));

	switch (operatorType) {
	case TOKEN_PLUS:			emitByte(OP_ADD); break;
	case TOKEN_MINUS:			emitByte(OP_SUBTRACT); break;
	case TOKEN_STAR:			emitByte(OP_MULTIPLY); break;
	case TOKEN_SLASH:			emitByte(OP_DIVIDE); break;
	case TOKEN_STAR_STAR:		emitByte(OP_POWER); break;

	case TOKEN_EQEQ:			emitByte(OP_EQUAL); break;
	case TOKEN_BANGEQ:			emitBytes(OP_EQUAL, OP_NOT); break;
	case TOKEN_GT:				emitByte(OP_GREATER); break;
	case TOKEN_GTEQ:			emitBytes(OP_LESS, OP_NOT); break;
	case TOKEN_LT:				emitByte(OP_LESS); break;
	case TOKEN_LTEQ:			emitBytes(OP_GREATER, OP_NOT); break;
	default: return; // Unreachable.
	}
}

static void literal() {
	switch (parser.previous.type) {
	case TOKEN_FALSE: emitByte(OP_FALSE); break;
	case TOKEN_NULL: emitByte(OP_NIL); break;
	case TOKEN_TRUE: emitByte(OP_TRUE); break;
	default: return; // Unreachable.
	}
}

static void string() {
	emitConstant(SOLIS_OBJECT_VALUE(solisCopyString(compiler.vm, parser.previous.start + 1, parser.previous.length - 2)));
}

static void expression()
{
	parsePrecedence(PREC_ASSIGNMENT);
}

static void parsePrecedence(Precedence precedence)
{
	advance();
	ParseFn prefixRule = getRule(parser.previous.type)->prefix;
	if (prefixRule == NULL) {
		error("Expect expression.");
		return;
	}

	prefixRule();

	while (precedence <= getRule(parser.current.type)->precedence) {
		advance();
		ParseFn infixRule = getRule(parser.previous.type)->infix;
		infixRule();
	}
}

static ParseRule* getRule(TokenType type) {
	return &rules[type];
}

bool solisCompile(VM* vm, const char* source, Chunk* chunk)
{
	parser.hadError = false;
	parser.panicMode = false;
	parser.tokenOffset = 0;

	// Setup the compiler

	initCompiler(vm, chunk);

	TokenList tokenList = solisScanSource(source);

	parser.tokenList = tokenList;

	advance();
	expression();


	endCompiler();
	solisFreeTokenList(&tokenList);
	return !parser.hadError;
}