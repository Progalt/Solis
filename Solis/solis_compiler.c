

#include "solis_compiler.h"

#include "solis_scanner.h"
#include <stdio.h>
#include <stdint.h>

#include "solis_hashtable.h"

#include <string.h>

#include "solis_object.h"
#include "solis_chunk.h"

#include "solis_gc.h"

#include "solis_vm.h"
#include "terminal.h"

typedef struct Upvalue
{
	uint8_t index;
	bool isLocal;
} Upvalue;


// Forward declare some functions

SOLIS_DECLARE_BUFFER(Int, int);


SOLIS_DEFINE_BUFFER(Int, int);



SOLIS_DECLARE_BUFFER(Upvalue, Upvalue);

SOLIS_DEFINE_BUFFER(Upvalue, Upvalue);


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
	PREC_SUBSCRIPT,	  // []
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
static void functionDeclaration();
static void enumDeclaration();
static void classDeclaration();

static void ifStatement();
static void whileStatement();
static void forStatement();
static void breakStatement();
static void returnStatement();

static void arrayCreate(bool canAssign);
static void arrayAssign(bool canAssign);


static void function(FunctionType type);

static void and_(bool canAssign);
static void or_(bool canAssign);

static void is_(bool canAssign);

static void dot(bool canAssign);

static void call(bool canAssign);

static void variable(bool canAssign);

static void self(bool canAssign);

static void block();

static void parsePrecedence(Precedence precedence);
static ParseRule* getRule(SolisTokenType type);

ParseRule rules[] = {
  [TOKEN_LEFT_PAREN] = {grouping, call,   PREC_CALL},
  [TOKEN_RIGHT_PAREN] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LEFT_BRACE] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RIGHT_BRACE] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LEFT_BRACKET] = {arrayCreate,     arrayAssign,   PREC_SUBSCRIPT},
  [TOKEN_RIGHT_BRACKET] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_COMMA] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_DOT] = {NULL,     dot,   PREC_CALL},
  [TOKEN_MINUS] = {unary,    binary, PREC_TERM},
  [TOKEN_PLUS] = {NULL,     binary, PREC_TERM},
  [TOKEN_SEMICOLON] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SLASH] = {NULL,     binary, PREC_FACTOR},
  [TOKEN_SLASH_SLASH] = {NULL,     binary, PREC_FACTOR},
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
  [TOKEN_CLASS] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE] = {literal,     NULL,   PREC_NONE},
  [TOKEN_FOR] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUNCTION] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NULL] = {literal,     NULL,   PREC_NONE},
  [TOKEN_OR] = {NULL,     or_,   PREC_OR},
  [TOKEN_RETURN] = {NULL,     NULL,   PREC_NONE},
  // [TOKEN_SUPER] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_TRUE] = {literal,     NULL,   PREC_NONE},
  [TOKEN_VAR] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_WHILE] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ERROR] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_BREAK] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IS] = { NULL, is_, PREC_CALL }, 
  [TOKEN_SELF] = { self, NULL, PREC_NONE },
  [TOKEN_DOT_DOT] = { NULL, binary, PREC_CALL },
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

	const char* source;
	const char* sourceName;

} Parser;

Parser parser;

void findLineIndices(const char* str, int lineNumber, int* startIndex, int* endIndex) {
	int currentLine = 0;
	int i = 0;

	*startIndex = -1;
	*endIndex = -1;

	while (str[i] != '\0') {
		if (currentLine == lineNumber) {
			if (*startIndex == -1) {
				*startIndex = i;
			}
			if (str[i] == '\n' || str[i + 1] == '\0') {
				*endIndex = (str[i] == '\n') ? i : i + 1;
				break;
			}
		}

		if (str[i] == '\n') {
			currentLine++;
		}

		i++;
	}

	if (*startIndex == -1 || *endIndex == -1) {
		printf("Line %d not found.\n", lineNumber);
	}
}


static void printSourceLine(FILE* const stream, const char* source, int line)
{
	int start = 0, end = 0;
	findLineIndices(source, line - 1, &start, &end);

	fprintf(stream, "%.*s\n", end - start, source + start);


}

static void errorAt(Token* token, const char* message) 
{

	if (parser.panicMode) return;

	parser.panicMode = true;
	//fprintf(stderr, "[line %d] Error", token->line);

	//if (token->type == TOKEN_EOF) {
	//	fprintf(stderr, " at end");
	//}
	//else if (token->type == TOKEN_ERROR) {
	//	// Nothing.
	//}
	//else {
	//	fprintf(stderr, " at '%.*s'", token->length, token->start);
	//}

	//fprintf(stderr, ": %s\n", message);
	parser.hadError = true;

	terminalPushForeground(TERMINAL_FG_RED);
	terminalPrintf("error");
	terminalPopStyle();
	if (token->type == TOKEN_ERROR)
		terminalPrintf(": % s\n", token->start);
	else 
		terminalPrintf(": % s\n", message);

	int lineStart, lineEnd;
	findLineIndices(parser.source, token->line - 1, &lineStart, &lineEnd);

	int tokenIndex = (int)(token->start - parser.source);

	int off = tokenIndex - lineStart;

	terminalPrintf("--> %s:%d:%d\n", parser.sourceName, token->line, off < 0 ? 0 : off);

	terminalPushForeground(TERMINAL_FG_BLUE);

	if (off < 0)
	{
		terminalPrintf("     |\n");
		terminalPrintf("%4d | ", token->line - 1);
		printSourceLine(stderr, parser.source, token->line - 1);
	}
	else
	{
		terminalPrintf("     |\n");
		terminalPrintf("%4d | ", token->line);
		printSourceLine(stderr, parser.source, token->line);
	}
	terminalPrintf("     | ");
	terminalPopStyle();

	// I want to print a little underline


	if (off >= 0)
	{
		for (int i = 0; i < off; i++)
			printf(" ");

		terminalPushForeground(TERMINAL_FG_RED);
		for (int i = 0; i < token->length; i++)
			terminalPrintf("^");

		terminalPopStyle();
	}
	else
	{
		findLineIndices(parser.source, token->line - 2, &lineStart, &lineEnd);

		int offset = lineEnd - lineStart;
		for (int i = 0; i < offset - 1; i++)
			printf(" ");

		terminalPushForeground(TERMINAL_FG_RED);
		for (int i = 0; i < 2; i++)
			terminalPrintf("^");

		terminalPopStyle();
	}

	printf("\n");
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
			return;
		}

		// Grab the next token
		parser.current = parser.tokenList.tokens[parser.tokenOffset++];

		if (parser.current.type != TOKEN_ERROR) break;
		
		// We have an error emitted from the scanner
		errorAtCurrent(parser.current.start);
		

	}
}

static void consume(SolisTokenType type, const char* message)
{
	if (parser.current.type == type) {
		advance();
		return;
	}

	errorAtCurrent(message);
}

static bool check(SolisTokenType type)
{
	return parser.current.type == type;
}

static bool match(SolisTokenType type)
{
	if (!check(type)) return false;
	advance();
	return true;
}

static bool matchLine()
{
	if (!match(TOKEN_LINE)) return false;

	while (match(TOKEN_LINE));

	return true;
}

static void ignoreNewlines()
{
	matchLine();
}

static void consumeLine(const char* msg)
{
	if (check(TOKEN_EOF))
		return;

	consume(TOKEN_LINE, msg);
	ignoreNewlines();
}

typedef struct
{
	Token name;
	int depth;
	bool isCaptured;
} Local;


struct sCompiler 
{
	struct sCompiler* parent;

	// this is the VM calling the compiler
	// This is needed for allocations
	VM* vm;

	// Chunk* chunk;

	int scopeDepth;

	// Each global variable is assigned an index as it is created 
	HashTable globalTable;
	int globalCount;


	Local locals[UINT8_COUNT];
	int localCount;

	// We store a list of current break statements
	IntBuffer breakStatements;
	bool withinLoop;

	ObjFunction* function;
	FunctionType type;

	UpvalueBuffer upvalues;
};

Compiler* current = NULL;


static Chunk* currentChunk()
{
	return &current->function->chunk;
}

static void emitByte(uint8_t byte)
{
	solisWriteChunk(current->vm, currentChunk(), byte);
}

static void emitBytes(uint8_t byte, uint8_t byte2)
{
	solisWriteChunk(current->vm, currentChunk(), byte);
	solisWriteChunk(current->vm, currentChunk(), byte2);
}

static void emitShort(uint16_t s)
{
	emitBytes((s >> 8) & 0xFF, s & 0xFF);
}

static void emitReturn() {

	if (current->type == TYPE_CONSTRUCTOR)
	{
		emitByte(OP_GET_LOCAL);
		emitShort(0);
	}
	else 
		emitByte(OP_NIL);

	emitByte(OP_RETURN);
}

static void initCompiler(VM* vm, Compiler* compiler, FunctionType type)
{
	compiler->function = NULL;
	compiler->type = type;
	compiler->scopeDepth = 0;
	compiler->localCount = 0;
	compiler->parent = NULL;
	

	compiler->vm = vm;
	compiler->globalCount = 0;
	compiler->withinLoop = false;

	compiler->parent = current;

	compiler->function = solisNewFunction(vm);

	solisIntBufferInit(vm, &compiler->breakStatements);

	solisInitHashTable(&compiler->globalTable, vm);

	solisUpvalueBufferInit(vm, &compiler->upvalues);

	current = compiler;

	if (type != TYPE_SCRIPT) 
	{
		current->function->name = solisCopyString(vm, parser.previous.start,
			parser.previous.length);
	}

	Local* local = &current->locals[current->localCount++];
	local->depth = 0;
	local->name.start = "";
	local->name.length = 0;
	local->isCaptured = false; 

	if (type != TYPE_FUNCTION) {
		local->name.start = "self";
		local->name.length = 4;
	}
	else {
		local->name.start = "";
		local->name.length = 0;
	}

}

static ObjFunction* endCompiler(Compiler* compiler)
{
	emitReturn();

	solisFreeHashTable(&compiler->globalTable);
	// solisUpvalueBufferClear(&compiler->upvalues);
	solisIntBufferClear(compiler->vm, &compiler->breakStatements);

	ObjFunction* function = current->function;

	current = compiler->parent;
	return function;
}

static void synchronize() {
	parser.panicMode = false;

	while (parser.current.type != TOKEN_EOF) {
		if (parser.previous.type == TOKEN_LINE) 
			return;
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
	int constant = solisAddConstant(current->vm, currentChunk(), value);

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
		if (current->locals[current->localCount - 1].isCaptured) 
		{
			emitByte(OP_CLOSE_UPVALUE);
		}
		else {
			emitByte(OP_POP);
		}
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
	SolisTokenType operatorType = parser.previous.type;

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

	SolisTokenType operatorType = parser.previous.type;
	ParseRule* rule = getRule(operatorType);

	parsePrecedence((Precedence)(rule->precedence + 1));

	switch (operatorType) {
	case TOKEN_PLUS:			emitByte(OP_ADD); break;
	case TOKEN_MINUS:			emitByte(OP_SUBTRACT); break;
	case TOKEN_STAR:			emitByte(OP_MULTIPLY); break;
	case TOKEN_SLASH:			emitByte(OP_DIVIDE); break;
	case TOKEN_STAR_STAR:		emitByte(OP_POWER); break;
	case TOKEN_SLASH_SLASH:		emitByte(OP_FLOOR_DIVIDE); break;

	case TOKEN_DOT_DOT:			emitByte(OP_DOTDOT); break;

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
	ignoreNewlines();

	// Return here if there is an EOF
	// Because we don't want to try and parse it
	if (check(TOKEN_EOF))
	{
		return;
	}

	if (match(TOKEN_FUNCTION))
	{
		functionDeclaration();
	}
	else if (match(TOKEN_CLASS))
	{
		classDeclaration();
	}
	else if (match(TOKEN_VAR))
	{
		variableDeclaration();
	}
	else if (match(TOKEN_ENUM))
	{
		enumDeclaration();
	}
	else
	{
		statement();
	}

	// consumeLine("Expected new line after statement.");

	// If we have an error ignore this statement to avoid unnecessary errors being printed
	if (parser.panicMode) synchronize();
}

static void statement()
{

	if (check(TOKEN_EOF))
	{
		return;
	}

	if (match(TOKEN_RETURN))
	{
		returnStatement();
	}
	else if (match(TOKEN_DO))
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
	else if (match(TOKEN_FOR))
	{
		forStatement();
	}
	else if (match(TOKEN_BREAK))
	{
		breakStatement();
	}
	else
	{
		expressionStatement();
	}

	consumeLine("Expected new line after expression");

}

static void expressionStatement()
{
	expression();
	// consume(TOKEN_SEMICOLON, "Expected ';' after expression.");
	emitByte(OP_POP);
}

static uint16_t identifierConstant(Token* name) 
{
	return makeConstant(SOLIS_OBJECT_VALUE(solisCopyString(current->vm, name->start, name->length)));
}

static int addLocal(Token name) 
{
	if (current->localCount == UINT8_COUNT) 
	{
		error("Too many local variables in (function).");
		return -1;
	}

	Local* local = &current->locals[current->localCount];
	local->name = name;
	local->depth = -1;
	local->isCaptured = false; 
	return current->localCount++;
}

static bool identifiersEqual(Token* a, Token* b) 
{
	if (a->length != b->length) return false;
	return memcmp(a->start, b->start, a->length) == 0;
}

static int declareVariable(Token* name) 
{
	if (current->scopeDepth == 0) return -1;


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

	return addLocal(*name);
}

static uint16_t parseVariable(const char* errorMessage) {
	consume(TOKEN_IDENTIFIER, errorMessage);

	declareVariable(&parser.previous);
	if (current->scopeDepth > 0) return 0;

	return identifierConstant(&parser.previous);
}

static void markInitialized()
{
	if (current->scopeDepth == 0) 
		return;

	current->locals[current->localCount - 1].depth =
		current->scopeDepth;
}

static void defineVariable(uint16_t global, bool addToGlobals) 
{


	// If we are in a local scope return 
	if (current->scopeDepth > 0) {
		markInitialized();
		return;
	}

	if (addToGlobals)
	{
		// Add the global to the hash table of globals
		double index = (double)current->globalCount;
		ObjString* str = SOLIS_AS_STRING(current->function->chunk.constants.data[global]);

		solisHashTableInsert(&current->globalTable, str, SOLIS_NUMERIC_VALUE(index));

		current->globalCount++;
	}

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

	consumeLine( "Expected new line after variable declaration.");

	defineVariable(global, true);

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

	consumeLine("Expected new line at the end of constant variable declaration.");

}

// Resolve a global variable from the hash tables of globals
static int resolveGlobalVariable(Compiler* compiler, Token name)
{
	// Get the name
	ObjString* nameStr = solisCopyString(compiler->vm, name.start, name.length);

	Value val;
	bool found = false;
	Compiler* globalCompiler = compiler;

	// NOTE: Keep an eye on this

	// Loop and find the compiler compiling the script
	// This compiler won't have a parent 
	while (globalCompiler->parent != NULL)
	{
		globalCompiler = globalCompiler->parent;
	}


	if (!solisHashTableGet(&globalCompiler->globalTable, nameStr, &val))
	{
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

static int addUpvalue(Compiler* compiler, uint8_t index, bool isLocal) 
{
	int upvalueCount = compiler->function->upvalueCount;
	for (int i = 0; i < upvalueCount; i++)
	{
		Upvalue* upvalue = &compiler->upvalues.data[i];
		if (upvalue->index == index && upvalue->isLocal == isLocal) 
			return i;
	}

	Upvalue upvalue;
	upvalue.index = index;
	upvalue.isLocal = isLocal;

	solisUpvalueBufferWrite(compiler->vm, &compiler->upvalues, upvalue);

	return compiler->function->upvalueCount++;
}

static int resolveUpvalue(Compiler* compiler, Token* name) 
{
	if (compiler->parent == NULL) return -1;

	int local = resolveLocalVariable(compiler->parent, name);
	if (local != -1) 
	{
		compiler->parent->locals[local].isCaptured = true;
		return addUpvalue(compiler, (uint8_t)local, true);
	}

	int upvalue = resolveUpvalue(compiler->parent, name);
	if (upvalue != -1) 
	{
		return addUpvalue(compiler, (uint8_t)upvalue, false);
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
	else if ((arg = resolveUpvalue(current, &name)) != -1)
	{
		getOp = OP_GET_UPVALUE;
		setOp = OP_SET_UPVALUE;
	}
	else
	{
		// resolve the global
		arg = resolveGlobalVariable(current, name);
		if (arg == -1)
		{
			// If we reach here its not local or global
			error("Could not resolve variable or not valid keyword.");
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

	ignoreNewlines();

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

		ignoreNewlines();

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
	solisIntBufferClear(current->vm, &current->breakStatements);

	
	current->withinLoop = false;
	
}

static void loadLocal(int slot)
{
	
	emitByte(OP_GET_LOCAL);
	emitShort(slot);
}

static void forStatement()
{
	beginScope();

	consume(TOKEN_IDENTIFIER, "Expected identifier in for loop.");

	const char* name = parser.previous.start;
	int length =  parser.previous.length;

	consume(TOKEN_IN, "Expected 'in' in for loop.");

	ignoreNewlines();

	expression();

	Token seqTk = {
		.start = "seq ",
		.length = 4
	};

	int seqSlot = addLocal(seqTk);

	emitByte(OP_NIL);

	Token iterTk = {
		.start = "iter ",
		.length = 5
	};
	int iterSlot = addLocal(iterTk);

	consume(TOKEN_DO, "Expected 'do' after for expression");


	current->withinLoop = true;
	int loopStart = currentChunk()->count;

	loadLocal(seqSlot);
	loadLocal(iterSlot);

	ObjString* iterValue = solisCopyString(current->vm, "iterate", 7);
	solisPush(current->vm, SOLIS_OBJECT_VALUE(iterValue));

	ObjString* iterValueValue = solisCopyString(current->vm, "iteratorValue", 13);
	solisPush(current->vm, SOLIS_OBJECT_VALUE(iterValueValue));

	uint16_t iterateMethod = makeConstant(SOLIS_OBJECT_VALUE(iterValue));
	uint16_t iteratorValueMethod = makeConstant(SOLIS_OBJECT_VALUE(iterValueValue));

	solisPop(current->vm);
	solisPop(current->vm);

	// Call iterate(x)
	emitByte(OP_INVOKE);
	emitShort(iterateMethod);
	emitByte(1);

	emitByte(OP_SET_LOCAL);
	emitShort(iterSlot);

	emitByte(OP_POP);
	
	// test if we should jump if its false
	int exitJump = emitJump(OP_JUMP_IF_FALSE);

	loadLocal(seqSlot);
	loadLocal(iterSlot);

	emitByte(OP_INVOKE);
	emitShort(iteratorValueMethod);
	emitByte(1);

	beginScope();

	Token localIter = {
		.start = name,
		.length = length
	};

	addLocal(localIter);
	markInitialized();

	block();

	endScope();

	emitLoop(loopStart);

	patchJump(exitJump);

	endScope();
}

static void breakStatement()
{
	// We only want to break in a loop
	// Error if we aren't in one
	if (!current->withinLoop)
	{
		error("Cannot 'break' when not within a loop.");
	}

	int exitJump = emitJump(OP_JUMP);

	// Write it into the int buffer
	solisIntBufferWrite(current->vm, &current->breakStatements, exitJump);
}

static void functionDeclaration()
{
	uint16_t global = parseVariable("Expect function name.");

	// Add the global to the hash table of globals
	// We need to do it ahead of time here to allow for recursion
	double index = (double)current->globalCount;
	solisHashTableInsert(&current->globalTable, SOLIS_AS_STRING(current->function->chunk.constants.data[global]), SOLIS_NUMERIC_VALUE(index));
	current->globalCount++;
	
	markInitialized();
	function(TYPE_FUNCTION);
	defineVariable(global, false);
}

static void function(FunctionType type)
{
	Compiler compiler;
	initCompiler(current->vm, &compiler, type);

	beginScope();

	consume(TOKEN_LEFT_PAREN, "Expected '(' after function name.");
	if (!check(TOKEN_RIGHT_PAREN)) 
	{
		do 
		{
			ignoreNewlines();

			current->function->arity++;
			if (current->function->arity > 255) 
			{
				errorAtCurrent("Can't have more than 255 parameters.");
			}
			uint16_t constant = parseVariable("Expect parameter name.");
			defineVariable(constant, true);
		} while (match(TOKEN_COMMA));
	}
	consume(TOKEN_RIGHT_PAREN, "Expected ')'  after function parameters.");

	ignoreNewlines();

	// TODO: Empty functions don't parse correctly
	block();

	ObjFunction* function = endCompiler(&compiler);

	// emitConstant(SOLIS_OBJECT_VALUE(function));

	// TODO: Only emit a closure when its not in global scope 

	emitByte(OP_CLOSURE);
	emitShort(makeConstant(SOLIS_OBJECT_VALUE(function)));

	// Handle the upvalues

	for (int i = 0; i < function->upvalueCount; i++)
	{
		emitByte(compiler.upvalues.data[i].isLocal ? 1 : 0);
		emitByte(compiler.upvalues.data[i].index);
	}

	// Free the upvalue memory 
	solisUpvalueBufferClear(compiler.vm, &compiler.upvalues);
}

static void enumDeclaration()
{
	// We have an enum

	uint16_t global = parseVariable("Expected identifier after enum declaration.");

	ObjEnum* enumObj = solisNewEnum(current->vm);

	int idx = 0;
	for(;;)
	{
		ignoreNewlines();

		consume(TOKEN_IDENTIFIER, "Expected an identifier in enum.");
		// printf("Enum: %.*s", parser.previous.length, parser.previous.start);

		ObjString* iden = solisCopyString(current->vm, parser.previous.start, parser.previous.length);

		solisHashTableInsert(&enumObj->fields, iden, SOLIS_NUMERIC_VALUE((double)idx));

		ignoreNewlines();

		if (match(TOKEN_END))
		{
			break;
		}
		else
		{
			consume(TOKEN_COMMA, "Expected ',' to seperate enum entries");
		}

		ignoreNewlines();

		idx++;
		enumObj->fieldCount++;
	}


	emitConstant(SOLIS_OBJECT_VALUE(enumObj));
	defineVariable(global, true);
}

static void method(bool isStatic, bool constructor)
{
	consume(TOKEN_IDENTIFIER, "Expect method name.");
	uint8_t constant = identifierConstant(&parser.previous);

	FunctionType type = TYPE_METHOD;

	if (constructor)
		type = TYPE_CONSTRUCTOR;
	function(type);

	if (!constructor)
		emitByte(isStatic ? OP_DEFINE_STATIC : OP_DEFINE_METHOD);
	else
		emitByte(OP_DEFINE_CONSTRUCTOR);

	emitShort(constant);
}

static void classDeclaration()
{
	consume(TOKEN_IDENTIFIER, "Expect class name.");
	Token className = parser.previous;
	uint16_t nameConstant = identifierConstant(&parser.previous);
	
	declareVariable(&parser.previous);

	emitByte(OP_CLASS);
	emitShort(nameConstant);
	defineVariable(nameConstant, true);

	if (match(TOKEN_INHERITS))
	{
		consume(TOKEN_IDENTIFIER, "Expected base class name.");
		variable(false);

		if (identifiersEqual(&className, &parser.previous)) 
		{
			error("A class can't inherit from itself.");
		}

		namedVariable(className, false);
		emitByte(OP_INHERIT);
	}

	bool foundConstructor = false;

	namedVariable(className, false);
	do
	{
		bool isStatic = false;

		ignoreNewlines();

		if (match(TOKEN_STATIC))
		{
			isStatic = true;
		}

		if (match(TOKEN_VAR))
		{

			consume(TOKEN_IDENTIFIER, "Expect class name.");
			uint16_t varName = identifierConstant(&parser.previous);

			if (match(TOKEN_EQ))
			{
				expression();
			}
			else
			{
				// Just push a nil for the undefined value
				emitByte(OP_NIL);
			}

			emitByte(isStatic ? OP_DEFINE_STATIC : OP_DEFINE_FIELD);
			emitShort(varName);

			consumeLine("Expected new line after class field declaration");
		}
		else if (match(TOKEN_FUNCTION))
		{
			method(isStatic, false);
		}
		// Constructors are declared with the class name 
		else if (check(TOKEN_IDENTIFIER) && strncmp(parser.current.start, className.start, className.length) == 0)
		{
			if (foundConstructor)
				error("A class cannot contain more than one constructor.");

			if (isStatic)
				error("Constructors cannot be static.");

			method(false, true);

			foundConstructor = true;
		}


	} while (!check(TOKEN_END));

	consume(TOKEN_END, "Expected 'end' after class body");
	ignoreNewlines();

	emitByte(OP_POP);
}

static void self(bool canAssign)
{
	variable(false);
}

static void returnStatement()
{
	if (current->type == TYPE_SCRIPT) 
	{
		error("Can't return from top-level code.");
	}

	if (current->type == TYPE_CONSTRUCTOR)
	{
		error("Can't return a value from a constructor");
	}

	if (!check(TOKEN_LINE)) 
	{
		expression();
		emitByte(OP_RETURN);
	}
	else 
	{

		emitReturn();
	}
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

static void is_(bool canAssign)
{
	// Null is a keyword but could also be used here
	if (check(TOKEN_NULL))
		consume(TOKEN_NULL, "Expected type name after 'is'.");
	else 
		consume(TOKEN_IDENTIFIER, "Expected type name after 'is'.");

	// This might not be the best but we always
	// emit a constant 
	uint16_t constant = identifierConstant(&parser.previous);

	// Get the string
	ObjString* str = SOLIS_AS_STRING(currentChunk()->constants.data[constant]);

	emitByte(OP_IS);

	uint8_t type = 0;

	// There isn't much we can do ahead of time here 
	// Other than make it so we don't have to do string comparisons at runtime 

	if (strcmp(str->chars, "number") == 0)
		type = VALUE_NUMERIC;
	else if (strcmp(str->chars, "bool") == 0)
		type = VALUE_TRUE;		// Just emit a true for bool 
	else if (strcmp(str->chars, "null") == 0)
		type = VALUE_NULL;
	else if (strcmp(str->chars, "string") == 0)
		type = VALUE_OBJECT + OBJ_STRING;
	else
	{
		// Its either a class or instance

		type = VALUE_OBJECT + OBJ_CLASS;
	}

	// Emit the type
	emitByte(type);
	emitShort(constant);

	
}

static uint8_t argumentList() 
{
	uint8_t argCount = 0;
	if (!check(TOKEN_RIGHT_PAREN)) 
	{
		do 
		{
			expression();

			if (argCount > 16) 
			{
				error("Can't have more than 16 arguments per function call.");
			}

			argCount++;
		} while (match(TOKEN_COMMA));
	}
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
	return argCount;
}

static void call(bool canAssign)
{
	uint8_t argCount = argumentList();
	// emitBytes(OP_CALL, argCount);

	emitByte(OP_CALL_0 + argCount);
}

static void dot(bool canAssign)
{
	consume(TOKEN_IDENTIFIER, "Expect property name after '.'.");
	uint16_t name = identifierConstant(&parser.previous);

	if (canAssign && match(TOKEN_EQ))
	{
		expression();
		emitByte(OP_SET_FIELD);
		emitShort(name);
	}
	else if (match(TOKEN_LEFT_PAREN))
	{
		uint8_t argCount = argumentList();
		emitByte(OP_INVOKE);
		emitShort(name);
		emitByte(argCount);
	}
	else
	{
		emitByte(OP_GET_FIELD);
		emitShort(name);
	}
}

static void arrayCreate(bool canAssign)
{
	uint16_t size = 0;
	// Check if we have any elements
	if (!check(TOKEN_RIGHT_BRACKET))
	{
		do {
			expression();

			size++;
		} while (match(TOKEN_COMMA));
	}

	consume(TOKEN_RIGHT_BRACKET, "Expected ']' at end of list");

	emitByte(OP_CREATE_LIST);
	emitShort(size);
}

static void arrayAssign(bool canAssign)
{
	expression();

	consume(TOKEN_RIGHT_BRACKET, "Expected ']' after subscript.");

	if (canAssign && match(TOKEN_EQ))
	{
		expression();

		emitByte(OP_SUBSCRIPT_SET);
	}
	else
	{
		emitByte(OP_SUBSCRIPT_GET);
	}
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

static ParseRule* getRule(SolisTokenType type) {
	return &rules[type];
}

ObjFunction* solisCompile(VM* vm, const char* source, HashTable* globals, int globalCount, const char* sourceName)
{
	parser.hadError = false;
	parser.panicMode = false;
	parser.tokenOffset = 0;

	// Setup the compiler

	Compiler compiler;
	initCompiler(vm, &compiler, TYPE_SCRIPT);

	// Copy our globals into the compiler globals
	if (globals != NULL)
	{
		solisHashTableCopy(globals, &compiler.globalTable);
		compiler.globalCount = globalCount;
	}

	TokenList tokenList = solisScanSource(vm, source);

	// solisPrintTokenList(&tokenList);

	parser.tokenList = tokenList;
	parser.source = source;
	parser.sourceName = sourceName;

	advance();
	
	while (!match(TOKEN_EOF))
	{
		declaration();
	}


	ObjFunction* function = endCompiler(&compiler);
	solisFreeTokenList(&tokenList);
	return parser.hadError ? NULL : function;
}



void solisMarkCompilerRoots(VM* vm)
{
	Compiler* compiler = current;
	while (compiler != NULL) {
		markObject(vm, (Object*)compiler->function);

		markTable(vm, &compiler->globalTable);
	
		

		compiler = compiler->parent;
	}
	
}