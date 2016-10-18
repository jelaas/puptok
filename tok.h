#include <stdio.h>

enum tokens {
  ERR,
  TEOF,
  SPACE,
  STR,
  NUM,
  LPAREN,
  RPAREN,
  STRLIT,
  LMAS,
  RMAS,
  HASHCOMMENT,
  NEWLINE,
  COLON,
  PASSIGN,
  COMMA,
  LARR,
  RARR,
  PSTRLIT,
  CCOMMENT,
  DIV,
  ARROW,
  EQUALS,
  VAR,
  MINUS,
  NODE,
  CLASS,
  DEFINE,
  NOTEQUALS,
  NOT,
  LESSTHAN,
  MORETHAN,
  LESSEQUALTHAN,
  MOREEQUALTHAN,
  PLUS,
  INHERITS,
  QMARK,
  PMATCH,
  NOTIFYARROW
};

struct tok {
  FILE *f;
  int token;
  int state;
  int count;
  int line;
  int eof, escape, prev;
  char val[16];
};

int tok(struct tok *t);
char *tokname(int token);
