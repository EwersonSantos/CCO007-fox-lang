%option noyywrap nodefault yylineno
%{
#include "parser.h"
#include "parser.tab.h"
%}

/* Expoente de um número fracionário */
EXP ([Ee][-+]?[0-9]+)

%%

 /* Operações de somente um caractere */
"+" |
"-" |
"*" |
"/" |
"=" |
"," |
";" |
"(" |
")" |
"{" |
"}"    { return yytext[0]; } // retorna o char

 /* Operações de comparação, todos usam o token CMP */
">"     { yylval.fn = 1; return CMP; }
"<"     { yylval.fn = 2; return CMP; }
"!="    { yylval.fn = 3; return CMP; }
"=="    { yylval.fn = 4; return CMP; }
">="    { yylval.fn = 5; return CMP; }
"<="    { yylval.fn = 6; return CMP; }

 /* Operações lógicas, todos usam o token LOGI */
"!"     { yylval.fn = 7; return LOGI; }
"&&"    { yylval.fn = 8; return LOGI; }
"||"    { yylval.fn = 9; return LOGI; }


 /* Palavras reservadas */
"if"    { return IF; }
"else"  { return ELSE; }
"while" { return WHILE; }
"int"   { return INT_KEYWORD; }
"float" { return FLOAT_KEYWORD; }
"char"  { return CHAR_KEYWORD; }

 /* Funções da linguagem */
 "sqrt"     { yylval.fn = B_sqrt; return FUNC; }
 "exp"      { yylval.fn = B_exp; return FUNC; }
 "log"      { yylval.fn = B_log; return FUNC; }
 "print"    { yylval.fn = B_print; return FUNC; }
 "scan"     { yylval.fn = B_scan; return FUNC; }

 /* Identificadores */
[a-zA-Z][a-zA-Z0-9_]* {
    if ( (yylval.s = getsymbol(yytext)) ) {
        return VAR;
    } else {
        yylval.st = yytext;
        return ID;
    }
}

 /* Números */
[0-9]+{EXP}? {
    yylval.i = atoi(yytext);
    return INT_LITERAL;
}
("."?[0-9]+{EXP}?)|([0-9]+"."[0-9]*{EXP}?) {
    yylval.d = atof(yytext);
    return FLOAT_LITERAL;
}

 /* Caractere */
 '(.)?' {
     yylval.c = yytext[1];
     return CHAR_LITERAL;
 }

"//".*  // ignora comentários da forma "//"
[ \t]   // ignora espaços em branco
\n      // ignora fim de linha */

.       { yyerror("Unknown character %c\n", *yytext); } // caracter não reconhecido

<<EOF>> { exit(0); }    // End-Of-File, final do arquivo

%%
