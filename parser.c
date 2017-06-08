/*
 *  Funções auxiliares
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include "parser.h"

static unsigned symhash(char *sym) {
    unsigned int hash = 0;
    unsigned c;

    while ( (c = *sym++) ) hash = hash * 9 ^ c;

    return hash;
}

struct symbol *getsymbol(char *sym) {
    struct symbol *sp = &symtab[symhash(sym) % NHASH];
    int scount = NHASH;

    while (--scount >= 0) {
        if (sp->name && !strcmp(sp->name, sym)) {
            return sp;
        }

        if (++sp >= symtab + NHASH) // tenta o próximo símbolo
            sp = symtab;
    }

    return NULL;
}

struct symbol *newsymbol(int type, char *sym) {
    struct symbol *sp = &symtab[symhash(sym) % NHASH];
    int scount = NHASH;

    while (--scount >= 0) {
        if (sp->name && !strcmp(sp->name, sym)) {
            yyerror("symbol '%s' already declared.", sp->name);
            abort();
        }

        // adiciona novo símbolo
        if (!sp->name) {
            sp->name = strdup(sym);
            sp->type = type;
            sp->value = 0;
            sp->func = NULL;
            sp->syms = NULL;
            return sp;
        }

        if (++sp >= symtab + NHASH) // tenta o próximo símbolo
            sp = symtab;
    }

    yyerror("symbol table overflow");
    abort();    // a tabela está cheia
}

struct ast *newast(int nodetype, struct ast *l, struct ast *r) {
    struct ast *a = malloc(sizeof(struct ast));

    if(!a) {
        yyerror("out of space");
        exit(0);
    }
    a->nodetype = nodetype;
    a->l = l;
    a->r = r;
    return a;
}

struct ast *newint(int i) {
    struct intval *a = malloc(sizeof(struct intval));

    if(!a) {
        yyerror("out of space");
        exit(0);
    }
    a->nodetype = 'X';
    a->number = i;
    return (struct ast *)a;
}

struct ast *newfloat(double f) {
    struct floatval *a = malloc(sizeof(struct floatval));

    if(!a) {
        yyerror("out of space");
        exit(0);
    }
    a->nodetype = 'Y';
    a->number = f;
    return (struct ast *)a;
}

struct ast *newchar(char c) {
    struct charval *a = malloc(sizeof(struct charval));

    if(!a) {
        yyerror("out of space");
        exit(0);
    }
    a->nodetype = 'Z';
    a->c = c;
    return (struct ast *)a;
}

struct ast *newcmp(int cmptype, struct ast *l, struct ast *r) {
    struct ast *a = malloc(sizeof(struct ast));

    if(!a) {
        yyerror("out of space");
        exit(0);
    }
    a->nodetype = '0' + cmptype;
    a->l = l;
    a->r = r;
    return a;
}

struct ast *newfunc(int functype, struct ast *l) {
    struct fncall *a = malloc(sizeof(struct fncall));

    if(!a) {
        yyerror("out of space");
        exit(0);
    }
    a->nodetype = 'F';
    a->l = l;
    a->functype = functype;
    return (struct ast *)a;
}

struct ast *newref(struct symbol *s) {
    struct symref *a = malloc(sizeof(struct symref));
    if(!a) {
        yyerror("out of space");
        exit(0);
    }
    a->nodetype = 'N';
    a->s = s;
    return (struct ast *)a;
}

struct ast *newasgn(struct symbol *s, struct ast *v) {
    struct symasgn *a = malloc(sizeof(struct symasgn));
    if(!a) {
        yyerror("out of space");
        exit(0);
    }
    a->nodetype = '=';
    a->s = s;
    a->v = v;
    return (struct ast *)a;
}

struct ast *newflow(int nodetype, struct ast *cond, struct ast *tl, struct ast *el) {
    struct flow *a = malloc(sizeof(struct flow));

    if(!a) {
        yyerror("out of space");
        exit(0);
    }
    a->nodetype = nodetype;
    a->cond = cond;
    a->tl = tl;
    a->el = el;
    return (struct ast *)a;
}

void treefree(struct ast *a) {
    switch(a->nodetype) {
        /* possuem duas sub-árvores (esquerda e direita) */
        case '+':
        case '-':
        case '*':
        case '/':
        case '1': case '2': case '3': case '4': case '5': case '6':
        case 'L':
            treefree(a->r);
        /* possuem uma sub-árvore (esquerda) */
        case '|':
        case 'M': case 'F':
            treefree(a->l);
        /* não possuem sub-árvores */
        case 'X': case 'Y': case 'Z': case 'N':
            break;
        case '=':
            free(((struct symasgn *)a)->v);
            break;
        /* possuem até três subárvores */
        case 'I': case 'W':
            free( ((struct flow *)a)->cond);
            if(((struct flow *)a)->tl)
                treefree(((struct flow *)a)->tl);
            if(((struct flow *)a)->el)
                treefree(((struct flow *)a)->el);
            break;
        default:
            printf("internal error: free bad node %c\n", a->nodetype);
    }

    free(a); // O próprio nó é sempre limpo
}

struct idlist *newidlist(char *id, struct idlist *next) {
    struct idlist *il = malloc(sizeof(struct idlist));
    if(!il) {
        yyerror("out of space");
        exit(0);
    }

    il->id = strdup(id);
    il->next = next;
    return il;
}

static double callbuiltin(struct fncall *f) {
    enum bifs functype = f->functype;
    double v;
    int iv;
    char cv;

    if (f->l)
        v = eval(f->l);

    switch(functype) {
        case B_sqrt:
            return sqrt(v);
        case B_exp:
            return exp(v);
        case B_log:
            return log(v);
        case B_print:
            switch (f->l->nodetype) {
                case 'X':
                    fprintf(yyout, "%d\n", (int)((struct intval *)f->l)->number);
                    break;
                case 'Y':
                    fprintf(yyout, "%f\n", (double)((struct floatval *)f->l)->number);
                    break;
                case 'Z':
                    fprintf(yyout, "%c\n", (char)((struct charval *)f->l)->c);
                    break;
                case 'N':
                    v = ((struct symref *)f->l)->s->value;
                    switch (((struct symref *)f->l)->s->type) {
                        case T_int:
                            fprintf(yyout, "%d\n", (int)v);
                            break;
                        case T_float:
                            fprintf(yyout, "%f\n", v);
                            break;
                        case T_char:
                            fprintf(yyout, "%c\n", (char)v);
                            break;
                    }
                default:
                    break;
            }
            return v;
        case B_scan:
            if (f->l->nodetype != 'N') {
                yyerror("scan: expected a variable argument");
                exit(0);
            } else {
                switch (((struct symref *)f->l)->s->type) {
                    case T_int:
                        scanf("%d\n", &iv);
                        ((struct symref *)f->l)->s->value = iv;
                        return iv;
                    case T_float:
                        scanf("%lf\n", &v);
                        ((struct symref *)f->l)->s->value = v;
                        return v;
                    case T_char:
                        scanf(" %c", &cv);
                        ((struct symref *)f->l)->s->value = cv;
                        return cv;
                    default:
                        yyerror("scan: an error occurred");
                        exit(0);
                }
            }
        default:
            yyerror("Unknown built-in function %d", functype);
            return 0.0;
    }
}

double eval(struct ast *a) {
    double v;
    if(!a) {
        yyerror("internal error, null eval");
        return 0.0;
    }
    switch(a->nodetype) {
        /* Número inteiro */
        case 'X':
            v = ((struct intval *)a)->number;
            break;
        /* ponto flutuante */
        case 'Y':
            v = ((struct floatval *)a)->number;
            break;
        /* Constante */
        case 'Z':
            v = ((struct charval *)a)->c;
            break;
        /* Identificador */
        case 'N':
            v = ((struct symref *)a)->s->value;
            break;
        /* Atribuição */
        case '=':
            v = ((struct symasgn *)a)->s->value = eval(((struct symasgn *)a)->v);
            break;
        /* Expressões */
        case '+':
            v = eval(a->l) + eval(a->r);
            break;
        case '-':
            v = eval(a->l) - eval(a->r);
            break;
        case '*':
            v = eval(a->l) * eval(a->r);
            break;
        case '/':
            v = eval(a->l) / eval(a->r);
            break;
        case '|':
            v = fabs(eval(a->l));
            break;
        case 'M':
            v = -eval(a->l);
            break;
        /* Comparações */
        case '1':
            v = (eval(a->l) >  eval(a->r)) ? 1 : 0;
            break;
        case '2':
            v = (eval(a->l) <  eval(a->r)) ? 1 : 0;
            break;
        case '3':
            v = (eval(a->l) != eval(a->r)) ? 1 : 0;
            break;
        case '4':
            v = (eval(a->l) == eval(a->r)) ? 1 : 0;
            break;
        case '5':
            v = (eval(a->l) >= eval(a->r)) ? 1 : 0;
            break;
        case '6':
            v = (eval(a->l) <= eval(a->r)) ? 1 : 0;
            break;
        /* Estruturas de controle */
        /* Expressões nulas são permitidas, portanto são checadas */
        /* if/then/else */
        case 'I':
            if(eval(((struct flow *)a)->cond) != 0) {
                if(((struct flow *)a)->tl) {
                    v = eval(((struct flow *)a)->tl);
                } else {
                    v = 0.0; // valor padrão
                }
            } else {
                if(((struct flow *)a)->el) {
                    v = eval(((struct flow *)a)->el);
                } else {
                    v = 0.0; // valor padrão
                }
            }
            break;
        case 'W':
            v = 0.0;  // valor padrão

            if(((struct flow *)a)->tl) {
                while(eval(((struct flow *)a)->cond) != 0)
                v = eval(((struct flow *)a)->tl);
            }
            break; // o valor da última declaração é o valor do while/do

        /* Lista de argumentos */
        case 'L':
            eval(a->l);
            v = eval(a->r);
            break;
        case 'F':
            v = callbuiltin((struct fncall *)a);
            break;
        default:
            printf("internal error: bad node %c\n", a->nodetype);
    }
    return v;
}

void defvar(int type, struct idlist *syms) {
    struct idlist *til;
    while (syms) {
        newsymbol(type, syms->id);

        til = syms->next;
        free(syms);
        syms = til;
    }
}

void yyerror(char *s, ...) {
    va_list ap;
    va_start(ap, s);

    fprintf(stderr, "%d: error: ", yylineno);
    vfprintf(stderr, s, ap);
    fprintf(stderr, "\n");
}

int main (int argc, char **argv) {
    if (argc >= 2) {
        if(!(yyin = fopen(argv[1], "r"))) {
            perror(argv[1]);
            return 1;
        }
    }
    if (argc >= 3) {
        if (!(yyout = fopen(argv[2], "w"))) {
            perror(argv[2]);
            return 1;
        }
    }

    return yyparse();
}
