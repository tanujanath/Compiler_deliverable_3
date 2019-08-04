/*
 * helper functions for UCML
 */
#  include <stdio.h>
#  include <stdlib.h>
#  include <stdarg.h>
#  include <string.h>
#  include <math.h>
#  include "library.h"
#  include "parser.tab.h"
    extern FILE* yyin;
    extern int yyparse();
    extern short shouldPrintDebugInfo;

/* symbol table */
/* hash a symbol */
static unsigned
symhash(char *sym)
{
  unsigned int hash = 0;
  unsigned c;

  while(c = *sym++) hash = hash*9 ^ c;

  return hash;
}

char *getTypeName(int type){
  switch (type) {
    case 'Z': return "DOUBLE";
    case 'K': return "INT";
    default: return "undefined";
  }
}

struct symbol *
lookup(char* sym)
{
  struct symbol *sp = &symtab[symhash(sym)%NHASH];
  int scount = NHASH;		/* how many have we looked at */

  while(--scount >= 0) {
    if(sp->name && !strcmp(sp->name, sym)) { return sp; }

    if(!sp->name) {		/* new entry */
      sp->name = strdup(sym);
      sp->value = 0;
      sp->func = NULL;
      sp->syms = NULL;
      return sp;
    }

    if(++sp >= symtab+NHASH) sp = symtab; /* try the next entry */
  }
  yyerror("symbol table overflow\n");
  abort(); /* tried them all, table is full */

}

void setReturnType(struct symbol *s, int TYPE_TOKEN){
  s->type = 'V';
  if (shouldPrintDebugInfo) printf("Setting type of %s to %d\n", s->name, TYPE_TOKEN);
  if (TYPE_TOKEN == 'K'){
    s->return_type = 'K';
  } else if (TYPE_TOKEN == 'Z'){
    s->return_type = 'Z';
  } else {
    printf("Invalid RETURN TYPE token : %d\n", TYPE_TOKEN);
  }
}
int getType(struct symbol *s){
  return s->return_type;
}
// void setReturnTypeAndExpression(struct symbol *s, int TYPE_TOKEN, struct ast *exp){
//   s->type = 'V';
//   printf("Setting type and value of %s to %d and %.2f\n", s->name, TYPE_TOKEN);
//   if (TYPE_TOKEN == 'K'){
//     s->return_type = 'K';
//   } else if (TYPE_TOKEN == 'Z'){
//     s->return_type = 'Z';
//   } else {
//     printf("Invalid RETURN TYPE token : %d\n", TYPE_TOKEN);
//   }

//   s->func = exp;
// }



struct ast *
newast(int nodetype, struct ast *l, struct ast *r)
{
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

struct ast *
newnum_int(int i)
{
  struct numval_int *a = malloc(sizeof(struct numval_int));
  
  if(!a) {
    yyerror("out of space");
    exit(0);
  }
  a->nodetype = 'K';
  a->number = i;
  if (shouldPrintDebugInfo) printf("NUMBER ADDED | %c => %d | %d\n", 'K', i, a->number);
  return (struct ast *)a;
}


struct ast *
newnum_double(double d)
{
  struct numval_double *a = malloc(sizeof(struct numval_double));
  
  if(!a) {
    yyerror("out of space");
    exit(0);
  }
  a->nodetype = 'Z';
  a->number = d;
  if (shouldPrintDebugInfo) printf("NUMBER ADDED | %c => %f\n", 'Z', d);
  return (struct ast *)a;
}

struct ast *
newcmp(int cmptype, struct ast *l, struct ast *r)
{
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

struct ast *
newfunc(int functype, struct ast *l)
{
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

struct ast *
newcall(struct symbol *s, struct ast *l)
{
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

struct ast *
newref(struct symbol *s)
{
  struct symref *a = malloc(sizeof(struct symref));
  
  if(!a) {
    yyerror("out of space");
    exit(0);
  }
  a->nodetype = 'N';
  a->s = s;
  return (struct ast *)a;
}

struct ast *
newasgn(struct symbol *s, struct ast *v)
{
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

struct ast *
newflow(int nodetype, struct ast *cond, struct ast *tl, struct ast *el)
{
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

struct symlist *
newsymlist(struct symbol *sym, struct symlist *next)
{
  struct symlist *sl = malloc(sizeof(struct symlist));
  
  if(!sl) {
    yyerror("out of space");
    exit(0);
  }
  sl->sym = sym;
  sl->next = next;
  return sl;
}

void
symlistfree(struct symlist *sl)
{
  struct symlist *nsl;

  while(sl) {
    nsl = sl->next;
    free(sl);
    sl = nsl;
  }
}

/* define a function */
void
dodef(struct symbol *name, struct symlist *syms, struct ast *func)
{
  if(name->syms) symlistfree(name->syms);
  if(name->func) treefree(name->func);
  name->syms = syms;
  name->func = func;
}


static double callbuiltin(struct fncall *);
static double calluser(struct ufncall *);


int get_return_type(struct ast *a){
  switch(a->nodetype){
    case 'K':return 'K';
    case 'Z':return 'Z';

    /* name reference */
    case 'N': return ((struct symref *)a)->s->return_type;

    /* assignment */
    // case '=': {
    //       struct symbol *s = ((struct symasgn *)a)->s;
    //       if (s->type != 'V'){
    //         printf("ERROR: variable %s not defined, but used.\n", s->name);
    //         exit(1);
    //       }
    //       if (shouldPrintDebugInfo) printf("Getting TYPE of %s\n", s->name); 
    //       return get_return_type(((struct symasgn *)a)->v);
    //     }

      /* expressions */
    case '+': 
    case '-': 
    case '*': {
      int left = get_return_type(a->l);
      int right = get_return_type(a->r);
      if (shouldPrintDebugInfo) printf("get return type of %.2f and %.2f", eval(a->l), eval(a->r)); 
      if (left == 'Z' || right == 'Z')
        return 'Z';
      else
        return 'K';
    }
    
    case '/': {
      return 'Z';
    }
    default: return 'Z';
  }
}

double
eval(struct ast *a)
{
  double v;

  if(!a) {
    yyerror("internal error, null eval");
    return 0.0;
  }

  switch(a->nodetype) {
    /* integer constant */
  case 'K': {
          v = (double)((struct numval_int *)a)->number; 
          if (shouldPrintDebugInfo) printf("Evaluating integer: %.2f\n", v); 
          break;
        }


    /* double constant */
  case 'Z': {v = 
    ((struct numval_double *)a)->number; 
    if (shouldPrintDebugInfo) printf("Evaluating double: %.2f\n", v); 
    break;}

    /* name reference */
  case 'N': v = ((struct symref *)a)->s->value; break;

    /* assignment */
  case '=': {
        struct symbol *s = ((struct symasgn *)a)->s;
        if (s->type != 'V'){
          printf("ERROR: variable %s not defined, but used.\n", s->name);
          exit(1);
        }
        int ret_t = get_return_type(((struct symasgn *)a)->v);

        // set return type accordingly
        if (s->return_type == 'K'){
          s->value_int =  (int) eval(((struct symasgn *)a)->v);
          if (shouldPrintDebugInfo) printf("casted to %d\n", s->value_int);
          v = s->value_int;
          s->value = s->value_int;
        } else {
          s->value =  eval(((struct symasgn *)a)->v);
          if (shouldPrintDebugInfo) printf("default type %.2f\n", s->value);
          v = s->value;
        }
        if (shouldPrintDebugInfo) printf("Setting %s to %d\n", ((struct symasgn *)a)->s->name, v); 
        break;
      }

    /* expressions */
  case '+': {
    v = eval(a->l) + eval(a->r); 
    if (shouldPrintDebugInfo) printf("%.2f + %.2f = %.2f\n", eval(a->l), eval(a->r), v); 
    break;
  }
  case '-': {
    v = eval(a->l) - eval(a->r); 
    if (shouldPrintDebugInfo) printf("%.2f - %.2f = %.2f\n", eval(a->l), eval(a->r), v); 
    break;
  }
  case '*': {
    v = eval(a->l) * eval(a->r); 
    if (shouldPrintDebugInfo) printf("%.2f * %.2f = %.2f\n", eval(a->l), eval(a->r), v); 
    break;
  }
  case '/': {
    v = eval(a->l) / eval(a->r); 
    if (shouldPrintDebugInfo) printf("%.2f / %.2f = %.2f\n", eval(a->l), eval(a->r), v); 
    break;
  }
  case '|': v = fabs(eval(a->l)); break;
  case 'M': v = -eval(a->l); break;

    /* comparisons */
  case '1': v = (eval(a->l) > eval(a->r))? 1 : 0; break;
  case '2': v = (eval(a->l) < eval(a->r))? 1 : 0; break;
  case '3': v = (eval(a->l) != eval(a->r))? 1 : 0; break;
  case '4': v = (eval(a->l) == eval(a->r))? 1 : 0; break;
  case '5': v = (eval(a->l) >= eval(a->r))? 1 : 0; break;
  case '6': v = (eval(a->l) <= eval(a->r))? 1 : 0; break;

  /* control flow */
  /* null if/else/do expressions allowed in the grammar, so check for them */
  case 'I': 
    if( eval( ((struct flow *)a)->cond) != 0) {
      if( ((struct flow *)a)->tl) {
	v = eval( ((struct flow *)a)->tl);
      } else
	v = 0.0;		/* a default value */
    } else {
      if( ((struct flow *)a)->el) {
        v = eval(((struct flow *)a)->el);
      } else
	v = 0.0;		/* a default value */
    }
    break;

  case 'W':
    v = 0.0;		/* a default value */
    
    if( ((struct flow *)a)->tl) {
      while( eval(((struct flow *)a)->cond) != 0)
	v = eval(((struct flow *)a)->tl);
    }
    break;			/* last value is value */
	              
  case 'L': eval(a->l); v = eval(a->r); break;

  case 'F': v = callbuiltin((struct fncall *)a); break;

  case 'C': v = calluser((struct ufncall *)a); break;

  default: printf("internal error: bad node %c\n", a->nodetype);
  }
  return v;
}

static double
callbuiltin(struct fncall *f)
{
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
   printf("= %4.4g\n", v);
   return v;
 default:
   yyerror("Unknown built-in function %d", functype);
   return 0.0;
 }
}

static double
calluser(struct ufncall *f)
{
  struct symbol *fn = f->s;	/* function name */
  struct symlist *sl;		/* dummy arguments */
  struct ast *args = f->l;	/* actual arguments */
  double *oldval, *newval;	/* saved arg values */
  double v;
  int nargs;
  int i;

  if(!fn->func) {
    yyerror("call to undefined function", fn->name);
    return 0;
  }

  /* count the arguments */
  sl = fn->syms;
  for(nargs = 0; sl; sl = sl->next)
    nargs++;

  /* prepare to save them */
  oldval = (double *)malloc(nargs * sizeof(double));
  newval = (double *)malloc(nargs * sizeof(double));
  if(!oldval || !newval) {
    yyerror("Out of space in %s", fn->name); return 0.0;
  }
  
  /* evaluate the arguments */
  for(i = 0; i < nargs; i++) {
    if(!args) {
      yyerror("too few args in call to %s", fn->name);
      free(oldval); free(newval);
      return 0;
    }

    if(args->nodetype == 'L') {	/* if this is a list node */
      newval[i] = eval(args->l);
      args = args->r;
    } else {			/* if it's the end of the list */
      newval[i] = eval(args);
      args = NULL;
    }
  }
		     
  /* save old values of dummies, assign new ones */
  sl = fn->syms;
  for(i = 0; i < nargs; i++) {
    struct symbol *s = sl->sym;

    oldval[i] = s->value;
    s->value = newval[i];
    sl = sl->next;
  }

  free(newval);

  /* evaluate the function */
  v = eval(fn->func);

  /* put the dummies back */
  sl = fn->syms;
  for(i = 0; i < nargs; i++) {
    struct symbol *s = sl->sym;

    s->value = oldval[i];
    sl = sl->next;
  }

  free(oldval);
  return v;
}


void
treefree(struct ast *a)
{
  switch(a->nodetype) {

    /* two subtrees */
  case '+':
  case '-':
  case '*':
  case '/':
  case '1':  case '2':  case '3':  case '4':  case '5':  case '6':
  case 'L':
    treefree(a->r);

    /* one subtree */
  case '|':
  case 'M': case 'C': case 'F':
    treefree(a->l);

    /* no subtree */
  case 'K': case 'N': case 'Z':
    break;

  case '=':
    free( ((struct symasgn *)a)->v);
    break;

  case 'I': case 'W':
    free( ((struct flow *)a)->cond);
    if( ((struct flow *)a)->tl) free( ((struct flow *)a)->tl);
    if( ((struct flow *)a)->el) free( ((struct flow *)a)->el);
    break;

  default: printf("internal error: free bad node %c\n", a->nodetype);
  }	  
  
  free(a); /* always free the node itself */

}

void
yyerror(char *s, ...)
{
  va_list ap;
  va_start(ap, s);

  fprintf(stderr, "%d: error: ", yylineno);
  vfprintf(stderr, s, ap);
  fprintf(stderr, "\n");
}

// int
// main()
// {
//   printf("> "); 
//   return yyparse();
// }

int main(int argc, char **argv){
  // extern int yydebug;
  // yydebug = 1;

  if(argc>1)
      {
        yyin = fopen(argv[1],"r");
        if(yyin == 0)
            yyin = stdin;
      }
  else
      yyin = stdin;

  yyparse();

  // DD Symbol Table
  if (shouldPrintDebugInfo){
    struct symbol *s;
    for (int i=0; i<NHASH; i++){
      s = &symtab[i];

      if (s->value != 0){
        if (s->return_type == 'Z') printf("Symbol '%s' : %c is %.2f\n", s->name, s->return_type, s->value);
        if (s->return_type == 'K') printf("Symbol '%s' : %c is %d\n", s->name, s->return_type, s->value_int);
      }
    }
  }

  return 0;
}

/* debugging: dump out an AST */
int debug = 0;
void
dumpast(struct ast *a, int level)
{

  printf("%*s", 2*level, "");	/* indent to this level */
  level++;

  if(!a) {
    printf("NULL\n");
    return;
  }

  switch(a->nodetype) {
    /* int constant */
  case 'K': printf("number %d\n", ((struct numval_int *)a)->number); break;

    /* double constant */
  case 'Z': printf("number %4.4g\n", ((struct numval_double *)a)->number); break;

    /* name reference */
  case 'N': printf("ref %s\n", ((struct symref *)a)->s->name); break;

    /* assignment */
  case '=': printf("= %s\n", ((struct symref *)a)->s->name);
    dumpast( ((struct symasgn *)a)->v, level); return;

    /* expressions */
  case '+': case '-': case '*': case '/': case 'L':
  case '1': case '2': case '3':
  case '4': case '5': case '6': 
    printf("binop %c\n", a->nodetype);
    dumpast(a->l, level);
    dumpast(a->r, level);
    return;

  case '|': case 'M': 
    printf("unop %c\n", a->nodetype);
    dumpast(a->l, level);
    return;

  case 'I': case 'W':
    printf("flow %c\n", a->nodetype);
    dumpast( ((struct flow *)a)->cond, level);
    if( ((struct flow *)a)->tl)
      dumpast( ((struct flow *)a)->tl, level);
    if( ((struct flow *)a)->el)
      dumpast( ((struct flow *)a)->el, level);
    return;
	              
  case 'F':
    printf("builtin %d\n", ((struct fncall *)a)->functype);
    dumpast(a->l, level);
    return;

  case 'C': printf("call %s\n", ((struct ufncall *)a)->s->name);
    dumpast(a->l, level);
    return;

  default: printf("bad %c\n", a->nodetype);
    return;
  }
}
