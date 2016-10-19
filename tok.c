/*
 * File: tok.c
 * Implements: puppet manifest tokenizer (partial)
 *
 * Copyright: Jens Låås, 2016
 * Copyright license: According to GPL, see file COPYING in this directory.
 *
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "tok.h"

static char *toknames[] = {
  "ERR",
  "EOF",
  "SPACE",
  "STR",
  "NUM",
  "LPAREN",
  "RPAREN",
  "STRLIT",
  "LMAS",
  "RMAS",
  "HASHCOMMENT",
  "NEWLINE",
  "COLON",
  "PASSIGN",
  "COMMA",
  "LARR",
  "RARR",
  "PSTRLIT",
  "CCOMMENT",
  "DIV",
  "ARROW",
  "EQUALS",
  "VAR",
  "MINUS",
  "NODE",
  "CLASS",
  "DEFINE",
  "NOTEQUALS",
  "NOT",
  "LESSTHAN",
  "MORETHAN",
  "LESSEQUALTHAN",
  "MOREEQUALTHAN",
  "PLUS",
  "INHERITS",
  "QMARK",
  "PMATCH",
  "NOTIFYARROW",
  "REGEX",
  "PNOTMATCH",
  "SEMICOLON",
  "RCOLL",
  "RECOLL",
  "LCOLL",
  "LECOLL",
  "LEFTSHIFT"
};

char *tokname(int token)
{
	return toknames[token];
}

static int dtok(struct tok *t, int c)
{
	t->state = ERR;
	if(isalpha(c)) {
		t->state = STR;
	}
	if(isdigit(c)) {
		t->state = NUM;
	}
	if(isspace(c)) {
		t->state = SPACE;
	}
	if(c == '\n') {
		t->state = NEWLINE;
	}
	if(c == '\'') {
		t->state = STRLIT;
	}
	if(c == '\"') {
		t->state = PSTRLIT;
	}
	if(c == '{') {
		t->state = LMAS;
	}
	if(c == '}') {
		t->state = RMAS;
	}
	if(c == '(') {
		t->state = LPAREN;
	}
	if(c == ')') {
		t->state = RPAREN;
	}
	if(c == '[') {
		t->state = LARR;
	}
	if(c == ']') {
		t->state = RARR;
	}
	if(c == '#') {
		t->state = HASHCOMMENT;
	}
	if(c == ':') {
		t->state = COLON;
	}
	if(c == ';') {
		t->state = SEMICOLON;
	}
	if(c == ',') {
		t->state = COMMA;
	}
	if(c == '?') {
		t->state = QMARK;
	}
	if(c == '=') {
		t->state = PASSIGN;
	}
	if(c == '!') {
		t->state = NOTEQUALS;
	}
	if(c == '-') {
		t->state = ARROW;
	}
	if(c == '~') {
		t->state = NOTIFYARROW;
	}
	if(c == '/') {
		t->state = CCOMMENT;
	}
	if(c == '$') {
		t->state = VAR;
	}
	if(c == '<') {
		t->state = LESSEQUALTHAN;
	}
	if(c == '>') {
		t->state = MOREEQUALTHAN;
	}
	if(c == '+') {
		t->state = PLUS;
	}
	if(c == '|') {
		t->state = RECOLL;
	}
	return 0;
}

static int rettok(struct tok *t, int tok)
{
	if(tok != SPACE && tok != NEWLINE && tok != HASHCOMMENT && tok != CCOMMENT)
		t->prevtok = tok;
	return tok;
}

int tok(struct tok *t)
{
	int c;
	
	while(t->state != ERR) {
		if(t->eof) return rettok(t, TEOF);
		c = getc(t->f);
		if(c == -1) {
			t->eof = 1;
			if(t->state == SPACE) return rettok(t, t->state);
			if(t->state == STR) return rettok(t, t->state);
			if(t->state == NUM) return rettok(t, t->state);
			if(t->state == HASHCOMMENT) return rettok(t, t->state);
			return rettok(t, TEOF);
		}
		ungetc(c, t->f);
		
		switch(t->state) {
		case SPACE:
			if(c == '\n') {
				t->line++;
				getc(t->f);
				t->count = 0;
				return rettok(t, NEWLINE);
			}
   			if(isspace(c)) {
				getc(t->f);
				t->count++;
				continue;
			}
			dtok(t, c);
			if(t->count) {
				t->count = 0;
				return rettok(t, SPACE);
			}
			break;
		case STR:
			if(isalnum(c)||strchr("_:", c)) {
				if(t->count < (sizeof(t->val)-1)) {
					t->val[t->count] = c;
					t->val[t->count+1] = 0;
				}
				getc(t->f);
				t->count++;
				continue;
			}
			dtok(t, c);
			if(t->count) {
				int n = STR;
				
				t->count = 0;
				if(!strcmp(t->val, "node")) n = NODE;
				if(!strcmp(t->val, "class")) n = CLASS;
				if(!strcmp(t->val, "define")) n = DEFINE;
				if(!strcmp(t->val, "inherits")) n = INHERITS;
				t->val[0] = 0;
				return rettok(t, n);
			}			
			break;
		case VAR:
			if(t->count == 0 || isalnum(c)||strchr("_:", c)) {
				getc(t->f);
				t->count++;
				continue;
			}
			dtok(t, c);
			if(t->count) {
				t->count = 0;
				return rettok(t, VAR);
			}
			break;
		case HASHCOMMENT:
			if(c != '\n') {
				getc(t->f);
				t->count++;
                                continue;
			}
			t->state = SPACE;
			t->count = 0;
			return rettok(t, HASHCOMMENT);
			break;
		case STRLIT:
			if(c != '\'' || t->escape) {
				if(c == '\\' && (t->escape == 0))
					t->escape = 1;
				else
					t->escape = 0;
				getc(t->f);
				t->count++;
				continue;
			}
			if(c == '\'') {
				if(t->count == 0) {
					getc(t->f);
					t->count++;
					continue;
				}
				getc(t->f);
			}
			t->state = SPACE;
			if(t->count) {
				t->count = 0;
				return rettok(t, STRLIT);
			}
			break;
		case PSTRLIT:
			if(c != '\"' || t->escape) {
				if(c == '\\' && (t->escape == 0))
					t->escape = 1;
				else
					t->escape = 0;
				if(c == '\n') t->line++;
				getc(t->f);
				t->count++;
				continue;
			}
			if(c == '\"') {
				if(t->count == 0) {
					getc(t->f);
					t->count++;
					continue;
				}
				getc(t->f);
			}
			t->state = SPACE;
			if(t->count) {
				t->count = 0;
				return rettok(t, PSTRLIT);
			}
			break;
		case PASSIGN:
			if(t->count == 0) {
				getc(t->f);
				t->count++;
				continue;
			}
			if(t->count == 1) {
				if(c == '>') {
					getc(t->f);
					t->count = 0;
					t->state = SPACE;
					return rettok(t, PASSIGN);
				}
			}
			if(t->count == 1) {
				if(c == '~') {
					getc(t->f);
					t->count = 0;
					t->state = REGEX;
					t->phase = SPACE;
					t->escape = 0;
					return rettok(t, PMATCH);
				}
			}
			dtok(t, c);
			t->count = 0;
			return rettok(t, EQUALS);
		case REGEX:
			switch(t->phase) {
			case SPACE:
				if(isspace(c)) {
					getc(t->f);
					t->count++;
					continue;
				}
				if(c == '/') {
					t->phase = DIV;
					getc(t->f);
					if(t->count) return rettok(t, SPACE);
					continue;
				}
			case DIV:
				if(c == '\\') {
					t->escape = 1;
					getc(t->f);
					continue;
				}
				if(t->escape == 0 && c == '/') {
					getc(t->f);
					t->state = SPACE;
					return rettok(t, REGEX);
				}
				getc(t->f);
				t->escape = 0;
				continue;
			}
			return rettok(t, ERR);
		case NOTEQUALS:
			if(t->count == 0) {
				getc(t->f);
				t->count++;
				continue;
			}
			if(t->count == 1) {
				if(c == '=') {
					getc(t->f);
					t->count = 0;
					t->state = SPACE;
					return rettok(t, NOTEQUALS);
				}
				if(c == '~') {
					getc(t->f);
					t->count = 0;
					t->state = REGEX;
					t->phase = SPACE;
					t->escape = 0;
					return rettok(t, PNOTMATCH);
				}
			}
			dtok(t, c);
			t->count = 0;
			return rettok(t, NOT);
		case LESSEQUALTHAN:
			if(t->count == 0) {
				getc(t->f);
				t->count++;
				continue;
			}
			if(t->count == 1) {
				if(c == '=') {
					getc(t->f);
					t->count = 0;
					t->state = SPACE;
					return rettok(t, LESSEQUALTHAN);
				}
				if(c == '|') {
					getc(t->f);
					t->count = 0;
					t->state = SPACE;
					return rettok(t, LCOLL);
				}
				if(c == '<') {
					getc(t->f);
					t->count = 0;
					t->state = LEFTSHIFT;
					continue;
				}
			}
			dtok(t, c);
			t->count = 0;
			return rettok(t, LESSTHAN);
		case RECOLL:
			if(t->count == 0) {
                                getc(t->f);
                                t->count++;
                                continue;
                        }
			if(t->count == 1) {
				if(c == '>') {
					getc(t->f);
					t->count++;
					continue;
				}
				return rettok(t, ERR);
			}
			if(c == '>') {
				getc(t->f);
				t->count = 0;
				t->state = SPACE;
				return rettok(t, RECOLL);
			}
			dtok(t, c);
                        t->count = 0;
			return rettok(t, RCOLL);
		case LEFTSHIFT:
			if(c == '|') {
				getc(t->f);
				t->count = 0;
				t->state = SPACE;
				return rettok(t, LECOLL);
			}
			dtok(t, c);
			t->count = 0;
			return rettok(t, LEFTSHIFT);
		case MOREEQUALTHAN:
			if(t->count == 0) {
				getc(t->f);
				t->count++;
				continue;
			}
			if(t->count == 1) {
				if(c == '=') {
					getc(t->f);
					t->count = 0;
					t->state = SPACE;
					return rettok(t, MOREEQUALTHAN);
				}
			}
			dtok(t, c);
			t->count = 0;
			return rettok(t, MORETHAN);
		case ARROW:
			if(t->count == 0) {
				getc(t->f);
				t->count++;
				continue;
			}
			if(t->count == 1) {
				if(c == '>') {
					getc(t->f);
					t->count = 0;
					t->state = SPACE;
					return rettok(t, ARROW);
				}
			}
			dtok(t, c);
			t->count = 0;
                        return rettok(t, MINUS);
			break;
		case NOTIFYARROW:
			if(t->count == 0) {
				getc(t->f);
				t->count++;
				continue;
			}
			if(t->count == 1) {
				if(c == '>') {
					getc(t->f);
					t->count = 0;
					t->state = SPACE;
					return rettok(t, NOTIFYARROW);
				}
			}
                        return rettok(t, ERR);
		case CCOMMENT:
			if(t->count == 0) {
				getc(t->f);
				t->count++;
				t->prev = 0;
				continue;
			}
			if(t->count == 1) {
				if(c != '*') {
					dtok(t, c);
					t->count = 0;
					if(t->prevtok == RMAS ||
					   t->prevtok == LMAS ||
					   t->prevtok == LPAREN ||
					   t->prevtok == LARR ||
					   t->prevtok == EQUALS ||
					   t->prevtok == COMMA
						) {
						t->state = REGEX;
						t->phase = DIV;
						t->count = 0;
						continue;
					}
					return rettok(t, DIV);
				}
			}
			if(c == '/' && t->prev == '*') {
				getc(t->f);
				t->count = 0;
				t->state = SPACE;
				return rettok(t, CCOMMENT);
			}
			t->prev = c;
			getc(t->f);
			t->count++;
			if(c == '\n') t->line++;
			continue;
		case NUM:
			if(isdigit(c)) {
				getc(t->f);
				t->count++;
				continue;
			}
			dtok(t, c);
			if(t->count) {
				t->count = 0;
				return rettok(t, NUM);
			}
			break;
		case LMAS:
			getc(t->f);
			t->state = SPACE;
			return rettok(t, LMAS);
		case RMAS:
			getc(t->f);
			t->state = SPACE;
			return rettok(t, RMAS);
		case LPAREN:
			getc(t->f);
			t->state = SPACE;
			return rettok(t, LPAREN);
		case RPAREN:
			getc(t->f);
			t->state = SPACE;
			return rettok(t, RPAREN);
		case LARR:
			getc(t->f);
			t->state = SPACE;
			return rettok(t, LARR);
		case RARR:
			getc(t->f);
			t->state = SPACE;
			return rettok(t, RARR);
		case NEWLINE:
			getc(t->f);
			t->line++;
			t->state = SPACE;
			return rettok(t, NEWLINE);
		case COLON:
			if(t->count == 0) {
				getc(t->f);
				t->count++;
				continue;
			}
			if(c == ':') {
				t->state = STR;
				continue;
			}
			dtok(t, c);
			t->count = 0;
			return rettok(t, COLON);
		case COMMA:
			getc(t->f);
			t->state = SPACE;
			return rettok(t, COMMA);
		case SEMICOLON:
			getc(t->f);
			t->state = SPACE;
			return rettok(t, SEMICOLON);
		case PLUS:
			getc(t->f);
			t->state = SPACE;
			return rettok(t, PLUS);
		case QMARK:
			getc(t->f);
			t->state = SPACE;
			return rettok(t, QMARK);
		}
	}
	return t->state;
}
