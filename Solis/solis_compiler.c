

#include "solis_compiler.h"

#include "solis_scanner.h"
#include <stdio.h>
#include <stdint.h>

#include "solis_hashtable.h"

#include <string.h>


// Forward declare some functions

SOLIS_DECLARE_BUFFER(Int, int);


SOLIS_DEFINE_BUFFER(Int, int);



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

typedef void (*ParseFn)(bool canAssign);

typedef struct {
	ParseFn prefix;
	ParseFn infix;
	Precedence precedence;
} ParseRule;



static void grouping(bool canAssign);
static void number(bool canAssign);
static void expression();
static void unary(bool canAssign);

static void binary(bool canAssign);
static void literal(bool canAssign);
static void string(bool canAssign);

static void declaration();
static void statement();

// Statements
static void expressionStatement();
static void variableDeclaration();
static void constDeclaration();

static void ifStatement();
static void whileStatement();
static void breakStatement();

static void and_(bool canAssign);
static void or_(bool canAssign);

static void variable(bool canAssign);

static void block();

static void parsePrecedence(Precedence precedence);
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
  [TOKEN_IDENTIFIER] = {variable,     NULL,   PREC_NONE},
  [TOKEN_STRING] = {string,     NULL,   PREC_NONE},
  [TOKEN_NUMBER] = {number,   NULL,   PREC_NONE},
  [TOKEN_AND] = {NULL,     and_,   PREC_AND},
  // [TOKEN_CLASS] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE] = {literal,     NULL,   PREC_NONE},
  [TOKEN_FOR] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUNCTION] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NULL] = {literal,     NULL,   PREC_NONE},
  [TOKEN_OR] = {NULL,     or_,   PREC_OR},
  // [TOKEN_PRINT] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RETURN] = {NULL,     NULL,   PREC_NONE},
  // [TOKEN_SUPER] = {NULL,     NULL,   PREC_NONE},
  //[TOKEN_THIS] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_TRUE] = {literal,     NULL,   PREC_NONE},
  [TOKEN_VAR] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_WHILE] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ERROR] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_BREAK] = {NULL,     NULL,   PREC_NONE},
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

static bool check(TokenType type)
{
	return parser.current.type == type;
}

static bool match(TokenType type) 
{
	if (!check(type)) return false;
	advance();
	return true;
}

typedef struct
{
	Token name;
	int depth;
} Local;


struct sCompiler 
{
	struct sCompiler* parent;

	// this is the VM calling the compiler
	// This is needed for allocations
	VM* vm;

	Chunk* chunk;

	int scopeDepth;

	// Each global variable is assigned an index as it is created 
	HashTable globalTable;
	int globalCount;


	Local locals[UINT8_COUNT];
	int localCount;

	// We store a list of current break statements
	IntBuffer breakStatements;
	bool withinLoop;
};

Compiler* current = NULL;


static Chunk* currentChunk()
{
	return current->chunk;
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

static void initCompiler(VM* vm, Chunk* chunk, Compiler* compiler)
{
	compiler->scopeDepth = 0;
	compiler->localCount = 0;
	compiler->chunk = chunk;

	compiler->vm = vm;
	compiler->globalCount = 0;
	compiler->withinLoop = false;

	solisIntBufferInit(&compiler->breakStatements);

	solisInitHashTable(&compiler->globalTable);

	current = compiler;

}

static void endCompiler(Compiler* compiler)
{
	emitReturn();

	solisFreeHashTable(&compiler->globalTable);
	solisIntBufferClear(&compiler->breakStatements);
}

static void synchronize() {
	parser.panicMode = false;

	while (parser.current.type != TOKEN_EOF) {
		if (parser.previous.type == TOKEN_SEMICOLON) return;
		switch (parser.current.type) {
		//case TOKEN_CLASS:
		case TOKEN_FUNCTION:
		case TOKEN_VAR:
		case TOKEN_FOR:
		case TOKEN_IF:
		case TOKEN_WHILE:
		case TOKEN_RETURN:
			return;

		default:
			; // Do nothing.
		}

		advance();
	}
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

static void beginScope() 
{
	current->scopeDepth++;
}

static void endScope() 
{
	current->scopeDepth--;

	while (current->localCount > 0 && current->locals[current->localCount - 1].depth > current->scopeDepth) 
	{
		emitByte(OP_POP);
		current->localCount--;
	}
}

static void grouping(bool canAssign) {
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number(bool canAssign) {
	double value = strtod(parser.previous.start, NULL);
	emitConstant(SOLIS_NUMERIC_VALUE(value));
}

static void unary(bool canAssign) {
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

static void binary(bool canAssign)
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

static void literal(bool canAssign) {
	switch (parser.previous.type) {
	case TOKEN_FALSE: emitByte(OP_FALSE); break;
	case TOKEN_NULL: emitByte(OP_NIL); break;
	case TOKEN_TRUE: emitByte(OP_TRUE); break;
	default: return; // Unreachable.
	}
}

static void string(bool canAssign) {
	emitConstant(SOLIS_OBJECT_VALUE(solisCopyString(current->vm, parser.previous.start + 1, parser.previous.length - 2)));
}

static void declaration()
{
	if (match(TOKEN_VAR))
	{
		variableDeclaration();
	}
	else
	{
		statement();
	}

	// If we have an error ignore this statement to avoid unnecessary errors being printed
	if (parser.panicMode) synchronize();
}

static void statement()
{
	if (match(TOKEN_DO))
	{
		beginScope();
		block();
		endScope();
		}
	else if (match(TOKEN_IF))
	{
		ifStatement();
	}
	else if (match(TOKEN_WHILE))
	{
		whileStatement();
	}
	else if (match(TOKEN_BREAK))
	{
		breakStatement();
	}
	else if (match(TOKEN_END))
	{
		if (current->scopeDepth - 1 <= -1)
		{
			error("Mismatched 'end' statements.");
		}

		endScope();
	}
	else
	{
		expressionStatement();
	}
}

static void expressionStatement()
{
	expression();
	consume(TOKEN_SEMICOLON, "Expected ';' after expression.");
	emitByte(OP_POP);
}

static uint16_t identifierConstant(Token* name) 
{
	return makeConstant(SOLIS_OBJECT_VALUE(solisCopyString(current->vm, name->start, name->length)));
}

static void addLocal(Token name) 
{
	if (current->localCount == UINT8_COUNT) 
	{
		error("Too many local variables in function.");
		return;
	}

	Local* local = &current->locals[current->localCount++];
	local->name = name;
	local->depth = -1;
}

static bool identifiersEqual(Token* a, Token* b) 
{
	if (a->length != b->length) return false;
	return memcmp(a->start, b->start, a->length) == 0;
}

static void declareVariable() 
{
	if (current->scopeDepth == 0) return;

	Token* name = &parser.previous;

	for (int i = current->localCount - 1; i >= 0; i--) 
	{
		Local* local = &current->locals[i];
		if (local->depth != -1 && local->depth < current->scopeDepth) 
		{
			break;
		}

		if (identifiersEqual(name, &local->name)) {
			error("Already a variable with this name in this scope.");
		}
	}

	addLocal(*name);
}

static uint16_t parseVariable(const char* errorMessage) {
	consume(TOKEN_IDENTIFIER, errorMessage);

	declareVariable();
	if (current->scopeDepth > 0) return 0;

	return identifierConstant(&parser.previous);
}

static void markInitialized()
{
	current->locals[current->localCount - 1].depth =
		current->scopeDepth;
}

static void defineVariable(uint16_t global) 
{
	// If we are in a local scope return 
	if (current->scopeDepth > 0) {
		markInitialized();
		return;
	}

	// Add the global to the hash table of globals
	double index = (double)current->globalCount;
	solisHashTableInsert(&current->globalTable, SOLIS_AS_STRING(current->chunk->constants.data[global]), SOLIS_NUMERIC_VALUE(index));
	current->globalCount++;

	emitByte(OP_DEFINE_GLOBAL);
	emitShort(global);
}

static void variableDeclaration()
{
	uint16_t global = parseVariable("Expected variable name.");

	if(match(TOKEN_EQ)) {
		expression();
	}
	else {
		emitByte(OP_NIL);
	}

	consume(TOKEN_SEMICOLON, "Expected ';' after variable declaration.");

	defineVariable(global);
}

static void constDeclaration()
{
	Token nameTk = parser.previous;
	ObjString* name = solisCopyString(current->vm, nameTk.start, nameTk.length);

	error("Constant variables are not currently supported");

	consume(TOKEN_EQ, "Constant variable declarations must be assigned a value.");

	// In theory we parse and evaluate this at compile time then store the constant in the constant table
	// Whenever this comes up in the compiler it emits an OP_CONSTANT with the value instead of a GET_* name 
	// TODO: Implement
	// Could this be implemented using an internal VM here? 

	// Just do this for now so we can still parse the rest 
	expression();

	consume(TOKEN_SEMICOLON, "Expected ';' at the end of constant variable declaration.");

}

// Resolve a global variable from the hash tables of globals
static int resolveGlobalVariable(Compiler* compiler, Token name)
{
	// Get the name
	ObjString* nameStr = solisCopyString(compiler->vm, name.start, name.length);

	Value val;
	if (!solisHashTableGet(&compiler->globalTable, nameStr, &val))
	{
		// Not found so return -1
		return -1;
	}

	// We should have an index here

	int idx = (int)SOLIS_AS_NUMBER(val);

	return idx;
}

static int resolveLocalVariable(Compiler* compiler, Token* name) 
{
	for (int i = compiler->localCount - 1; i >= 0; i--) 
	{
		Local* local = &compiler->locals[i];
		if (identifiersEqual(name, &local->name)) 
		{
			if (local->depth == -1) 
			{
				error("Can't read local variable in its own initializer.");
			}

			return i;
		}
	}

	return -1;
}

static void namedVariable(Token name, bool canAssign)
{
	uint8_t getOp, setOp;
	int arg = resolveLocalVariable(current, &name);
	if (arg != -1)
	{
		getOp = OP_GET_LOCAL;
		setOp = OP_SET_LOCAL;
	}
	else
	{
		// resolve the global
		arg = resolveGlobalVariable(current, name);
		if (arg == -1)
		{
			// If we reach here its not local or global
			error("Could not resolve variable.");
		}

		getOp = OP_GET_GLOBAL;
		setOp = OP_SET_GLOBAL;
	}

	if (match(TOKEN_EQ) && canAssign) 
	{
		expression();
		emitByte(setOp);
		emitShort((uint16_t)arg);
	}
	else 
	{
		emitByte(getOp);
		emitShort((uint16_t)arg);
	}


}

static void variable(bool canAssign)
{
	namedVariable(parser.previous, canAssign);
}

static void block()
{
	while (!check(TOKEN_END) && !check(TOKEN_EOF)) 
	{
		declaration();
	}

	consume(TOKEN_END, "Expected 'end' after block.");
}

static int emitJump(uint8_t instruction) 
{
	emitByte(instruction);
	emitByte(0xff);
	emitByte(0xff);
	return currentChunk()->count - 2;
}

static void patchJump(int offset) 
{
	// -2 to adjust for the bytecode for the jump offset itself.
	int jump = currentChunk()->count - offset - 2;

	if (jump > UINT16_MAX) {
		error("Too much code to jump over.");
	}

	currentChunk()->code[offset] = (jump >> 8) & 0xff;
	currentChunk()->code[offset + 1] = jump & 0xff;
}

static void emitLoop(int loopStart) {
	emitByte(OP_LOOP);

	int offset = currentChunk()->count - loopStart + 2;
	if (offset > UINT16_MAX) error("Loop body too large.");

	emitByte((offset >> 8) & 0xff);
	emitByte(offset & 0xff);
}

static void ifStatement()
{
	expression();

	consume(TOKEN_THEN, "Expected 'then' after if condition");

	// Begin a scope
	// Scopes are handled by if statements here 
	beginScope();

	int thenJump = emitJump(OP_JUMP_IF_FALSE);

	emitByte(OP_POP);

	// We can't do block here since we do all the consuming manually 
	while (!check(TOKEN_END) && !check(TOKEN_EOF) && !check(TOKEN_ELSE))
	{
		declaration();
	}

	int elseJump = emitJump(OP_JUMP);

	patchJump(thenJump);
	emitByte(OP_POP);

	endScope();

	if (match(TOKEN_ELSE))
	{
		beginScope();

		while (!check(TOKEN_END) && !check(TOKEN_EOF))
		{
			declaration();
		}

		consume(TOKEN_END, "Expected 'end' after else block.");

		endScope();
	}
	else
	{
		// if there is no else we need an end 
		consume(TOKEN_END, "Expected 'end' after if block.");
	}

	patchJump(elseJump);
}

static void whileStatement()
{
	int loopStart = currentChunk()->count;

	expression();

	consume(TOKEN_DO, "Expected 'do' after while expression.");

	current->withinLoop = true;
	beginScope();

	int exitJump = emitJump(OP_JUMP_IF_FALSE);
	emitByte(OP_POP);

	block();

	endScope();

	emitLoop(loopStart);

	patchJump(exitJump);

	emitByte(OP_POP);

	for (int i = 0; i < current->breakStatements.count; i++)
	{
		// Patch all break the jumps
		int offset = current->breakStatements.data[i];
		patchJump(offset);
	}

	// Clear the buffer for the next loop
	solisIntBufferClear(&current->breakStatements);

	
	current->withinLoop = false;
	
}

static void breakStatement()
{
	/*if (!current->withinLoop)
	{
		error("Cannot 'break' when not within a loop.");
	}*/

	consume(TOKEN_SEMICOLON, "Expected ';' after break statement.");

	int exitJump = emitJump(OP_JUMP);

	// Write it into the int buffer
	solisIntBufferWrite(&current->breakStatements, exitJump);
}

static void and_(bool canAssign)
{
	int endJump = emitJump(OP_JUMP_IF_FALSE);

	emitByte(OP_POP);
	parsePrecedence(PREC_AND);

	patchJump(endJump);
}

static void or_(bool canAssign)
{
	int elseJump = emitJump(OP_JUMP_IF_FALSE);
	int endJump = emitJump(OP_JUMP);

	patchJump(elseJump);
	emitByte(OP_POP);

	parsePrecedence(PREC_OR);
	patchJump(endJump);
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

	bool canAssign = precedence <= PREC_ASSIGNMENT;
	prefixRule(canAssign);

	while (precedence <= getRule(parser.current.type)->precedence) {
		advance();
		ParseFn infixRule = getRule(parser.previous.type)->infix;
		infixRule(canAssign);
	}

	if (canAssign && match(TOKEN_EQ)) {
		error("Invalid assignment target.");
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

	Compiler compiler;
	initCompiler(vm, chunk, &compiler);

	TokenList tokenList = solisScanSource(source);

	parser.tokenList = tokenList;

	advance();
	
	while (!match(TOKEN_EOF))
	{
		declaration();
	}


	endCompiler(&compiler);
	solisFreeTokenList(&tokenList);
	return !parser.hadError;
}