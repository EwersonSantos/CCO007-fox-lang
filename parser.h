/**
 *  Declarações usadas pelo lexer da linguagem
 */

/* Interface para o lexer */
extern int yylineno;        // armazena o número da linha
extern int yylex();         // executa o scanner
extern int yyparse();       // executa o parser
extern FILE *yyin;          // entrada (padrão - stdin)
extern FILE *yyout;         // saída (padrão - stdout)
void yyerror(char *s, ...); // método padrão de notificação de erros

/* Funções da linguagem */
enum symtype {
    T_int = 1,
    T_float,
    T_char,
};

/* Estrutura usada para representar um símbolo, na tabela de símbolos */
struct symbol {
    char *name;             // nome do símbolo
    int type;               // tipo
    double value;           // valor numérico do símbolo (se variável)
    struct ast *func;       // AST que representa as declarações de uma função
    struct symlist *syms;   // lista de argumentos da função
};

#define NHASH 9997  // tamanho da tabela de símbolos
/* Tabela de símbolos */
struct symbol symtab[NHASH];

/* Busca na tabela de símbolos.
 * Se o nome buscado existe, o símbolo é retornado. Se não,
 * e existe espaço livre na tabela, o novo símbolo é inserido.
 */
struct symbol *newsymbol(int type, char *sym);
struct symbol *getsymbol(char *sym);

/* Lista de Identificadores, usado na declaração de variáveis */
struct idlist {
    char *id;
    struct idlist *next;
};

/* Cria lista de identificadores */
struct idlist *newidlist(char *id, struct idlist *next);

/* Limpa uma lista identificadores */
void idlistfree(struct idlist *il);


/*
 * Opções para nodetype
 * + - * /
 * 0-7  operações de comparação
 * M    menos unário
 * L    expressão ou lista de declarações (statements)
 * I    IF
 * W    WHILE
 * N    símbolo
 * =    atribuição
 * S    lista de símbolos
 * F    função da linguagem
*/

/* Funções da linguagem */
enum bifs {
    B_sqrt = 1,
    B_exp,
    B_log,
    B_print,
    B_scan
};

/* Nós da Abstract Syntax Tree (AST)
 * Cada tipo de nó possui um nodetype (int), e um valor padrão. Isso permite
 * saber o tipo do nó durante a avaliação da árvore.
 */
 /* Nó usado para expressões e comparação */
struct ast {
    int nodetype;       // tipo L ou 0-7
    struct ast *l;      // nó da esquerda
    struct ast *r;      // nó da direita
};

/* Nó de função da linguagem */
struct fncall {
    int nodetype;       // tipo F
    struct ast *l;      // expressão passada como parâmetro
    enum bifs functype; // número da função
};

/* Nó de estruturas de controle */
struct flow {
    int nodetype;       // tipo I ou W
    struct ast *cond;   // condição
    struct ast *tl;     // se condição - lista de declarações
    struct ast *el;     // senão (opcional) - declarações
};

/* Nó de número inteiro */
struct intval {
    int nodetype;       // tipo X
    int number;         // valor numérico
};

/* Nó de ponto flutuante */
struct floatval {
    int nodetype;       // tipo Y
    double number;      // valor numérico
};

/* Nó de caractere */
struct charval {
    int nodetype;       // tipo Z
    char c;        // valor numérico
};

/* Nó de símbolo */
struct symref {
    int nodetype;       // tipo N
    struct symbol *s;   // símbolo
};

/* Nó de atribuição */
struct symasgn {
    int nodetype;       // tipo '='
    struct symbol *s;   // símbolo
    struct ast *v;      // valor
};


/* Cia um nó da AST */
struct ast *newast(int nodetype, struct ast *l, struct ast *r);
/* Cia um nó de comparação */
struct ast *newcmp(int cmptype, struct ast *l, struct ast *r);
/* Cia um nó de função da linguagem */
struct ast *newfunc(int functype, struct ast *l);
/* Cia um nó de símbolo */
struct ast *newref(struct symbol *s);
/* Cia um nó de atribuição */
struct ast *newasgn(struct symbol *s, struct ast *v);
/* Cria um nó de número inteiro*/
struct ast *newint(int i);
/* Cria um nó de ponto flutuante*/
struct ast *newfloat(double f);
/* Cria um nó de caractere*/
struct ast *newchar(char c);
/* Cia um nó de estrutura de controle */
struct ast *newflow(int nodetype, struct ast *cond, struct ast *tl, struct ast *tr);

/* Define uma variável */
void defvar(int type, struct idlist *syms);

/* Avalia a expressão representada por uma AST */
double eval(struct ast *);
/* Limpa uma AST */
void treefree(struct ast *);
