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
	int verbose, files, module;
	int quiet, silent;
} conf;

struct {
	int nodes, classes, defines;
	int paren, arr, mas, global, topmas, topmasline;
} var;

int check(struct tok *t, const char *fn)
{
	int token = SPACE, rc=0;
	char buf[256];
	int prev = SPACE;
	int indefine=0;

	if(conf.verbose) printf("\n%d ", t->line);
	while(token != TEOF) {
		if(token != SPACE) prev = token;
		token = tok(t);
		if(token == NODE) var.nodes++;
		if(token == CLASS && var.mas == 0) {
			indefine = 1;
			var.classes++;
		}
		if(token == DEFINE && var.mas == 0) {
			indefine = 1;
			var.defines++;
		}
		if(token == LMAS) {
			if(var.paren == 0) indefine = 0;
			if(indefine == 0 && var.mas == 0) {
				var.topmas++;
				var.topmasline = t->line;
			}
			var.mas++;
		}
		if(token == RMAS) var.mas--;
		if(token == LARR) var.arr++;
		if(token == RARR) var.arr--;
		if(token == LPAREN) var.paren++;
		if(token == RPAREN) {
			if(var.paren == 1) indefine = 0;
			var.paren--;
		}
		if(token == STR && var.mas == 0 && var.global == 0 ) {
			if(conf.module == 0)
				var.global = t->line;
			else {
				if(indefine == 0 && prev != CLASS && prev != DEFINE && prev != INHERITS)
					var.global = t->line;
			}
		}
		if(token == ERR) {
			if(conf.verbose) printf("\n");
			fflush(stdout);
			if(!conf.silent) {
				if(fn) fprintf(stderr, "%s: ", fn);
				fprintf(stderr, "SYNTAX ERROR! Line %d at: %s\n", t->line, fgets(buf, sizeof(buf), t->f));
			}
			return 1;
		}
		if(conf.verbose) {
			printf("%s ", tokname(token));
			if(token == NEWLINE) printf("\n%d ", t->line);
		}
	}
	if(conf.verbose) printf("\n");
	if(!conf.quiet) {
		if(fn) fprintf(stderr, "%s: ", fn);
		fprintf(stderr, "SYNTAX OK\n");
	}
	if(conf.module == 0 && var.nodes > 1) {
		if(!conf.silent) {
			if(fn) fprintf(stderr, "%s: ", fn);
			fprintf(stderr, "GRAMMAR ERROR! Multiple node definitions!\n");
		}
		rc = 1;
	}
	if(conf.module && (var.classes + var.defines) > 1) {
		if(!conf.silent) {
			if(fn) fprintf(stderr, "%s: ", fn);
			fprintf(stderr, "GRAMMAR ERROR! Multiple definitions in module!\n");
		}
		rc = 1;
	}
	if(conf.module && var.nodes) {
		if(!conf.silent) {
			if(fn) fprintf(stderr, "%s: ", fn);
			fprintf(stderr, "GRAMMAR ERROR! Node definition in module!\n");
		}
		rc = 1;
	}
	if(conf.module == 0 && var.nodes == 0) {
		if(!conf.silent) {
			if(fn) fprintf(stderr, "%s: ", fn);
			fprintf(stderr, "GRAMMAR ERROR! Node definition missing!\n");
		}
		rc = 1;
	}
	if(conf.module && (var.classes + var.defines) == 0) {
		if(!conf.silent) {
			if(fn) fprintf(stderr, "%s: ", fn);
			fprintf(stderr, "GRAMMAR ERROR! Class or define missing!\n");
		}
		rc = 1;
	}
	if(var.mas) {
		if(!conf.silent) {
			if(fn) fprintf(stderr, "%s: ", fn);
			fprintf(stderr, "GRAMMAR ERROR! Unbalanced {} !\n");
		}
		rc = 1;
	}
	if(var.arr) {
		if(!conf.silent) {
			if(fn) fprintf(stderr, "%s: ", fn);
			fprintf(stderr, "GRAMMAR ERROR! Unbalanced [] !\n");
		}
		rc = 1;
	}
	if(var.paren) {
		if(!conf.silent) {
			if(fn) fprintf(stderr, "%s: ", fn);
			fprintf(stderr, "GRAMMAR ERROR! Unbalanced () !\n");
		}
		rc = 1;
	}
	if(var.global) {
		if(!conf.silent) {
			if(fn) fprintf(stderr, "%s: ", fn);
			fprintf(stderr, "GRAMMAR ERROR! Multiple GLOBAL definitions at line: %d!\n", var.global);
		}
		rc = 1;
	}
	if(var.topmas > 1) {
		if(!conf.silent) {
			if(fn) fprintf(stderr, "%s: ", fn);
			fprintf(stderr, "GRAMMAR ERROR! Multiple GLOBAL definitions in {} at line: %d!\n", var.topmasline);
		}
		rc = 1;
	}
	if(rc == 0) {
		if(!conf.quiet) {
			if(fn) fprintf(stderr, "%s: ", fn);
			fprintf(stderr, "GRAMMAR OK\n");
		}
	}
	return rc;
}


int main(int argc, char **argv)
{
	int rc=0;
	struct tok t;

	while(argc > 1) {
		if(strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
			printf("Usage: pup [-v] [-h] [-q] [-s]\n"
			       " -v        verbose\n"
			       " -h        help\n"
			       " -q        quiet\n"
			       " --files   read filenames on stdin. check each file\n"
			       " --module  check puppet module instead of node manifest\n"
			       " -s        completely silent\n\n"
			       " Expects puppet node manifest on stdin.\n"
			       " If --module is given check module manifest\n"
				);
			exit(0);
		}
		if(!strcmp(argv[1], "-q")) conf.quiet = 1;
		if(!strcmp(argv[1], "-s")) conf.quiet = conf.silent = 1;
		if(!strcmp(argv[1], "-v")) conf.verbose++;
		if(!strcmp(argv[1], "--files")) conf.files = 1;
		if(!strcmp(argv[1], "-m")) conf.module = 1;
		if(!strcmp(argv[1], "--mod")) conf.module = 1;
		if(!strcmp(argv[1], "--module")) conf.module = 1;
		if(!strcmp(argv[1], "--verbose")) conf.verbose++;
		argc--;
		argv++;
	}

	if(!conf.files) {
		memset(&t, 0, sizeof(struct tok));
		t.state = SPACE;
		t.f = stdin;
		t.line = 1;
		rc = check(&t, (void*)0);
	}

	if(conf.files) {
		size_t fnsize = 2048;
		char *fn, *p;
		FILE *f;

		fn = malloc(fnsize);
		while(fgets(fn, fnsize, stdin)) {
			if(!strchr(fn, '\n')) {
				if(!conf.quiet) fprintf(stderr, "INPUT ERROR. MISSING NEWLINE IN %s\n", fn);
				exit(2);
			}
			p = strrchr(fn, '\n');
			*p = 0;
			f = fopen(fn, "r");
			if(!f) {
				if(!conf.quiet) fprintf(stderr, "INPUT ERROR. Could not open file '%s'\n", fn);
				exit(2);
			}
			if(conf.verbose > 1) fprintf(stderr, "Processing file '%s'\n", fn);
			memset(&t, 0, sizeof(struct tok));
			t.state = SPACE;
			t.f = f;
			t.line = 1;

			memset(&var, 0, sizeof(var));
			rc |= check(&t, fn);
			fclose(f);
		}
	}
	exit(rc);
}

