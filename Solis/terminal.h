
#ifndef TERMINAL_H
#define TERMINAL_H

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

// A simple library for ANSI escape codes in the terminal 
// Its not made to be super fast, its made to be functional

// Currently fairly primitive doesn't allow for inline styling, only line styling. 

// Written by Henry Gronow initially for Solis Command Line stuff

#define ESC "\x1B["



typedef enum StyleType
{
	TERMINAL_FOREGROUND, 
	TERMINAL_BACKGROUND, 
	TERMINAL_TEXT_STYLE

} StyleType;

/*
	All the available foreground colours
*/
typedef enum Foreground
{
	TERMINAL_FG_BLACK = 30,
	TERMINAL_FG_RED = 31,
	TERMINAL_FG_GREEN = 32,
	TERMINAL_FG_YELLOW = 33,
	TERMINAL_FG_BLUE = 34,
	TERMINAL_FG_MAGENTA = 35,
	TERMINAL_FG_CYAN = 36,
	TERMINAL_FG_WHITE = 37,
			  
	TERMINAL_FG_DEFAULT = 39,
			  
} Foreground;

/*
	All the available background colours
*/
typedef enum Background
{
	TERMINAL_BG_BLACK = 40,
	TERMINAL_BG_RED = 41,
	TERMINAL_BG_GREEN = 42,
	TERMINAL_BG_YELLOW = 43,
	TERMINAL_BG_BLUE = 44,
	TERMINAL_BG_MAGENTA = 45,
	TERMINAL_BG_CYAN = 46,
	TERMINAL_BG_WHITE = 47,

	TERMINAL_BG_DEFAULT = 49,
} Background;

typedef enum TextStyle
{
	TERMINAL_TEXT_BOLD = 1, 

} TextStyle;

/*
	This struct details an individual styling parameter that can be pushed onto the style stack
*/
typedef struct TerminalStyle {

	StyleType type;

	union
	{
		Foreground fg;
		Background bg;
		TextStyle ts;
	};

} TerminalStyle;

/*
	Don't touch: These aren't even read only, you should never need to read from it. The terminal functions are the only things that should read and write here
*/
static TerminalStyle __styleStack[256];
static int __styleStackHead = 0;

#ifdef _WIN32
static DWORD __cachedDwMode = 0;
static HANDLE __hOut = NULL;
#endif

/*
	This will make sure everything is initialised and ready 
*/
static inline int terminalInit()
{
#ifdef _WIN32
	__hOut = GetStdHandle(STD_OUTPUT_HANDLE);

	if (__hOut == INVALID_HANDLE_VALUE)
	{
		printf("Failed to initialise Terminal Processing for windows.\n");
		return 0;
	}

	if (!GetConsoleMode(__hOut, &__cachedDwMode))
	{
		printf("Failed to get Terminal dw Mode.\n");
		return 0;
	}

	DWORD dwMode = __cachedDwMode;

	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	if (!SetConsoleMode(__hOut, dwMode))
	{
		printf("Failed to set Terminal dw Mode for vtp.\n");
		return 0;
	}

#endif

	return 1;
}

static inline void terminalShutdown()
{
#ifdef _WIN32

#endif
	printf(ESC "0;0m");
}

/*
	Push a new style 
*/
static inline void terminalPushStyle(TerminalStyle style)
{
	assert(__styleStackHead != 255 && "Cannot push anymore styles onto the style stack.");

	__styleStack[__styleStackHead] = style;
	__styleStackHead++;
}

static inline void terminalPopStyle()
{
	assert(__styleStackHead != 0 && "Cannot Pop when there are no styles on the stack");

	__styleStackHead--;
}


static inline void terminalPushForeground(Foreground fg)
{
	TerminalStyle style = {
		.type = TERMINAL_FOREGROUND,
		.fg = fg
	};

	terminalPushStyle(style);
}

static inline void terminalPushBackground(Background bg)
{
	TerminalStyle style = {
		.type = TERMINAL_BACKGROUND,
		.bg = bg
	};

	terminalPushStyle(style);
}

static inline void terminalPushTextStyle(TextStyle bg)
{
	TerminalStyle style = {
		.type = TERMINAL_TEXT_STYLE,
		.ts = bg
	};

	terminalPushStyle(style);
}


/*
	This is the meaty function really. It applies styles on the stack backwards. If a style has already been applied to that 'area' (idk what to call it) like foreground
	It skips it and moves on, if its applied the maximum number of styles it just doesn't go any further. So its fairly optimised as long as you don't push loads of unique styles onto the stack. 
*/
static inline void terminalPrintf(const char* fmt, ...)
{
	unsigned char appliedBitmask = 0;

	for (int i = __styleStackHead; i != 0; i--)
	{
		TerminalStyle* h = &__styleStack[i - 1];
		switch (h->type)
		{
		case TERMINAL_FOREGROUND:

			if (!(appliedBitmask & 1))
			{
				printf(ESC "%dm", (int)h->fg);
				appliedBitmask |= 1;
			}

			break;
		case TERMINAL_BACKGROUND:

			if (!((appliedBitmask >> 1) & 1))
			{
				printf(ESC "%dm", (int)h->bg);
				appliedBitmask |= 2;
			}

			break;
		case TERMINAL_TEXT_STYLE:
			if (!((appliedBitmask >> 2) & 1))
			{
				printf(ESC "%dm", (int)h->ts);
				appliedBitmask |= 4;
			}
			break;
		}

		if (appliedBitmask == 7)
			break;
	}


	// Print the final string now with the styles applied
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);

	// Reset the style back to default
	printf(ESC "0;0m");
}


#endif // TERMINAL_H