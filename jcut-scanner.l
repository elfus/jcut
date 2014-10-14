%top {
//===-- jcut/JCUTScanner.h - Flex generated scanner ---------------*- C -*-===//
//
// Copyright (c) 2014 Adrián Ortega García
// All rights reserved.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief
///
//===----------------------------------------------------------------------===//
#include <string>
#define YY_DECL extern "C" int yylex()
extern void comment();
extern void jtl_comment();
extern void count();
extern void print(const std::string&);

enum TokenType {
        TOK_ERR = -1,
        TOK_EOF = -2,
        TOK_ASCII_CHAR = -3,
        TOK_IDENTIFIER = -4, // An identifier is a string representing a function or variable name
        TOK_EQ_OP = -5,
        TOK_INT = -6,
        TOK_FLOAT = -7, // Will handle floats too
        TOK_CHAR = -8,
        TOK_STRING = -9,
        TOK_BUFF_ALLOC = -10,
        TOK_ARRAY_INIT = -11,
        TOK_COMPARISON_OP = -12,
        TOK_STRUCT_INIT = -13,
        // keywords
        TOK_BEFORE = -100,
        TOK_AFTER = -101,
        TOK_MOCKUP = -102,
        TOK_BEFORE_ALL = -103,
        TOK_AFTER_ALL = -104,
        TOK_MOCKUP_ALL = -105,
        TOK_TEST_DATA = -106,
        TOK_GROUP = -107,
    };
}

%{
//===-- jcut/JCUTScanner.cpp - Flex generated scanner -------------*- C -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief
///
//===----------------------------------------------------------------------===//
#include <iostream>
#include <string>

using namespace std;
%}
/* Tell the scanner to keep track of the line number */
%option yylineno
%option outfile="JCUTScanner.cpp" header-file="JCUTScanner.h"
/** Watch for this option */
/* %option nounistd */

/* If you know that you only want to parse one file at a time 
 * (usually a good assumption), you don't need to provide your
 * own yywrap function that would just return 1. Inside your flex
 * file, you can say %option noyywrap and the generated output
 * will define yywrap as a local macro returning 1. This has the 
 * added bonus of removing the requirement to link with the flex
 * libraries, since they're only needed to get the default yywrap
 * defined as a function returning 1.avoid linking to flex library
 */
%option noyywrap

D			[0-9]
L			[a-zA-Z_]
H			[a-fA-F0-9]
E			[Ee][+-]?{D}+
FS			(f|F|l|L)
IS			(u|U|l|L)*

IDENTIFIER		{L}({L}|{D})*
			/* Hexadecimal  | Octal      | Decimal   | CHAR CONST! */
CONSTANT		(0[xX]{H}+{IS}?)|(0{D}+{IS}?)|({D}+{IS}?)|(L?'(\\.|[^\\'])+')
			/* 23e3434f   |   34.34343             | 34.34e3434 */
FLOAT			({D}+{E}{FS}?)|({D}*"."{D}+({E})?{FS}?)|({D}+"."{D}*({E})?{FS}?)

CHAR_CONST L?'(\\.|[^\\'])+'
STRING   L?\"(\\.|[^\\"])*\"

%%
"/*"	  { comment(); }
"#"	  {jtl_comment();}
group	{ count(); return TOK_GROUP;}
before	{ count(); return TOK_BEFORE;}
after	{ count(); return TOK_AFTER;}
mockup	{ count(); return TOK_MOCKUP;}
before_all	{ count(); return TOK_BEFORE_ALL;}
after_all	{ count(); return TOK_AFTER_ALL;}
mockup_all	{ count(); return TOK_MOCKUP_ALL;}
data	{ count(); return TOK_TEST_DATA;}

{IDENTIFIER}      { count(); return TOK_IDENTIFIER; }

{STRING}  { count(); return TOK_STRING; }
{CHAR_CONST} { count(); return TOK_CHAR; }

[-]?{FLOAT}   { count(); return TOK_FLOAT; }
[-]?{CONSTANT} { count(); return TOK_INT; }

";"       { count(); return ';'; }
","       { count(); return ','; }
"("       { count(); return '('; }
")"       { count(); return ')'; }
"{"       { count(); return '{'; }
"}"       { count(); return '}'; }
"["       { count(); return '['; }
"]"       { count(); return ']'; }
":"       { count(); return ':'; }
"=="	  { count(); return TOK_COMPARISON_OP; }
"!="	  { count(); return TOK_COMPARISON_OP; }
"<="	  { count(); return TOK_COMPARISON_OP; }
">="	  { count(); return TOK_COMPARISON_OP; }
"<"       { count(); return TOK_COMPARISON_OP; }
">"       { count(); return TOK_COMPARISON_OP; }
"="	  { count(); return '='; }

"@"		  { count(); return '@'; }

[ \t\v\n\f]		{ count(); }

.         { print("IGNORED");}/* ignore unprocessed characters */
<<EOF>>   { count(); return TOK_EOF;}
%%
void jtl_comment()
{
	char c;
	while ((c = yyinput()) != '\n'  && c != 0 && c != EOF);
		/** putchar(c); **/
}

void comment()
{
	char c, c1;

loop:
	while ((c = yyinput()) != '*' && c != 0);
		/** putchar(c);**/

	if ((c1 = yyinput()) != '/' && c != 0)
	{
		unput(c1);
		goto loop;
	}

	/* if (c != 0)
		putchar(c1); */
}


unsigned column = 0;
unsigned ccount = 0;

void print(const string& str) {
	cout << yylineno <<":"<<column<<":"<<str<<": "<<string(yytext,yyleng)<<endl;
} 

void count()
{
	int i;

	for (i = 0; yytext[i] != '\0'; i++)
		if (yytext[i] == '\n')
			ccount = 0;
		else if (yytext[i] == '\t')
			ccount += 8 - (ccount % 8);
		else
			ccount++;
	
	column = ccount - i + 1;

	/* ECHO; */
}

