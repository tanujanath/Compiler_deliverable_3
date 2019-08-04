%{
    #include<stdlib.h>
    #include "library.h"
    #include "parser.tab.h"
    // int yyerror(const char *s);
    short shouldPrintDebugInfo = 0;
%}

alpha       [a-zA-Z]
digit       [0-9]
alnum       {alpha}|{digit}
print       [ -~]

ID          {alpha}+{alnum}*
ICONST      "0"|[0-9]{digit}*
FCONST      "0"|{digit}*"."{digit}+

%option noyywrap yylineno

%%

"//".* {/*ignore comments*/}
"\"mode\ debug\"" {shouldPrintDebugInfo = 1;printf("DEBUG MODE ENABLED\n");}
"int" {return INT;}
"double" {return DOUBLE;}
"return" {return RETURN;}
"extern" {return EXTERN;}
"if" {return IF;}
"else" {return ELSE;}
"for" {return FOR;}
"in" {return IN;}
"to" {return TO;}
"by" {return BY;}
"def" {return DEF;}
"=>" {return ARROW;}
"=="    { yylval.fn = 4; return OP_EQ; }
"!="    { yylval.fn = 3; return OP_NE;}
"<"     { yylval.fn = 2; return OP_LT;}
"<="    { yylval.fn = 6; return OP_LTE;}
">"     { yylval.fn = 1; return OP_GT;}
">="    { yylval.fn = 5; return OP_GTE;}
"," {return COMMA;}
{ID} {yylval.s = lookup(yytext);return ID;}
{ICONST} {yylval.i = atoi(yytext); return INT_VAL;}
{FCONST} {yylval.d = atof(yytext); return DOUBLE_VAL;}
[\(\)\{\}\:\+\-\*\/\=\<\>\%\!] return yytext[0];
[ \t] ;
[\n] ;
.		{ yyerror("Unknown Character"); exit(1);	}

%%