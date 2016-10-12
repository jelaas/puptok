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
  "NODE"
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
	if(c == ',') {
		t->state = COMMA;
	}
	if(c == '=') {
		t->state = PASSIGN;
	}
	if(c == '-') {
		t->state = ARROW;
	}
	if(c == '/') {
		t->state = CCOMMENT;
	}
	if(c == '$') {
		t->state = VAR;
	}
	return 0;
}

int tok(struct tok *t)
{
	int c;
	
	while(t->state != ERR) {
		if(t->eof) return TEOF;
		c = getc(t->f);
		if(c == -1) {
			t->eof = 1;
			if(t->state == SPACE) return t->state;
			if(t->state == STR) return t->state;
			if(t->state == NUM) return t->state;
			if(t->state == HASHCOMMENT) return t->state;
			return TEOF;
		}
		ungetc(c, t->f);
		
		switch(t->state) {
		case SPACE:
			if(c == '\n') {
				t->line++;
				getc(t->f);
				t->count = 0;
				return NEWLINE;
			}
			if(isspace(c)) {
				getc(t->f);
				t->count++;
				continue;
			}
			dtok(t, c);
			if(t->count) {
				t->count = 0;
				return SPACE;
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
				t->count = 0;
				if(!strcmp(t->val, "node")) {
					t->val[0] = 0;
					return NODE;
				}
				t->val[0] = 0;
				return STR;
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
				return VAR;
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
			return HASHCOMMENT;
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
				return STRLIT;
			}
			break;
		case PSTRLIT:
			if(c != '\"' || t->escape) {
				if(c == '\\' && (t->escape == 0))
					t->escape = 1;
				else
					t->escape = 0;
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
				return PSTRLIT;
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
					return PASSIGN;
				}
			}
			dtok(t, c);
			t->count = 0;
			t->state = SPACE;
			return EQUALS;
			break;
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
					return ARROW;
				}
			}
			dtok(t, c);
			t->count = 0;
                        t->state = SPACE;
                        return MINUS;
			break;
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
					return DIV;
				}
			}
			if(c == '/' && t->prev == '*') {
				getc(t->f);
				t->count = 0;
				t->state = SPACE;
				return CCOMMENT;
			}
			t->prev = c;
			getc(t->f);
			t->count++;
			if(c == '\n') t->line++;
			continue;
			break;
		case NUM:
			if(isdigit(c)) {
				getc(t->f);
				t->count++;
				continue;
			}
			dtok(t, c);
			if(t->count) {
				t->count = 0;
				return NUM;
			}			
			break;
		case LMAS:
			getc(t->f);
			t->state = SPACE;
			return LMAS;
			break;
		case RMAS:
			getc(t->f);
			t->state = SPACE;
			return RMAS;
			break;
		case LPAREN:
			getc(t->f);
			t->state = SPACE;
			return LPAREN;
			break;
		case RPAREN:
			getc(t->f);
			t->state = SPACE;
			return RPAREN;
			break;
		case LARR:
			getc(t->f);
			t->state = SPACE;
			return LARR;
			break;
		case RARR:
			getc(t->f);
			t->state = SPACE;
			return RARR;
			break;
		case NEWLINE:
			getc(t->f);
			t->line++;
			t->state = SPACE;
			return NEWLINE;
			break;
		case COLON:
			getc(t->f);
			t->state = SPACE;
			return COLON;
			break;
		case COMMA:
			getc(t->f);
			t->state = SPACE;
			return COMMA;
			break;
		}
	}
	return t->state;
}
