%option noyywrap

%{
#include <stdio.h>

#define YY_DECL int yylex()

#include "structured_variable_name.tab.h"

%}

q_name		      "'"({q_char}|{s_escape})+"'"
nondigit	      [_a-zA-Z]
digit		      [0-9]
q_char		      {nondigit}|{digit}|[!#$%&()*+,-\./:;<>=?@\[\]\^{}|~ ]
s_escape	      ("\\'")|("\\\"")|("\\?")|("\\\\")|("\\a")|("\\b")|("\\f")|("\\n")|("\\r")|("\\t")|("\\v")
unsigned_integer  {digit}+

%%

"der("             { return DER; }
"("                { return '('; }
")"                { return ')'; }
","                { return ','; }
"."                { return '.'; }
"["                { return '['; }
"]"                { return ']'; }
{q_name}           { return Q_NAME; }
{nondigit}         { return NONDIGIT; }
{unsigned_integer} { return UNSIGNED_INTEGER; }
.                  { return *yytext; }

%%

void set_input_string(const char* in) {
	yy_scan_string(in);
}

void end_lexical_scan(void) {
	yy_delete_buffer(YY_CURRENT_BUFFER);
}
