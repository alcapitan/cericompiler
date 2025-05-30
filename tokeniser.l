%{
// This is our Lexical tokeniser 
// It should be compiled into cpp with :
// flex++ -d -otokeniser.cpp tokeniser.l 
// And then compiled into object with
// g++ -c tokeniser.cpp
// tokens can be read using lexer->yylex()
// lexer->yylex() returns the type of the lexicon entry (see enum TOKEN in tokeniser.h)
// and lexer->YYText() returns the lexicon entry as a string

#include "tokeniser.h"
#include <iostream>

using namespace std;

%}

%option noyywrap
%option c++
%option yylineno

stringconst  \"[^\n"]+\"
ws      [ \t\n\r]+
alpha   [A-Za-z]
digit   [0-9]
number  -?{digit}+
boolval (True|False)
vardecl (DECLARE|CONST)
vartype (INT|BOOL)
keyword	(PRINT|IF|THEN|ELSE|ENDIF|FOR|TO|DOWNTO|ENDFOR|WHILE|DO|ENDWHILE)
id	{alpha}({alpha}|{digit})*
addop	(\+|\-|\|\|)
mulop	(\*|\/|%|\&\&)
relop	(\<|\>|"=="|\<=|\>=|!=)
unknown [^\"A-Za-z0-9 \n\r\t\(\)\<\>\=\!\%\&\|\}\-\;\.]+

%%

{addop}		return ADDOP;
{mulop}		return MULOP;
{relop}		return RELOP;
{number}	return NUMBER;
{boolval}		return BOOLVAL;
{vardecl}	return VARDECL;
{vartype}	return VARTYPE;
{keyword}	return KEYWORD;
"EXIT"		return EXIT;
{id}		return ID;
{stringconst}	return STRINGCONST;
"["		return RBRACKET;
"]"		return LBRACKET;
","		return COMMA;
";"		return SEMICOLON;
":="		return ASSIGN;
"("		return RPARENT;
")"		return LPARENT;
"!"		return NOT;
<<EOF>>		return FEOF;
{ws}	{/* skip blanks and tabs */};
"/*"	{/* Skip comments between / * and * / */
		int c;
		while((c = yyinput()) != 0){
			if(c == '*'){
				if((c = yyinput()) == '/')
					break;
				else
					unput(c);
			}	
		}
	};

{unknown}	return UNKNOWN;

%%

