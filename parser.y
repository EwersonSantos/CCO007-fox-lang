%{
#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
%}

%union {
    struct ast *a;
    int i;
    double d;
    char c;
    char *st;
    struct symbol *s;
    struct idlist *il;
    int fn;             // função
}

/* Declaração dos tokens */
%token <i> INT_LITERAL      // número inteiro
%token <d> FLOAT_LITERAL    // ponto flutuante
%token <c> CHAR_LITERAL     // caractere
%token <fn> FUNC            // função da linguagem
%token <s> VAR              // variável
%token <st> ID              // identificador
%token <st> INT_KEYWORD     // palavra reservada para tipo
%token <st> FLOAT_KEYWORD   // palavra reservada para tipo
%token <st> CHAR_KEYWORD    // palavra reservada para tipo
%token IF ELSE              // if/else
%token WHILE                // while

/* Precedência dos operadores */
%nonassoc <fn> CMP  // comparadores
%right '='
%left '+' '-'
%left '*' '/'
%nonassoc UMINUS
%nonassoc NO_ELSE
%nonassoc ELSE

/* Tipo de cada símbolo não-terminal */
%type <a> stmt stmt_list
%type <a> exp explist
%type <a> if_stmt while_stmt
%type <a> assigment b_function
%type <a> var_def code
%type <il> idlist
%type <i> var_type

/* Símbolo inicial */
%start lines

%%

/* Declarações */
stmt: if_stmt
    | while_stmt
    | assigment ';'
    | b_function ';'
    ;

/* if/else */
if_stmt: IF exp stmt %prec NO_ELSE {    // if, única declaração
        $$ = newflow('I', $2, $3, NULL);
    }
    | IF exp stmt ELSE stmt {           // if/else, única declaração
        $$ = newflow('I', $2, $3, $5);
    }
    | IF exp '{' stmt_list '}' %prec NO_ELSE {          // if, múltiplas declarações
        $$ = newflow('I', $2, $4, NULL);
    }
    | IF exp '{' stmt_list '}' ELSE '{' stmt_list '}' { // if/else, múltiplas declarações
        $$ = newflow('I', $2, $4, $8);
    }
    ;

/* while */
while_stmt: WHILE exp stmt {                // while, única declaração
        $$ = newflow('W', $2, $3, NULL);
    }
    | WHILE exp '{' stmt_list '}' {         // while, múltiplas declarações
        $$ = newflow('W', $2, $4, NULL);
    }
    ;

/* Lista de declarações */
stmt_list: stmt  { $$ = $1; }
    | stmt_list stmt {
        $$ = newast('L', $2, $1);
    }
    ;

/* Atribuição de variáveis */
assigment: VAR '=' exp                   { $$ = newasgn($1, $3); }
    ;
/* Chamada de funções definidas pelo usuário */
b_function: FUNC '(' ')'    { $$ = newfunc($1, NULL); }
    | FUNC '(' explist ')'  { $$ = newfunc($1, $3); }
    ;

/* Expressão */
exp: exp CMP exp            { $$ = newcmp($2, $1, $3); }
    | exp '+' exp           { $$ = newast('+', $1, $3); }
    | exp '-' exp           { $$ = newast('-', $1, $3); }
    | exp '*' exp           { $$ = newast('*', $1, $3); }
    | exp '/' exp           { $$ = newast('/', $1, $3); }
    | '(' exp ')'           { $$ = $2; }
    | '-' exp %prec UMINUS  { $$ = newast('M', $2, NULL); }
    | INT_LITERAL           { $$ = newint($1); }
    | FLOAT_LITERAL         { $$ = newfloat($1); }
    | CHAR_LITERAL          { $$ = newchar($1); }
    | VAR                   { $$ = newref($1); }
    | assigment
    | b_function
    ;

/* Lista de expressões */
explist: exp
    | exp ',' explist       { $$ = newast('L', $1, $3); }
    ;

/* Tipos de dados */
var_type: INT_KEYWORD   { $$ = T_int; }
    | FLOAT_KEYWORD     { $$ = T_float; }
    | CHAR_KEYWORD      { $$ = T_char; }
    ;

/* Lista de identificadores */
idlist: ID              { $$ = newidlist($1, NULL); }
    | idlist ',' ID     { $$ = newidlist($3, $1); }
    ;

/* Código */
code: var_def
    | stmt {
        eval($1);
        treefree($1);
    }
    ;

/* Declaração de variáveis */
var_def: var_type idlist ';'    { defvar($1, $2); }
   ;

/* Linhas da entrada */
lines: code         // única declaração
    | lines code    // múltiplas declarações
    | lines error { // erro no reconhecimento da linguagem
        yyerrok;
    }
    ;
%%
