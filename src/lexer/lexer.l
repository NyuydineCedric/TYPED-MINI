%option noyywrap
%option yylineno

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../parser/parser.tab.h"
%}

DIGIT       [0-9]
LETTER      [a-zA-Z_]
IDENTIFIER  {LETTER}({LETTER}|{DIGIT})*
INTEGER     {DIGIT}+
EXPONENT    ([eE][+-]?[0-9]+)?
FLOAT_NUM   [0-9]+"."[0-9]+{EXPONENT}

%%

[ \t\r\n]+              { /* Ignore whitespace */ }

"int"                   { return INT; }
"float"                 { return FLOAT; }
"bool"                  { return BOOL; }
"char"                  { return CHAR; }

"if"                    { return IF; }
"else"                  { return ELSE; }
"while"                 { return WHILE; }

"true"                  { return TRUE; }
"false"                 { return FALSE; }

"=="                    { return EQUAL; }
"!="                    { return NOT_EQUAL; }
"<="                    { return LESS_EQUAL; }
">="                    { return GREATER_EQUAL; }

"&&"                    { return LOGICAL_AND; }
"||"                    { return LOGICAL_OR; }
"!"                     { return LOGICAL_NOT; }

"+"                     { return PLUS; }
"-"                     { return MINUS; }
"*"                     { return MULTIPLY; }
"/"                     { return DIVIDE; }

"="                     { return ASSIGN; }
"<"                     { return LESS_THAN; }
">"                     { return GREATER_THAN; }

";"                     { return SEMICOLON; }
","                     { return COMMA; }
"("                     { return LEFT_PAREN; }
")"                     { return RIGHT_PAREN; }
"{"                     { return LEFT_BRACE; }
"}"                     { return RIGHT_BRACE; }

{FLOAT_NUM}             {
                            yylval.floating = strtod(yytext, NULL);
                            return FLOAT_LITERAL;
                        }

{INTEGER}               {
                            yylval.integer = strtol(yytext, NULL, 10);
                            return INTEGER_LITERAL;
                        }

\'([^\\'\n]|\\.)\'      {
                            yylval.character = yytext[1];
                            return CHAR_LITERAL;
                        }

{IDENTIFIER}            {
                            yylval.string = strdup(yytext);

                            if (yylval.string == NULL) {
                                fprintf(stderr, "Lexer memory allocation failed\n");
                                exit(EXIT_FAILURE);
                            }

                            return IDENTIFIER;
                        }

"//".*                  { /* Ignore single-line comments */ }

.                       {
                            fprintf(stderr,
                                    "LEXICAL ERROR at line %d: Invalid character '%s'\n",
                                    yylineno, yytext);
                            return yytext[0];
                        }

%%