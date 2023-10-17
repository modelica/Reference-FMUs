%{

#include <stdio.h>
#include <stdlib.h>

extern int yylex();
extern int yyparse();
extern FILE* yyin;

void yyerror(void* variable, const char* s);
%}

%parse-param {void* variable}

%define parse.error verbose

%token DER UNSIGNED_INTEGER NONDIGIT Q_NAME

%start name

%%

name:
	identifier_list
	| DER identifier_list ')'
	| DER identifier_list ',' UNSIGNED_INTEGER ')'
;

identifier_list:
	identifier
	| identifier_list '.' identifier
;

identifier:
	bname
	| bname '[' array_indices ']'
;

bname:
	NONDIGIT nondigit_or_digit
	| Q_NAME
;

nondigit_or_digit:
	/* empty */
	| UNSIGNED_INTEGER nondigit_or_digit
	| NONDIGIT nondigit_or_digit
;

array_indices:
	UNSIGNED_INTEGER
	| array_indices ',' UNSIGNED_INTEGER
;

%%
