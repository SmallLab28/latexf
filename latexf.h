#ifndef LATEX_H_INCLUDED
#define LATEX_H_INCLUDED

#include <ctype.h>
#include <math.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* PDF font resource names used by emitted LaTeXF content streams. */
#ifndef LATEXF_PDF_FONT_NORMAL_NAME
#define LATEXF_PDF_FONT_NORMAL_NAME "F1"
#endif
#ifndef LATEXF_PDF_FONT_ITALIC_NAME
#define LATEXF_PDF_FONT_ITALIC_NAME "F2"
#endif

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#define ISATTY _isatty
#define FILENO _fileno
#else
#include <unistd.h>
#define ISATTY isatty
#define FILENO fileno
#endif

#define MAX_PARSE_DEPTH  512
#define MAX_LAYOUT_DEPTH 512
#define MAX_STEPS        10000000
#define MAX_INPUT_BYTES  (10 * 1024 * 1024) /* 10 MB */

/* ---------------- 0. BULLETPROOF MEMORY HELPERS ---------------- */

static void *xmalloc(size_t sz) {
	if (sz == 0) sz = 1;
	void *p = malloc(sz);
	if (!p) {
		fprintf(stderr, "[FATAL] Out of memory (%zu bytes)\n", sz);
		exit(EXIT_FAILURE);
	}
	return p;
}

static void *xcalloc(size_t count, size_t sz) {
	if (count == 0 || sz == 0) { count = 1; sz = 1; }
	void *p = calloc(count, sz);
	if (!p) {
		fprintf(stderr, "[FATAL] Out of memory (%zu x %zu bytes)\n", count, sz);
		exit(EXIT_FAILURE);
	}
	return p;
}

static void *xrealloc(void *ptr, size_t sz) {
	if (sz == 0) sz = 1;
	void *p = realloc(ptr, sz);
	if (!p) {
		fprintf(stderr, "[FATAL] Out of memory on realloc (%zu bytes)\n", sz);
		free(ptr);
		exit(EXIT_FAILURE);
	}
	return p;
}

static char *xstrdup_range(const char *start, size_t len) {
	char *out = (char *)xmalloc(len + 1);
	if (len > 0 && start) memcpy(out, start, len);
	out[len] = '\0';
	return out;
}

/* ---------------- 1. SYMBOL & FUNCTION TABLE ---------------- */

typedef enum {
	SYM_TEXT,
	SYM_FUNC,
	SYM_BIG_OP
} SymKind;

typedef struct {
	const char *tex_name;
	const char *utf8;
	SymKind kind;
} SymbolEntry;

static const SymbolEntry SYMBOL_TABLE[] = {
	/* Lowercase Greek */
	{"alpha",      "\xce\xb1", SYM_TEXT},
	{"beta",       "\xce\xb2", SYM_TEXT},
	{"gamma",      "\xce\xb3", SYM_TEXT},
	{"delta",      "\xce\xb4", SYM_TEXT},
	{"epsilon",    "\xcf\xb5", SYM_TEXT},
	{"varepsilon", "\xce\xb5", SYM_TEXT},
	{"zeta",       "\xce\xb6", SYM_TEXT},
	{"eta",        "\xce\xb7", SYM_TEXT},
	{"theta",      "\xce\xb8", SYM_TEXT},
	{"vartheta",   "\xcf\x91", SYM_TEXT},
	{"iota",       "\xce\xb9", SYM_TEXT},
	{"kappa",      "\xce\xba", SYM_TEXT},
	{"lambda",     "\xce\xbb", SYM_TEXT},
	{"mu",         "\xce\xbc", SYM_TEXT},
	{"nu",         "\xce\xbd", SYM_TEXT},
	{"xi",         "\xce\xbe", SYM_TEXT},
	{"pi",         "\xcf\x80", SYM_TEXT},
	{"varpi",      "\xcf\x96", SYM_TEXT},
	{"rho",        "\xcf\x81", SYM_TEXT},
	{"varrho",     "\xcf\xb1", SYM_TEXT},
	{"sigma",      "\xcf\x83", SYM_TEXT},
	{"varsigma",   "\xcf\x82", SYM_TEXT},
	{"tau",        "\xcf\x84", SYM_TEXT},
	{"upsilon",    "\xcf\x85", SYM_TEXT},
	{"phi",        "\xcf\x95", SYM_TEXT},
	{"varphi",     "\xcf\x86", SYM_TEXT},
	{"chi",        "\xcf\x87", SYM_TEXT},
	{"psi",        "\xcf\x88", SYM_TEXT},
	{"omega",      "\xcf\x89", SYM_TEXT},
	{"digamma",    "\xcf\x9d", SYM_TEXT},
	{"varkappa",   "\xcf\xb0", SYM_TEXT},
	{"backepsilon","\xcf\xb6", SYM_TEXT},
	{"varTheta",   "\xcf\xb4", SYM_TEXT},
	{"hbar",       "\xc4\xa7", SYM_TEXT},
	{"hslash",     "\xe2\x84\x8f", SYM_TEXT},
	{"ell",        "\xe2\x84\x93", SYM_TEXT},
	
	/* Uppercase Greek */
	{"Gamma",      "\xce\x93", SYM_TEXT},
	{"Delta",      "\xce\x94", SYM_TEXT},
	{"Theta",      "\xce\x98", SYM_TEXT},
	{"Lambda",     "\xce\x9b", SYM_TEXT},
	{"Xi",         "\xce\x9e", SYM_TEXT},
	{"Pi",         "\xce\xa0", SYM_TEXT},
	{"Sigma",      "\xce\xa3", SYM_TEXT},
	{"Upsilon",    "\xce\xa5", SYM_TEXT},
	{"Phi",        "\xce\xa6", SYM_TEXT},
	{"Psi",        "\xce\xa8", SYM_TEXT},
	{"Omega",      "\xce\xa9", SYM_TEXT},
	
	/* Operators, relations & dots */
	{"times",      "\xc3\x97", SYM_TEXT},
	{"cdot",       "\xe2\x8b\x85", SYM_TEXT},
	{"div",        "\xc3\xb7", SYM_TEXT},
	{"pm",         "\xc2\xb1", SYM_TEXT},
	{"mp",         "\xe2\x88\x93", SYM_TEXT},
	{"ast",        "\xe2\x88\x97", SYM_TEXT},
	{"star",       "\xe2\x8b\x86", SYM_TEXT},
	{"circ",       "\xe2\x88\x98", SYM_TEXT},
	{"bullet",     "\xe2\x88\x99", SYM_TEXT},
	{"oplus",      "\xe2\x8a\x95", SYM_TEXT},
	{"ominus",     "\xe2\x8a\x96", SYM_TEXT},
	{"oslash",     "\xe2\x8a\x98", SYM_TEXT},
	{"odot",       "\xe2\x8a\x99", SYM_TEXT},
	{"circledcirc","\xe2\x8a\x9a", SYM_TEXT},
	{"circledast", "\xe2\x8a\x9b", SYM_TEXT},
	{"circleddash","\xe2\x8a\x9d", SYM_TEXT},
	{"boxplus",    "\xe2\x8a\x9e", SYM_TEXT},
	{"boxtimes",   "\xe2\x8a\xa0", SYM_TEXT},
	{"boxdot",     "\xe2\x8a\xa1", SYM_TEXT},
	{"otimes",     "\xe2\x8a\x97", SYM_TEXT},
	{"leqslant",   "\xe2\xa9\xbd", SYM_TEXT},
	{"geqslant",   "\xe2\xa9\xbe", SYM_TEXT},
	{"lesssim",    "\xe2\x89\xb2", SYM_TEXT},
	{"gtrsim",     "\xe2\x89\xb3", SYM_TEXT},
	{"doteq",      "\xe2\x89\x90", SYM_TEXT},
	{"asymp",      "\xe2\x89\x8d", SYM_TEXT},
	{"triangleq",  "\xe2\x89\x9c", SYM_TEXT},
	{"pitchfork",  "\xe2\x8b\x94", SYM_TEXT},
	{"wr",         "\xe2\x89\x80", SYM_TEXT},
	{"intercal",   "\xe2\x8a\xba", SYM_TEXT},
	{"lhd",        "\xe2\x8a\xb2", SYM_TEXT},
	{"rhd",        "\xe2\x8a\xb3", SYM_TEXT},
	{"leq",        "\xe2\x89\xa4", SYM_TEXT},
	{"le",         "\xe2\x89\xa4", SYM_TEXT},
	{"geq",        "\xe2\x89\xa5", SYM_TEXT},
	{"ge",         "\xe2\x89\xa5", SYM_TEXT},
	{"neq",        "\xe2\x89\xa0", SYM_TEXT},
	{"ne",         "\xe2\x89\xa0", SYM_TEXT},
	{"approx",     "\xe2\x89\x88", SYM_TEXT},
	{"equiv",      "\xe2\x89\xa1", SYM_TEXT},
	{"cong",       "\xe2\x89\x85", SYM_TEXT},
	{"sim",        "\xe2\x88\xbc", SYM_TEXT},
	{"simeq",      "\xe2\x89\x83", SYM_TEXT},
	{"propto",     "\xe2\x88\x9d", SYM_TEXT},
	{"ll",         "\xe2\x89\xaa", SYM_TEXT},
	{"gg",         "\xe2\x89\xab", SYM_TEXT},
	{"perp",       "\xe2\x8a\xa5", SYM_TEXT},
	{"parallel",   "\xe2\x88\xa5", SYM_TEXT},
	{"nparallel",  "\xe2\x88\xa6", SYM_TEXT},
	{"mid",        "\xe2\x88\xa3", SYM_TEXT},
	{"nmid",       "\xe2\x88\xa4", SYM_TEXT},
	{"therefore",  "\xe2\x88\xb4", SYM_TEXT},
	{"because",    "\xe2\x88\xb5", SYM_TEXT},
	{"top",        "\xe2\x8a\xa4", SYM_TEXT},
	{"bot",        "\xe2\x8a\xa5", SYM_TEXT},
	{"vdash",      "\xe2\x8a\xa2", SYM_TEXT},
	{"dashv",      "\xe2\x8a\xa3", SYM_TEXT},
	{"models",     "\xe2\x8a\xa8", SYM_TEXT},
	{"smile",      "\xe2\x8c\xa3", SYM_TEXT},
	{"frown",      "\xe2\x8c\xa2", SYM_TEXT},
	{"triangleleft", "\xe2\x97\x81", SYM_TEXT},
	{"triangleright","\xe2\x96\xb9", SYM_TEXT},
	{"nless",      "\xe2\x89\xae", SYM_TEXT},
	{"ngtr",       "\xe2\x89\xaf", SYM_TEXT},
	{"nleq",       "\xe2\x89\xb0", SYM_TEXT},
	{"ngeq",       "\xe2\x89\xb1", SYM_TEXT},
	{"nsubseteq",  "\xe2\x8a\x88", SYM_TEXT},
	{"nsupseteq",  "\xe2\x8a\x89", SYM_TEXT},
	{"ldots",      "\xe2\x80\xa6", SYM_TEXT},
	{"cdots",      "\xe2\x8b\xaf", SYM_TEXT},
	{"vdots",      "\xe2\x8b\xae", SYM_TEXT},
	{"ddots",      "\xe2\x8b\xb1", SYM_TEXT},
	{"dots",       "\xe2\x80\xa6", SYM_TEXT},
	
	/* Sets, logic & calculus */
	{"in",         "\xe2\x88\x88", SYM_TEXT},
	{"notin",      "\xe2\x88\x89", SYM_TEXT},
	{"ni",         "\xe2\x88\x8b", SYM_TEXT},
	{"subset",     "\xe2\x8a\x82", SYM_TEXT},
	{"subseteq",   "\xe2\x8a\x86", SYM_TEXT},
	{"supset",     "\xe2\x8a\x83", SYM_TEXT},
	{"supseteq",   "\xe2\x8a\x87", SYM_TEXT},
	{"cup",        "\xe2\x88\xaa", SYM_TEXT},
	{"cap",        "\xe2\x88\xa9", SYM_TEXT},
	{"setminus",   "\xe2\x88\x96", SYM_TEXT},
	{"emptyset",   "\xe2\x88\x85", SYM_TEXT},
	{"varnothing", "\xe2\x88\x85", SYM_TEXT},
	{"forall",     "\xe2\x88\x80", SYM_TEXT},
	{"exists",     "\xe2\x88\x83", SYM_TEXT},
	{"nexists",    "\xe2\x88\x84", SYM_TEXT},
	{"neg",        "\xc2\xac", SYM_TEXT},
	{"lnot",       "\xc2\xac", SYM_TEXT},
	{"land",       "\xe2\x88\xa7", SYM_TEXT},
	{"wedge",      "\xe2\x88\xa7", SYM_TEXT},
	{"lor",        "\xe2\x88\xa8", SYM_TEXT},
	{"vee",        "\xe2\x88\xa8", SYM_TEXT},
	{"infty",      "\xe2\x88\x9e", SYM_TEXT},
	{"partial",    "\xe2\x88\x82", SYM_TEXT},
	{"nabla",      "\xe2\x88\x87", SYM_TEXT},
	{"prime",      "\xe2\x80\xb2", SYM_TEXT},
	{"angle",      "\xe2\x88\xa0", SYM_TEXT},
	{"triangle",   "\xe2\x96\xb3", SYM_TEXT},
	{"square",     "\xe2\x96\xa1", SYM_TEXT},
	{"blacksquare","\xe2\x96\xaa", SYM_TEXT},
	{"lozenge",    "\xe2\x97\x8a", SYM_TEXT},
	{"bigstar",    "\xe2\x98\x85", SYM_TEXT},
	{"flat",       "\xe2\x99\xad", SYM_TEXT},
	{"natural",    "\xe2\x99\xae", SYM_TEXT},
	{"sharp",      "\xe2\x99\xaf", SYM_TEXT},
	{"dagger",     "\xe2\x80\xa0", SYM_TEXT},
	{"ddagger",    "\xe2\x80\xa1", SYM_TEXT},
	{"aleph",      "\xe2\x84\xb5", SYM_TEXT},
	{"Re",         "\xe2\x84\x9c", SYM_TEXT},
	{"Im",         "\xe2\x84\x91", SYM_TEXT},
	{"wp",         "\xe2\x84\x98", SYM_TEXT},
	
	/* Arrows */
	{"to",             "\xe2\x86\x92", SYM_TEXT},
	{"rightarrow",     "\xe2\x86\x92", SYM_TEXT},
	{"leftarrow",      "\xe2\x86\x90", SYM_TEXT},
	{"leftrightarrow", "\xe2\x86\x94", SYM_TEXT},
	{"Rightarrow",     "\xe2\x87\x92", SYM_TEXT},
	{"implies",        "\xe2\x87\x92", SYM_TEXT},
	{"Leftarrow",      "\xe2\x87\x90", SYM_TEXT},
	{"Leftrightarrow", "\xe2\x87\x94", SYM_TEXT},
	{"iff",            "\xe2\x87\x94", SYM_TEXT},
	{"mapsto",         "\xe2\x86\xa6", SYM_TEXT},
	{"longrightarrow", "\xe2\x9f\xb6", SYM_TEXT},
	{"longleftarrow",  "\xe2\x9f\xb5", SYM_TEXT},
	{"Longrightarrow", "\xe2\x9f\xb9", SYM_TEXT},
	{"Longleftarrow",  "\xe2\x9f\xb8", SYM_TEXT},
	{"Longleftrightarrow", "\xe2\x9f\xba", SYM_TEXT},
	{"hookrightarrow", "\xe2\x86\xaa", SYM_TEXT},
	{"hookleftarrow",  "\xe2\x86\xa9", SYM_TEXT},
	{"twoheadrightarrow", "\xe2\x86\xa0", SYM_TEXT},
	{"twoheadleftarrow",  "\xe2\x86\x9e", SYM_TEXT},
	{"nearrow",          "\xe2\x86\x97", SYM_TEXT},
	{"searrow",          "\xe2\x86\x98", SYM_TEXT},
	{"swarrow",          "\xe2\x86\x99", SYM_TEXT},
	{"nwarrow",          "\xe2\x86\x96", SYM_TEXT},
	{"nleftarrow",       "\xe2\x86\x9a", SYM_TEXT},
	{"nrightarrow",      "\xe2\x86\x9b", SYM_TEXT},
	{"nleftrightarrow",  "\xe2\x86\xae", SYM_TEXT},
	{"leftrightharpoons","\xe2\x87\x8b", SYM_TEXT},
	{"rightleftharpoons","\xe2\x87\x8c", SYM_TEXT},
	{"leftrightsquigarrow","\xe2\x86\xad", SYM_TEXT},
	{"Uparrow",          "\xe2\x87\x91", SYM_TEXT},
	{"Downarrow",        "\xe2\x87\x93", SYM_TEXT},
	{"nLeftarrow",       "\xe2\x87\x8d", SYM_TEXT},
	{"nRightarrow",      "\xe2\x87\x8f", SYM_TEXT},
	{"nLeftrightarrow",  "\xe2\x87\x8e", SYM_TEXT},
	{"leftharpoonup",  "\xe2\x86\xbc", SYM_TEXT},
	{"leftharpoondown","\xe2\x86\xbd", SYM_TEXT},
	{"rightharpoonup", "\xe2\x87\x80", SYM_TEXT},
	{"rightharpoondown","\xe2\x87\x81", SYM_TEXT},
	{"updownarrow",    "\xe2\x86\x95", SYM_TEXT},
	{"Updownarrow",    "\xe2\x87\x95", SYM_TEXT},
	{"uparrow",        "\xe2\x86\x91", SYM_TEXT},
	{"downarrow",      "\xe2\x86\x93", SYM_TEXT},
	
	/* Big operators */
	{"sum",        "\xe2\x88\x91", SYM_BIG_OP},
	{"prod",       "\xe2\x88\x8f", SYM_BIG_OP},
	{"coprod",     "\xe2\x88\x90", SYM_BIG_OP},
	{"int",        "\xe2\x88\xab", SYM_BIG_OP},
	{"iint",       "\xe2\x88\xac", SYM_BIG_OP},
	{"iiint",      "\xe2\x88\xad", SYM_BIG_OP},
	{"oint",       "\xe2\x88\xae", SYM_BIG_OP},
	{"bigcup",     "\xe2\x8b\x83", SYM_BIG_OP},
	{"bigcap",     "\xe2\x8b\x82", SYM_BIG_OP},
	{"bigoplus",   "\xe2\xa8\x81", SYM_BIG_OP},
	{"bigotimes",  "\xe2\xa8\x82", SYM_BIG_OP},
	{"bigvee",     "\xe2\x8b\x81", SYM_BIG_OP},
	{"bigwedge",   "\xe2\x8b\x80", SYM_BIG_OP},
	{"biguplus",   "\xe2\xa8\x84", SYM_BIG_OP},
	{"bigsqcup",   "\xe2\xa8\x86", SYM_BIG_OP},
	{"oiint",      "\xe2\x88\xaf", SYM_BIG_OP},
	{"oiiint",     "\xe2\x88\xb0", SYM_BIG_OP},
	{"smallint",   "\xe2\x88\xab", SYM_BIG_OP},
	
	/* Standard functions */
	{"sin",        "sin",    SYM_FUNC},
	{"cos",        "cos",    SYM_FUNC},
	{"tan",        "tan",    SYM_FUNC},
	{"cot",        "cot",    SYM_FUNC},
	{"sec",        "sec",    SYM_FUNC},
	{"csc",        "csc",    SYM_FUNC},
	{"arcsin",     "arcsin", SYM_FUNC},
	{"arccos",     "arccos", SYM_FUNC},
	{"arctan",     "arctan", SYM_FUNC},
	{"arccot",     "arccot", SYM_FUNC},
	{"arcsec",     "arcsec", SYM_FUNC},
	{"arccsc",     "arccsc", SYM_FUNC},
	{"sinh",       "sinh",   SYM_FUNC},
	{"cosh",       "cosh",   SYM_FUNC},
	{"tanh",       "tanh",   SYM_FUNC},
	{"coth",       "coth",   SYM_FUNC},
	{"log",        "log",    SYM_FUNC},
	{"ln",         "ln",     SYM_FUNC},
	{"lg",         "lg",     SYM_FUNC},
	{"exp",        "exp",    SYM_FUNC},
	{"lim",        "lim",    SYM_FUNC},
	{"limsup",     "limsup", SYM_FUNC},
	{"liminf",     "liminf", SYM_FUNC},
	{"max",        "max",    SYM_FUNC},
	{"min",        "min",    SYM_FUNC},
	{"sup",        "sup",    SYM_FUNC},
	{"inf",        "inf",    SYM_FUNC},
	{"arg",        "arg",    SYM_FUNC},
	{"det",        "det",    SYM_FUNC},
	{"dim",        "dim",    SYM_FUNC},
	{"ker",        "ker",    SYM_FUNC},
	{"deg",        "deg",    SYM_FUNC},
	{"gcd",        "gcd",    SYM_FUNC},
	{"lcm",        "lcm",    SYM_FUNC},
	{"Pr",         "Pr",     SYM_FUNC},
	{"hom",        "hom",    SYM_FUNC},
	{"tr",         "tr",     SYM_FUNC},
	{"Tr",         "Tr",     SYM_FUNC},
	{"rank",       "rank",   SYM_FUNC},
	{"sgn",        "sgn",    SYM_FUNC},
	{"supp",       "supp",   SYM_FUNC},
	{"span",       "span",   SYM_FUNC},
	{"argmin",     "argmin", SYM_FUNC},
	{"argmax",     "argmax", SYM_FUNC},
	{"Res",        "Res",    SYM_FUNC},
	{"varlim",     "lim",    SYM_FUNC},
	{"varlimsup",  "limsup", SYM_FUNC},
	{"varliminf",  "liminf", SYM_FUNC},
};

static const SymbolEntry *find_symbol(const char *name) {
	if (!name) return NULL;
	size_t count = sizeof(SYMBOL_TABLE) / sizeof(SYMBOL_TABLE[0]);
	for (size_t i = 0; i < count; i++) {
		if (strcmp(SYMBOL_TABLE[i].tex_name, name) == 0) {
			return &SYMBOL_TABLE[i];
		}
	}
	return NULL;
}

/* ---------------- 2. AST DEFINITIONS ---------------- */

typedef enum {
	NODE_SEQ,
	NODE_ATOM,
	NODE_GROUP,
	NODE_FRAC,
	NODE_BINOM,
	NODE_SQRT,
	NODE_SUP,
	NODE_SUB,
	NODE_SUBSUP,
	NODE_DELIM,
	NODE_MATRIX,
	NODE_STACK,
	NODE_ACCENT,
	NODE_PDF_TEXT,
	NODE_PHANTOM,
	NODE_BREAK,
	NODE_SPACE,
	NODE_COLOR,
	NODE_OVERS,
	NODE_XARROW,
	NODE_UNKNOWN
} NodeType;

typedef struct {
	double width;
	double ascent;
	double descent;
} BoxMetrics;

/* Available line width used by the recursive math layout pass. */
static double g_formula_line_width = 0.0;

typedef struct AstNode {
	NodeType type;
	char *text;
	const char *utf8;
	int is_func;
	int is_big_op;
	int limits_mode;
	int rows;
	int cols;
	char *align_spec;
	double space_em;
	double space_pt;
	double rule_em;
	double rule_pt;
	double color_r;
	double color_g;
	double color_b;
	struct AstNode **children;
	size_t child_count;
	size_t child_cap;
	double cached_fs;
	double cached_line_width;
	BoxMetrics cached_m;
} AstNode;

static AstNode *node_new(NodeType type) {
	AstNode *n = (AstNode *)xcalloc(1, sizeof(AstNode));
	n->type = type;
	n->cached_fs = -1.0;
	n->cached_line_width = -1.0;
	return n;
}

static void node_add_child(AstNode *parent, AstNode *child) {
	if (!parent || !child) return;
	if (parent->child_count == parent->child_cap) {
		size_t next_cap = parent->child_cap == 0 ? 4 : parent->child_cap * 2;
		parent->children = (AstNode **)xrealloc(parent->children, next_cap * sizeof(AstNode *));
		parent->child_cap = next_cap;
	}
	parent->children[parent->child_count++] = child;
}

static void node_free(AstNode *n) {
	if (!n) return;
	free(n->text);
	free(n->align_spec);
	for (size_t i = 0; i < n->child_count; i++) {
		node_free(n->children[i]);
	}
	free(n->children);
	free(n);
}

/* ---------------- 3. SAFE RECURSIVE LATEX PARSER ---------------- */

typedef struct {
	const char *src;
	size_t pos;
	size_t len;
	int depth;
	int steps;
} Parser;

typedef struct {
	int stop_rbrace;
	int stop_rbracket;
	int stop_right;
	int stop_env_end;
	int stop_cell_sep;
} StopFlags;

static int parser_tick(Parser *p) {
	if (!p) return 0;
	p->steps++;
	if (p->steps > MAX_STEPS) return 0;
	if (p->depth > MAX_PARSE_DEPTH) return 0;
	return 1;
}

static void skip_spaces_and_comments(Parser *p) {
	while (p->pos < p->len) {
		char c = p->src[p->pos];
		if (isspace((unsigned char)c)) {
			p->pos++;
		} else if (c == '%') {
			while (p->pos < p->len && p->src[p->pos] != '\n' && p->src[p->pos] != '\r') {
				p->pos++;
			}
		} else {
			break;
		}
	}
}

static char *parse_command_name(Parser *p) {
	if (p->pos >= p->len) return xstrdup_range("", 0);
	
	size_t start = p->pos;
	if (isalpha((unsigned char)p->src[p->pos])) {
		while (p->pos < p->len && isalpha((unsigned char)p->src[p->pos])) {
			p->pos++;
		}
		if (p->pos < p->len && p->src[p->pos] == '*') {
			p->pos++;
		}
		return xstrdup_range(p->src + start, p->pos - start);
	}
	
	p->pos++;
	return xstrdup_range(p->src + start, 1);
}

static char *parse_braced_raw_string(Parser *p) {
	skip_spaces_and_comments(p);
	if (p->pos >= p->len || p->src[p->pos] != '{') {
		return xstrdup_range("", 0);
	}
	p->pos++;
	size_t start = p->pos;
	int brace_depth = 1;
	while (p->pos < p->len && brace_depth > 0 && parser_tick(p)) {
		if (p->src[p->pos] == '\\' && p->pos + 1 < p->len) {
			p->pos += 2;
			continue;
		}
		if (p->src[p->pos] == '{') brace_depth++;
		else if (p->src[p->pos] == '}') brace_depth--;
		if (brace_depth > 0) p->pos++;
	}
	size_t end = p->pos;
	if (p->pos < p->len && p->src[p->pos] == '}') {
		p->pos++;
	}
	return xstrdup_range(p->src + start, end - start);
}

static int parse_tex_length(const char *spec, double *em, double *pt) {
	char *end = NULL;
	double value;
	const char *unit;
	if (!spec || !em || !pt) return 0;
	value = strtod(spec, &end);
	if (end == spec) return 0;
	while (*end && isspace((unsigned char)*end)) ++end;
	unit = end;
	*em = 0.0;
	*pt = 0.0;
	if (*unit == '\0' || strcmp(unit, "em") == 0) *em = value;
	else if (strcmp(unit, "ex") == 0) *em = value * 0.45;
	else if (strcmp(unit, "mu") == 0) *em = value / 18.0;
	else if (strcmp(unit, "pt") == 0) *pt = value;
	else if (strcmp(unit, "bp") == 0) *pt = value * 72.0 / 72.27;
	else if (strcmp(unit, "pc") == 0) *pt = value * 12.0;
	else if (strcmp(unit, "in") == 0) *pt = value * 72.0;
	else if (strcmp(unit, "cm") == 0) *pt = value * 72.0 / 2.54;
	else if (strcmp(unit, "mm") == 0) *pt = value * 72.0 / 25.4;
	else return 0;
	return 1;
}

static void parse_math_color(const char *spec, double *r, double *g, double *b) {
	static const struct { const char *name; double r,g,b; } colors[] = {
		{"black",0,0,0},{"white",1,1,1},{"red",1,0,0},{"green",0,0.5,0},
		{"blue",0,0,1},{"cyan",0,1,1},{"magenta",1,0,1},{"yellow",1,1,0},
		{"gray",0.5,0.5,0.5},{"grey",0.5,0.5,0.5},{"orange",1,0.5,0},
		{"purple",0.5,0,0.5},{"brown",0.6,0.3,0.1},{"teal",0,0.5,0.5}
	};
	if (!r || !g || !b) return;
	*r = *g = *b = 0.0;
	if (!spec) return;
	if (spec[0] == '#' && strlen(spec) == 7) {
		char part[3] = {0,0,0};
		part[0] = spec[1]; part[1] = spec[2]; *r = strtol(part, NULL, 16) / 255.0;
		part[0] = spec[3]; part[1] = spec[4]; *g = strtol(part, NULL, 16) / 255.0;
		part[0] = spec[5]; part[1] = spec[6]; *b = strtol(part, NULL, 16) / 255.0;
		return;
	}
	for (size_t i = 0; i < sizeof(colors) / sizeof(colors[0]); ++i) {
		if (strcmp(spec, colors[i].name) == 0) {
			*r = colors[i].r; *g = colors[i].g; *b = colors[i].b;
			return;
		}
	}
	if (strncmp(spec, "rgb:", 4) == 0) {
		if (sscanf(spec + 4, "%lf,%lf,%lf", r, g, b) == 3) {
			*r = fmax(0.0, fmin(1.0, *r)); *g = fmax(0.0, fmin(1.0, *g)); *b = fmax(0.0, fmin(1.0, *b));
		}
	}
}

static char *normalize_fraction_delimiter(const char *raw) {
	if (!raw || !*raw || strcmp(raw, ".") == 0) return xstrdup_range(".", 1);
	if (strcmp(raw, "\\{") == 0 || strcmp(raw, "\\lbrace") == 0) return xstrdup_range("{", 1);
	if (strcmp(raw, "\\}") == 0 || strcmp(raw, "\\rbrace") == 0) return xstrdup_range("}", 1);
	if (strcmp(raw, "\\langle") == 0) return xstrdup_range("<", 1);
	if (strcmp(raw, "\\rangle") == 0) return xstrdup_range(">", 1);
	if (strcmp(raw, "\\lvert") == 0 || strcmp(raw, "\\rvert") == 0 || strcmp(raw, "|") == 0) return xstrdup_range("|", 1);
	if (strcmp(raw, "\\lVert") == 0 || strcmp(raw, "\\rVert") == 0 || strcmp(raw, "||") == 0) return xstrdup_range("||", 2);
	if (strcmp(raw, "\\lfloor") == 0) return xstrdup_range("[", 1);
	if (strcmp(raw, "\\rfloor") == 0) return xstrdup_range("]", 1);
	if (strcmp(raw, "\\lceil") == 0) return xstrdup_range("[", 1);
	if (strcmp(raw, "\\rceil") == 0) return xstrdup_range("]", 1);
	return xstrdup_range(raw, strlen(raw));
}

static void skip_optional_bracket_arg(Parser *p) {
	if (!p) return;
	skip_spaces_and_comments(p);
	if (p->pos >= p->len || p->src[p->pos] != '[') return;
	p->pos++;
	int depth = 1;
	while (p->pos < p->len && depth > 0 && parser_tick(p)) {
		char ch = p->src[p->pos++];
		if (ch == '\\' && p->pos < p->len) { p->pos++; continue; }
		if (ch == '[') depth++;
		else if (ch == ']') depth--;
	}
}

static AstNode *parse_seq_ex(Parser *p, StopFlags f);

static int peek_cmd_is(Parser *p, const char *target) {
	if (!p || !target || p->pos >= p->len || p->src[p->pos] != '\\') return 0;
	size_t save = p->pos;
	p->pos++;
	char *cmd = parse_command_name(p);
	int match = (cmd && strcmp(cmd, target) == 0);
	free(cmd);
	p->pos = save;
	return match;
}

static int is_wrapper_or_accent_cmd(const char *cmd) {
	if (!cmd) return 0;
	const char *list[] = {
		"mathcal", "mathbb", "mathrm", "mathbf", "mathit", "mathsf", "mathtt", "mathfrak", "mathscr",
		"boldsymbol", "bm", "operatorname", "operatorname*",
		"text", "textbf", "textit", "textrm", "textsf", "texttt", "mbox", "hbox",
		"mathop", "mathrel", "mathbin", "mathord", "mathopen", "mathclose", "mathpunct", "mathinner",
		"dot", "ddot", "dddot", "hat", "widehat", "bar", "overline", "underline", "tilde", "widetilde", "vec",
		"acute", "grave", "breve", "check", "mathring", "wideparen",
		"overbrace", "underbrace", "overrightarrow", "overleftarrow", "overleftrightarrow", "overline", "underbar",
		"cancel", "bcancel", "xcancel", "boxed"
	};
	size_t n = sizeof(list) / sizeof(list[0]);
	for (size_t i = 0; i < n; i++) {
		if (strcmp(cmd, list[i]) == 0) return 1;
	}
	return 0;
}

static int is_ignorable_directive(const char *cmd) {
	if (!cmd) return 0;
	const char *list[] = {
		"limits", "nolimits", "displaylimits",
		"displaystyle", "textstyle", "scriptstyle", "scriptscriptstyle",
		"nonumber", "notag", "allowbreak", "strut", "mathstrut",
		"relax", "protect", "ignorespaces", "unskip",
		"big", "Big", "bigg", "Bigg",
		"bigl", "Bigl", "biggl", "Biggl",
		"bigr", "Bigr", "biggr", "Biggr",
		"bigm", "Bigm", "biggm", "Biggm"
	};
	size_t n = sizeof(list) / sizeof(list[0]);
	for (size_t i = 0; i < n; i++) {
		if (strcmp(cmd, list[i]) == 0) return 1;
	}
	return 0;
}

static int math_style_codepoint(const char *style, unsigned char ch, unsigned int *out) {
	static const unsigned int script_caps[26] = {
		0x1D49C,0x212C,0x1D49E,0x1D49F,0x2130,0x2131,0x1D4A2,0x210B,0x2110,0x1D4A5,
		0x1D4A6,0x2112,0x2133,0x1D4A9,0x1D4AA,0x1D4AB,0x1D4AC,0x211B,0x1D4AE,0x1D4AF,
		0x1D4B0,0x1D4B1,0x1D4B2,0x1D4B3,0x1D4B4,0x1D4B5
	};
	static const unsigned int frak_caps[26] = {
		0x1D504,0x1D505,0x212D,0x1D507,0x1D508,0x1D509,0x1D50A,0x210C,0x2111,0x1D50D,
		0x1D50E,0x1D50F,0x1D510,0x1D511,0x1D512,0x1D513,0x1D514,0x211C,0x1D516,0x1D517,
		0x1D518,0x1D519,0x1D51A,0x1D51B,0x1D51C,0x2128
	};
	static const unsigned int bb_caps[26] = {
		0x1D538,0x1D539,0x2102,0x1D53B,0x1D53C,0x1D53D,0x1D53E,0x210D,0x1D540,0x1D541,
		0x1D542,0x1D543,0x1D544,0x2115,0x1D546,0x2119,0x211A,0x211D,0x1D54A,0x1D54B,
		0x1D54C,0x1D54D,0x1D54E,0x1D54F,0x1D550,0x2124
	};
	int base = -1;
	if (!style || !out) return 0;
	if (ch >= 'A' && ch <= 'Z') {
		if (strcmp(style, "mathcal") == 0 || strcmp(style, "mathscr") == 0) { *out = script_caps[ch - 'A']; return 1; }
		if (strcmp(style, "mathfrak") == 0) { *out = frak_caps[ch - 'A']; return 1; }
		if (strcmp(style, "mathbb") == 0) { *out = bb_caps[ch - 'A']; return 1; }
		if (strcmp(style, "mathbf") == 0 || strcmp(style, "boldsymbol") == 0 || strcmp(style, "bm") == 0) base = 0x1D400;
		else if (strcmp(style, "mathit") == 0) base = 0x1D434;
		else if (strcmp(style, "mathsf") == 0) base = 0x1D5A0;
		else if (strcmp(style, "mathtt") == 0) base = 0x1D670;
		if (base >= 0) { *out = (unsigned int)base + (unsigned int)(ch - 'A'); return 1; }
	} else if (ch >= 'a' && ch <= 'z') {
		if (strcmp(style, "mathcal") == 0 || strcmp(style, "mathscr") == 0) { *out = 0x1D4B6 + (unsigned int)(ch - 'a'); return 1; }
		if (strcmp(style, "mathfrak") == 0) { *out = 0x1D51E + (unsigned int)(ch - 'a'); return 1; }
		if (strcmp(style, "mathbb") == 0) { *out = 0x1D552 + (unsigned int)(ch - 'a'); return 1; }
		if (strcmp(style, "mathbf") == 0 || strcmp(style, "boldsymbol") == 0 || strcmp(style, "bm") == 0) base = 0x1D41A;
		else if (strcmp(style, "mathit") == 0) {
			if (ch == 'h') { *out = 0x210E; return 1; }
			base = ch > 'h' ? 0x1D456 : 0x1D44E;
		}
		else if (strcmp(style, "mathsf") == 0) base = 0x1D5BA;
		else if (strcmp(style, "mathtt") == 0) base = 0x1D68A;
		if (base >= 0) { *out = (unsigned int)base + (unsigned int)(ch - 'a'); return 1; }
	} else if (ch >= '0' && ch <= '9') {
		if (strcmp(style, "mathbb") == 0) base = 0x1D7D8;
		else if (strcmp(style, "mathbf") == 0 || strcmp(style, "boldsymbol") == 0 || strcmp(style, "bm") == 0) base = 0x1D7CE;
		else if (strcmp(style, "mathsf") == 0) base = 0x1D7E2;
		else if (strcmp(style, "mathtt") == 0) base = 0x1D7F6;
		if (base >= 0) { *out = (unsigned int)base + (unsigned int)(ch - '0'); return 1; }
	}
	return 0;
}

static int codepoint_to_utf8(unsigned int cp, char out[5]) {
	if (cp <= 0x7F) { out[0] = (char)cp; out[1] = '\0'; }
	else if (cp <= 0x7FF) { out[0] = (char)(0xC0 | (cp >> 6)); out[1] = (char)(0x80 | (cp & 0x3F)); out[2] = '\0'; }
	else if (cp <= 0xFFFF) { out[0] = (char)(0xE0 | (cp >> 12)); out[1] = (char)(0x80 | ((cp >> 6) & 0x3F)); out[2] = (char)(0x80 | (cp & 0x3F)); out[3] = '\0'; }
	else if (cp <= 0x10FFFF) { out[0] = (char)(0xF0 | (cp >> 18)); out[1] = (char)(0x80 | ((cp >> 12) & 0x3F)); out[2] = (char)(0x80 | ((cp >> 6) & 0x3F)); out[3] = (char)(0x80 | (cp & 0x3F)); out[4] = '\0'; }
	else return 0;
	return 1;
}

static void apply_math_style(AstNode *n, const char *style) {
	if (!n || !style) return;
	if (strcmp(style, "mathrm") == 0) {
		if (n->type == NODE_ATOM && n->text && !n->utf8 && strlen(n->text) == 1) n->is_func = 1;
		for (size_t i = 0; i < n->child_count; ++i) apply_math_style(n->children[i], style);
		return;
	}
	if (n->type == NODE_ATOM && n->text && !n->utf8 && strlen(n->text) == 1) {
		unsigned int cp;
		char glyph[5];
		if (math_style_codepoint(style, (unsigned char)n->text[0], &cp) && codepoint_to_utf8(cp, glyph)) {
			free(n->text);
			n->text = xstrdup_range(glyph, strlen(glyph));
			n->utf8 = n->text;
		}
		return;
	}
	for (size_t i = 0; i < n->child_count; ++i) apply_math_style(n->children[i], style);
}

static AstNode *parse_environment(Parser *p) {
	char *env_name = parse_braced_raw_string(p);
	char *raw_col_spec = NULL;
	
	skip_spaces_and_comments(p);
	if (p->pos < p->len && p->src[p->pos] == '[') {
		p->pos++;
		int bdepth = 1;
		while (p->pos < p->len && bdepth > 0 && parser_tick(p)) {
			if (p->src[p->pos] == '[') bdepth++;
			else if (p->src[p->pos] == ']') bdepth--;
			p->pos++;
		}
	}
	skip_spaces_and_comments(p);
	if (env_name && (strcmp(env_name, "array") == 0 || strcmp(env_name, "tabular") == 0 ||
		strcmp(env_name, "subarray") == 0) &&
		p->pos < p->len && p->src[p->pos] == '{') {
		raw_col_spec = parse_braced_raw_string(p);
	} else if (env_name && (strcmp(env_name, "alignedat") == 0 || strcmp(env_name, "alignat") == 0 ||
		strcmp(env_name, "alignat*") == 0 || strcmp(env_name, "flalign") == 0 || strcmp(env_name, "flalign*") == 0) &&
		p->pos < p->len && p->src[p->pos] == '{') {
		char *pair_count = parse_braced_raw_string(p);
		free(pair_count);
	}
	
	int max_cols = 1;
	int row_count = 0;
	AstNode **cells = NULL;
	int *cell_rows = NULL;
	int *cell_cols = NULL;
	int total_cells = 0;
	int cap_cells = 0;
	
	int cur_row = 0;
	int cur_col = 0;
	
	StopFlags cell_stop = {0, 0, 0, 1, 1};
	p->depth++;
	
	while (p->pos < p->len && parser_tick(p)) {
		skip_spaces_and_comments(p);
		if (peek_cmd_is(p, "end")) {
			p->pos++;
			char *end_cmd = parse_command_name(p);
			free(end_cmd);
			char *closed_name = parse_braced_raw_string(p);
			free(closed_name);
			break;
		}
		
		if (peek_cmd_is(p, "hline") || peek_cmd_is(p, "toprule") ||
			peek_cmd_is(p, "midrule") || peek_cmd_is(p, "bottomrule")) {
			p->pos++;
			char *hl = parse_command_name(p);
			free(hl);
			continue;
		}
		if (peek_cmd_is(p, "cline")) {
			p->pos++;
			char *cl = parse_command_name(p);
			free(cl);
			char *arg = parse_braced_raw_string(p);
			free(arg);
			continue;
		}
		
		size_t before = p->pos;
		AstNode *cell_seq = parse_seq_ex(p, cell_stop);
		if (total_cells == cap_cells) {
			int next_cap = cap_cells == 0 ? 16 : cap_cells * 2;
			cells = (AstNode **)xrealloc(cells, (size_t)next_cap * sizeof(AstNode *));
			cell_rows = (int *)xrealloc(cell_rows, (size_t)next_cap * sizeof(int));
			cell_cols = (int *)xrealloc(cell_cols, (size_t)next_cap * sizeof(int));
			cap_cells = next_cap;
		}
		cells[total_cells] = cell_seq;
		cell_rows[total_cells] = cur_row;
		cell_cols[total_cells] = cur_col;
		total_cells++;
		
		if (cur_col + 1 > max_cols) max_cols = cur_col + 1;
		if (cur_row + 1 > row_count) row_count = cur_row + 1;
		
		skip_spaces_and_comments(p);
		if (p->pos < p->len && p->src[p->pos] == '&') {
			p->pos++;
			cur_col++;
		} else if (peek_cmd_is(p, "\\")) {
			p->pos += 2;
			skip_spaces_and_comments(p);
			if (p->pos < p->len && p->src[p->pos] == '[') {
				p->pos++;
				while (p->pos < p->len && p->src[p->pos] != ']' && parser_tick(p)) p->pos++;
				if (p->pos < p->len && p->src[p->pos] == ']') p->pos++;
			}
			cur_row++;
			cur_col = 0;
		} else if (peek_cmd_is(p, "cr")) {
			p->pos++;
			char *c = parse_command_name(p);
			free(c);
			cur_row++;
			cur_col = 0;
		} else if (p->pos == before && p->pos < p->len) {
			p->pos++;
		}
	}
	
	p->depth--;
	
	if (row_count < 1) row_count = 1;
	if (max_cols < 1) max_cols = 1;
	
	AstNode *mat = node_new(NODE_MATRIX);
	mat->rows = row_count;
	mat->cols = max_cols;
	mat->text = env_name;
	if (raw_col_spec) {
		mat->align_spec = raw_col_spec;
	} else if (env_name && (strcmp(env_name, "cases") == 0 || strcmp(env_name, "dcases") == 0 ||
		strcmp(env_name, "rcases") == 0)) {
		mat->align_spec = xstrdup_range("ll", 2);
	} else if (env_name && (strcmp(env_name, "aligned") == 0 || strcmp(env_name, "alignedat") == 0 ||
		strcmp(env_name, "align") == 0 || strcmp(env_name, "align*") == 0 || strcmp(env_name, "alignat") == 0 ||
		strcmp(env_name, "alignat*") == 0 || strcmp(env_name, "flalign") == 0 || strcmp(env_name, "flalign*") == 0 ||
		strcmp(env_name, "split") == 0)) {
		size_t alen = (size_t)max_cols;
		mat->align_spec = (char *)xmalloc(alen + 1);
		for (size_t c = 0; c < alen; ++c) mat->align_spec[c] = (c % 2 == 0) ? 'r' : 'l';
		mat->align_spec[alen] = '\0';
	} else if (env_name && (strcmp(env_name, "gather") == 0 || strcmp(env_name, "gather*") == 0 ||
		strcmp(env_name, "gathered") == 0 || strcmp(env_name, "multline") == 0 ||
		strcmp(env_name, "multline*") == 0)) {
		size_t alen = (size_t)max_cols;
		mat->align_spec = (char *)xmalloc(alen + 1);
		memset(mat->align_spec, 'c', alen);
		mat->align_spec[alen] = '\0';
	}
	
	size_t grid_total = (size_t)row_count * (size_t)max_cols;
	AstNode **grid = (AstNode **)xcalloc(grid_total, sizeof(AstNode *));
	for (int i = 0; i < total_cells; i++) {
		int r = cell_rows[i];
		int c = cell_cols[i];
		if (r >= 0 && r < row_count && c >= 0 && c < max_cols) {
			size_t idx = (size_t)r * (size_t)max_cols + (size_t)c;
			if (grid[idx]) node_free(grid[idx]);
			grid[idx] = cells[i];
		} else {
			node_free(cells[i]);
		}
	}
	for (size_t i = 0; i < grid_total; i++) {
		if (!grid[i]) grid[i] = node_new(NODE_SEQ);
		node_add_child(mat, grid[i]);
	}
	
	free(grid);
	free(cells);
	free(cell_rows);
	free(cell_cols);
	return mat;
}

static AstNode *parse_substack(Parser *p) {
	char *raw = parse_braced_raw_string(p);
	AstNode *st = node_new(NODE_STACK);
	if (!raw) return st;
	
	size_t len = strlen(raw);
	size_t start = 0;
	size_t i = 0;
	int brace_depth = 0;
	
	while (i <= len && parser_tick(p)) {
		if (i < len) {
			if (raw[i] == '{') brace_depth++;
			else if (raw[i] == '}' && brace_depth > 0) brace_depth--;
		}
		if ((brace_depth == 0 && i + 1 < len && raw[i] == '\\' && raw[i + 1] == '\\') || i == len) {
			char *line_str = xstrdup_range(raw + start, i - start);
			Parser lp = {
				.src = line_str,
				.pos = 0,
				.len = strlen(line_str),
				.depth = p->depth + 1,
				.steps = p->steps
			};
			StopFlags f = {0};
			AstNode *line_node = parse_seq_ex(&lp, f);
			p->steps = lp.steps;
			node_add_child(st, line_node);
			free(line_str);
			if (i + 1 < len && raw[i] == '\\' && raw[i + 1] == '\\') {
				i += 2;
				while (i < len && isspace((unsigned char)raw[i])) i++;
				if (i < len && raw[i] == '[') {
					while (i < len && raw[i] != ']') i++;
					if (i < len && raw[i] == ']') i++;
				}
				start = i;
				continue;
			}
		}
		i++;
	}
	
	free(raw);
	return st;
}

static AstNode *parse_group_or_single_ex(Parser *p, StopFlags f) {
	(void)f;
	if (!parser_tick(p)) return NULL;
	skip_spaces_and_comments(p);
	if (p->pos >= p->len) return NULL;
	
	if (p->src[p->pos] == '{') {
		p->pos++;
		p->depth++;
		AstNode *grp = node_new(NODE_GROUP);
		StopFlags inner_f = {1, 0, 0, 0, 0};
		AstNode *inner = parse_seq_ex(p, inner_f);
		p->depth--;
		if (p->pos < p->len && p->src[p->pos] == '}') {
			p->pos++;
		}
		node_add_child(grp, inner);
		return grp;
	}
	
	if (p->src[p->pos] == '~') {
		p->pos++;
		AstNode *sp = node_new(NODE_ATOM);
		sp->text = xstrdup_range(" ", 1);
		return sp;
	}
	
	if (p->src[p->pos] == '\\') {
		p->pos++;
		char *cmd = parse_command_name(p);
		if (!cmd) return NULL;
		if (strcmp(cmd, "\\") == 0 || strcmp(cmd, "newline") == 0 ||
			strcmp(cmd, "linebreak") == 0 || strcmp(cmd, "break") == 0) {
			free(cmd);
			return node_new(NODE_BREAK);
		}
		if (strcmp(cmd, "phantom") == 0 || strcmp(cmd, "hphantom") == 0 ||
			strcmp(cmd, "vphantom") == 0 || strcmp(cmd, "smash") == 0) {
			StopFlags empty_f = {0};
			p->depth++;
			AstNode *arg = parse_group_or_single_ex(p, empty_f);
			p->depth--;
			AstNode *ghost = node_new(NODE_PHANTOM);
			ghost->text = cmd;
			if (arg) node_add_child(ghost, arg);
			return ghost;
		}
		if (strcmp(cmd, "hspace") == 0 || strcmp(cmd, "hspace*") == 0 ||
			strcmp(cmd, "kern") == 0 || strcmp(cmd, "mkern") == 0 ||
			strcmp(cmd, "hskip") == 0 || strcmp(cmd, "mskip") == 0) {
			char *spec;
			AstNode *space = node_new(NODE_SPACE);
			skip_spaces_and_comments(p);
			if (p->pos < p->len && p->src[p->pos] == '{') {
				spec = parse_braced_raw_string(p);
			} else {
				size_t begin = p->pos;
				while (p->pos < p->len && (isdigit((unsigned char)p->src[p->pos]) ||
					p->src[p->pos] == '+' || p->src[p->pos] == '-' || p->src[p->pos] == '.' ||
					isalpha((unsigned char)p->src[p->pos]))) p->pos++;
				spec = xstrdup_range(p->src + begin, p->pos - begin);
			}
			parse_tex_length(spec, &space->space_em, &space->space_pt);
			free(spec);
			free(cmd);
			return space;
		}
		if (strcmp(cmd, "not") == 0) {
			StopFlags empty_f = {0};
			p->depth++;
			AstNode *arg = parse_group_or_single_ex(p, empty_f);
			p->depth--;
			AstNode *negated = node_new(NODE_ACCENT);
			negated->text = cmd;
			if (arg) node_add_child(negated, arg);
			return negated;
		}
		if (strcmp(cmd, "label") == 0) {
			char *label = parse_braced_raw_string(p);
			free(label);
			free(cmd);
			return node_new(NODE_SEQ);
		}
		if (strcmp(cmd, "tag") == 0 || strcmp(cmd, "tag*") == 0) {
			AstNode *tag = node_new(NODE_PDF_TEXT);
			tag->text = parse_braced_raw_string(p);
			free(cmd);
			return tag;
		}
		if (strcmp(cmd, "newcommand") == 0 || strcmp(cmd, "newcommand*") == 0 ||
			strcmp(cmd, "renewcommand") == 0 || strcmp(cmd, "renewcommand*") == 0 ||
			strcmp(cmd, "providecommand") == 0 || strcmp(cmd, "providecommand*") == 0) {
			char *name = parse_braced_raw_string(p);
			free(name);
			skip_optional_bracket_arg(p);
			skip_optional_bracket_arg(p);
			char *definition = parse_braced_raw_string(p);
			free(definition);
			free(cmd);
			return node_new(NODE_SEQ);
		}
		if (strcmp(cmd, "DeclareMathOperator") == 0 || strcmp(cmd, "DeclareMathOperator*") == 0) {
			char *name = parse_braced_raw_string(p);
			char *definition = parse_braced_raw_string(p);
			free(name);
			free(definition);
			free(cmd);
			return node_new(NODE_SEQ);
		}
		if (strcmp(cmd, "ref") == 0 || strcmp(cmd, "eqref") == 0 || strcmp(cmd, "autoref") == 0) {
			char *label = parse_braced_raw_string(p);
			AstNode *ref = node_new(NODE_PDF_TEXT);
			const char *prefix = strcmp(cmd, "eqref") == 0 ? "(" : "";
			const char *suffix = strcmp(cmd, "eqref") == 0 ? ")" : "";
			size_t len = strlen(prefix) + strlen(label) + strlen(suffix);
			ref->text = (char *)xmalloc(len + 1);
			snprintf(ref->text, len + 1, "%s%s%s", prefix, label, suffix);
			free(label);
			free(cmd);
			return ref;
		}
		if (strcmp(cmd, "bmod") == 0 || strcmp(cmd, "mod") == 0) {
			AstNode *mod = node_new(NODE_PDF_TEXT);
			mod->text = xstrdup_range("mod", 3);
			free(cmd);
			return mod;
		}
		if (strcmp(cmd, "pmod") == 0) {
			char *raw = parse_braced_raw_string(p);
			Parser inner = {.src = raw ? raw : "", .pos = 0, .len = raw ? strlen(raw) : 0,
				.depth = p->depth + 1, .steps = p->steps};
			AstNode *body = node_new(NODE_SEQ);
			AstNode *mod = node_new(NODE_PDF_TEXT);
			AstNode *space = node_new(NODE_ATOM);
			space->text = xstrdup_range(" ", 1);
			mod->text = xstrdup_range("mod", 3);
			node_add_child(body, mod);
			node_add_child(body, space);
			node_add_child(body, parse_seq_ex(&inner, (StopFlags){0}));
			p->steps = inner.steps;
			free(raw);
			AstNode *delim = node_new(NODE_DELIM);
			delim->text = xstrdup_range("(\x1f)", 3);
			node_add_child(delim, body);
			free(cmd);
			return delim;
		}
		
		if (is_ignorable_directive(cmd)) {
			free(cmd);
			return node_new(NODE_SEQ);
		}
		
		if (strcmp(cmd, "begin") == 0) {
			free(cmd);
			return parse_environment(p);
		}
		
		if (strcmp(cmd, "substack") == 0) {
			free(cmd);
			return parse_substack(p);
		}
		if (strcmp(cmd, "text") == 0 || strcmp(cmd, "mbox") == 0 || strcmp(cmd, "hbox") == 0 ||
			strcmp(cmd, "textbf") == 0 || strcmp(cmd, "textit") == 0 || strcmp(cmd, "textrm") == 0 ||
			strcmp(cmd, "textsf") == 0 || strcmp(cmd, "texttt") == 0 || strcmp(cmd, "textnormal") == 0 ||
			strcmp(cmd, "operatorname") == 0 || strcmp(cmd, "operatorname*") == 0) {
			AstNode *text = node_new(NODE_PDF_TEXT);
			text->text = parse_braced_raw_string(p);
			free(cmd);
			return text;
		}
		if (strcmp(cmd, "textcolor") == 0 || strcmp(cmd, "colorbox") == 0) {
			skip_optional_bracket_arg(p);
			char *color = parse_braced_raw_string(p);
			AstNode *colored = node_new(NODE_COLOR);
			colored->text = cmd;
			parse_math_color(color, &colored->color_r, &colored->color_g, &colored->color_b);
			free(color);
			StopFlags empty_f = {0};
			p->depth++;
			AstNode *content = parse_group_or_single_ex(p, empty_f);
			p->depth--;
			if (content) node_add_child(colored, content);
			return colored;
		}
		if (strcmp(cmd, "color") == 0 || strcmp(cmd, "pagecolor") == 0) {
			char *color = parse_braced_raw_string(p);
			free(color);
			free(cmd);
			return node_new(NODE_SEQ);
		}
		if (strcmp(cmd, "overset") == 0 || strcmp(cmd, "stackrel") == 0) {
			StopFlags empty_f = {0};
			p->depth++;
			AstNode *over = parse_group_or_single_ex(p, empty_f);
			AstNode *base = parse_group_or_single_ex(p, empty_f);
			p->depth--;
			AstNode *node = node_new(NODE_OVERS);
			node_add_child(node, base ? base : node_new(NODE_SEQ));
			node_add_child(node, over ? over : node_new(NODE_SEQ));
			free(cmd);
			return node;
		}
		if (strcmp(cmd, "xrightarrow") == 0 || strcmp(cmd, "xleftarrow") == 0 ||
			strcmp(cmd, "xleftrightarrow") == 0 || strcmp(cmd, "xRightarrow") == 0 ||
			strcmp(cmd, "xLeftarrow") == 0 || strcmp(cmd, "xLeftrightarrow") == 0 ||
			strcmp(cmd, "xmapsto") == 0) {
			AstNode *below = NULL;
			skip_spaces_and_comments(p);
			if (p->pos < p->len && p->src[p->pos] == '[') {
				p->pos++;
				StopFlags brk_f = {0, 1, 0, 0, 0};
				p->depth++;
				below = parse_seq_ex(p, brk_f);
				p->depth--;
				if (p->pos < p->len && p->src[p->pos] == ']') p->pos++;
			}
			StopFlags empty_f = {0};
			p->depth++;
			AstNode *above = parse_group_or_single_ex(p, empty_f);
			p->depth--;
			AstNode *node = node_new(NODE_XARROW);
			node->text = cmd;
			node_add_child(node, below ? below : node_new(NODE_SEQ));
			node_add_child(node, above ? above : node_new(NODE_SEQ));
			return node;
		}
		
		if (strcmp(cmd, "frac") == 0 || strcmp(cmd, "dfrac") == 0 ||
			strcmp(cmd, "tfrac") == 0 || strcmp(cmd, "cfrac") == 0) {
			free(cmd);
			StopFlags empty_f = {0};
			p->depth++;
			AstNode *num = parse_group_or_single_ex(p, empty_f);
			AstNode *den = parse_group_or_single_ex(p, empty_f);
			p->depth--;
			AstNode *frac = node_new(NODE_FRAC);
			node_add_child(frac, num ? num : node_new(NODE_SEQ));
			node_add_child(frac, den ? den : node_new(NODE_SEQ));
			return frac;
		}
		if (strcmp(cmd, "genfrac") == 0) {
			char *left_raw = parse_braced_raw_string(p);
			char *right_raw = parse_braced_raw_string(p);
			char *thickness = parse_braced_raw_string(p);
			char *style = parse_braced_raw_string(p);
			char *left = normalize_fraction_delimiter(left_raw);
			char *right = normalize_fraction_delimiter(right_raw);
			size_t dlen = strlen(left) + strlen(right) + 1;
			StopFlags empty_f = {0};
			p->depth++;
			AstNode *num = parse_group_or_single_ex(p, empty_f);
			AstNode *den = parse_group_or_single_ex(p, empty_f);
			p->depth--;
			AstNode *frac = node_new(NODE_FRAC);
			frac->text = (char *)xmalloc(dlen + 1);
			snprintf(frac->text, dlen + 1, "%s\x1f%s", left, right);
			if (!parse_tex_length(thickness, &frac->rule_em, &frac->rule_pt)) frac->rule_em = 0.06;
			node_add_child(frac, num ? num : node_new(NODE_SEQ));
			node_add_child(frac, den ? den : node_new(NODE_SEQ));
			free(left_raw); free(right_raw); free(thickness); free(style); free(left); free(right); free(cmd);
			return frac;
		}
		
		if (strcmp(cmd, "binom") == 0 || strcmp(cmd, "dbinom") == 0 || strcmp(cmd, "tbinom") == 0) {
			free(cmd);
			StopFlags empty_f = {0};
			p->depth++;
			AstNode *top = parse_group_or_single_ex(p, empty_f);
			AstNode *bot = parse_group_or_single_ex(p, empty_f);
			p->depth--;
			AstNode *bn = node_new(NODE_BINOM);
			node_add_child(bn, top ? top : node_new(NODE_SEQ));
			node_add_child(bn, bot ? bot : node_new(NODE_SEQ));
			return bn;
		}
		
		if (strcmp(cmd, "sqrt") == 0) {
			free(cmd);
			AstNode *deg = NULL;
			skip_spaces_and_comments(p);
			if (p->pos < p->len && p->src[p->pos] == '[') {
				p->pos++;
				StopFlags brk_f = {0, 1, 0, 0, 0};
				p->depth++;
				deg = parse_seq_ex(p, brk_f);
				p->depth--;
				if (p->pos < p->len && p->src[p->pos] == ']') {
					p->pos++;
				}
			}
			StopFlags empty_f = {0};
			p->depth++;
			AstNode *rad = parse_group_or_single_ex(p, empty_f);
			p->depth--;
			AstNode *sqrt_node = node_new(NODE_SQRT);
			if (deg) node_add_child(sqrt_node, deg);
			node_add_child(sqrt_node, rad ? rad : node_new(NODE_SEQ));
			return sqrt_node;
		}
		
		if (is_wrapper_or_accent_cmd(cmd)) {
			StopFlags empty_f = {0};
			p->depth++;
			AstNode *arg = parse_group_or_single_ex(p, empty_f);
			p->depth--;
			if (strcmp(cmd, "mathcal") == 0 || strcmp(cmd, "mathscr") == 0 ||
				strcmp(cmd, "mathbb") == 0 || strcmp(cmd, "mathfrak") == 0 ||
				strcmp(cmd, "mathbf") == 0 || strcmp(cmd, "mathit") == 0 ||
				strcmp(cmd, "mathsf") == 0 || strcmp(cmd, "mathtt") == 0 ||
				strcmp(cmd, "boldsymbol") == 0 || strcmp(cmd, "bm") == 0)
				apply_math_style(arg, cmd);
			AstNode *acc = node_new(NODE_ACCENT);
			acc->text = cmd;
			if (arg) node_add_child(acc, arg);
			return acc;
		}
		
		if (strcmp(cmd, ",") == 0 || strcmp(cmd, ";") == 0 ||
			strcmp(cmd, ":") == 0 || strcmp(cmd, "!") == 0 ||
			strcmp(cmd, " ") == 0 || strcmp(cmd, "quad") == 0 || strcmp(cmd, "qquad") == 0 ||
			strcmp(cmd, "enspace") == 0 || strcmp(cmd, "enskip") == 0 || strcmp(cmd, "thinspace") == 0 ||
			strcmp(cmd, "negthinspace") == 0 || strcmp(cmd, "negmedspace") == 0 ||
			strcmp(cmd, "negthickspace") == 0 || strcmp(cmd, "medspace") == 0 || strcmp(cmd, "thickspace") == 0) {
			AstNode *sp = node_new(NODE_ATOM);
			if (strcmp(cmd, "!") == 0 || strcmp(cmd, "negthinspace") == 0 ||
				strcmp(cmd, "negmedspace") == 0 || strcmp(cmd, "negthickspace") == 0) {
				/* Keep TeX's negative thin space distinct from a literal exclamation mark. */
				free(cmd);
				sp->text = xstrdup_range("\x1f", 1);
			} else {
				sp->text = cmd;
			}
			return sp;
		}
		
		if (strlen(cmd) == 1 && strchr("{}%$&#_|", cmd[0]) != NULL) {
			AstNode *lit = node_new(NODE_ATOM);
			if (cmd[0] == '|') {
				free(cmd);
				lit->text = xstrdup_range("||", 2);
			} else {
				lit->text = cmd;
			}
			return lit;
		}
		
		const SymbolEntry *sym = find_symbol(cmd);
		if (sym) {
			AstNode *atom = node_new(NODE_ATOM);
			atom->text = cmd;
			atom->utf8 = sym->utf8;
			atom->is_func = (sym->kind == SYM_FUNC);
			atom->is_big_op = (sym->kind == SYM_BIG_OP);
			return atom;
		}
		
		AstNode *unk = node_new(NODE_UNKNOWN);
		unk->text = cmd;
		skip_spaces_and_comments(p);
		if (p->pos < p->len && p->src[p->pos] == '[') {
			p->pos++;
			StopFlags brk_f = {0, 1, 0, 0, 0};
			p->depth++;
			AstNode *opt = parse_seq_ex(p, brk_f);
			p->depth--;
			if (p->pos < p->len && p->src[p->pos] == ']') p->pos++;
			node_free(opt);
			skip_spaces_and_comments(p);
		}
		if (p->pos < p->len && p->src[p->pos] == '{') {
			StopFlags empty_f = {0};
			p->depth++;
			AstNode *opt_arg = parse_group_or_single_ex(p, empty_f);
			p->depth--;
			if (opt_arg) node_add_child(unk, opt_arg);
		}
		return unk;
	}
	
	unsigned char c0 = (unsigned char)p->src[p->pos];
	size_t char_len = 1;
	if ((c0 & 0xE0) == 0xC0 && p->pos + 1 < p->len) char_len = 2;
	else if ((c0 & 0xF0) == 0xE0 && p->pos + 2 < p->len) char_len = 3;
	else if ((c0 & 0xF8) == 0xF0 && p->pos + 3 < p->len) char_len = 4;
	
	AstNode *atom = node_new(NODE_ATOM);
	atom->text = xstrdup_range(p->src + p->pos, char_len);
	p->pos += char_len;
	return atom;
}

static char *parse_delim_token(Parser *p) {
	skip_spaces_and_comments(p);
	if (p->pos >= p->len) return xstrdup_range(".", 1);
	if (p->src[p->pos] == '\\' && p->pos + 1 < p->len) {
		if (p->src[p->pos + 1] == '|') {
			p->pos += 2;
			return xstrdup_range("||", 2);
		}
		if (p->src[p->pos + 1] == '{' || p->src[p->pos + 1] == '}') {
			char ch = p->src[p->pos + 1];
			p->pos += 2;
			return xstrdup_range(&ch, 1);
		}
		p->pos++;
		char *cmd = parse_command_name(p);
		if (cmd && (strcmp(cmd, "langle") == 0 || strcmp(cmd, "lfloor") == 0 || strcmp(cmd, "lceil") == 0)) {
			free(cmd);
			return xstrdup_range("<", 1);
		}
		if (cmd && (strcmp(cmd, "rangle") == 0 || strcmp(cmd, "rfloor") == 0 || strcmp(cmd, "rceil") == 0)) {
			free(cmd);
			return xstrdup_range(">", 1);
		}
		if (cmd && (strcmp(cmd, "lvert") == 0 || strcmp(cmd, "rvert") == 0)) {
			free(cmd);
			return xstrdup_range("|", 1);
		}
		if (cmd && (strcmp(cmd, "lVert") == 0 || strcmp(cmd, "rVert") == 0)) {
			free(cmd);
			return xstrdup_range("||", 2);
		}
		free(cmd);
		return xstrdup_range(".", 1);
	}
	char *tok = xstrdup_range(p->src + p->pos, 1);
	p->pos++;
	return tok;
}

static AstNode *parse_primary_ex(Parser *p, StopFlags f) {
	if (!parser_tick(p)) return NULL;
	skip_spaces_and_comments(p);
	if (p->pos >= p->len) return NULL;
	
	/* Leading _ or ^ without an explicit base (e.g., ^t A or _2F_1) */
	if (p->src[p->pos] == '_' || p->src[p->pos] == '^') {
		return node_new(NODE_SEQ);
	}
	
	if (p->src[p->pos] == '\\') {
		size_t save = p->pos;
		p->pos++;
		char *cmd = parse_command_name(p);
		
		if (cmd && strcmp(cmd, "left") == 0) {
			free(cmd);
			char *open_delim = parse_delim_token(p);
			StopFlags body_f = {f.stop_rbrace, f.stop_rbracket, 1, f.stop_env_end, f.stop_cell_sep};
			p->depth++;
			AstNode *body = parse_seq_ex(p, body_f);
			p->depth--;
			
			char *close_delim = xstrdup_range(".", 1);
			if (peek_cmd_is(p, "right")) {
				p->pos++;
				char *r_cmd = parse_command_name(p);
				free(r_cmd);
				free(close_delim);
				close_delim = parse_delim_token(p);
			}
			
			AstNode *delim = node_new(NODE_DELIM);
			size_t d_len = strlen(open_delim) + strlen(close_delim);
			delim->text = (char *)xmalloc(d_len + 2);
			snprintf(delim->text, d_len + 2, "%s\x1f%s", open_delim, close_delim);
			free(open_delim);
			free(close_delim);
			node_add_child(delim, body);
			return delim;
		}
		
		free(cmd);
		p->pos = save;
	}
	
	return parse_group_or_single_ex(p, f);
}

static AstNode *parse_term_ex(Parser *p, StopFlags f) {
	if (!parser_tick(p)) return NULL;
	AstNode *base = parse_primary_ex(p, f);
	if (!base) return NULL;
	
	AstNode *sub = NULL;
	AstNode *sup = NULL;
	StopFlags empty_f = {0};
	
	while (p->pos < p->len && parser_tick(p)) {
		skip_spaces_and_comments(p);
		if (p->pos >= p->len) break;
		if (peek_cmd_is(p, "limits") || peek_cmd_is(p, "nolimits") || peek_cmd_is(p, "displaylimits")) {
			p->pos++;
			char *mode = parse_command_name(p);
			base->limits_mode = strcmp(mode, "nolimits") == 0 ? -1 : 1;
			free(mode);
			continue;
		}
		
		char ch = p->src[p->pos];
		if (ch == '\'') {
			p->pos++;
			AstNode *pr = node_new(NODE_ATOM);
			pr->text = xstrdup_range("'", 1);
			if (!sup) {
				sup = pr;
			} else {
				AstNode *grp = node_new(NODE_SEQ);
				node_add_child(grp, sup);
				node_add_child(grp, pr);
				sup = grp;
			}
		} else if (ch == '_') {
			p->pos++;
			p->depth++;
			AstNode *next_sub = parse_group_or_single_ex(p, empty_f);
			p->depth--;
			if (!sub) {
				sub = next_sub;
			} else if (next_sub) {
				AstNode *grp = node_new(NODE_SEQ);
				node_add_child(grp, sub);
				node_add_child(grp, next_sub);
				sub = grp;
			}
		} else if (ch == '^') {
			p->pos++;
			p->depth++;
			AstNode *next_sup = parse_group_or_single_ex(p, empty_f);
			p->depth--;
			if (!sup) {
				sup = next_sup;
			} else if (next_sup) {
				AstNode *grp = node_new(NODE_SEQ);
				node_add_child(grp, sup);
				node_add_child(grp, next_sup);
				sup = grp;
			}
		} else {
			break;
		}
	}
	
	if (sub && sup) {
		AstNode *n = node_new(NODE_SUBSUP);
		node_add_child(n, base);
		node_add_child(n, sub);
		node_add_child(n, sup);
		return n;
	}
	if (sup) {
		AstNode *n = node_new(NODE_SUP);
		node_add_child(n, base);
		node_add_child(n, sup);
		return n;
	}
	if (sub) {
		AstNode *n = node_new(NODE_SUB);
		node_add_child(n, base);
		node_add_child(n, sub);
		return n;
	}
	return base;
}

static AstNode *parse_seq_ex(Parser *p, StopFlags f) {
	AstNode *seq = node_new(NODE_SEQ);
	while (p->pos < p->len && parser_tick(p)) {
		skip_spaces_and_comments(p);
		if (p->pos >= p->len) break;
		
		char ch = p->src[p->pos];
		if (f.stop_rbrace && ch == '}') break;
		if (f.stop_rbracket && ch == ']') break;
		if (f.stop_right && peek_cmd_is(p, "right")) break;
		if (f.stop_env_end && peek_cmd_is(p, "end")) break;
		if (f.stop_cell_sep && (ch == '&' || peek_cmd_is(p, "\\") || peek_cmd_is(p, "cr"))) break;
		
		if ((!f.stop_rbrace && ch == '}') || (!f.stop_rbracket && ch == ']')) {
			AstNode *lit = node_new(NODE_ATOM);
			lit->text = xstrdup_range(&ch, 1);
			p->pos++;
			node_add_child(seq, lit);
			continue;
		}
		
		if (peek_cmd_is(p, "over") || peek_cmd_is(p, "choose")) {
			int is_choose = peek_cmd_is(p, "choose");
			p->pos++;
			char *op = parse_command_name(p);
			free(op);
			AstNode *den = parse_seq_ex(p, f);
			AstNode *frac = node_new(is_choose ? NODE_BINOM : NODE_FRAC);
			node_add_child(frac, seq);
			node_add_child(frac, den);
			AstNode *wrapper = node_new(NODE_SEQ);
			node_add_child(wrapper, frac);
			return wrapper;
		}
		
		size_t before = p->pos;
		AstNode *item = parse_term_ex(p, f);
		if (!item) break;
		node_add_child(seq, item);
		if (p->pos == before && p->pos < p->len) {
			p->pos++;
		}
	}
	return seq;
}

static AstNode *parse_seq(Parser *p, int stop_at_rbrace, int stop_at_rbracket, int stop_at_right) {
	StopFlags f = {stop_at_rbrace, stop_at_rbracket, stop_at_right, 0, 0};
	return parse_seq_ex(p, f);
}

/* ---------------- 4. CONSOLE OUTPUT HELPERS ---------------- */

static void print_assembled(const AstNode *n) {
	if (!n) return;
	
	switch (n->type) {
	case NODE_SEQ:
	case NODE_GROUP:
		for (size_t i = 0; i < n->child_count; i++) {
			print_assembled(n->children[i]);
		}
		break;
		
	case NODE_ATOM:
		if (n->text && (strcmp(n->text, ",") == 0 || strcmp(n->text, ";") == 0 ||
						strcmp(n->text, ":") == 0 || strcmp(n->text, " ") == 0 ||
						strcmp(n->text, "thinspace") == 0 || strcmp(n->text, "medspace") == 0 ||
						strcmp(n->text, "thickspace") == 0 || strcmp(n->text, "enspace") == 0 ||
						strcmp(n->text, "enskip") == 0)) {
			printf(" ");
		} else if (n->text && (strcmp(n->text, "quad") == 0 || strcmp(n->text, "qquad") == 0)) {
			printf("  ");
		} else if (n->text && strcmp(n->text, "\x1f") == 0) {
			/* negative thin space */
		} else if (n->utf8 && n->utf8[0] != '\0') {
			printf("%s", n->utf8);
		} else if (n->text) {
			printf("%s", n->text);
		}
		break;
		
	case NODE_UNKNOWN:
		if (n->text) printf("\\%s", n->text);
		if (n->child_count >= 1) {
			printf("{");
			print_assembled(n->children[0]);
			printf("}");
		}
		break;
		
	case NODE_ACCENT:
		if (n->child_count >= 1) print_assembled(n->children[0]);
		break;
	case NODE_PHANTOM:
		if (n->child_count >= 1) print_assembled(n->children[0]);
		break;
	case NODE_BREAK:
		printf(" / ");
		break;
	case NODE_SPACE:
		printf(" ");
		break;
	case NODE_COLOR:
		if (n->child_count >= 1) print_assembled(n->children[0]);
		break;
	case NODE_PDF_TEXT:
		if (n->text) printf("%s", n->text);
		break;
	case NODE_OVERS:
		if (n->child_count >= 2) {
			print_assembled(n->children[0]);
			printf("^( "); print_assembled(n->children[1]); printf(" )");
		}
		break;
	case NODE_XARROW:
		if (n->child_count >= 2) {
			printf("["); print_assembled(n->children[0]);
			printf("]%s[", n->text && strcmp(n->text, "xleftarrow") == 0 ? "<---" : "--->");
			print_assembled(n->children[1]); printf("]");
		}
		break;
		
	case NODE_STACK:
		for (size_t i = 0; i < n->child_count; i++) {
			if (i > 0) printf(" / ");
			print_assembled(n->children[i]);
		}
		break;
		
	case NODE_MATRIX:
		printf("[");
		for (int r = 0; r < n->rows; r++) {
			if (r > 0) printf("; ");
			for (int c = 0; c < n->cols; c++) {
				if (c > 0) printf(", ");
				size_t idx = (size_t)r * (size_t)n->cols + (size_t)c;
				if (idx < n->child_count) print_assembled(n->children[idx]);
			}
		}
		printf("]");
		break;
		
	case NODE_FRAC:
		printf("(");
		if (n->child_count >= 1) print_assembled(n->children[0]);
		printf(")/(");
		if (n->child_count >= 2) print_assembled(n->children[1]);
		printf(")");
		break;
		
	case NODE_BINOM:
		printf("C(");
		if (n->child_count >= 1) print_assembled(n->children[0]);
		printf(", ");
		if (n->child_count >= 2) print_assembled(n->children[1]);
		printf(")");
		break;
		
	case NODE_SQRT:
		if (n->child_count >= 2) {
			printf("[");
			print_assembled(n->children[0]);
			printf("]\xe2\x88\x9a(");
			print_assembled(n->children[1]);
			printf(")");
		} else if (n->child_count >= 1) {
			printf("\xe2\x88\x9a(");
			print_assembled(n->children[0]);
			printf(")");
		}
		break;
		
	case NODE_SUP:
		if (n->child_count >= 1) print_assembled(n->children[0]);
		printf("^(");
		if (n->child_count >= 2) print_assembled(n->children[1]);
		printf(")");
		break;
		
	case NODE_SUB:
		if (n->child_count >= 1) print_assembled(n->children[0]);
		printf("_(");
		if (n->child_count >= 2) print_assembled(n->children[1]);
		printf(")");
		break;
		
	case NODE_SUBSUP:
		if (n->child_count >= 1) print_assembled(n->children[0]);
		if (n->child_count >= 2) {
			printf("_(");
			print_assembled(n->children[1]);
			printf(")");
		}
		if (n->child_count >= 3) {
			printf("^(");
			print_assembled(n->children[2]);
			printf(")");
		}
		break;
		
		case NODE_DELIM: {
			const char *sep = n->text ? strchr(n->text, '\x1f') : NULL;
			if (sep) {
				int olen = (int)(sep - n->text);
				if (!(olen == 1 && n->text[0] == '.')) printf("%.*s", olen, n->text);
				if (n->child_count >= 1) print_assembled(n->children[0]);
				if (strcmp(sep + 1, ".") != 0) printf("%s", sep + 1);
			} else if (n->child_count >= 1) {
				print_assembled(n->children[0]);
			}
			break;
		}
	}
}

/* ---------------- 5. 2D LAYOUT & MULTI-PAGE PDF EMITTER ---------------- */

typedef struct {
	char *buf;
	size_t len;
	size_t cap;
} PdfStream;

static void pdf_append(PdfStream *s, const char *str) {
	if (!s || !str) return;
	size_t add = strlen(str);
	if (s->len + add + 1 > s->cap) {
		size_t next = s->cap == 0 ? 8192 : s->cap * 2;
		while (next < s->len + add + 1) next *= 2;
		s->buf = (char *)xrealloc(s->buf, next);
		s->cap = next;
	}
	memcpy(s->buf + s->len, str, add);
	s->len += add;
	s->buf[s->len] = '\0';
}

static void pdf_printf(PdfStream *s, const char *fmt, ...) {
	char tmp[2048];
	va_list args;
	va_start(args, fmt);
	vsnprintf(tmp, sizeof(tmp), fmt, args);
	va_end(args);
	pdf_append(s, tmp);
}

/* Convert Cambria Math glyph outlines into PDF vector paths on Windows. */
#ifdef _WIN32
typedef DWORD (WINAPI *WzGetGlyphOutlineWFn)(HDC, UINT, UINT, LPGLYPHMETRICS, DWORD, LPVOID, const MAT2 *);
typedef DWORD (WINAPI *WzGetFontDataFn)(HDC, DWORD, DWORD, LPVOID, DWORD);
typedef HFONT (WINAPI *WzCreateFontAFn)(int, int, int, int, int, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, LPCSTR);
typedef HDC (WINAPI *WzCreateCompatibleDCFn)(HDC);
typedef HGDIOBJ (WINAPI *WzSelectObjectFn)(HDC, HGDIOBJ);
typedef int (WINAPI *WzGetTextFaceAFn)(HDC, int, LPSTR);

typedef struct {
	HMODULE module;
	HDC dc;
	HFONT font;
	WzGetGlyphOutlineWFn get_glyph_outline;
	WzGetFontDataFn get_font_data;
	WzCreateFontAFn create_font;
	WzCreateCompatibleDCFn create_dc;
	WzSelectObjectFn select_object;
	WzGetTextFaceAFn get_text_face;
	int initialized;
	int available;
} WzMathFont;

static WzMathFont g_math_font;

static WzMathFont *get_math_font(void) {
	if (g_math_font.initialized) return g_math_font.available ? &g_math_font : NULL;
	g_math_font.initialized = 1;
	g_math_font.module = LoadLibraryA("gdi32.dll");
	if (!g_math_font.module) return NULL;
	g_math_font.get_glyph_outline = (WzGetGlyphOutlineWFn)GetProcAddress(g_math_font.module, "GetGlyphOutlineW");
	g_math_font.get_font_data = (WzGetFontDataFn)GetProcAddress(g_math_font.module, "GetFontData");
	g_math_font.create_font = (WzCreateFontAFn)GetProcAddress(g_math_font.module, "CreateFontA");
	g_math_font.create_dc = (WzCreateCompatibleDCFn)GetProcAddress(g_math_font.module, "CreateCompatibleDC");
	g_math_font.select_object = (WzSelectObjectFn)GetProcAddress(g_math_font.module, "SelectObject");
	g_math_font.get_text_face = (WzGetTextFaceAFn)GetProcAddress(g_math_font.module, "GetTextFaceA");
	if (!g_math_font.get_glyph_outline || !g_math_font.get_font_data || !g_math_font.create_font || !g_math_font.create_dc ||
		!g_math_font.select_object || !g_math_font.get_text_face) return NULL;
	g_math_font.dc = g_math_font.create_dc(NULL);
	if (!g_math_font.dc) return NULL;
	g_math_font.font = g_math_font.create_font(-1000, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, "Cambria Math");
	if (!g_math_font.font || !g_math_font.select_object(g_math_font.dc, g_math_font.font)) return NULL;
	{
		char face[LF_FACESIZE] = {0};
		if (!g_math_font.get_text_face(g_math_font.dc, LF_FACESIZE, face) || _stricmp(face, "Cambria Math") != 0) return NULL;
	}
	g_math_font.available = 1;
	return &g_math_font;
}

static int math_utf8_to_codepoint(const char *s, UINT *out) {
	const unsigned char *p = (const unsigned char *)s;
	if (!p || !p[0] || !out) return 0;
	if (p[0] < 0x80) { *out = (UINT)p[0]; return p[1] == 0; }
	if ((p[0] & 0xE0) == 0xC0 && (p[1] & 0xC0) == 0x80) {
		*out = (UINT)(((p[0] & 0x1F) << 6) | (p[1] & 0x3F)); return p[2] == 0;
	}
	if ((p[0] & 0xF0) == 0xE0 && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80) {
		*out = (UINT)(((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F)); return p[3] == 0;
	}
	if ((p[0] & 0xF8) == 0xF0 && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80 && (p[3] & 0xC0) == 0x80) {
		UINT cp = ((UINT)(p[0] & 0x07) << 18) | ((UINT)(p[1] & 0x3F) << 12) |
			((UINT)(p[2] & 0x3F) << 6) | (UINT)(p[3] & 0x3F);
		if (cp > 0x10FFFF) return 0;
		*out = cp;
		return p[4] == 0;
	}
	return 0;
}

static unsigned int math_read_be16(const unsigned char *p) {
	return ((unsigned int)p[0] << 8) | (unsigned int)p[1];
}

static unsigned int math_read_be32(const unsigned char *p) {
	return ((unsigned int)p[0] << 24) | ((unsigned int)p[1] << 16) |
		((unsigned int)p[2] << 8) | (unsigned int)p[3];
}

/* Supplementary mathematical alphabets need cmap format 12 -> glyph-index lookup. */
static int math_supplementary_glyph(WzMathFont *font, UINT codepoint, WORD *glyph) {
	DWORD size;
	unsigned char *cmap;
	unsigned int count, i;
	int found = 0;
	if (!font || !font->get_font_data || !glyph || codepoint <= 0xFFFF) return 0;
	/* GetFontData takes the OpenType tag in Windows little-endian DWORD order. */
	size = font->get_font_data(font->dc, 0x70616D63u, 0, NULL, 0);
	if (size < 4 || size > 4 * 1024 * 1024) return 0;
	cmap = (unsigned char *)xmalloc(size);
	if (font->get_font_data(font->dc, 0x70616D63u, 0, cmap, size) == GDI_ERROR) { free(cmap); return 0; }
	count = math_read_be16(cmap + 2);
	if (4u + (unsigned long long)count * 8u > size) { free(cmap); return 0; }
	for (i = 0; i < count; ++i) {
		const unsigned char *record = cmap + 4 + i * 8;
		unsigned int offset = math_read_be32(record + 4);
		unsigned int length, groups, g;
		if (offset > size || size - offset < 16 || math_read_be16(cmap + offset) != 12) continue;
		length = math_read_be32(cmap + offset + 4);
		groups = math_read_be32(cmap + offset + 12);
		if (length < 16 || length > size - offset || 16u + (unsigned long long)groups * 12u > length) continue;
		for (g = 0; g < groups; ++g) {
			const unsigned char *range = cmap + offset + 16 + g * 12;
			unsigned int first = math_read_be32(range);
			unsigned int last = math_read_be32(range + 4);
			unsigned int first_glyph = math_read_be32(range + 8);
			if (codepoint >= first && codepoint <= last) {
				unsigned int mapped = first_glyph + (codepoint - first);
				if (mapped > 0 && mapped <= 0xFFFF) { *glyph = (WORD)mapped; found = 1; }
				break;
			}
		}
		if (found) break;
	}
	free(cmap);
	return found;
}

static int math_gdi_glyph(WzMathFont *font, UINT codepoint, UINT *glyph, UINT *format) {
	if (!font || !glyph || !format || codepoint > 0x10FFFF) return 0;
	if (codepoint <= 0xFFFF) {
		*glyph = codepoint;
		*format = GGO_NATIVE;
		return 1;
	} else {
		WORD glyph_index;
		if (!math_supplementary_glyph(font, codepoint, &glyph_index)) return 0;
		*glyph = glyph_index;
		*format = GGO_NATIVE | GGO_GLYPH_INDEX;
		return 1;
	}
}

static double math_point(FIXED v) { return (double)v.value + (double)v.fract / 65536.0; }

static int math_get_outline(const char *utf8, GLYPHMETRICS *gm, unsigned char **out, DWORD *out_size) {
	WzMathFont *font = get_math_font();
	UINT codepoint, glyph, format;
	MAT2 mat;
	DWORD size;
	unsigned char *buf;
	if (!font || !math_utf8_to_codepoint(utf8, &codepoint) || !math_gdi_glyph(font, codepoint, &glyph, &format) || !gm || !out || !out_size) return 0;
	memset(&mat, 0, sizeof(mat));
	mat.eM11.value = mat.eM22.value = 1;
	size = font->get_glyph_outline(font->dc, glyph, format, gm, 0, NULL, &mat);
	if (size == GDI_ERROR || size == 0 || size > 4 * 1024 * 1024) return 0;
	buf = (unsigned char *)xmalloc(size);
	if (font->get_glyph_outline(font->dc, glyph, format, gm, size, buf, &mat) == GDI_ERROR) { free(buf); return 0; }
	*out = buf;
	*out_size = size;
	return 1;
}

static int draw_math_font_glyph(PdfStream *s, const char *utf8, double x, double y, double fs, int draw) {
	GLYPHMETRICS gm;
	unsigned char *buf = NULL;
	DWORD size = 0, off;
	if (!draw) {
		WzMathFont *font = get_math_font(); UINT codepoint, glyph, format; MAT2 mat;
		if (!font || !math_utf8_to_codepoint(utf8, &codepoint) || !math_gdi_glyph(font, codepoint, &glyph, &format)) return 0;
		memset(&mat, 0, sizeof(mat)); mat.eM11.value = mat.eM22.value = 1;
		return font->get_glyph_outline(font->dc, glyph, format, &gm, 0, NULL, &mat) != GDI_ERROR;
	}
	if (!s || !math_get_outline(utf8, &gm, &buf, &size)) return 0;
	{
		double scale = fs / 1000.0;
		pdf_append(s, "q\n");
		for (off = 0; off + sizeof(TTPOLYGONHEADER) <= size;) {
			TTPOLYGONHEADER *poly = (TTPOLYGONHEADER *)(buf + off);
			DWORD pos, end;
			double cx, cy;
			if (poly->cb < sizeof(TTPOLYGONHEADER) || off + poly->cb > size) break;
			end = off + poly->cb;
			cx = x + math_point(poly->pfxStart.x) * scale; cy = y + math_point(poly->pfxStart.y) * scale;
			pdf_printf(s, "%.3f %.3f m\n", cx, cy);
			pos = off + sizeof(TTPOLYGONHEADER);
			while (pos + offsetof(TTPOLYCURVE, apfx) <= end) {
				TTPOLYCURVE *curve = (TTPOLYCURVE *)(buf + pos);
				DWORD head = (DWORD)offsetof(TTPOLYCURVE, apfx);
				DWORD need = head + (DWORD)curve->cpfx * (DWORD)sizeof(POINTFX);
				if (!curve->cpfx || pos + need > end) break;
				if (curve->wType == TT_PRIM_LINE) {
					for (WORD i = 0; i < curve->cpfx; ++i)
						pdf_printf(s, "%.3f %.3f l\n", x + math_point(curve->apfx[i].x) * scale, y + math_point(curve->apfx[i].y) * scale);
				} else if (curve->wType == TT_PRIM_QSPLINE) {
					for (WORD i = 0; i < curve->cpfx; ++i) {
						double qx = x + math_point(curve->apfx[i].x) * scale, qy = y + math_point(curve->apfx[i].y) * scale;
						double ex = qx, ey = qy, c1x, c1y, c2x, c2y;
						if (i + 1 < curve->cpfx) {
							ex = x + (math_point(curve->apfx[i].x) + math_point(curve->apfx[i + 1].x)) * 0.5 * scale;
							ey = y + (math_point(curve->apfx[i].y) + math_point(curve->apfx[i + 1].y)) * 0.5 * scale;
						}
						c1x = cx + (qx - cx) * (2.0 / 3.0); c1y = cy + (qy - cy) * (2.0 / 3.0);
						c2x = ex + (qx - ex) * (2.0 / 3.0); c2y = ey + (qy - ey) * (2.0 / 3.0);
						pdf_printf(s, "%.3f %.3f %.3f %.3f %.3f %.3f c\n", c1x, c1y, c2x, c2y, ex, ey);
						cx = ex; cy = ey;
					}
				} else if (curve->wType == TT_PRIM_CSPLINE && curve->cpfx % 3 == 0) {
					for (WORD i = 0; i + 2 < curve->cpfx; i += 3) {
						double x1 = x + math_point(curve->apfx[i].x) * scale, y1 = y + math_point(curve->apfx[i].y) * scale;
						double x2 = x + math_point(curve->apfx[i + 1].x) * scale, y2 = y + math_point(curve->apfx[i + 1].y) * scale;
						cx = x + math_point(curve->apfx[i + 2].x) * scale; cy = y + math_point(curve->apfx[i + 2].y) * scale;
						pdf_printf(s, "%.3f %.3f %.3f %.3f %.3f %.3f c\n", x1, y1, x2, y2, cx, cy);
					}
				}
				pos += need;
			}
			pdf_append(s, "h\n"); off = end;
		}
		pdf_append(s, "f Q\n");
	}
	free(buf);
	return 1;
}

static double math_font_glyph_advance(const char *utf8, double fs) {
	WzMathFont *font = get_math_font();
	UINT codepoint, glyph, format;
	MAT2 mat;
	GLYPHMETRICS gm;
	if (!font || !math_utf8_to_codepoint(utf8, &codepoint) || !math_gdi_glyph(font, codepoint, &glyph, &format)) return -1.0;
	memset(&mat, 0, sizeof(mat)); mat.eM11.value = mat.eM22.value = 1;
	if (font->get_glyph_outline(font->dc, glyph, (format & GGO_GLYPH_INDEX) | GGO_METRICS, &gm, 0, NULL, &mat) == GDI_ERROR) return -1.0;
	return (double)gm.gmCellIncX * fs / 1000.0;
}
#else
static int draw_math_font_glyph(PdfStream *s, const char *utf8, double x, double y, double fs, int draw) {
	(void)s; (void)utf8; (void)x; (void)y; (void)fs; (void)draw; return 0;
}
static double math_font_glyph_advance(const char *utf8, double fs) {
	(void)utf8; (void)fs; return -1.0;
}
#endif

static void emit_pdf_ascii_text(PdfStream *s, double x, double y, double font_size, int italic, const char *txt) {
	if (!txt || !s) return;
	pdf_printf(s, "BT /%s %.2f Tf 1 0 0 1 %.2f %.2f Tm (",
			   italic ? LATEXF_PDF_FONT_ITALIC_NAME : LATEXF_PDF_FONT_NORMAL_NAME, font_size, x, y);
	for (const char *c = txt; *c; c++) {
		unsigned char uc = (unsigned char)*c;
		if (uc == '(' || uc == ')' || uc == '\\') {
			pdf_printf(s, "\\%c", *c);
		} else if (uc >= 32 && uc <= 126) {
			pdf_printf(s, "%c", *c);
		} else if ((uc & 0xC0) != 0x80) {
			pdf_append(s, "?");
		}
	}
	pdf_append(s, ") Tj ET\n");
}

static int draw_vector_symbol(PdfStream *s, const char *tex_name, double x, double y, double h, int draw) {
	double w = h * 0.62;
	double lw = h * 0.072;
	if (!tex_name) return 0;
	{
		const SymbolEntry *entry = find_symbol(tex_name);
		if (entry && entry->utf8 && draw_math_font_glyph(s, entry->utf8, x, y, h, draw)) return 1;
	}
	
	/* Greek letters & mathematical symbols rendered as crisp vector paths */
	if (strcmp(tex_name, "lambda") == 0) {
		if (draw) {
			pdf_printf(s, "q %.2f w 1 J %.2f %.2f m %.2f %.2f l S %.2f %.2f m %.2f %.2f l S Q\n",
					   lw,
					   x + w * 0.15, y + h * 0.78, x + w * 0.85, y,
					   x + w * 0.50, y + h * 0.42, x + w * 0.10, y);
		}
		return 1;
	}
	if (strcmp(tex_name, "hbar") == 0) {
		if (draw) {
			emit_pdf_ascii_text(s, x, y, h, 1, "h");
			pdf_printf(s, "q %.2f w 1 J %.2f %.2f m %.2f %.2f l S Q\n",
					   lw * 0.9, x, y + h * 0.62, x + w * 0.75, y + h * 0.68);
		}
		return 1;
	}
	if (strcmp(tex_name, "epsilon") == 0 || strcmp(tex_name, "varepsilon") == 0) {
		if (draw) {
			double cy = y + h * 0.30;
			double rx = w * 0.42, ry = h * 0.26;
			pdf_printf(s, "q %.2f w 1 J %.2f %.2f m %.2f %.2f %.2f %.2f %.2f %.2f c %.2f %.2f %.2f %.2f %.2f %.2f c S %.2f %.2f m %.2f %.2f l S Q\n",
					   lw,
					   x + rx * 1.8, cy + ry * 0.75,
					   x + rx * 0.2, cy + ry * 1.2, x, cy + ry * 0.2, x, cy,
					   x, cy - ry * 0.2, x + rx * 0.2, cy - ry * 1.2, x + rx * 1.8, cy - ry * 0.75,
					   x + rx * 0.1, cy, x + rx * 1.4, cy);
		}
		return 1;
	}
	if (strcmp(tex_name, "alpha") == 0) {
		if (draw) {
			double cy = y + h * 0.28;
			pdf_printf(s, "q %.2f w 1 J %.2f %.2f m %.2f %.2f %.2f %.2f %.2f %.2f c %.2f %.2f %.2f %.2f %.2f %.2f c S Q\n",
					   lw,
					   x + w * 0.92, cy + h * 0.22,
					   x + w * 0.55, cy - h * 0.36, x + w * 0.05, cy - h * 0.32, x + w * 0.05, cy,
					   x + w * 0.05, cy + h * 0.32, x + w * 0.55, cy + h * 0.36, x + w * 0.92, cy - h * 0.22);
		}
		return 1;
	}
	if (strcmp(tex_name, "beta") == 0) {
		if (draw) {
			pdf_printf(s, "q %.2f w 1 J %.2f %.2f m %.2f %.2f l %.2f %.2f %.2f %.2f %.2f %.2f c %.2f %.2f %.2f %.2f %.2f %.2f c S Q\n",
					   lw,
					   x + w * 0.12, y - h * 0.18, x + w * 0.12, y + h * 0.72,
					   x + w * 0.78, y + h * 0.78, x + w * 0.78, y + h * 0.42, x + w * 0.22, y + h * 0.38,
					   x + w * 0.88, y + h * 0.34, x + w * 0.88, y - h * 0.02, x + w * 0.14, y + h * 0.02);
		}
		return 1;
	}
	if (strcmp(tex_name, "gamma") == 0) {
		if (draw) {
			pdf_printf(s, "q %.2f w 1 J %.2f %.2f m %.2f %.2f %.2f %.2f %.2f %.2f c %.2f %.2f m %.2f %.2f l S Q\n",
					   lw,
					   x + w * 0.05, y + h * 0.46,
					   x + w * 0.30, y + h * 0.55, x + w * 0.48, y + h * 0.20, x + w * 0.45, y - h * 0.20,
					   x + w * 0.85, y + h * 0.52, x + w * 0.45, y - h * 0.20);
		}
		return 1;
	}
	if (strcmp(tex_name, "delta") == 0) {
		if (draw) {
			pdf_printf(s, "q %.2f w 1 J %.2f %.2f m %.2f %.2f %.2f %.2f %.2f %.2f c %.2f %.2f %.2f %.2f %.2f %.2f c S Q\n",
					   lw,
					   x + w * 0.72, y + h * 0.72,
					   x + w * 0.10, y + h * 0.82, x + w * 0.88, y + h * 0.38, x + w * 0.82, y + h * 0.14,
					   x + w * 0.76, y - h * 0.06, x + w * 0.10, y - h * 0.04, x + w * 0.45, y + h * 0.42);
		}
		return 1;
	}
	if (strcmp(tex_name, "zeta") == 0) {
		if (draw) {
			pdf_printf(s, "q %.2f w 1 J %.2f %.2f m %.2f %.2f l %.2f %.2f %.2f %.2f %.2f %.2f c %.2f %.2f %.2f %.2f %.2f %.2f c S Q\n",
					   lw,
					   x + w * 0.25, y + h * 0.76, x + w * 0.78, y + h * 0.76,
					   x + w * 0.15, y + h * 0.48, x + w * 0.08, y + h * 0.08, x + w * 0.55, y + h * 0.05,
					   x + w * 0.85, y + h * 0.03, x + w * 0.75, y - h * 0.18, x + w * 0.45, y - h * 0.18);
		}
		return 1;
	}
	if (strcmp(tex_name, "theta") == 0 || strcmp(tex_name, "vartheta") == 0) {
		if (draw) {
			double cx = x + w * 0.48, cy = y + h * 0.36, rx = w * 0.38, ry = h * 0.38;
			pdf_printf(s, "q %.2f w %.2f %.2f m %.2f %.2f %.2f %.2f %.2f %.2f c %.2f %.2f %.2f %.2f %.2f %.2f c %.2f %.2f m %.2f %.2f l S Q\n",
					   lw,
					   cx, cy + ry, cx + rx * 1.3, cy + ry, cx + rx * 1.3, cy - ry, cx, cy - ry,
					   cx - rx * 1.3, cy - ry, cx - rx * 1.3, cy + ry, cx, cy + ry,
					   cx - rx, cy, cx + rx, cy);
		}
		return 1;
	}
	if (strcmp(tex_name, "mu") == 0) {
		if (draw) {
			pdf_printf(s, "q %.2f w 1 J %.2f %.2f m %.2f %.2f l %.2f %.2f m %.2f %.2f %.2f %.2f %.2f %.2f c %.2f %.2f l S Q\n",
					   lw,
					   x + w * 0.14, y - h * 0.20, x + w * 0.14, y + h * 0.50,
					   x + w * 0.14, y + h * 0.14, x + w * 0.24, y - h * 0.04, x + w * 0.65, y - h * 0.02, x + w * 0.76, y + h * 0.16,
					   x + w * 0.76, y + h * 0.50);
		}
		return 1;
	}
	if (strcmp(tex_name, "nu") == 0) {
		if (draw) {
			pdf_printf(s, "q %.2f w 1 J %.2f %.2f m %.2f %.2f l %.2f %.2f %.2f %.2f %.2f %.2f c S Q\n",
					   lw,
					   x + w * 0.15, y + h * 0.50, x + w * 0.25, y,
					   x + w * 0.55, y + h * 0.12, x + w * 0.78, y + h * 0.34, x + w * 0.82, y + h * 0.50);
		}
		return 1;
	}
	if (strcmp(tex_name, "pi") == 0) {
		if (draw) {
			pdf_printf(s, "q %.2f w 1 J %.2f %.2f m %.2f %.2f l S %.2f %.2f m %.2f %.2f l S %.2f %.2f m %.2f %.2f l S Q\n",
					   lw,
					   x + w * 0.05, y + h * 0.48, x + w * 0.95, y + h * 0.48,
					   x + w * 0.28, y + h * 0.48, x + w * 0.24, y,
					   x + w * 0.70, y + h * 0.48, x + w * 0.74, y);
		}
		return 1;
	}
	if (strcmp(tex_name, "rho") == 0) {
		if (draw) {
			double cx = x + w * 0.52, cy = y + h * 0.26, r = h * 0.22;
			pdf_printf(s, "q %.2f w 1 J %.2f %.2f m %.2f %.2f l %.2f %.2f %.2f %.2f %.2f %.2f c %.2f %.2f %.2f %.2f %.2f %.2f c S Q\n",
					   lw,
					   cx - r, y - h * 0.20, cx - r, cy,
					   cx - r, cy + r * 1.3, cx + r, cy + r * 1.3, cx + r, cy,
					   cx + r, cy - r * 1.3, cx - r, cy - r * 1.3, cx - r, cy);
		}
		return 1;
	}
	if (strcmp(tex_name, "sigma") == 0) {
		if (draw) {
			double cx = x + w * 0.42, cy = y + h * 0.25, r = h * 0.23;
			pdf_printf(s, "q %.2f w 1 J %.2f %.2f m %.2f %.2f l %.2f %.2f m %.2f %.2f %.2f %.2f %.2f %.2f c %.2f %.2f %.2f %.2f %.2f %.2f c S Q\n",
					   lw,
					   cx, cy + r, x + w * 0.96, cy + r,
					   cx, cy + r, cx + r * 1.3, cy + r, cx + r * 1.3, cy - r, cx, cy - r,
					   cx - r * 1.3, cy - r, cx - r * 1.3, cy + r, cx, cy + r);
		}
		return 1;
	}
	if (strcmp(tex_name, "tau") == 0) {
		if (draw) {
			pdf_printf(s, "q %.2f w 1 J %.2f %.2f m %.2f %.2f l S %.2f %.2f m %.2f %.2f l S Q\n",
					   lw,
					   x + w * 0.08, y + h * 0.48, x + w * 0.92, y + h * 0.48,
					   x + w * 0.50, y + h * 0.48, x + w * 0.46, y);
		}
		return 1;
	}
	if (strcmp(tex_name, "phi") == 0 || strcmp(tex_name, "varphi") == 0) {
		if (draw) {
			double cx = x + w * 0.48, cy = y + h * 0.26, rx = w * 0.36, ry = h * 0.23;
			pdf_printf(s, "q %.2f w 1 J %.2f %.2f m %.2f %.2f %.2f %.2f %.2f %.2f c %.2f %.2f %.2f %.2f %.2f %.2f c %.2f %.2f m %.2f %.2f l S Q\n",
					   lw,
					   cx, cy + ry, cx + rx * 1.3, cy + ry, cx + rx * 1.3, cy - ry, cx, cy - ry,
					   cx - rx * 1.3, cy - ry, cx - rx * 1.3, cy + ry, cx, cy + ry,
					   cx + w * 0.06, y + h * 0.72, cx - w * 0.06, y - h * 0.20);
		}
		return 1;
	}
	if (strcmp(tex_name, "psi") == 0) {
		if (draw) {
			double cx = x + w * 0.50;
			pdf_printf(s, "q %.2f w 1 J %.2f %.2f m %.2f %.2f %.2f %.2f %.2f %.2f c %.2f %.2f m %.2f %.2f l S Q\n",
					   lw,
					   x + w * 0.10, y + h * 0.48,
					   x + w * 0.14, y - h * 0.04, x + w * 0.86, y - h * 0.04, x + w * 0.90, y + h * 0.48,
					   cx, y + h * 0.72, cx, y - h * 0.20);
		}
		return 1;
	}
	if (strcmp(tex_name, "omega") == 0) {
		if (draw) {
			double mw = w * 1.15;
			pdf_printf(s, "q %.2f w 1 J %.2f %.2f m %.2f %.2f %.2f %.2f %.2f %.2f c %.2f %.2f %.2f %.2f %.2f %.2f c S Q\n",
					   lw,
					   x + mw * 0.14, y + h * 0.48,
					   x - mw * 0.05, y + h * 0.05, x + mw * 0.42, y - h * 0.04, x + mw * 0.50, y + h * 0.24,
					   x + mw * 0.58, y - h * 0.04, x + mw * 1.05, y + h * 0.05, x + mw * 0.86, y + h * 0.48);
		}
		return 1;
	}
	if (strcmp(tex_name, "Gamma") == 0) {
		if (draw) {
			pdf_printf(s, "q %.2f w 1 J 1 j %.2f %.2f m %.2f %.2f l %.2f %.2f l S Q\n",
					   lw * 1.1,
					   x + w * 0.12, y,
					   x + w * 0.12, y + h * 0.74,
					   x + w * 0.88, y + h * 0.74);
		}
		return 1;
	}
	if (strcmp(tex_name, "Delta") == 0) {
		if (draw) {
			pdf_printf(s, "q %.2f w 1 J 1 j %.2f %.2f m %.2f %.2f l %.2f %.2f l %.2f %.2f l S Q\n",
					   lw * 1.1,
					   x + w * 0.06, y,
					   x + w * 0.50, y + h * 0.74,
					   x + w * 0.94, y,
					   x + w * 0.06, y);
		}
		return 1;
	}
	if (strcmp(tex_name, "Lambda") == 0) {
		if (draw) {
			pdf_printf(s, "q %.2f w 1 J 1 j %.2f %.2f m %.2f %.2f l %.2f %.2f l S Q\n",
					   lw * 1.1,
					   x + w * 0.08, y,
					   x + w * 0.50, y + h * 0.74,
					   x + w * 0.92, y);
		}
		return 1;
	}
	if (strcmp(tex_name, "Sigma") == 0) {
		if (draw) {
			pdf_printf(s, "q %.2f w 1 J 1 j %.2f %.2f m %.2f %.2f l %.2f %.2f l %.2f %.2f l %.2f %.2f l S Q\n",
					   lw * 1.1,
					   x + w * 0.90, y + h * 0.74,
					   x + w * 0.10, y + h * 0.74,
					   x + w * 0.52, y + h * 0.37,
					   x + w * 0.10, y,
					   x + w * 0.90, y);
		}
		return 1;
	}
	if (strcmp(tex_name, "Phi") == 0) {
		if (draw) {
			double cx = x + w * 0.50, cy = y + h * 0.37, rx = w * 0.38, ry = h * 0.22;
			pdf_printf(s, "q %.2f w 1 J %.2f %.2f m %.2f %.2f %.2f %.2f %.2f %.2f c %.2f %.2f %.2f %.2f %.2f %.2f c %.2f %.2f m %.2f %.2f l S Q\n",
					   lw * 1.1,
					   cx, cy + ry, cx + rx * 1.3, cy + ry, cx + rx * 1.3, cy - ry, cx, cy - ry,
					   cx - rx * 1.3, cy - ry, cx - rx * 1.3, cy + ry, cx, cy + ry,
					   cx, y + h * 0.74, cx, y);
		}
		return 1;
	}
	if (strcmp(tex_name, "Psi") == 0) {
		if (draw) {
			double cx = x + w * 0.50;
			pdf_printf(s, "q %.2f w 1 J %.2f %.2f m %.2f %.2f %.2f %.2f %.2f %.2f c %.2f %.2f m %.2f %.2f l S Q\n",
					   lw * 1.1,
					   x + w * 0.10, y + h * 0.64,
					   x + w * 0.14, y + h * 0.14, x + w * 0.86, y + h * 0.14, x + w * 0.90, y + h * 0.64,
					   cx, y + h * 0.74, cx, y);
		}
		return 1;
	}
	if (strcmp(tex_name, "Omega") == 0) {
		if (draw) {
			double cx = x + w * 0.50, cy = y + h * 0.42, r = h * 0.30;
			pdf_printf(s, "q %.2f w 1 J %.2f %.2f m %.2f %.2f l %.2f %.2f %.2f %.2f %.2f %.2f c %.2f %.2f %.2f %.2f %.2f %.2f c %.2f %.2f l S Q\n",
					   lw * 1.1,
					   x + w * 0.08, y,
					   x + w * 0.34, y,
					   x + w * 0.05, cy - r * 0.4, x + w * 0.12, cy + r * 1.1, cx, cy + r * 1.1,
					   x + w * 0.88, cy + r * 1.1, x + w * 0.95, cy - r * 0.4, x + w * 0.66, y,
					   x + w * 0.92, y);
		}
		return 1;
	}
	if (strcmp(tex_name, "pm") == 0 || strcmp(tex_name, "mp") == 0) {
		int plus_top = (strcmp(tex_name, "pm") == 0);
		double cy = y + h * 0.45;
		double arm = h * 0.22;
		double bar_y = plus_top ? (cy - h * 0.28) : (cy + h * 0.28);
		if (draw) {
			pdf_printf(s, "q %.2f w 1 J %.2f %.2f m %.2f %.2f l S %.2f %.2f m %.2f %.2f l S %.2f %.2f m %.2f %.2f l S Q\n",
					   h * 0.08, x + arm, cy - arm, x + arm, cy + arm,
					   x, cy, x + arm * 2.0, cy,
					   x, bar_y, x + arm * 2.0, bar_y);
		}
		return 1;
	}
	if (strcmp(tex_name, "times") == 0) {
		double cy = y + h * 0.38;
		double d = h * 0.22;
		if (draw) {
			pdf_printf(s, "q %.2f w 1 J %.2f %.2f m %.2f %.2f l S %.2f %.2f m %.2f %.2f l S Q\n",
					   h * 0.08, x, cy - d, x + d * 2.0, cy + d,
					   x, cy + d, x + d * 2.0, cy - d);
		}
		return 1;
	}
	if (strcmp(tex_name, "div") == 0) {
		double cy = y + h * 0.36;
		double r = h * 0.06;
		if (draw) {
			pdf_printf(s, "q %.2f w 1 J %.2f %.2f m %.2f %.2f l S %.2f %.2f %.2f %.2f re f %.2f %.2f %.2f %.2f re f Q\n",
					   h * 0.08, x, cy, x + w, cy,
					   x + w * 0.5 - r, cy + h * 0.18 - r, r * 2.0, r * 2.0,
					   x + w * 0.5 - r, cy - h * 0.18 - r, r * 2.0, r * 2.0);
		}
		return 1;
	}
	if (strcmp(tex_name, "cdot") == 0 || strcmp(tex_name, "bullet") == 0) {
		double cy = y + h * 0.35;
		double r = strcmp(tex_name, "bullet") == 0 ? h * 0.14 : h * 0.09;
		if (draw) {
			pdf_printf(s, "q %.2f %.2f %.2f %.2f re f Q\n", x + r, cy - r, r * 2.0, r * 2.0);
		}
		return 1;
	}
	if (strcmp(tex_name, "leq") == 0 || strcmp(tex_name, "le") == 0 ||
		strcmp(tex_name, "geq") == 0 || strcmp(tex_name, "ge") == 0) {
		int is_le = (strcmp(tex_name, "leq") == 0 || strcmp(tex_name, "le") == 0);
		double mid_y = y + h * 0.48;
		double d = h * 0.24;
		double bar_y = y + h * 0.12;
		if (draw) {
			if (is_le) {
				pdf_printf(s, "q %.2f w 1 J 1 j %.2f %.2f m %.2f %.2f l %.2f %.2f l S %.2f %.2f m %.2f %.2f l S Q\n",
						   h * 0.08, x + w, mid_y + d, x, mid_y, x + w, mid_y - d,
						   x, bar_y, x + w, bar_y);
			} else {
				pdf_printf(s, "q %.2f w 1 J 1 j %.2f %.2f m %.2f %.2f l %.2f %.2f l S %.2f %.2f m %.2f %.2f l S Q\n",
						   h * 0.08, x, mid_y + d, x + w, mid_y, x, mid_y - d,
						   x, bar_y, x + w, bar_y);
			}
		}
		return 1;
	}
	if (strcmp(tex_name, "neq") == 0 || strcmp(tex_name, "ne") == 0) {
		double y1 = y + h * 0.48;
		double y2 = y + h * 0.28;
		if (draw) {
			pdf_printf(s, "q %.2f w 1 J %.2f %.2f m %.2f %.2f l S %.2f %.2f m %.2f %.2f l S %.2f %.2f m %.2f %.2f l S Q\n",
					   h * 0.08, x, y1, x + w, y1,
					   x, y2, x + w, y2,
					   x + w * 0.75, y + h * 0.72, x + w * 0.25, y + h * 0.05);
		}
		return 1;
	}
	if (strcmp(tex_name, "equiv") == 0) {
		if (draw) {
			pdf_printf(s, "q %.2f w 1 J %.2f %.2f m %.2f %.2f l S %.2f %.2f m %.2f %.2f l S %.2f %.2f m %.2f %.2f l S Q\n",
					   h * 0.07,
					   x, y + h * 0.54, x + w, y + h * 0.54,
					   x, y + h * 0.36, x + w, y + h * 0.36,
					   x, y + h * 0.18, x + w, y + h * 0.18);
		}
		return 1;
	}
	if (strcmp(tex_name, "approx") == 0 || strcmp(tex_name, "sim") == 0) {
		if (draw) {
			double y1 = y + h * 0.46;
			pdf_printf(s, "q %.2f w %.2f %.2f m %.2f %.2f %.2f %.2f %.2f %.2f c S Q\n",
					   h * 0.07, x, y1, x + w * 0.33, y1 + h * 0.18, x + w * 0.66, y1 - h * 0.18, x + w, y1);
			if (strcmp(tex_name, "approx") == 0) {
				double y2 = y + h * 0.26;
				pdf_printf(s, "q %.2f w %.2f %.2f m %.2f %.2f %.2f %.2f %.2f %.2f c S Q\n",
						   h * 0.07, x, y2, x + w * 0.33, y2 + h * 0.18, x + w * 0.66, y2 - h * 0.18, x + w, y2);
			}
		}
		return 1;
	}
	if (strcmp(tex_name, "to") == 0 || strcmp(tex_name, "rightarrow") == 0 ||
		strcmp(tex_name, "Rightarrow") == 0 || strcmp(tex_name, "implies") == 0) {
		double cy = y + h * 0.36;
		double aw = w * 1.3;
		if (draw) {
			pdf_printf(s, "q %.2f w 1 J 1 j %.2f %.2f m %.2f %.2f l S %.2f %.2f m %.2f %.2f l %.2f %.2f l S Q\n",
					   h * 0.08, x, cy, x + aw, cy,
					   x + aw - h * 0.2, cy + h * 0.18, x + aw, cy, x + aw - h * 0.2, cy - h * 0.18);
		}
		return 1;
	}
	if (strcmp(tex_name, "leftarrow") == 0 || strcmp(tex_name, "Leftarrow") == 0) {
		double cy = y + h * 0.36;
		double aw = w * 1.3;
		if (draw) {
			pdf_printf(s, "q %.2f w 1 J 1 j %.2f %.2f m %.2f %.2f l S %.2f %.2f m %.2f %.2f l %.2f %.2f l S Q\n",
					   h * 0.08, x, cy, x + aw, cy,
					   x + h * 0.2, cy + h * 0.18, x, cy, x + h * 0.2, cy - h * 0.18);
		}
		return 1;
	}
	if (strcmp(tex_name, "sum") == 0) {
		double sw = h * 0.72;
		if (draw) {
			pdf_printf(s, "q %.2f w 1 J 1 j %.2f %.2f m %.2f %.2f l %.2f %.2f l %.2f %.2f l %.2f %.2f l S Q\n",
					   h * 0.09,
					   x + sw, y + h * 0.88,
					   x + sw * 0.12, y + h * 0.88,
					   x + sw * 0.58, y + h * 0.44,
					   x + sw * 0.12, y,
					   x + sw, y);
		}
		return 1;
	}
	if (strcmp(tex_name, "prod") == 0) {
		double pw = h * 0.74;
		if (draw) {
			pdf_printf(s, "q %.2f w 1 J %.2f %.2f m %.2f %.2f l S %.2f %.2f m %.2f %.2f l S %.2f %.2f m %.2f %.2f l S Q\n",
					   h * 0.09,
					   x, y + h * 0.86, x + pw, y + h * 0.86,
					   x + pw * 0.26, y + h * 0.86, x + pw * 0.26, y,
					   x + pw * 0.74, y + h * 0.86, x + pw * 0.74, y);
		}
		return 1;
	}
	if (strcmp(tex_name, "int") == 0 || strcmp(tex_name, "iint") == 0 ||
		strcmp(tex_name, "iiint") == 0 || strcmp(tex_name, "oint") == 0) {
		int cnt = 1;
		if (strcmp(tex_name, "iint") == 0) cnt = 2;
		else if (strcmp(tex_name, "iiint") == 0) cnt = 3;
		if (draw) {
			for (int k = 0; k < cnt; k++) {
				double cx = x + h * (0.28 + k * 0.26);
				pdf_printf(s, "q %.2f w 1 J %.2f %.2f m %.2f %.2f l S %.2f %.2f %.2f %.2f re f %.2f %.2f %.2f %.2f re f Q\n",
						   h * 0.08, cx, y + h * 0.12, cx, y + h * 0.82,
						   cx + h * 0.04, y + h * 0.78, h * 0.16, h * 0.12,
						   cx - h * 0.20, y + h * 0.04, h * 0.16, h * 0.12);
			}
			if (strcmp(tex_name, "oint") == 0) {
				double cx = x + h * 0.28;
				double r = h * 0.20;
				pdf_printf(s, "q %.2f w %.2f %.2f m %.2f %.2f %.2f %.2f %.2f %.2f c S Q\n",
						   h * 0.06,
						   cx, y + h * 0.45 + r,
						   cx + r, y + h * 0.45 + r, cx + r, y + h * 0.45, cx + r, y + h * 0.45,
						   cx + r, y + h * 0.45 - r, cx, y + h * 0.45 - r, cx, y + h * 0.45 - r);
				pdf_printf(s, "q %.2f w %.2f %.2f m %.2f %.2f %.2f %.2f %.2f %.2f c S Q\n",
						   h * 0.06,
						   cx, y + h * 0.45 - r,
						   cx - r, y + h * 0.45 - r, cx - r, y + h * 0.45, cx - r, y + h * 0.45,
						   cx - r, y + h * 0.45 + r, cx, y + h * 0.45 + r, cx, y + h * 0.45 + r);
			}
		}
		return 1;
	}
	if (strcmp(tex_name, "infty") == 0) {
		double iw = h * 0.85;
		double cy = y + h * 0.36;
		double r = h * 0.18;
		if (draw) {
			pdf_printf(s, "q %.2f w %.2f %.2f m %.2f %.2f %.2f %.2f %.2f %.2f c %.2f %.2f %.2f %.2f %.2f %.2f c S Q\n",
					   h * 0.07,
					   x + iw * 0.5, cy,
					   x + iw * 0.75, cy + r * 1.4, x + iw, cy + r * 1.1, x + iw, cy,
					   x + iw, cy - r * 1.1, x + iw * 0.75, cy - r * 1.4, x + iw * 0.5, cy);
			pdf_printf(s, "q %.2f w %.2f %.2f m %.2f %.2f %.2f %.2f %.2f %.2f c %.2f %.2f %.2f %.2f %.2f %.2f c S Q\n",
					   h * 0.07,
					   x + iw * 0.5, cy,
					   x + iw * 0.25, cy - r * 1.4, x, cy - r * 1.1, x, cy,
					   x, cy + r * 1.1, x + iw * 0.25, cy + r * 1.4, x + iw * 0.5, cy);
		}
		return 1;
	}
	if (strcmp(tex_name, "partial") == 0) {
		double pw = h * 0.52;
		if (draw) {
			pdf_printf(s, "q %.2f w %.2f %.2f m %.2f %.2f %.2f %.2f %.2f %.2f c %.2f %.2f %.2f %.2f %.2f %.2f c %.2f %.2f %.2f %.2f %.2f %.2f c S Q\n",
					   h * 0.075,
					   x + pw * 0.20, y + h * 0.68,
					   x + pw * 0.65, y + h * 0.85, x + pw * 0.90, y + h * 0.55, x + pw * 0.82, y + h * 0.25,
					   x + pw * 0.75, y - h * 0.04, x + pw * 0.15, y - h * 0.04, x + pw * 0.12, y + h * 0.22,
					   x + pw * 0.10, y + h * 0.45, x + pw * 0.65, y + h * 0.48, x + pw * 0.80, y + h * 0.30);
		}
		return 1;
	}
	if (strcmp(tex_name, "nabla") == 0) {
		double nw = h * 0.68;
		if (draw) {
			pdf_printf(s, "q %.2f w 1 J 1 j %.2f %.2f m %.2f %.2f l %.2f %.2f l %.2f %.2f l S Q\n",
					   h * 0.075,
					   x, y + h * 0.74,
					   x + nw, y + h * 0.74,
					   x + nw * 0.5, y + h * 0.04,
					   x, y + h * 0.74);
		}
		return 1;
	}
	if (strcmp(tex_name, "ldots") == 0 || strcmp(tex_name, "cdots") == 0 || strcmp(tex_name, "dots") == 0) {
		double cy = strcmp(tex_name, "cdots") == 0 ? (y + h * 0.35) : y;
		double r = h * 0.05;
		if (draw) {
			for (int k = 0; k < 3; k++) {
				double dx = x + h * (0.12 + k * 0.22);
				pdf_printf(s, "q %.2f %.2f %.2f %.2f re f Q\n", dx - r, cy - r, r * 2.0, r * 2.0);
			}
		}
		return 1;
	}
	if (strcmp(tex_name, "vdots") == 0) {
		double cx = x + h * 0.22;
		double r = h * 0.05;
		if (draw) {
			for (int k = 0; k < 3; k++) {
				double dy = y + h * (0.15 + k * 0.28);
				pdf_printf(s, "q %.2f %.2f %.2f %.2f re f Q\n", cx - r, dy - r, r * 2.0, r * 2.0);
			}
		}
		return 1;
	}
	if (strcmp(tex_name, "ddots") == 0) {
		double r = h * 0.05;
		if (draw) {
			for (int k = 0; k < 3; k++) {
				double dx = x + h * (0.12 + k * 0.22);
				double dy = y + h * (0.72 - k * 0.26);
				pdf_printf(s, "q %.2f %.2f %.2f %.2f re f Q\n", dx - r, dy - r, r * 2.0, r * 2.0);
			}
		}
		return 1;
	}
	return 0;
}

static double symbol_advance_width(const char *tex_name, double h) {
	if (!tex_name) return h * 0.55;
	{
		const SymbolEntry *entry = find_symbol(tex_name);
		if (entry && entry->utf8) {
			double advance = math_font_glyph_advance(entry->utf8, h);
			if (advance > 0.0) return advance;
		}
	}
	if (strcmp(tex_name, "omega") == 0) return h * 0.72;
	if (strcmp(tex_name, "pm") == 0 || strcmp(tex_name, "mp") == 0 || strcmp(tex_name, "times") == 0) return h * 0.52;
	if (strcmp(tex_name, "div") == 0) return h * 0.62;
	if (strcmp(tex_name, "cdot") == 0 || strcmp(tex_name, "bullet") == 0) return h * 0.28;
	if (strcmp(tex_name, "leq") == 0 || strcmp(tex_name, "le") == 0 ||
		strcmp(tex_name, "geq") == 0 || strcmp(tex_name, "ge") == 0 ||
		strcmp(tex_name, "neq") == 0 || strcmp(tex_name, "ne") == 0 ||
		strcmp(tex_name, "equiv") == 0 || strcmp(tex_name, "approx") == 0 || strcmp(tex_name, "sim") == 0) return h * 0.68;
	if (strcmp(tex_name, "to") == 0 || strcmp(tex_name, "rightarrow") == 0 ||
		strcmp(tex_name, "Rightarrow") == 0 || strcmp(tex_name, "implies") == 0 ||
		strcmp(tex_name, "leftarrow") == 0 || strcmp(tex_name, "Leftarrow") == 0) return h * 0.85;
	if (strcmp(tex_name, "sum") == 0 || strcmp(tex_name, "prod") == 0) return h * 0.82;
	if (strcmp(tex_name, "int") == 0 || strcmp(tex_name, "oint") == 0) return h * 0.58;
	if (strcmp(tex_name, "iint") == 0) return h * 0.84;
	if (strcmp(tex_name, "iiint") == 0) return h * 1.10;
	if (strcmp(tex_name, "infty") == 0) return h * 0.92;
	if (strcmp(tex_name, "partial") == 0) return h * 0.58;
	if (strcmp(tex_name, "nabla") == 0) return h * 0.72;
	if (strcmp(tex_name, "ldots") == 0 || strcmp(tex_name, "cdots") == 0 ||
		strcmp(tex_name, "ddots") == 0 || strcmp(tex_name, "dots") == 0) return h * 0.72;
	if (strcmp(tex_name, "vdots") == 0) return h * 0.44;
	return h * 0.62;
}

static void draw_tall_paren(PdfStream *s, const char *delim, double x, double bottom_y, double top_y, double lw) {
	if (!delim || strcmp(delim, ".") == 0) return;
	double mid_y = (bottom_y + top_y) * 0.5;
	double h = top_y - bottom_y;
	double dx = h * 0.16;
	if (dx < 3.5) dx = 3.5;
	if (dx > 14.0) dx = 14.0;
	
	if (strcmp(delim, "(") == 0) {
		pdf_printf(s, "q %.2f w 1 j %.2f %.2f m %.2f %.2f %.2f %.2f %.2f %.2f c %.2f %.2f %.2f %.2f %.2f %.2f c S Q\n",
				   lw, x + dx, top_y,
				   x + dx * 0.12, top_y - h * 0.18, x, mid_y + h * 0.18, x, mid_y,
				   x, mid_y - h * 0.18, x + dx * 0.12, bottom_y + h * 0.18, x + dx, bottom_y);
	} else if (strcmp(delim, ")") == 0) {
		pdf_printf(s, "q %.2f w 1 j %.2f %.2f m %.2f %.2f %.2f %.2f %.2f %.2f c %.2f %.2f %.2f %.2f %.2f %.2f c S Q\n",
				   lw, x, top_y,
				   x + dx * 0.88, top_y - h * 0.18, x + dx, mid_y + h * 0.18, x + dx, mid_y,
				   x + dx, mid_y - h * 0.18, x + dx * 0.88, bottom_y + h * 0.18, x, bottom_y);
	} else if (strcmp(delim, "[") == 0) {
		pdf_printf(s, "q %.2f w 1 J 1 j %.2f %.2f m %.2f %.2f l %.2f %.2f l %.2f %.2f l S Q\n",
				   lw, x + dx, top_y, x, top_y, x, bottom_y, x + dx, bottom_y);
	} else if (strcmp(delim, "]") == 0) {
		pdf_printf(s, "q %.2f w 1 J 1 j %.2f %.2f m %.2f %.2f l %.2f %.2f l %.2f %.2f l S Q\n",
				   lw, x, top_y, x + dx, top_y, x + dx, bottom_y, x, bottom_y);
	} else if (strcmp(delim, "|") == 0) {
		pdf_printf(s, "q %.2f w %.2f %.2f m %.2f %.2f l S Q\n", lw, x + dx * 0.5, top_y, x + dx * 0.5, bottom_y);
	} else if (strcmp(delim, "||") == 0) {
		pdf_printf(s, "q %.2f w %.2f %.2f m %.2f %.2f l S %.2f %.2f m %.2f %.2f l S Q\n",
				   lw, x + dx * 0.3, top_y, x + dx * 0.3, bottom_y, x + dx * 0.75, top_y, x + dx * 0.75, bottom_y);
	} else if (strcmp(delim, "{") == 0) {
		pdf_printf(s, "q %.2f w 1 j %.2f %.2f m %.2f %.2f %.2f %.2f %.2f %.2f c %.2f %.2f %.2f %.2f %.2f %.2f c S Q\n",
				   lw, x + dx, top_y, x, top_y, x, mid_y + h * 0.22, x + dx, mid_y,
				   x, mid_y - h * 0.22, x, bottom_y, x + dx, bottom_y);
	} else if (strcmp(delim, "}") == 0) {
		pdf_printf(s, "q %.2f w 1 j %.2f %.2f m %.2f %.2f %.2f %.2f %.2f %.2f c %.2f %.2f %.2f %.2f %.2f %.2f c S Q\n",
				   lw, x, top_y, x + dx, top_y, x + dx, mid_y + h * 0.22, x, mid_y,
				   x + dx, mid_y - h * 0.22, x + dx, bottom_y, x, bottom_y);
	} else if (strcmp(delim, "<") == 0) {
		pdf_printf(s, "q %.2f w 1 J 1 j %.2f %.2f m %.2f %.2f l %.2f %.2f l S Q\n",
				   lw, x + dx, top_y, x, mid_y, x + dx, bottom_y);
	} else if (strcmp(delim, ">") == 0) {
		pdf_printf(s, "q %.2f w 1 J 1 j %.2f %.2f m %.2f %.2f l %.2f %.2f l S Q\n",
				   lw, x, top_y, x + dx, mid_y, x, bottom_y);
	}
}

typedef struct {
	size_t start;
	size_t end;
	double width;
	double ascent;
	double descent;
	double baseline;
} MathLine;

static int is_math_break_operator(const AstNode *n) {
	if (!n) return 0;
	if (n->type == NODE_XARROW) return 1;
	if (n->type != NODE_ATOM || !n->text) return 0;
	return strcmp(n->text, "+") == 0 || strcmp(n->text, "-") == 0 ||
		strcmp(n->text, "=") == 0 || strcmp(n->text, "<") == 0 || strcmp(n->text, ">") == 0 ||
		strcmp(n->text, "leq") == 0 || strcmp(n->text, "le") == 0 ||
		strcmp(n->text, "geq") == 0 || strcmp(n->text, "ge") == 0 ||
		strcmp(n->text, "neq") == 0 || strcmp(n->text, "ne") == 0 ||
		strcmp(n->text, "equiv") == 0 || strcmp(n->text, "approx") == 0 ||
		strcmp(n->text, "to") == 0 || strcmp(n->text, "rightarrow") == 0 ||
		strcmp(n->text, "leftarrow") == 0 || strcmp(n->text, "times") == 0 ||
		strcmp(n->text, "cdot") == 0 || strcmp(n->text, "pm") == 0 || strcmp(n->text, "mp") == 0;
}

static int mathcal_glyph_for_ascii(char ch, char out[5]) {
	static const unsigned int script_caps[26] = {
		0x1D49C, 0x212C, 0x1D49E, 0x1D49F, 0x2130, 0x2131, 0x1D4A2,
		0x210B, 0x2110, 0x1D4A5, 0x1D4A6, 0x2112, 0x2133, 0x1D4A9,
		0x1D4AA, 0x1D4AB, 0x1D4AC, 0x211B, 0x1D4AE, 0x1D4AF,
		0x1D4B0, 0x1D4B1, 0x1D4B2, 0x1D4B3, 0x1D4B4, 0x1D4B5
	};
	unsigned int cp;
	if (ch < 'A' || ch > 'Z') return 0;
	cp = script_caps[ch - 'A'];
	if (cp <= 0x7F) { out[0] = (char)cp; out[1] = '\0'; }
	else if (cp <= 0x7FF) {
		out[0] = (char)(0xC0 | (cp >> 6)); out[1] = (char)(0x80 | (cp & 0x3F)); out[2] = '\0';
	} else if (cp <= 0xFFFF) {
		out[0] = (char)(0xE0 | (cp >> 12)); out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
		out[2] = (char)(0x80 | (cp & 0x3F)); out[3] = '\0';
	} else {
		out[0] = (char)(0xF0 | (cp >> 18)); out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
		out[2] = (char)(0x80 | ((cp >> 6) & 0x3F)); out[3] = (char)(0x80 | (cp & 0x3F)); out[4] = '\0';
	}
	return 1;
}

static const AstNode *unwrap_mathcal_letter(const AstNode *arg) {
	const AstNode *node = arg;
	while (node && (node->type == NODE_GROUP || node->type == NODE_SEQ) && node->child_count == 1)
		node = node->children[0];
	return node;
}

static double measure_text_advance(const char *text, double fs) {
	double width = 0.0;
	if (!text) return 0.0;
	for (const unsigned char *p = (const unsigned char *)text; *p;) {
		size_t bytes = 1;
		char glyph[5] = {0,0,0,0,0};
		if ((*p & 0xE0) == 0xC0 && p[1]) bytes = 2;
		else if ((*p & 0xF0) == 0xE0 && p[1] && p[2]) bytes = 3;
		else if ((*p & 0xF8) == 0xF0 && p[1] && p[2] && p[3]) bytes = 4;
		memcpy(glyph, p, bytes);
		double advance = bytes > 1 ? math_font_glyph_advance(glyph, fs) : -1.0;
		if (advance > 0.0) width += advance;
		else if (bytes > 1) width += fs * 0.58;
		else if (*p == ' ' || *p == '\t') width += fs * (*p == '\t' ? 1.12 : 0.28);
		else if (ispunct(*p)) width += fs * 0.32;
		else if (isupper(*p)) width += fs * 0.60;
		else width += fs * 0.50;
		p += bytes;
	}
	return width;
}

static void emit_unicode_text(PdfStream *s, double x, double y, double fs, int italic, const char *text) {
	const unsigned char *p = (const unsigned char *)text;
	if (!s || !text) return;
	while (*p) {
		if (*p < 0x80) {
			const unsigned char *end = p;
			while (*end && *end < 0x80) ++end;
			char *run = xstrdup_range((const char *)p, (size_t)(end - p));
			emit_pdf_ascii_text(s, x, y, fs, italic, run);
			x += measure_text_advance(run, fs);
			free(run);
			p = end;
		} else {
			size_t bytes = ((*p & 0xE0) == 0xC0) ? 2 : (((*p & 0xF0) == 0xE0) ? 3 : 4);
			char glyph[5] = {0,0,0,0,0};
			double advance;
			if (!p[1] || (bytes >= 3 && !p[2]) || (bytes == 4 && !p[3])) bytes = 1;
			memcpy(glyph, p, bytes);
			advance = math_font_glyph_advance(glyph, fs);
			if (advance <= 0.0) advance = fs * 0.58;
			if (!draw_math_font_glyph(s, glyph, x, y, fs, 1)) emit_pdf_ascii_text(s, x, y, fs, italic, "?");
			x += advance;
			p += bytes;
		}
	}
}

static void draw_horizontal_brace(PdfStream *s, double x, double edge_y, double width,
								  double depth, double lw, int above) {
	double dir = above ? 1.0 : -1.0;
	double shoulder_y = edge_y + dir * depth * 0.55;
	double neck_y = edge_y + dir * depth * 0.12;
	double tip_y = edge_y + dir * depth * 1.15;
	if (!s || width <= 0.0 || depth <= 0.0) return;
	pdf_printf(s, "q %.2f w 1 j %.2f %.2f m\n", lw, x, edge_y);
	pdf_printf(s, "%.2f %.2f %.2f %.2f %.2f %.2f c\n",
			x + width * 0.04, edge_y, x + width * 0.07, shoulder_y, x + width * 0.16, shoulder_y);
	pdf_printf(s, "%.2f %.2f %.2f %.2f %.2f %.2f c\n",
			x + width * 0.25, shoulder_y, x + width * 0.30, neck_y, x + width * 0.40, neck_y);
	pdf_printf(s, "%.2f %.2f %.2f %.2f %.2f %.2f c\n",
			x + width * 0.46, neck_y, x + width * 0.46, tip_y, x + width * 0.50, tip_y);
	pdf_printf(s, "%.2f %.2f %.2f %.2f %.2f %.2f c\n",
			x + width * 0.54, tip_y, x + width * 0.54, neck_y, x + width * 0.60, neck_y);
	pdf_printf(s, "%.2f %.2f %.2f %.2f %.2f %.2f c\n",
			x + width * 0.70, neck_y, x + width * 0.75, shoulder_y, x + width * 0.84, shoulder_y);
	pdf_printf(s, "%.2f %.2f %.2f %.2f %.2f %.2f c S Q\n",
			x + width * 0.93, shoulder_y, x + width * 0.96, edge_y, x + width, edge_y);
}

static int accent_is(const AstNode *n, const char *name) {
	return n && n->type == NODE_ACCENT && n->text && strcmp(n->text, name) == 0;
}

static int uses_display_limits(const AstNode *n) {
	if (!n) return 0;
	if (n->limits_mode > 0) return 1;
	if (n->limits_mode < 0) return 0;
	if (accent_is(n, "mathop") || accent_is(n, "overbrace") || accent_is(n, "underbrace")) return 1;
	if (n->type != NODE_ATOM || !n->text) return 0;
	if (n->is_big_op && strcmp(n->text, "int") != 0 && strcmp(n->text, "iint") != 0 &&
		strcmp(n->text, "iiint") != 0 && strcmp(n->text, "oint") != 0 &&
		strcmp(n->text, "oiint") != 0 && strcmp(n->text, "oiiint") != 0 && strcmp(n->text, "smallint") != 0) return 1;
	if (!n->is_func) return 0;
	return strcmp(n->text, "lim") == 0 || strcmp(n->text, "limsup") == 0 ||
		strcmp(n->text, "liminf") == 0 || strcmp(n->text, "max") == 0 ||
		strcmp(n->text, "min") == 0 || strcmp(n->text, "sup") == 0 ||
		strcmp(n->text, "inf") == 0 || strcmp(n->text, "argmin") == 0 ||
		strcmp(n->text, "argmax") == 0 || strcmp(n->text, "varlim") == 0 ||
		strcmp(n->text, "varlimsup") == 0 || strcmp(n->text, "varliminf") == 0;
}

static char matrix_column_alignment(const AstNode *n, int column) {
	if (!n || !n->align_spec || column < 0) return 'c';
	int found = 0, depth = 0;
	for (const char *p = n->align_spec; *p; ++p) {
		if (*p == '{') { ++depth; continue; }
		if (*p == '}') { if (depth > 0) --depth; continue; }
		if (depth > 0) continue;
		if (*p == 'l' || *p == 'c' || *p == 'r') {
			if (found == column) return *p;
			++found;
		}
	}
	return 'c';
}

static BoxMetrics layout_or_draw_node_d(PdfStream *s, const AstNode *n, double x, double y, double fs, int draw, int depth) {
	BoxMetrics m = {0.0, fs * 0.80, fs * 0.25};
	/* Keep deeply nested scripts visible; the final PDF transform handles tiny fit scales. */
	if (!n || depth > MAX_LAYOUT_DEPTH || fs < 0.02) return m;
	
	if (!draw && fabs(n->cached_fs - fs) < 1e-6 && fabs(n->cached_line_width - g_formula_line_width) < 1e-6) {
		return n->cached_m;
	}
	
	switch (n->type) {
	case NODE_SEQ:
		case NODE_GROUP: {
		size_t count = n->child_count;
		size_t line_count = 0, start = 0;
		BoxMetrics *child_m = (BoxMetrics *)xcalloc(count, sizeof(BoxMetrics));
		MathLine *lines = (MathLine *)xcalloc(count + 1, sizeof(MathLine));
		double base_pad = fs * 0.18;
		double atom_gap = fs * 0.12;
		double indent = fs * 1.20;
		double available = g_formula_line_width > 0.0 ? g_formula_line_width : 1e30;
		double lowest = 0.0, highest = 0.0;
		
		for (size_t i = 0; i < count; ++i)
			child_m[i] = layout_or_draw_node_d(s, n->children[i], 0, 0, fs, 0, depth + 1);
		
		while (start < count) {
			MathLine *line = &lines[line_count];
			size_t j = start, last_break = (size_t)-1;
			size_t next_start = start;
			int forced_break = 0;
			double width = 0.0;
			double line_indent = line_count ? indent : 0.0;
			double line_limit = available - line_indent;
			if (line_limit < fs * 2.0) line_limit = fs * 2.0;
			line->start = start;
			line->ascent = fs * 0.75;
			line->descent = fs * 0.22;
			while (j < count) {
				if (n->children[j] && n->children[j]->type == NODE_BREAK) {
					line->end = j;
					next_start = j + 1;
					forced_break = 1;
					break;
				}
				int is_op = is_math_break_operator(n->children[j]);
				double pad = is_op && j > start ? base_pad : 0.0;
				double item_width = child_m[j].width + pad * 2.0 + (j > start ? atom_gap : 0.0);
				if (is_op && j > start) last_break = j;
				/* Keep a leading operator with its following term; don't orphan it. */
				if (width + item_width > line_limit && j > start + 1) {
					size_t end = (last_break != (size_t)-1 && last_break > start) ? last_break : j;
					if (end <= start) end = j;
					line->end = end;
					break;
				}
				width += item_width;
				if (child_m[j].ascent > line->ascent) line->ascent = child_m[j].ascent;
				if (child_m[j].descent > line->descent) line->descent = child_m[j].descent;
				++j;
			}
			if (!forced_break) {
				if (j >= count) line->end = count;
				next_start = line->end;
			}
			if (line->end < start) line->end = start;
			if (!forced_break && line->end == start) line->end = start + 1;
			/* Recompute when we backed up to an operator break point. */
			width = 0.0;
			line->ascent = fs * 0.75;
			line->descent = fs * 0.22;
			for (size_t k = start; k < line->end; ++k) {
				int is_op = is_math_break_operator(n->children[k]);
				double pad = is_op && k > start ? base_pad : 0.0;
				width += child_m[k].width + pad * 2.0 + (k > start ? atom_gap : 0.0);
				if (child_m[k].ascent > line->ascent) line->ascent = child_m[k].ascent;
				if (child_m[k].descent > line->descent) line->descent = child_m[k].descent;
			}
			line->width = width;
			if (line_count == 0) line->baseline = 0.0;
			else {
				MathLine *prev = &lines[line_count - 1];
				line->baseline = prev->baseline - prev->descent - line->ascent - fs * 0.42;
			}
			if (line_indent + line->width > m.width) m.width = line_indent + line->width;
			if (line->baseline + line->ascent > highest) highest = line->baseline + line->ascent;
			if (line->baseline - line->descent < lowest) lowest = line->baseline - line->descent;
			start = next_start;
			++line_count;
		}
		m.ascent = highest > 0.0 ? highest : fs * 0.75;
		m.descent = lowest < 0.0 ? -lowest : fs * 0.22;
		
		if (draw) {
			for (size_t r = 0; r < line_count; ++r) {
				MathLine *line = &lines[r];
				double cur_x = x + (r ? indent : 0.0);
				for (size_t i = line->start; i < line->end; ++i) {
					int is_op = is_math_break_operator(n->children[i]);
					double pad = is_op && i > line->start ? base_pad : 0.0;
					cur_x += pad;
					layout_or_draw_node_d(s, n->children[i], cur_x, y + line->baseline, fs, 1, depth + 1);
					cur_x += child_m[i].width + pad + (i + 1 < line->end ? atom_gap : 0.0);
				}
			}
		}
		free(child_m);
		free(lines);
		break;
	}

	case NODE_PHANTOM:
		if (n->child_count >= 1) {
			m = layout_or_draw_node_d(s, n->children[0], 0, 0, fs, 0, depth + 1);
			if (n->text && strcmp(n->text, "hphantom") == 0) m.ascent = m.descent = 0.0;
			else if (n->text && strcmp(n->text, "vphantom") == 0) m.width = 0.0;
			else if (n->text && strcmp(n->text, "smash") == 0) m.ascent = m.descent = 0.0;
		}
		break;

	case NODE_BREAK:
		m.width = 0.0;
		m.ascent = m.descent = 0.0;
		break;

	case NODE_SPACE:
		m.width = n->space_em * fs + n->space_pt;
		m.ascent = m.descent = 0.0;
		break;

	case NODE_COLOR:
		if (n->child_count >= 1) {
			const AstNode *content = n->children[0];
			BoxMetrics cm = layout_or_draw_node_d(s, content, 0, 0, fs, 0, depth + 1);
			if (n->text && strcmp(n->text, "colorbox") == 0) {
				double pad = fs * 0.10;
				m.width = cm.width + pad * 2.0;
				m.ascent = cm.ascent + pad;
				m.descent = cm.descent + pad;
				if (draw) {
					pdf_printf(s, "q %.3f %.3f %.3f rg %.2f %.2f %.2f %.2f re f Q\n", n->color_r, n->color_g, n->color_b,
						x - pad, y - cm.descent - pad, cm.width + pad * 2.0, cm.ascent + cm.descent + pad * 2.0);
					layout_or_draw_node_d(s, content, x, y, fs, 1, depth + 1);
				}
			} else {
				m = cm;
				if (draw) {
					pdf_append(s, "q\n");
					pdf_printf(s, "%.3f %.3f %.3f rg %.3f %.3f %.3f RG\n", n->color_r, n->color_g, n->color_b, n->color_r, n->color_g, n->color_b);
					layout_or_draw_node_d(s, content, x, y, fs, 1, depth + 1);
					pdf_append(s, "Q\n");
				}
			}
		} else {
			m.width = 0.0;
			m.ascent = m.descent = 0.0;
		}
		break;

	case NODE_PDF_TEXT: {
		double width = measure_text_advance(n->text, fs);
		if (draw && n->text) emit_unicode_text(s, x, y, fs, 0, n->text);
		m.width = width;
		m.ascent = fs * 0.78;
		m.descent = fs * 0.18;
		break;
	}

	case NODE_OVERS: {
		const AstNode *base = n->child_count >= 1 ? n->children[0] : NULL;
		const AstNode *over = n->child_count >= 2 ? n->children[1] : NULL;
		double over_fs = fs * 0.62;
		BoxMetrics bm = layout_or_draw_node_d(s, base, 0, 0, fs, 0, depth + 1);
		BoxMetrics om = layout_or_draw_node_d(s, over, 0, 0, over_fs, 0, depth + 1);
		double width = fmax(bm.width, om.width) + fs * 0.08;
		double base_x = x + (width - bm.width) * 0.5;
		double over_x = x + (width - om.width) * 0.5;
		double over_y = y + bm.ascent + fs * 0.08 + om.descent;
		if (draw) {
			layout_or_draw_node_d(s, base, base_x, y, fs, 1, depth + 1);
			layout_or_draw_node_d(s, over, over_x, over_y, over_fs, 1, depth + 1);
		}
		m.width = width;
		m.ascent = over_y - y + om.ascent;
		m.descent = bm.descent;
		break;
	}

	case NODE_XARROW: {
		const AstNode *below = n->child_count >= 1 ? n->children[0] : NULL;
		const AstNode *above = n->child_count >= 2 ? n->children[1] : NULL;
		double above_fs = fs * 0.62, below_fs = fs * 0.58;
		BoxMetrics am = {0.0, 0.0, 0.0}, bm = {0.0, 0.0, 0.0};
		if (!below || !((below->type == NODE_SEQ || below->type == NODE_GROUP) && below->child_count == 0))
			bm = layout_or_draw_node_d(s, below, 0, 0, below_fs, 0, depth + 1);
		if (!above || !((above->type == NODE_SEQ || above->type == NODE_GROUP) && above->child_count == 0))
			am = layout_or_draw_node_d(s, above, 0, 0, above_fs, 0, depth + 1);
		double arrow_width = fmax(fs * 1.55, fmax(am.width, bm.width) + fs * 0.42);
		double width = arrow_width + fs * 0.14;
		double arrow_x = x + (width - arrow_width) * 0.5;
		double x0 = arrow_x + fs * 0.06, x1 = arrow_x + arrow_width - fs * 0.06;
		double head = fs * 0.23, rise = fs * 0.14;
		double above_y = y + fs * 0.28 + am.descent;
		double below_y = y - fs * 0.28 - bm.ascent;
		if (draw) {
			if (am.ascent > 0.0 || am.width > 0.0)
				layout_or_draw_node_d(s, above, arrow_x + (arrow_width - am.width) * 0.5, above_y, above_fs, 1, depth + 1);
			if (bm.ascent > 0.0 || bm.width > 0.0)
				layout_or_draw_node_d(s, below, arrow_x + (arrow_width - bm.width) * 0.5, below_y, below_fs, 1, depth + 1);
			int left_only = n->text && (strcmp(n->text, "xleftarrow") == 0 || strcmp(n->text, "xLeftarrow") == 0);
			int both = n->text && (strcmp(n->text, "xleftrightarrow") == 0 || strcmp(n->text, "xLeftrightarrow") == 0);
			int double_line = n->text && (strcmp(n->text, "xRightarrow") == 0 || strcmp(n->text, "xLeftarrow") == 0 || strcmp(n->text, "xLeftrightarrow") == 0);
			int is_mapsto = n->text && strcmp(n->text, "xmapsto") == 0;
			pdf_printf(s, "q %.2f w 1 J 1 j %.2f %.2f m %.2f %.2f l S\n", fs * 0.055, x0, y, x1, y);
			if (double_line) pdf_printf(s, "%.2f %.2f m %.2f %.2f l S\n", x0, y - fs * 0.075, x1, y - fs * 0.075);
			if (!left_only || both) pdf_printf(s, "%.2f %.2f m %.2f %.2f l %.2f %.2f l S\n", x1 - head, y + rise, x1, y, x1 - head, y - rise);
			if (left_only || both) pdf_printf(s, "%.2f %.2f m %.2f %.2f l %.2f %.2f l S\n", x0 + head, y + rise, x0, y, x0 + head, y - rise);
			if (is_mapsto) pdf_printf(s, "%.2f %.2f m %.2f %.2f l S\n", x0, y - fs * 0.16, x0, y + fs * 0.16);
			pdf_append(s, "Q\n");
		}
		m.width = width;
		m.ascent = fmax(fs * 0.30, am.ascent > 0.0 ? above_y - y + am.ascent : fs * 0.30);
		m.descent = fmax(fs * 0.30, bm.ascent > 0.0 ? y - below_y + bm.descent : fs * 0.30);
		break;
	}
		
	case NODE_ATOM:
		case NODE_UNKNOWN: {
			if (n->type == NODE_ATOM && n->text) {
				if (strcmp(n->text, "\x1f") == 0) {
					m.width = -fs * 0.12;
					break;
				}
				if (strcmp(n->text, ",") == 0 || strcmp(n->text, "thinspace") == 0) {
					m.width = fs * 0.20;
					break;
				}
				if (strcmp(n->text, ":") == 0 || strcmp(n->text, "medspace") == 0) {
					m.width = fs * 0.30;
					break;
				}
				if (strcmp(n->text, ";") == 0 || strcmp(n->text, "thickspace") == 0) {
					m.width = fs * 0.40;
					break;
				}
				if (strcmp(n->text, " ") == 0 || strcmp(n->text, "enspace") == 0 || strcmp(n->text, "enskip") == 0) {
					m.width = fs * 0.50;
					break;
				}
				if (strcmp(n->text, "quad") == 0) {
					m.width = fs * 0.80;
					break;
				}
				if (strcmp(n->text, "qquad") == 0) {
					m.width = fs * 1.60;
					break;
				}
			}
			if (n->type == NODE_UNKNOWN) {
				const char *t = n->text ? n->text : "";
				double w = strlen(t) * fs * 0.52 + fs * 0.06;
				if (draw) emit_pdf_ascii_text(s, x, y, fs * 0.85, 0, t);
				double cur_w = w;
				double a = fs * 0.80, d = fs * 0.25;
				if (n->child_count >= 1) {
					BoxMetrics am = layout_or_draw_node_d(s, n->children[0], x + cur_w, y, fs * 0.90, draw, depth + 1);
					cur_w += am.width;
					if (am.ascent > a) a = am.ascent;
					if (am.descent > d) d = am.descent;
				}
				m.width = cur_w;
				m.ascent = a;
				m.descent = d;
				break;
			}
			if (n->is_func && n->text) {
				double w = strlen(n->text) * fs * 0.54 + fs * 0.12;
				if (draw) emit_pdf_ascii_text(s, x, y, fs, 0, n->text);
				m.width = w;
				break;
			}
			if (n->utf8 && n->utf8[0] != '\0' && n->text) {
				double h_scale = n->is_big_op ? fs * 1.35 : fs;
				int drawn = draw_vector_symbol(s, n->text, x, y, h_scale, draw);
				if (!drawn) drawn = draw_math_font_glyph(s, n->utf8, x, y, h_scale, draw);
				if (drawn) {
					double advance = math_font_glyph_advance(n->utf8, h_scale);
					if (advance <= 0.0) advance = symbol_advance_width(n->text, h_scale);
					m.width = advance + fs * 0.06;
					m.ascent = n->is_big_op ? fs * 1.10 : fs * 0.85;
					m.descent = n->is_big_op ? fs * 0.40 : fs * 0.25;
					break;
				}
				double w = strlen(n->text) * fs * 0.52 + fs * 0.08;
				if (draw) emit_pdf_ascii_text(s, x, y, fs * 0.90, 1, n->text);
				m.width = w;
				break;
			}
			const char *t = n->text ? n->text : "";
			int is_var = (strlen(t) == 1 && isalpha((unsigned char)t[0]));
			double w = strlen(t) * fs * (is_var ? 0.56 : 0.54) + fs * 0.04;
			if (strcmp(t, "-") == 0) w = fs * 0.48;
			if (draw) emit_pdf_ascii_text(s, x, y, fs, is_var, t);
			m.width = w;
			break;
		}
		
		case NODE_ACCENT: {
			const AstNode *arg = n->child_count >= 1 ? n->children[0] : NULL;
			if (n->text && strcmp(n->text, "mathcal") == 0) {
				const AstNode *letter = unwrap_mathcal_letter(arg);
				char script_glyph[5];
				if (letter && letter->type == NODE_ATOM && letter->text && strlen(letter->text) == 1 &&
					mathcal_glyph_for_ascii(letter->text[0], script_glyph)) {
					double advance = math_font_glyph_advance(script_glyph, fs);
					if (advance > 0.0 && draw_math_font_glyph(s, script_glyph, x, y, fs, draw)) {
						m.width = advance + fs * 0.06;
						m.ascent = fs * 0.86;
						m.descent = fs * 0.22;
						break;
					}
				}
			}
			double box_pad = (n->text && strcmp(n->text, "boxed") == 0) ? fs * 0.13 : 0.0;
			BoxMetrics am = layout_or_draw_node_d(s, arg, x + box_pad, y, fs, draw, depth + 1);
			if (am.width < fs * 0.35) am.width = fs * 0.35;
			
			if (n->text) {
				if (strcmp(n->text, "underbrace") == 0 || strcmp(n->text, "overbrace") == 0) {
					double depth_y = fmax(fs * 0.42, fmin(am.width * 0.025, fs * 0.90));
					int over = strcmp(n->text, "overbrace") == 0;
					double edge_y = over ? y + am.ascent + fs * 0.08 : y - am.descent - fs * 0.08;
					if (draw) draw_horizontal_brace(s, x, edge_y, am.width, depth_y, fs * 0.045, over);
					if (over) am.ascent += depth_y * 1.35 + fs * 0.12;
					else am.descent += depth_y * 1.35 + fs * 0.12;
				} else if (strcmp(n->text, "dot") == 0 || strcmp(n->text, "ddot") == 0 || strcmp(n->text, "dddot") == 0) {
					double dy = y + am.ascent + fs * 0.10;
					double r = fs * 0.045;
					if (draw) {
						int dots = strcmp(n->text, "dot") == 0 ? 1 : (strcmp(n->text, "ddot") == 0 ? 2 : 3);
						for (int i = 0; i < dots; ++i) {
							double dx = x + am.width * ((double)(i + 1) / (double)(dots + 1));
							pdf_printf(s, "q %.2f %.2f %.2f %.2f re f Q\n", dx - r, dy - r, r * 2.0, r * 2.0);
						}
					}
					am.ascent += fs * 0.18;
				} else if (strcmp(n->text, "bar") == 0 || strcmp(n->text, "overline") == 0) {
					double oy = y + am.ascent + fs * 0.10;
					if (draw) pdf_printf(s, "q %.2f w %.2f %.2f m %.2f %.2f l S Q\n", fs * 0.05, x, oy, x + am.width, oy);
					am.ascent += fs * 0.18;
				} else if (strcmp(n->text, "tilde") == 0 || strcmp(n->text, "widetilde") == 0) {
					double width = fmax(am.width, fs * 0.55);
					double amp = fs * (strcmp(n->text, "widetilde") == 0 ? 0.24 : 0.18);
					double ty = y + am.ascent + fs * 0.10;
					if (draw) {
						pdf_printf(s, "q %.2f w 1 J 1 j %.2f %.2f m %.2f %.2f %.2f %.2f %.2f %.2f c %.2f %.2f %.2f %.2f %.2f %.2f c S Q\n",
								   fs * 0.052,
								   x, ty + amp * 0.10,
								   x + width * 0.18, ty + amp * 0.55,
								   x + width * 0.27, ty + amp * 0.50,
								   x + width * 0.48, ty + amp * 0.18,
								   x + width * 0.69, ty - amp * 0.12,
								   x + width * 0.81, ty + amp * 0.52,
								   x + width, ty + amp * 0.28);
					}
					am.ascent += amp * 0.72 + fs * 0.10;
				} else if (strcmp(n->text, "underline") == 0 || strcmp(n->text, "underbar") == 0) {
					double uy = y - am.descent - fs * 0.08;
					if (draw) pdf_printf(s, "q %.2f w %.2f %.2f m %.2f %.2f l S Q\n", fs * 0.05, x, uy, x + am.width, uy);
					am.descent += fs * 0.16;
				} else if (strcmp(n->text, "hat") == 0 || strcmp(n->text, "widehat") == 0) {
					double hy = y + am.ascent + fs * 0.08;
					if (draw) {
						pdf_printf(s, "q %.2f w 1 J 1 j %.2f %.2f m %.2f %.2f l %.2f %.2f l S Q\n",
								   fs * 0.05, x, hy, x + am.width * 0.5, hy + fs * 0.16, x + am.width, hy);
					}
					am.ascent += fs * 0.22;
				} else if (strcmp(n->text, "acute") == 0 || strcmp(n->text, "grave") == 0 || strcmp(n->text, "check") == 0) {
					double hy = y + am.ascent + fs * 0.08;
					if (draw) {
						if (strcmp(n->text, "check") == 0)
							pdf_printf(s, "q %.2f w 1 J 1 j %.2f %.2f m %.2f %.2f l %.2f %.2f l S Q\n", fs * 0.05, x + am.width * 0.30, hy + fs * 0.16, x + am.width * 0.48, hy, x + am.width * 0.72, hy + fs * 0.19);
						else if (strcmp(n->text, "acute") == 0)
							pdf_printf(s, "q %.2f w %.2f %.2f m %.2f %.2f l S Q\n", fs * 0.05, x + am.width * 0.42, hy, x + am.width * 0.70, hy + fs * 0.20);
						else
							pdf_printf(s, "q %.2f w %.2f %.2f m %.2f %.2f l S Q\n", fs * 0.05, x + am.width * 0.58, hy + fs * 0.20, x + am.width * 0.30, hy);
					}
					am.ascent += fs * 0.24;
				} else if (strcmp(n->text, "breve") == 0 || strcmp(n->text, "wideparen") == 0) {
					double hy = y + am.ascent + fs * 0.08, h = fs * 0.18;
					if (draw) pdf_printf(s, "q %.2f w 1 J 1 j %.2f %.2f m %.2f %.2f %.2f %.2f %.2f %.2f c S Q\n", fs * 0.05,
						x, hy, x + am.width * 0.20, hy + h, x + am.width * 0.80, hy + h, x + am.width, hy);
					am.ascent += h + fs * 0.06;
				} else if (strcmp(n->text, "mathring") == 0) {
					double r = fs * 0.095, cx = x + am.width * 0.5, cy = y + am.ascent + fs * 0.16;
					double k = r * 0.55228475;
					if (draw) pdf_printf(s, "q %.2f w %.2f %.2f m %.2f %.2f %.2f %.2f %.2f %.2f c %.2f %.2f %.2f %.2f %.2f %.2f c %.2f %.2f %.2f %.2f %.2f %.2f c %.2f %.2f %.2f %.2f %.2f %.2f c S Q\n",
						fs * 0.045, cx + r, cy, cx + r, cy + k, cx + k, cy + r, cx, cy + r,
						cx - k, cy + r, cx - r, cy + k, cx - r, cy,
						cx - r, cy - k, cx - k, cy - r, cx, cy - r,
						cx + k, cy - r, cx + r, cy - k, cx + r, cy);
					am.ascent += fs * 0.26;
				} else if (strcmp(n->text, "vec") == 0 || strcmp(n->text, "overrightarrow") == 0 ||
					strcmp(n->text, "overleftarrow") == 0 || strcmp(n->text, "overleftrightarrow") == 0) {
					double vy = y + am.ascent + fs * 0.12;
					if (draw) {
						int left = strcmp(n->text, "overleftarrow") == 0 || strcmp(n->text, "overleftrightarrow") == 0;
						int right = strcmp(n->text, "overleftarrow") != 0;
						pdf_printf(s, "q %.2f w 1 J 1 j %.2f %.2f m %.2f %.2f l S\n", fs * 0.05, x, vy, x + am.width, vy);
						if (right) pdf_printf(s, "%.2f %.2f m %.2f %.2f l %.2f %.2f l S\n", x + am.width - fs * 0.12, vy + fs * 0.08, x + am.width, vy, x + am.width - fs * 0.12, vy - fs * 0.08);
						if (left) pdf_printf(s, "%.2f %.2f m %.2f %.2f l %.2f %.2f l S\n", x + fs * 0.12, vy + fs * 0.08, x, vy, x + fs * 0.12, vy - fs * 0.08);
						pdf_append(s, "Q\n");
					}
					am.ascent += fs * 0.22;
				} else if (strcmp(n->text, "not") == 0 || strcmp(n->text, "cancel") == 0 ||
					strcmp(n->text, "bcancel") == 0 || strcmp(n->text, "xcancel") == 0 || strcmp(n->text, "boxed") == 0) {
					double pad = fs * 0.08;
					double x0 = x - pad, x1 = x + am.width + pad;
					double y0 = y - am.descent - pad, y1 = y + am.ascent + pad;
					if (draw) {
						if (strcmp(n->text, "cancel") == 0 || strcmp(n->text, "xcancel") == 0)
							pdf_printf(s, "q %.2f w %.2f %.2f m %.2f %.2f l S Q\n", fs * 0.055, x0, y0, x1, y1);
						if (strcmp(n->text, "bcancel") == 0 || strcmp(n->text, "xcancel") == 0 || strcmp(n->text, "not") == 0)
							pdf_printf(s, "q %.2f w %.2f %.2f m %.2f %.2f l S Q\n", fs * 0.055, x0, y1, x1, y0);
						if (strcmp(n->text, "boxed") == 0)
							pdf_printf(s, "q %.2f w %.2f %.2f %.2f %.2f re S Q\n", fs * 0.05, x, y - am.descent - fs * 0.08, am.width + box_pad * 2.0, am.ascent + am.descent + fs * 0.16);
					}
					if (strcmp(n->text, "boxed") == 0) { am.width += box_pad * 2.0; am.ascent += fs * 0.08; am.descent += fs * 0.08; }
				}
			}
			if (!n->text || (strcmp(n->text, "boxed") != 0 && strcmp(n->text, "not") != 0 &&
				strcmp(n->text, "cancel") != 0 && strcmp(n->text, "bcancel") != 0 && strcmp(n->text, "xcancel") != 0))
				am.width += fs * 0.06;
			m = am;
			break;
		}
		
		case NODE_STACK: {
			double line_fs = fs * 0.65;
			double gap = line_fs * 0.35;
			size_t cnt = n->child_count > 0 ? n->child_count : 1;
			BoxMetrics *lms = (BoxMetrics *)xcalloc(cnt, sizeof(BoxMetrics));
			double max_w = 0.0;
			double total_h = 0.0;
			
			for (size_t i = 0; i < n->child_count; i++) {
				lms[i] = layout_or_draw_node_d(s, n->children[i], 0, 0, line_fs, 0, depth + 1);
				if (lms[i].width > max_w) max_w = lms[i].width;
				total_h += lms[i].ascent + lms[i].descent;
				if (i > 0) total_h += gap;
			}
			
			double cur_top = y + total_h * 0.5;
			for (size_t i = 0; i < n->child_count; i++) {
				double ly = cur_top - lms[i].ascent;
				double lx = x + (max_w - lms[i].width) * 0.5;
				if (draw) layout_or_draw_node_d(s, n->children[i], lx, ly, line_fs, 1, depth + 1);
				cur_top -= (lms[i].ascent + lms[i].descent + gap);
			}
			
			free(lms);
			m.width = max_w + fs * 0.10;
			m.ascent = total_h * 0.5 + fs * 0.05;
			m.descent = total_h * 0.5 + fs * 0.05;
			break;
		}
		
		case NODE_MATRIX: {
			int rows = n->rows > 0 ? n->rows : 1;
			int cols = n->cols > 0 ? n->cols : 1;
			double cell_fs = fs * 0.72;
		double col_gap = cell_fs * 0.95;
		double row_gap = cell_fs * 0.72;
			
			size_t total = (size_t)rows * (size_t)cols;
			BoxMetrics *gm = (BoxMetrics *)xcalloc(total, sizeof(BoxMetrics));
			double *col_w = (double *)xcalloc((size_t)cols, sizeof(double));
			double *row_h = (double *)xcalloc((size_t)rows, sizeof(double));
			
			for (int r = 0; r < rows; r++) {
				for (int c = 0; c < cols; c++) {
					size_t idx = (size_t)r * (size_t)cols + (size_t)c;
					const AstNode *cell = idx < n->child_count ? n->children[idx] : NULL;
					gm[idx] = layout_or_draw_node_d(s, cell, 0, 0, cell_fs, 0, depth + 1);
					if (gm[idx].width > col_w[c]) col_w[c] = gm[idx].width;
					double ch = gm[idx].ascent + gm[idx].descent;
					if (ch > row_h[r]) row_h[r] = ch;
				}
			}
			
			double grid_w = 0.0;
			for (int c = 0; c < cols; c++) {
				grid_w += col_w[c];
				if (c > 0) grid_w += col_gap;
			}
			double grid_h = 0.0;
			for (int r = 0; r < rows; r++) {
				grid_h += row_h[r];
				if (r > 0) grid_h += row_gap;
			}
			
			const char *open_d = "(";
			const char *close_d = ")";
			if (n->text) {
				if (strcmp(n->text, "bmatrix") == 0) { open_d = "["; close_d = "]"; }
				else if (strcmp(n->text, "Bmatrix") == 0 || strcmp(n->text, "cases") == 0 ||
					strcmp(n->text, "dcases") == 0) {
					open_d = "{";
					close_d = (strcmp(n->text, "cases") == 0 || strcmp(n->text, "dcases") == 0) ? "." : "}";
				}
				else if (strcmp(n->text, "rcases") == 0) { open_d = "."; close_d = "}"; }
				else if (strcmp(n->text, "vmatrix") == 0) { open_d = "|"; close_d = "|"; }
				else if (strcmp(n->text, "Vmatrix") == 0) { open_d = "||"; close_d = "||"; }
				else if (strcmp(n->text, "matrix") == 0 || strcmp(n->text, "array") == 0 ||
						 strcmp(n->text, "aligned") == 0 || strcmp(n->text, "align") == 0 ||
						 strcmp(n->text, "align*") == 0 || strcmp(n->text, "gather") == 0 ||
						 strcmp(n->text, "gather*") == 0 || strcmp(n->text, "gathered") == 0 ||
						 strcmp(n->text, "alignedat") == 0 || strcmp(n->text, "alignat") == 0 || strcmp(n->text, "alignat*") == 0 ||
						 strcmp(n->text, "flalign") == 0 || strcmp(n->text, "flalign*") == 0 ||
						 strcmp(n->text, "split") == 0 || strcmp(n->text, "smallmatrix") == 0 ||
						 strcmp(n->text, "subarray") == 0 || strcmp(n->text, "equation") == 0 ||
						 strcmp(n->text, "equation*") == 0 || strcmp(n->text, "displaymath") == 0 ||
						 strcmp(n->text, "multline") == 0 || strcmp(n->text, "multline*") == 0 ||
						 strcmp(n->text, "tabular") == 0) {
					open_d = "."; close_d = ".";
				}
			}
			
			double paren_w = (strcmp(open_d, ".") == 0 && strcmp(close_d, ".") == 0) ? 0.0 : fs * 0.45;
			double top_y = y + grid_h * 0.5 + fs * 0.14;
			double bot_y = y - grid_h * 0.5 - fs * 0.14;
			
			if (draw) {
				draw_tall_paren(s, open_d, x + fs * 0.04, bot_y, top_y, fs * 0.06);
				double cur_y = y + grid_h * 0.5;
				for (int r = 0; r < rows; r++) {
					double cur_x = x + paren_w + fs * 0.10;
					double base_r = cur_y - row_h[r] * 0.5;
					for (int c = 0; c < cols; c++) {
						size_t idx = (size_t)r * (size_t)cols + (size_t)c;
						const AstNode *cell = idx < n->child_count ? n->children[idx] : NULL;
						char calign = matrix_column_alignment(n, c);
						double cx = cur_x;
						if (calign == 'r') cx += col_w[c] - gm[idx].width;
						else if (calign != 'l') cx += (col_w[c] - gm[idx].width) * 0.5;
						layout_or_draw_node_d(s, cell, cx, base_r, cell_fs, 1, depth + 1);
						cur_x += col_w[c] + col_gap;
					}
					cur_y -= (row_h[r] + row_gap);
				}
				draw_tall_paren(s, close_d, x + paren_w + grid_w + fs * 0.14, bot_y, top_y, fs * 0.06);
			}
			
			free(gm);
			free(col_w);
			free(row_h);
			
			m.width = paren_w * 2.0 + grid_w + fs * 0.26;
			m.ascent = top_y - y + fs * 0.06;
			m.descent = y - bot_y + fs * 0.06;
			break;
		}
		
	case NODE_FRAC:
		case NODE_BINOM: {
			const AstNode *num = n->child_count >= 1 ? n->children[0] : NULL;
			const AstNode *den = n->child_count >= 2 ? n->children[1] : NULL;
			double sub_fs = fs * 0.76;
			BoxMetrics nm = layout_or_draw_node_d(s, num, 0, 0, sub_fs, 0, depth + 1);
			BoxMetrics dm = layout_or_draw_node_d(s, den, 0, 0, sub_fs, 0, depth + 1);
			char left_delim_buf[16] = ".", right_delim_buf[16] = ".";
			int has_custom_delims = 0;
			if (n->type == NODE_FRAC && n->text) {
				const char *sep = strchr(n->text, '\x1f');
				if (sep) {
					size_t llen = (size_t)(sep - n->text), rlen = strlen(sep + 1);
					if (llen < sizeof(left_delim_buf)) { memcpy(left_delim_buf, n->text, llen); left_delim_buf[llen] = '\0'; }
					if (rlen < sizeof(right_delim_buf)) memcpy(right_delim_buf, sep + 1, rlen + 1);
					has_custom_delims = 1;
				}
			}
			double inner_w = (nm.width > dm.width ? nm.width : dm.width) + fs * 0.36;
			double left_w = (n->type == NODE_BINOM || (has_custom_delims && strcmp(left_delim_buf, ".") != 0)) ? fs * 0.44 : 0.0;
			double right_w = (n->type == NODE_BINOM || (has_custom_delims && strcmp(right_delim_buf, ".") != 0)) ? fs * 0.44 : 0.0;
			double fw = inner_w + left_w + right_w;
			
			double axis_y = y + fs * 0.28;
			double gap = fs * 0.18;
			double num_y = axis_y + gap + nm.descent;
			double den_y = axis_y - gap - dm.ascent;
			double top_y = num_y + nm.ascent + fs * 0.06;
			double bot_y = den_y - dm.descent - fs * 0.06;
			
			if (draw) {
				double num_x = x + left_w + (inner_w - nm.width) * 0.5;
				double den_x = x + left_w + (inner_w - dm.width) * 0.5;
				layout_or_draw_node_d(s, num, num_x, num_y, sub_fs, 1, depth + 1);
				layout_or_draw_node_d(s, den, den_x, den_y, sub_fs, 1, depth + 1);
				if (n->type == NODE_FRAC) {
					double rule = has_custom_delims ? n->rule_em * fs + n->rule_pt : fs * 0.06;
					if (rule > 0.0) pdf_printf(s, "q %.2f w %.2f %.2f m %.2f %.2f l S Q\n",
							   rule, x + left_w + fs * 0.06, axis_y, x + left_w + inner_w - fs * 0.06, axis_y);
					if (has_custom_delims) {
						if (strcmp(left_delim_buf, ".") != 0) draw_tall_paren(s, left_delim_buf, x + fs * 0.04, bot_y, top_y, fs * 0.06);
						if (strcmp(right_delim_buf, ".") != 0) draw_tall_paren(s, right_delim_buf, x + left_w + inner_w + fs * 0.04, bot_y, top_y, fs * 0.06);
					}
				} else {
					draw_tall_paren(s, "(", x + fs * 0.04, bot_y, top_y, fs * 0.06);
					draw_tall_paren(s, ")", x + left_w + inner_w + fs * 0.04, bot_y, top_y, fs * 0.06);
				}
			}
			m.width = fw + fs * 0.14;
			m.ascent = top_y - y;
			m.descent = y - bot_y;
			break;
		}
		
		case NODE_SQRT: {
			const AstNode *deg = (n->child_count >= 2) ? n->children[0] : NULL;
			const AstNode *rad = (n->child_count >= 2) ? n->children[1] : (n->child_count >= 1 ? n->children[0] : NULL);
			BoxMetrics rm = layout_or_draw_node_d(s, rad, 0, 0, fs, 0, depth + 1);
			BoxMetrics dm = {0, 0, 0};
			double deg_fs = fs * 0.58;
			if (deg) dm = layout_or_draw_node_d(s, deg, 0, 0, deg_fs, 0, depth + 1);
			
			double pad_y = fs * 0.16;
			double top_bar_y = y + rm.ascent + pad_y;
			double bot_y = y - rm.descent - fs * 0.10;
			double deg_w = deg ? (dm.width + fs * 0.10) : 0.0;
			double hook_w = fs * 0.46 + deg_w;
			double total_w = hook_w + rm.width + fs * 0.14;
			
			if (draw) {
				layout_or_draw_node_d(s, rad, x + hook_w + fs * 0.05, y, fs, 1, depth + 1);
				if (deg) {
					layout_or_draw_node_d(s, deg, x, y + rm.ascent * 0.35, deg_fs, 1, depth + 1);
				}
				double sx = x + deg_w;
				pdf_printf(s, "q %.2f w 1 J 1 j %.2f %.2f m %.2f %.2f l %.2f %.2f l %.2f %.2f l S Q\n",
						   fs * 0.06,
						   sx, y + rm.ascent * 0.35,
						   sx + fs * 0.16, bot_y,
						   sx + fs * 0.36, top_bar_y,
						   x + total_w, top_bar_y);
			}
			m.width = total_w + fs * 0.06;
			m.ascent = top_bar_y - y + fs * 0.08;
			m.descent = y - bot_y + fs * 0.08;
			break;
		}
		
		case NODE_SUP: {
			const AstNode *base = n->child_count >= 1 ? n->children[0] : NULL;
			const AstNode *sup = n->child_count >= 2 ? n->children[1] : NULL;
			BoxMetrics bm = layout_or_draw_node_d(s, base, x, y, fs, draw, depth + 1);
			double s_fs = fs * 0.66;
			if (uses_display_limits(base)) {
				BoxMetrics sm = layout_or_draw_node_d(s, sup, 0, 0, s_fs, 0, depth + 1);
				double sup_y = y + bm.ascent + fs * 0.08 + sm.descent;
				double width = fmax(bm.width, sm.width);
				if (draw) layout_or_draw_node_d(s, sup, x + (width - sm.width) * 0.5, sup_y, s_fs, 1, depth + 1);
				m.width = width;
				m.ascent = fmax(bm.ascent, sup_y - y + sm.ascent);
				m.descent = bm.descent;
				break;
			}
			double sup_y = y + bm.ascent * 0.55;
			BoxMetrics sm = layout_or_draw_node_d(s, sup, x + bm.width + fs * 0.06, sup_y, s_fs, draw, depth + 1);
			m.width = bm.width + sm.width + fs * 0.10;
			m.ascent = fmax(bm.ascent, (sup_y - y) + sm.ascent + fs * 0.04);
			m.descent = bm.descent;
			break;
		}
		
		case NODE_SUB: {
			const AstNode *base = n->child_count >= 1 ? n->children[0] : NULL;
			const AstNode *sub = n->child_count >= 2 ? n->children[1] : NULL;
			BoxMetrics bm = layout_or_draw_node_d(s, base, x, y, fs, draw, depth + 1);
			double s_fs = fs * 0.66;
			if (uses_display_limits(base) || accent_is(base, "underbrace")) {
				BoxMetrics sm = layout_or_draw_node_d(s, sub, 0, 0, s_fs, 0, depth + 1);
				double sub_y = y - bm.descent - fs * 0.08 - sm.ascent;
				double width = fmax(bm.width, sm.width);
				if (draw) layout_or_draw_node_d(s, sub, x + (width - sm.width) * 0.5, sub_y, s_fs, 1, depth + 1);
				m.width = width;
				m.ascent = bm.ascent;
				m.descent = fmax(bm.descent, y - (sub_y - sm.descent));
				break;
			}
			double sub_y = y - bm.descent * 0.65 - fs * 0.14;
			BoxMetrics sm = layout_or_draw_node_d(s, sub, x + bm.width + fs * 0.06, sub_y, s_fs, draw, depth + 1);
			m.width = bm.width + sm.width + fs * 0.10;
			m.ascent = bm.ascent;
			m.descent = fmax(bm.descent, y - (sub_y - sm.descent) + fs * 0.04);
			break;
		}
		
		case NODE_SUBSUP: {
			const AstNode *base = n->child_count >= 1 ? n->children[0] : NULL;
			const AstNode *sub = n->child_count >= 2 ? n->children[1] : NULL;
			const AstNode *sup = n->child_count >= 3 ? n->children[2] : NULL;
			BoxMetrics bm = layout_or_draw_node_d(s, base, x, y, fs, draw, depth + 1);
			double s_fs = fs * 0.64;
			if (uses_display_limits(base) || accent_is(base, "underbrace") || accent_is(base, "overbrace")) {
				BoxMetrics sub_m = layout_or_draw_node_d(s, sub, 0, 0, s_fs, 0, depth + 1);
				BoxMetrics sup_m = layout_or_draw_node_d(s, sup, 0, 0, s_fs, 0, depth + 1);
				double width = fmax(bm.width, fmax(sub_m.width, sup_m.width));
				double sub_y = y - bm.descent - fs * 0.08 - sub_m.ascent;
				double sup_y = y + bm.ascent + fs * 0.08 + sup_m.descent;
				if (draw) {
					layout_or_draw_node_d(s, sub, x + (width - sub_m.width) * 0.5, sub_y, s_fs, 1, depth + 1);
					layout_or_draw_node_d(s, sup, x + (width - sup_m.width) * 0.5, sup_y, s_fs, 1, depth + 1);
				}
				m.width = width;
				m.ascent = fmax(bm.ascent, sup_y - y + sup_m.ascent);
				m.descent = fmax(bm.descent, y - (sub_y - sub_m.descent));
				break;
			}
			double sx = x + bm.width + fs * 0.06;
			double sub_y = y - bm.descent * 0.65 - fs * 0.14;
			double sup_y = y + bm.ascent * 0.58;
			BoxMetrics sub_m = layout_or_draw_node_d(s, sub, sx, sub_y, s_fs, draw, depth + 1);
			BoxMetrics sup_m = layout_or_draw_node_d(s, sup, sx, sup_y, s_fs, draw, depth + 1);
			double max_sw = sub_m.width > sup_m.width ? sub_m.width : sup_m.width;
			m.width = bm.width + max_sw + fs * 0.10;
			m.ascent = fmax(bm.ascent, (sup_y - y) + sup_m.ascent + fs * 0.04);
			m.descent = fmax(bm.descent, y - (sub_y - sub_m.descent) + fs * 0.04);
			break;
		}
		
		case NODE_DELIM: {
			const AstNode *body = n->child_count >= 1 ? n->children[0] : NULL;
			char open_buf[8] = "(";
			char close_buf[8] = ")";
			if (n->text) {
				const char *sep = strchr(n->text, '\x1f');
				if (sep) {
					size_t ol = (size_t)(sep - n->text);
					if (ol > 0 && ol < sizeof(open_buf)) {
						memcpy(open_buf, n->text, ol);
						open_buf[ol] = '\0';
					}
					if (sep[1] != '\0' && strlen(sep + 1) < sizeof(close_buf)) {
						strcpy(close_buf, sep + 1);
					}
				}
			}
			double dw_open = (strcmp(open_buf, ".") == 0) ? 0.0 : fs * 0.46;
			double dw_close = (strcmp(close_buf, ".") == 0) ? 0.0 : fs * 0.46;
			double saved_line_width = g_formula_line_width;
			if (saved_line_width > 0.0) {
				double reserve = dw_open + dw_close + fs * 0.24;
				g_formula_line_width = fmax(fs * 2.0, saved_line_width - reserve);
			}
			BoxMetrics inner = layout_or_draw_node_d(s, body, 0, 0, fs, 0, depth + 1);
			double bot_y = y - inner.descent - fs * 0.10;
			double top_y = y + inner.ascent + fs * 0.10;
			if (draw) {
				draw_tall_paren(s, open_buf, x + fs * 0.04, bot_y, top_y, fs * 0.065);
				layout_or_draw_node_d(s, body, x + dw_open + fs * 0.06, y, fs, 1, depth + 1);
				draw_tall_paren(s, close_buf, x + dw_open + inner.width + fs * 0.10, bot_y, top_y, fs * 0.065);
			}
			g_formula_line_width = saved_line_width;
			m.width = inner.width + dw_open + dw_close + fs * 0.18;
			m.ascent = top_y - y + fs * 0.04;
			m.descent = y - bot_y + fs * 0.04;
			break;
		}
	}
	
	if (!draw) {
		AstNode *mut = (AstNode *)n;
		mut->cached_fs = fs;
		mut->cached_line_width = g_formula_line_width;
		mut->cached_m = m;
	}
	return m;
}

typedef struct {
	PdfStream *pages;
	int page_count;
	int page_cap;
} PdfDocument;

/* All geometry is in PDF points. bbox x/y use the selected origin; baseline_y_pt is always bottom-left PDF coordinates. */
typedef enum {
	LATEXF_ALIGN_LEFT = 0,
	LATEXF_ALIGN_CENTER = 1,
	LATEXF_ALIGN_RIGHT = 2
} LatexfAlign;

typedef enum {
	LATEXF_VALIGN_CENTER = 0,
	LATEXF_VALIGN_TOP = 1,
	LATEXF_VALIGN_BOTTOM = 2,
	LATEXF_VALIGN_BASELINE = 3
} LatexfVerticalAlign;

typedef enum {
	LATEXF_FIT_NONE = 0,
	LATEXF_FIT_SHRINK = 1,
	LATEXF_FIT_CONTAIN = 2
} LatexfFitMode;

typedef enum {
	LATEXF_ORIGIN_TOP_LEFT = 0,
	LATEXF_ORIGIN_BOTTOM_LEFT = 1
} LatexfOrigin;

typedef struct {
	double page_width_pt;
	double page_height_pt;
	int target_page_1based; /* 1-based destination page for the host PDF merge step. */
	double bbox_x_pt;
	double bbox_y_pt;
	double bbox_width_pt;
	double bbox_height_pt;
	double font_size_pt;
	double padding_pt;
	double baseline_y_pt; /* Absolute PDF point coordinate when vertical_align is BASELINE. */
	unsigned int color_rgb; /* 0xRRGGBB */
	LatexfAlign align;
	LatexfVerticalAlign vertical_align;
	LatexfFitMode fit;
	LatexfOrigin origin;
	int include_page_header;
} LatexfRenderOptions;

typedef struct {
	int overlay_page_count;
	int target_page_1based;
	double font_size_used_pt;
	double content_x_pt;
	double baseline_y_pt;
	double content_width_pt;
	double content_height_pt;
	int overflowed_bbox;
} LatexfRenderResult;

static LatexfRenderOptions latexf_render_options_default(void) {
	LatexfRenderOptions options;
	memset(&options, 0, sizeof(options));
	options.page_width_pt = 842.0;
	options.page_height_pt = 595.0;
	options.target_page_1based = 1;
	options.bbox_x_pt = 36.0;
	options.bbox_y_pt = 36.0;
	options.bbox_width_pt = 0.0;
	options.bbox_height_pt = 0.0;
	options.font_size_pt = 15.0;
	options.align = LATEXF_ALIGN_LEFT;
	options.vertical_align = LATEXF_VALIGN_CENTER;
	options.fit = LATEXF_FIT_SHRINK;
	options.origin = LATEXF_ORIGIN_TOP_LEFT;
	return options;
}

static PdfStream *doc_new_page(PdfDocument *doc, double page_h, double margin_x, int include_header) {
	if (doc->page_count == doc->page_cap) {
		int next = doc->page_cap == 0 ? 4 : doc->page_cap * 2;
		doc->pages = (PdfStream *)xrealloc(doc->pages, (size_t)next * sizeof(PdfStream));
		doc->page_cap = next;
	}
	PdfStream *s = &doc->pages[doc->page_count++];
	memset(s, 0, sizeof(PdfStream));
	char hdr[128];
	if (include_header) {
		snprintf(hdr, sizeof(hdr), "LaTeX Formula Rendered Output - Page %d", doc->page_count);
		emit_pdf_ascii_text(s, margin_x, page_h - 36.0, 11.0, 0, hdr);
	}
	return s;
}

static void render_formula_multipage(PdfDocument *doc, const AstNode *root,
									 double page_w, double page_h, double margin_x, double fs) {
	double top_start_y = page_h - 95.0;
	double avail_w = page_w - 2.0 * margin_x;
	if (avail_w < 80.0) avail_w = 80.0;
	double avail_h = top_start_y - 45.0;
	double effective_fs = fs;
	PdfStream *page = doc_new_page(doc, page_h, margin_x, 1);
	g_formula_line_width = avail_w;
	BoxMetrics metrics = layout_or_draw_node_d(page, root, 0, 0, effective_fs, 0, 0);
	if (metrics.ascent + metrics.descent > avail_h && metrics.ascent + metrics.descent > 1.0) {
		effective_fs *= avail_h / (metrics.ascent + metrics.descent);
		if (effective_fs < 3.0) effective_fs = 3.0;
		metrics = layout_or_draw_node_d(page, root, 0, 0, effective_fs, 0, 0);
	}
	layout_or_draw_node_d(page, root, margin_x, top_start_y, effective_fs, 1, 0);
	g_formula_line_width = 0.0;
}

static int render_formula_in_bbox(PdfDocument *doc, const AstNode *root,
								  const LatexfRenderOptions *options,
								  LatexfRenderResult *result) {
	double page_h = options->page_height_pt;
	double box_x = options->bbox_x_pt;
	double box_y = options->origin == LATEXF_ORIGIN_TOP_LEFT
		? page_h - options->bbox_y_pt - options->bbox_height_pt : options->bbox_y_pt;
	double box_w = options->bbox_width_pt;
	double box_h = options->bbox_height_pt;
	double pad = options->padding_pt;
	double inner_x = box_x + pad, inner_y = box_y + pad;
	double inner_w = box_w - pad * 2.0, inner_h = box_h - pad * 2.0;
	double fs = options->font_size_pt;
	double rr = (double)((options->color_rgb >> 16) & 0xff) / 255.0;
	double gg = (double)((options->color_rgb >> 8) & 0xff) / 255.0;
	double bb = (double)(options->color_rgb & 0xff) / 255.0;
	PdfStream *page;
	BoxMetrics metrics;
	if (!doc || !root || inner_w <= 0.0 || inner_h <= 0.0 || fs <= 0.0) return 0;
	page = doc_new_page(doc, page_h, 0.0, options->include_page_header);
	/* Keep the formula on one natural layout, then scale all text and vector paths
	   together in PDF space. This avoids fit drift from repeated font-size rounding. */
	g_formula_line_width = 1e30;
	metrics = layout_or_draw_node_d(page, root, 0.0, 0.0, fs, 0, 0);
	{
		double content_h = metrics.ascent + metrics.descent;
		/* Reserve a small glyph-ink margin in addition to caller padding. */
		double guard_x = fmin(fmax(0.75, fs * 0.10), inner_w * 0.08);
		double guard_y = fmin(fmax(0.75, fs * 0.14), inner_h * 0.08);
		double fit_x = inner_x + guard_x;
		double fit_y = inner_y + guard_y;
		double fit_w = inner_w - guard_x * 2.0;
		double fit_h = inner_h - guard_y * 2.0;
		double sx = metrics.width > 0.0 ? fit_w / metrics.width : 1.0;
		double sy = content_h > 0.0 ? fit_h / content_h : 1.0;
		double scale = fmin(sx, sy);
		double draw_x, baseline;
		if (!isfinite(scale) || scale <= 0.0) return 0;
		if (options->fit == LATEXF_FIT_NONE) scale = 1.0;
		else if (options->fit == LATEXF_FIT_SHRINK) scale = fmin(1.0, scale) * 0.985;
		else scale = fmin(4.0, scale) * 0.985;
		if (scale <= 0.0 || !isfinite(scale)) return 0;
		draw_x = fit_x;
		if (options->align == LATEXF_ALIGN_CENTER) draw_x += (fit_w - metrics.width * scale) * 0.5;
		else if (options->align == LATEXF_ALIGN_RIGHT) draw_x += fit_w - metrics.width * scale;
		if (options->vertical_align == LATEXF_VALIGN_TOP) baseline = fit_y + fit_h - metrics.ascent * scale;
		else if (options->vertical_align == LATEXF_VALIGN_BOTTOM) baseline = fit_y + metrics.descent * scale;
		else if (options->vertical_align == LATEXF_VALIGN_BASELINE) baseline = options->baseline_y_pt;
		else baseline = fit_y + (fit_h - content_h * scale) * 0.5 + metrics.descent * scale;
		pdf_printf(page, "q %.3f %.3f %.3f %.3f re W n\n", box_x, box_y, box_w, box_h);
		pdf_printf(page, "%.4f %.4f %.4f rg %.4f %.4f %.4f RG\n", rr, gg, bb, rr, gg, bb);
		pdf_printf(page, "q %.8f 0 0 %.8f %.4f %.4f cm\n", scale, scale, draw_x, baseline);
		layout_or_draw_node_d(page, root, 0.0, 0.0, fs, 1, 0);
		pdf_append(page, "Q\n");
		pdf_append(page, "Q\n");
		if (result) {
			result->overlay_page_count = 1;
			result->target_page_1based = options->target_page_1based;
			result->font_size_used_pt = fs * scale;
			result->content_x_pt = draw_x;
			result->baseline_y_pt = baseline;
			result->content_width_pt = metrics.width * scale;
			result->content_height_pt = content_h * scale;
			result->overflowed_bbox = (metrics.width * scale > fit_w + 0.1 || content_h * scale > fit_h + 0.1 ||
				baseline - metrics.descent * scale < fit_y - 0.1 || baseline + metrics.ascent * scale > fit_y + fit_h + 0.1);
		}
	}
	g_formula_line_width = 0.0;
	return 1;
}

static int write_pdf_file_ex(const char *filename, const AstNode *root, int *out_pages,
							 const LatexfRenderOptions *options, LatexfRenderResult *result) {
	FILE *fp = fopen(filename, "wb");
	if (!fp) return 0;
	
	double page_w = options && options->page_width_pt > 0.0 ? options->page_width_pt : 842.0;
	double page_h = options && options->page_height_pt > 0.0 ? options->page_height_pt : 595.0;
	const double margin_x = 36.0;
	double fs = options && options->font_size_pt > 0.0 ? options->font_size_pt : 15.0;
	
	PdfDocument doc = {0};
	if (options) {
		LatexfRenderOptions normalized = *options;
		normalized.page_width_pt = page_w;
		normalized.page_height_pt = page_h;
		normalized.font_size_pt = fs;
		if (result) memset(result, 0, sizeof(*result));
		if (!render_formula_in_bbox(&doc, root, &normalized, result)) {
			fclose(fp);
			return 0;
		}
	} else {
		render_formula_multipage(&doc, root, page_w, page_h, margin_x, fs);
	}
	
	int P = doc.page_count;
	if (out_pages) *out_pages = P;
	
	int total_objs = 4 + 2 * P;
	long *offsets = (long *)xcalloc((size_t)(total_objs + 1), sizeof(long));
	
	fprintf(fp, "%%PDF-1.4\n");
	
	offsets[1] = ftell(fp);
	fprintf(fp, "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n");
	
	offsets[2] = ftell(fp);
	fprintf(fp, "2 0 obj\n<< /Type /Pages /Kids [");
	for (int i = 0; i < P; i++) {
		fprintf(fp, "%d 0 R ", 5 + i);
	}
	fprintf(fp, "] /Count %d >>\nendobj\n", P);
	
	offsets[3] = ftell(fp);
	fprintf(fp, "3 0 obj\n<< /Type /Font /Subtype /Type1 /BaseFont /Times-Roman >>\nendobj\n");
	
	offsets[4] = ftell(fp);
	fprintf(fp, "4 0 obj\n<< /Type /Font /Subtype /Type1 /BaseFont /Times-Italic >>\nendobj\n");
	
	for (int i = 0; i < P; i++) {
		int page_obj = 5 + i;
		int stream_obj = 5 + P + i;
		offsets[page_obj] = ftell(fp);
		fprintf(fp, "%d 0 obj\n<< /Type /Page /Parent 2 0 R /MediaBox [0 0 %.3f %.3f] "
				"/Resources << /Font << /%s 3 0 R /%s 4 0 R >> >> /Contents %d 0 R >>\nendobj\n",
				page_obj, page_w, page_h, LATEXF_PDF_FONT_NORMAL_NAME,
				LATEXF_PDF_FONT_ITALIC_NAME, stream_obj);
	}
	
	for (int i = 0; i < P; i++) {
		int stream_obj = 5 + P + i;
		offsets[stream_obj] = ftell(fp);
		fprintf(fp, "%d 0 obj\n<< /Length %zu >>\nstream\n", stream_obj, doc.pages[i].len);
		if (doc.pages[i].buf && doc.pages[i].len > 0) {
			fwrite(doc.pages[i].buf, 1, doc.pages[i].len, fp);
		}
		fprintf(fp, "\nendstream\nendobj\n");
		free(doc.pages[i].buf);
	}
	
	long xref_pos = ftell(fp);
	fprintf(fp, "xref\n0 %d\n", total_objs + 1);
	fprintf(fp, "0000000000 65535 f \n");
	for (int i = 1; i <= total_objs; i++) {
		fprintf(fp, "%010ld 00000 n \n", offsets[i]);
	}
	
	fprintf(fp, "trailer\n<< /Size %d /Root 1 0 R >>\nstartxref\n%ld\n%%%%EOF\n", total_objs + 1, xref_pos);
	
	free(offsets);
	free(doc.pages);
	fclose(fp);
	return 1;
}

static int write_pdf_file(const char *filename, const AstNode *root, int *out_pages) {
	return write_pdf_file_ex(filename, root, out_pages, NULL, NULL);
}

/* ---------------- 6. INPUT NORMALIZER & MULTI-SOURCE LOADER ---------------- */

static void auto_unescape_latex(char *buf, size_t *io_len) {
	if (!buf || !io_len || *io_len < 4) return;
	size_t n = *io_len;
	int has_escaped_cmd = 0;
	int has_raw_cmd = 0;
	
	for (size_t i = 0; i + 3 < n; i++) {
		if (buf[i] == '\\') {
			if (buf[i + 1] == '\\') {
				if (isalpha((unsigned char)buf[i + 2]) && isalpha((unsigned char)buf[i + 3])) {
					has_escaped_cmd = 1;
				}
				i++; /* skip second backslash */
			} else if (isalpha((unsigned char)buf[i + 1]) && isalpha((unsigned char)buf[i + 2])) {
				has_raw_cmd = 1;
			}
		}
	}
	
	/* Only unescape if input is JSON/C-escaped (e.g. \\frac, \\mathcal) and not raw LaTeX */
	if (has_escaped_cmd && !has_raw_cmd) {
		size_t w = 0;
		for (size_t r = 0; r < n; r++) {
			if (buf[r] == '\\' && r + 1 < n && (buf[r + 1] == '\\' || buf[r + 1] == '"')) {
				buf[w++] = buf[r + 1];
				r++;
			} else {
				buf[w++] = buf[r];
			}
		}
		buf[w] = '\0';
		*io_len = w;
	}
}

static char *normalize_raw_buffer(unsigned char *raw, size_t n) {
	if (!raw || n == 0) {
		free(raw);
		return NULL;
	}
	
	/* Auto-decode UTF-16 LE / BE */
	if (n >= 2 && ((raw[0] == 0xFF && raw[1] == 0xFE) || (raw[0] == 0xFE && raw[1] == 0xFF))) {
		int is_le = (raw[0] == 0xFF);
		char *utf8 = (char *)xmalloc(n * 2 + 1);
		size_t out_i = 0;
		for (size_t i = 2; i + 1 < n; i += 2) {
			unsigned int cp = is_le ? (raw[i] | (raw[i + 1] << 8)) : ((raw[i] << 8) | raw[i + 1]);
			if (cp == 0) continue;
			if (cp < 0x80) {
				utf8[out_i++] = (char)cp;
			} else if (cp < 0x800) {
				utf8[out_i++] = (char)(0xC0 | (cp >> 6));
				utf8[out_i++] = (char)(0x80 | (cp & 0x3F));
			} else {
				utf8[out_i++] = (char)(0xE0 | (cp >> 12));
				utf8[out_i++] = (char)(0x80 | ((cp >> 6) & 0x3F));
				utf8[out_i++] = (char)(0x80 | (cp & 0x3F));
			}
		}
		utf8[out_i] = '\0';
		free(raw);
		raw = (unsigned char *)utf8;
		n = out_i;
	}
	
	char *buf = (char *)raw;
	size_t start = 0;
	if (n >= 3 && (unsigned char)buf[0] == 0xEF &&
		(unsigned char)buf[1] == 0xBB && (unsigned char)buf[2] == 0xBF) {
		start = 3;
	}
	
	while (start < n && isspace((unsigned char)buf[start])) start++;
	while (n > start && isspace((unsigned char)buf[n - 1])) buf[--n] = '\0';
	
	/* Strip surrounding quotes if pasted from a JSON/C string literal */
	if (n - start >= 2 && buf[start] == '"' && buf[n - 1] == '"') {
		start++;
		buf[--n] = '\0';
	}
	
	if (start > 0 && n > start) {
		memmove(buf, buf + start, n - start + 1);
		n -= start;
		start = 0;
	}
	
	auto_unescape_latex(buf, &n);
	
	/* Strip surrounding $...$, $$...$$, \[...\], or \(...\) */
	while (n - start >= 2) {
		if (buf[start] == '$' && buf[n - 1] == '$') {
			start++;
			buf[--n] = '\0';
		} else if (n - start >= 4 && buf[start] == '\\' &&
				   (buf[start + 1] == '[' || buf[start + 1] == '(') &&
				   buf[n - 2] == '\\' &&
				   (buf[n - 1] == ']' || buf[n - 1] == ')')) {
			start += 2;
			n -= 2;
			buf[n] = '\0';
		} else {
			break;
		}
		while (start < n && isspace((unsigned char)buf[start])) start++;
		while (n > start && isspace((unsigned char)buf[n - 1])) buf[--n] = '\0';
	}
	
	if (n <= start) {
		free(buf);
		return NULL;
	}
	if (start > 0) {
		memmove(buf, buf + start, n - start + 1);
	}
	return buf;
}

static char *load_formula_from_stream(FILE *f) {
	if (!f) return NULL;
	size_t cap = 4096;
	size_t len = 0;
	unsigned char *buf = (unsigned char *)xmalloc(cap + 2);
	
	while (len < MAX_INPUT_BYTES) {
		if (len + 1024 > cap) {
			cap *= 2;
			buf = (unsigned char *)xrealloc(buf, cap + 2);
		}
		size_t r = fread(buf + len, 1, 1024, f);
		len += r;
		if (r < 1024) break;
	}
	buf[len] = '\0';
	buf[len + 1] = '\0';
	return normalize_raw_buffer(buf, len);
}

static char *load_formula_from_file(const char *path) {
	FILE *f = fopen(path, "rb");
	if (!f) return NULL;
	char *res = load_formula_from_stream(f);
	fclose(f);
	return res;
}

#ifndef LATEX_STANDALONE
/* Render a formula into an owned PDF content-stream buffer. The caller must add
 * LATEXF_PDF_FONT_NORMAL_NAME and LATEXF_PDF_FONT_ITALIC_NAME to the page's /Font
 * resources. Coordinates and sizing follow LatexfRenderOptions. */
static int latexf_render_to_content(const char *formula,
									const LatexfRenderOptions *options,
									char **out_content, size_t *out_length,
									LatexfRenderResult *result) {
	unsigned char *copy;
	char *input;
	Parser parser;
	AstNode *root;
	LatexfRenderOptions normalized = options ? *options : latexf_render_options_default();
	PdfDocument doc = {0};
	size_t len;
	int ok;
	if (out_content) *out_content = NULL;
	if (out_length) *out_length = 0;
	if (!formula || !out_content || !out_length) return 0;
	if (!isfinite(normalized.page_width_pt) || !isfinite(normalized.page_height_pt) ||
		!isfinite(normalized.bbox_x_pt) || !isfinite(normalized.bbox_y_pt) ||
		!isfinite(normalized.bbox_width_pt) || !isfinite(normalized.bbox_height_pt) ||
		!isfinite(normalized.font_size_pt) || !isfinite(normalized.padding_pt) ||
		!isfinite(normalized.baseline_y_pt)) return 0;
	if (normalized.page_width_pt <= 0.0) normalized.page_width_pt = 842.0;
	if (normalized.page_height_pt <= 0.0) normalized.page_height_pt = 595.0;
	if (normalized.bbox_width_pt <= 0.0 && normalized.bbox_height_pt <= 0.0) {
		normalized.bbox_width_pt = normalized.page_width_pt - normalized.bbox_x_pt - 36.0;
		normalized.bbox_height_pt = normalized.page_height_pt - normalized.bbox_y_pt - 36.0;
	}
	if (normalized.bbox_width_pt <= 0.0 || normalized.bbox_height_pt <= 0.0 || normalized.padding_pt < 0.0) return 0;
	if (normalized.font_size_pt <= 0.0) normalized.font_size_pt = 15.0;
	if (normalized.target_page_1based < 1) normalized.target_page_1based = 1;
	if (normalized.align < LATEXF_ALIGN_LEFT || normalized.align > LATEXF_ALIGN_RIGHT) normalized.align = LATEXF_ALIGN_LEFT;
	if (normalized.vertical_align < LATEXF_VALIGN_CENTER || normalized.vertical_align > LATEXF_VALIGN_BASELINE) normalized.vertical_align = LATEXF_VALIGN_CENTER;
	if (normalized.fit < LATEXF_FIT_NONE || normalized.fit > LATEXF_FIT_CONTAIN) normalized.fit = LATEXF_FIT_SHRINK;
	if (normalized.origin != LATEXF_ORIGIN_TOP_LEFT && normalized.origin != LATEXF_ORIGIN_BOTTOM_LEFT) normalized.origin = LATEXF_ORIGIN_TOP_LEFT;
	len = strlen(formula);
	if (len == 0 || len > MAX_INPUT_BYTES) return 0;
	copy = (unsigned char *)xmalloc(len + 2);
	memcpy(copy, formula, len + 1);
	copy[len + 1] = '\0';
	input = normalize_raw_buffer(copy, len);
	if (!input) return 0;
	parser.src = input;
	parser.pos = 0;
	parser.len = strlen(input);
	parser.depth = 0;
	parser.steps = 0;
	root = parse_seq(&parser, 0, 0, 0);
	ok = render_formula_in_bbox(&doc, root, &normalized, result);
	if (ok && doc.page_count == 1 && doc.pages[0].buf) {
		*out_content = doc.pages[0].buf;
		*out_length = doc.pages[0].len;
		doc.pages[0].buf = NULL;
	}
	for (int i = 0; i < doc.page_count; ++i) free(doc.pages[i].buf);
	free(doc.pages);
	node_free(root);
	free(input);
	return ok && *out_content != NULL;
}

/* Render a one-page transparent vector overlay sized to the host page. The caller merges
 * this overlay onto options->target_page_1based in the host PDF. */
static int latexf_render_to_pdf(const char *formula, const char *output_path,
								const LatexfRenderOptions *options, LatexfRenderResult *result) {
	unsigned char *copy;
	char *input;
	Parser parser;
	AstNode *root;
	LatexfRenderOptions normalized = options ? *options : latexf_render_options_default();
	size_t len;
	int ok;
	if (!formula || !output_path) return 0;
	if (!isfinite(normalized.page_width_pt) || !isfinite(normalized.page_height_pt) ||
		!isfinite(normalized.bbox_x_pt) || !isfinite(normalized.bbox_y_pt) ||
		!isfinite(normalized.bbox_width_pt) || !isfinite(normalized.bbox_height_pt) ||
		!isfinite(normalized.font_size_pt) || !isfinite(normalized.padding_pt) ||
		!isfinite(normalized.baseline_y_pt)) return 0;
	if (normalized.page_width_pt <= 0.0) normalized.page_width_pt = 842.0;
	if (normalized.page_height_pt <= 0.0) normalized.page_height_pt = 595.0;
	if (normalized.bbox_width_pt <= 0.0 && normalized.bbox_height_pt <= 0.0) {
		normalized.bbox_width_pt = normalized.page_width_pt - normalized.bbox_x_pt - 36.0;
		normalized.bbox_height_pt = normalized.page_height_pt - normalized.bbox_y_pt - 36.0;
	}
	if (normalized.bbox_width_pt <= 0.0 || normalized.bbox_height_pt <= 0.0 || normalized.padding_pt < 0.0) return 0;
	if (normalized.font_size_pt <= 0.0) normalized.font_size_pt = 15.0;
	if (normalized.target_page_1based < 1) normalized.target_page_1based = 1;
	if (normalized.align < LATEXF_ALIGN_LEFT || normalized.align > LATEXF_ALIGN_RIGHT) normalized.align = LATEXF_ALIGN_LEFT;
	if (normalized.vertical_align < LATEXF_VALIGN_CENTER || normalized.vertical_align > LATEXF_VALIGN_BASELINE) normalized.vertical_align = LATEXF_VALIGN_CENTER;
	if (normalized.fit < LATEXF_FIT_NONE || normalized.fit > LATEXF_FIT_CONTAIN) normalized.fit = LATEXF_FIT_SHRINK;
	if (normalized.origin != LATEXF_ORIGIN_TOP_LEFT && normalized.origin != LATEXF_ORIGIN_BOTTOM_LEFT) normalized.origin = LATEXF_ORIGIN_TOP_LEFT;
	len = strlen(formula);
	if (len == 0 || len > MAX_INPUT_BYTES) return 0;
	copy = (unsigned char *)xmalloc(len + 2);
	memcpy(copy, formula, len + 1);
	copy[len + 1] = '\0';
	input = normalize_raw_buffer(copy, len);
	if (!input) return 0;
	parser.src = input;
	parser.pos = 0;
	parser.len = strlen(input);
	parser.depth = 0;
	parser.steps = 0;
	root = parse_seq(&parser, 0, 0, 0);
	ok = write_pdf_file_ex(output_path, root, NULL, &normalized, result);
	node_free(root);
	free(input);
	return ok;
}

/* Backwards-compatible full-page entry point. */
static int latex_render_to_pdf(const char *formula, const char *output_path, int *out_pages) {
	unsigned char *copy;
	char *input;
	Parser parser;
	AstNode *root;
	int ok;
	size_t len;
	if (!formula || !output_path) return 0;
	len = strlen(formula);
	if (len == 0 || len > MAX_INPUT_BYTES) return 0;
	copy = (unsigned char *)xmalloc(len + 2);
	memcpy(copy, formula, len + 1);
	copy[len + 1] = '\0';
	input = normalize_raw_buffer(copy, len);
	if (!input) return 0;
	parser.src = input;
	parser.pos = 0;
	parser.len = strlen(input);
	parser.depth = 0;
	parser.steps = 0;
	root = parse_seq(&parser, 0, 0, 0);
	ok = write_pdf_file(output_path, root, out_pages);
	node_free(root);
	free(input);
	return ok;
}
#endif

#ifdef LATEX_STANDALONE
int main(int argc, char **argv) {
#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8);
#endif
	
	const char *fallback =
	"\\mathcal{Z}(\\lambda,\\hbar)=\\lim_{\\substack{N\\to\\infty\\\\\\varepsilon\\to0^{+}}}\\left\\{\\frac{1}{\\Gamma\\left(\\frac{d}{2}\\right)}\\int_{\\mathbb{R}^{d}}\\oint_{\\partial\\Omega}\\frac{e^{-\\frac{\\|x\\|^{2}}{2\\sigma^{2}}}}{(2\\pi\\sigma^{2})^{d/2}}\\left[\\sum_{n=1}^{N}\\frac{(-1)^{n+1}}{n!}\\left(\\frac{\\partial^{2n}}{\\partial x^{n}\\partial y^{n}}\\Phi(x,y)\\right)^{2}\\right]\\,d\\omega\\,d^{d}x+\\det\\begin{pmatrix}\\frac{\\partial^{2}\\mathcal{L}}{\\partial q_1^{2}}+\\lambda&\\frac{\\partial^{2}\\mathcal{L}}{\\partial q_1\\partial q_2}&\\cdots\\\\\\frac{\\partial^{2}\\mathcal{L}}{\\partial q_2\\partial q_1}&\\frac{\\partial^{2}\\mathcal{L}}{\\partial q_2^{2}}+\\lambda&\\cdots\\\\\\vdots&\\vdots&\\ddots\\end{pmatrix}\\right\\}\\times\\exp\\left[-\\frac{1}{\\hbar}\\int_{0}^{T}\\left(\\frac12 g_{\\mu\\nu}(q)\\dot q^{\\mu}\\dot q^{\\nu}+\\sum_{k=1}^{\\infty}\\frac{\\zeta(2k)}{(2k)!}\\lambda^{2k}\\right)dt\\right]+\\sqrt{\\frac{\\prod_{j=1}^{\\infty}\\left(1+\\frac{\\lambda^{2}}{j^{2}}\\right)}{\\sum_{m=0}^{\\infty}\\frac{\\Gamma(m+\\frac12)}{m!\\Gamma(\\frac12)}\\varepsilon^{m}}}";
	
	char *dynamic_input = NULL;
	const char *source_label = "built-in sample";
	
	if (argc >= 2 && argv[1] && argv[1][0] != '\0') {
		/* Try treating argv[1] as a file path first; if not a file, treat it as raw LaTeX */
		dynamic_input = load_formula_from_file(argv[1]);
		if (dynamic_input) {
			source_label = argv[1];
		} else {
			size_t alen = strlen(argv[1]);
			unsigned char *copy = (unsigned char *)xmalloc(alen + 2);
			memcpy(copy, argv[1], alen + 1);
			copy[alen + 1] = '\0';
			dynamic_input = normalize_raw_buffer(copy, alen);
			if (dynamic_input) source_label = "CLI argument";
		}
	}
	
	if (!dynamic_input) {
		dynamic_input = load_formula_from_file("formula.tex");
		if (dynamic_input) source_label = "formula.tex";
	}
	
	if (!dynamic_input && !ISATTY(FILENO(stdin))) {
		dynamic_input = load_formula_from_stream(stdin);
		if (dynamic_input) source_label = "stdin";
	}
	
	const char *input_tex = dynamic_input ? dynamic_input : fallback;
	
	Parser p = { .src = input_tex, .pos = 0, .len = strlen(input_tex), .depth = 0, .steps = 0 };
	AstNode *root = parse_seq(&p, 0, 0, 0);
	
	printf("Source:    %s (%zu bytes, %d steps)\n", source_label, p.len, p.steps);
	printf("Assembled: ");
	print_assembled(root);
	printf("\n");
	
	int pages = 0;
	if (write_pdf_file("latex_output.pdf", root, &pages)) {
		printf("Saved PDF: latex_output.pdf (%d page%s)\n", pages, pages == 1 ? "" : "s");
	} else {
		fprintf(stderr, "Failed to write latex_output.pdf\n");
	}
	
	node_free(root);
	free(dynamic_input);
	return 0;
}
#endif /* LATEX_STANDALONE */

#endif /* LATEX_H_INCLUDED */
