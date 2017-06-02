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

struct symbol *lookup(char *sym) {
    struct symbol *sp = &symtab[symhash(sym) % NHASH];
    int scount = NHASH;

    while (--scount >= 0) {
        if (sp->name && !strcmp(sp->name, sym)) {
            return sp;
        }

        // adiciona novo símbolo
        if (!sp->name) {
            sp->name = strdup(sym);
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

struct ast *newcall(struct symbol *s, struct ast *l) {
    struct ufncall *a = malloc(sizeof(struct ufncall));

    if(!a) {
        yyerror("out of space");
        exit(0);
    }
    a->nodetype = 'C';
    a->l = l;
    a->s = s;
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
        case 'M': case 'C': case 'F':
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

struct symlist *newsymlist(struct symbol *sym, struct symlist *next) {
    struct symlist *sl = malloc(sizeof(struct symlist));

    if(!sl) {
        yyerror("out of space");
        exit(0);
    }
    sl->sym = sym;
    sl->next = next;
    return sl;
}

void symlistfree(struct symlist *sl) {
    struct symlist *nsl;
    while(sl) {
        nsl = sl->next;
        free(sl);
        sl = nsl;
    }
}

static double callbuiltin(struct fncall *f) {
    enum bifs functype = f->functype;
    double v = eval(f->l);
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
                    fprintf(yyout, "%d\n", ((struct intval *)f->l)->number);
                    break;
                case 'Y':
                    fprintf(yyout, "%f\n", ((struct floatval *)f->l)->number);
                    break;
                case 'Z':
                    fprintf(yyout, "%c\n", ((struct charval *)f->l)->c);
                    break;
                default:
                    break;
            }
            return v;
        default:
            yyerror("Unknown built-in function %d", functype);
            return 0.0;
    }
}

static double calluser(struct ufncall *f) {
    struct symbol *fn = f->s;   // nome da função
    struct symlist *sl;         // argumentos da função
    struct ast *args = f->l;    // argumentos passados para a função
    double *oldval, *newval;    // valores dos argumentos salvos
    double v;
    int nargs;
    int i;
    if(!fn->func) {
        yyerror("call to undefined function", fn->name);
        return 0;
    }

    /* Contagem dos argumentos */
    sl = fn->syms;
    for (nargs = 0; sl; sl = sl->next)
        nargs++;

    /* Prepara para salvá-los */
    oldval = (double *)malloc(nargs * sizeof(double));
    newval = (double *)malloc(nargs * sizeof(double));
    if(!oldval || !newval) {
        yyerror("Out of space in %s", fn->name);
        return 0.0;
    }

    /* Avalia os argumentos */
    for(i = 0; i < nargs; i++) {
        if(!args) {
            yyerror("too few args in call to %s", fn->name);
            free(oldval);
            free(newval);
            return 0.0;
        }
        if(args->nodetype == 'L') { // se é um nó do tipo lista
            newval[i] = eval(args->l);
            args = args->r;
        } else { // se é o fim da lista
            newval[i] = eval(args);
            args = NULL;
        }
    }

    /* Salva os antigos valores dos argumentos, atribui os novos */
    sl = fn->syms;
    for(i = 0; i < nargs; i++) {
        struct symbol *s = sl->sym;
        oldval[i] = s->value;
        s->value = newval[i];
        sl = sl->next;
    }
    free(newval);

    /* Avalia a função */
    v = eval(fn->func);

    /* Coloca os antigos valores de volta nos argumentos da função */
    sl = fn->syms;
    for(i = 0; i < nargs; i++) {
        struct symbol *s = sl->sym;
        s->value = oldval[i];
        sl = sl->next;
    }
    free(oldval);

    return v;
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
        case 'C':
            v = calluser((struct ufncall *)a);
            break;
        default:
            printf("internal error: bad node %c\n", a->nodetype);
    }
    return v;
}

/* Define uma função */
void dodef(struct symbol *name, struct symlist *syms, struct ast *func) {
    if(name->syms)
        symlistfree(name->syms);
    if(name->func)
        treefree(name->func);

    name->syms = syms;
    name->func = func;
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
    if (argc == 3) {
        if (!(yyout = fopen(argv[2], "w"))) {
            perror(argv[2]);
            return 1;
        }
    }

    return yyparse();
}
