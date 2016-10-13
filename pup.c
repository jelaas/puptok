/*
 * File: pup.c
 * Implements: puppet manifest checker
 *
 * Copyright: Jens Låås, 2016
 * Copyright license: According to GPL, see file COPYING in this directory.
 *
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "tok.h"

struct {
	int verbose;
	int quiet, silent;
} conf;

struct {
	int nodes;
	int paren, arr, mas, global, topmas, topmasline;
} var;

int main(int argc, char **argv)
{
	int token, rc=0;
	struct tok t;
	char buf[256];

	while(argc > 1) {
		if(strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
			printf("Usage: pup [-v] [-h] [-q] [-s]\n"
			       " -v        verbose\n"
			       " -h        help\n"
			       " -q        quiet\n"
			       " -s        completely silent\n\n"
			       " Expects puppet node manifest on stdin.\n");
			exit(0);
		}
		if(!strcmp(argv[1], "-q")) conf.quiet = 1;
		if(!strcmp(argv[1], "-s")) conf.quiet = conf.silent = 1;
		if(!strcmp(argv[1], "-v")) conf.verbose++;
		if(!strcmp(argv[1], "--verbose")) conf.verbose++;
		argc--;
		argv++;
	}

	memset(&t, 0, sizeof(struct tok));
	t.state = SPACE;
	t.f = stdin;
	t.line = 1;
	
	if(conf.verbose) printf("\n%d ", t.line);
	while(token != TEOF) {
		token = tok(&t);
		if(token == NODE) var.nodes++;
		if(token == LMAS) {
			if(var.mas == 0) {
				var.topmas++;
				var.topmasline = t.line;
			}
			var.mas++;
		}
		if(token == RMAS) var.mas--;
		if(token == LARR) var.arr++;
		if(token == RARR) var.arr--;
		if(token == LPAREN) var.paren++;
		if(token == RPAREN) var.paren--;
		if(token == STR && var.mas == 0 && var.global == 0) var.global = t.line;
		if(token == ERR) {
			if(conf.verbose) printf("\n");
			fflush(stdout);
			if(!conf.silent) fprintf(stderr, "SYNTAX ERROR! Line %d at: %s\n", t.line, fgets(buf, sizeof(buf), t.f));
			exit(1);
		}
		if(conf.verbose) {
			printf("%s ", tokname(token));
			if(token == NEWLINE) printf("\n%d ", t.line);
		}
	}
	if(conf.verbose) printf("\n");
	if(!conf.quiet) fprintf(stderr, "SYNTAX OK\n");
	if(var.nodes > 1) {
		if(!conf.silent) fprintf(stderr, "GRAMMAR ERROR! Multiple node definitions!\n");
		rc = 1;
	}
	if(var.nodes == 0) {
		if(!conf.silent) fprintf(stderr, "GRAMMAR ERROR! Node definition missing!\n");
		rc = 1;
	}
	if(var.mas) {
		if(!conf.silent) fprintf(stderr, "GRAMMAR ERROR! Unbalanced {} !\n");
		rc = 1;
	}
	if(var.arr) {
		if(!conf.silent) fprintf(stderr, "GRAMMAR ERROR! Unbalanced [] !\n");
		rc = 1;
	}
	if(var.paren) {
		if(!conf.silent) fprintf(stderr, "GRAMMAR ERROR! Unbalanced () !\n");
		rc = 1;
	}
	if(var.global) {
		if(!conf.silent) fprintf(stderr, "GRAMMAR ERROR! Multiple GLOBAL definitions at line: %d!\n", var.global);
		rc = 1;
	}
	if(var.topmas > 1) {
		if(!conf.silent) fprintf(stderr, "GRAMMAR ERROR! Multiple GLOBAL definitions at line: %d!\n", var.topmasline);
		rc = 1;
	}
	if(rc == 0) {
		if(!conf.quiet) fprintf(stderr, "GRAMMAR OK\n");
	}
	exit(rc);
}
