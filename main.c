#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./vendor/stretchy_buffer.h"

typedef enum
{
	TOK_FUNCTION,
	TOK_LET,
	TOK_TYPE,
	TOK_RETURN,
	TOK_EQ,
	TOK_NUMBER,
	TOK_BOOL,
	TOK_IDENT,
	TOK_SEMICOLON,
	TOK_COLON,
	TOK_END_OF_FILE,
	TOK_UNKNOWN,
} TokenKind;

char *token_kind_name(TokenKind kind)
{
	switch (kind)
	{
	case TOK_FUNCTION:
		return "TOK_FUNCTION";
	case TOK_LET:
		return "TOK_LET";
	case TOK_TYPE:
		return "TOK_TYPE";
	case TOK_RETURN:
		return "TOK_RETURN";
	case TOK_EQ:
		return "TOK_EQ";
	case TOK_NUMBER:
		return "TOK_NUMBER";
	case TOK_BOOL:
		return "TOK_BOOL";
	case TOK_IDENT:
		return "TOK_IDENT";
	case TOK_SEMICOLON:
		return "TOK_SEMICOLON";
	case TOK_COLON:
		return "TOK_COLON";
	case TOK_END_OF_FILE:
		return "TOK_END_OF_FILE";
	case TOK_UNKNOWN:
		return "TOK_UNKNOWN";
	default:
		return "(unknown token kind)";
	}
}

typedef struct
{
	TokenKind kind;
	char *text;
} Token;

Token *token_create(TokenKind kind, char *text)
{
	Token *token = malloc(sizeof(Token));
	token->kind = kind;
	token->text = text;
	return token;
}

typedef struct
{
	Token *prev_token;
	Token *token;
	size_t pos;
	const char *source;
	size_t source_len;
} Lexer;

Lexer *lexer_create(const char *source)
{
	Lexer *lexer = malloc(sizeof(Lexer));
	lexer->token = NULL;
	lexer->prev_token = NULL;
	lexer->pos = 0;
	lexer->source = source;
	lexer->source_len = strlen(source);
	return lexer;
}

bool lexer_has_more_chars(Lexer *lexer)
{
	return lexer->pos < lexer->source_len;
}

char lexer_char(Lexer *lexer)
{
	return lexer->source[lexer->pos];
}

char *substr(const char *orig, size_t from, size_t to)
{
	size_t len = to - from;
	char *sub = calloc(len + 1, sizeof(char));
	strncpy(sub, orig + from, len);
	sub[len] = '\0';
	return sub;
}

bool is_digit(char c)
{
	return '0' <= c && c <= '9';
}

bool is_alpha(char c)
{
	return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z');
}

bool is_alphanumeric(char c)
{
	return is_alpha(c) || is_digit(c);
}

bool is_identifier_char(char c)
{
	return is_alphanumeric(c) || c == '_';
}

void lexer_set_token(Lexer *lexer, Token *token)
{
	if (lexer->prev_token != NULL)
	{
		free(lexer->prev_token);
	}
	lexer->prev_token = lexer->token;
	lexer->token = token;
}

#define STRNCMP(s1, s2) strncmp(s1, s2, sizeof(s2) - 1) == 0

void lexer_scan(Lexer *lexer)
{
	if (lexer->token != NULL && lexer->token->kind == TOK_END_OF_FILE)
	{
		return;
	}

	while (lexer_has_more_chars(lexer) && isspace(lexer_char(lexer)))
	{
		lexer->pos++;
	}

	size_t start = lexer->pos;
	if (!lexer_has_more_chars(lexer))
	{
		lexer_set_token(lexer, token_create(TOK_END_OF_FILE, "EOF"));
		return;
	}

	if (is_digit(lexer_char(lexer)))
	{
		while (lexer_has_more_chars(lexer) && is_digit(lexer_char(lexer)))
		{
			lexer->pos++;
		}

		char *text = substr(lexer->source, start, lexer->pos);
		lexer_set_token(lexer, token_create(TOK_NUMBER, text));
		return;
	}

	if (is_alpha(lexer_char(lexer)))
	{
		while (lexer_has_more_chars(lexer) && is_identifier_char(lexer_char(lexer)))
		{
			lexer->pos++;
		}

		char *text = substr(lexer->source, start, lexer->pos);
		TokenKind kind;
		if (STRNCMP(text, "function"))
		{
			kind = TOK_FUNCTION;
		}
		else if (STRNCMP(text, "let"))
		{
			kind = TOK_LET;
		}
		else if (STRNCMP(text, "kind"))
		{
			kind = TOK_TYPE;
		}
		else if (STRNCMP(text, "return"))
		{
			kind = TOK_RETURN;
		}
		else if ((STRNCMP(text, "true") || STRNCMP(text, "false")))
		{
			kind = TOK_BOOL;
		}
		else
		{
			kind = TOK_IDENT;
		}
		lexer_set_token(lexer, token_create(kind, text));
		return;
	}

	lexer->pos++;
	switch (lexer->source[lexer->pos - 1])
	{
	case '=':
		lexer_set_token(lexer, token_create(TOK_EQ, "="));
		break;
	case ';':
		lexer_set_token(lexer, token_create(TOK_SEMICOLON, ";"));
		break;
	case ':':
		lexer_set_token(lexer, token_create(TOK_COLON, ":"));
		break;
	default:
	{
		char *text = substr(lexer->source, start, lexer->pos);
		lexer_set_token(lexer, token_create(TOK_UNKNOWN, text));
		break;
	}
	}
}

typedef struct
{
	size_t pos;
} Location;

typedef enum
{
	EXPR_IDENT,
	EXPR_NUM,
	EXPR_BOOL,
	EXPR_ASSIGNMENT,
} ExprKind;

typedef struct
{
	const char *text;
} Ident;

typedef struct
{
	double value;
} Number;

typedef struct Expr_ Expr;

typedef struct
{
	Ident name;
	Expr *expr;
} Assignment;

struct Expr_
{
	ExprKind kind;
	Location location;

	union
	{
		// EXPR_IDENT
		Ident ident;
		// EXPR_NUM
		Number num;
		// EXPR_BOOL
		bool boolean;
		// EXPR_ASSIGNMENT
		Assignment assignment;
	};
};

Expr expr_ident_create(Location location, const char *text)
{
	Expr expr;
	expr.kind = EXPR_IDENT;
	expr.location = location;
	Ident ident = { .text = text };
	expr.ident = ident;
	return expr;
}

Expr expr_num_create(Location location, double value)
{
	Expr expr;
	expr.kind = EXPR_NUM;
	expr.location = location;
	Number num = { .value = value };
	expr.num = num;
	return expr;
}

Expr expr_assignment_create(Location location, Ident name, Expr *value)
{
	Assignment assignment = { .expr = value, .name = name };
	Expr expr = {
		.kind = EXPR_ASSIGNMENT,
		.location = location,
		.assignment = assignment,
	};
	return expr;
}

Expr expr_bool_create(Location location, bool value)
{
	Expr expr = {
		.kind = EXPR_BOOL,
		.location = location,
		.boolean = value,
	};
	return expr;
}

typedef struct Scope_ Scope;

typedef enum
{
	DECL_LET,
	DECL_TYPE_ALIAS,
} DeclKind;

typedef struct
{
	Ident name;
	Ident *type_name;
	Expr init;
} Let;

typedef struct
{
	Ident name;
	Ident type_name;
} TypeAlias;

typedef struct
{
	DeclKind kind;
	Location location;

	union
	{
		// DECL_LET
		Let let;
		// DECL_TYPE_ALIAS
		TypeAlias type_alias;
	};
} Decl;

Decl decl_let_create(Location location, Ident name, Ident *type_name, Expr init)
{
	Decl decl;
	decl.kind = DECL_LET;
	decl.location = location;

	Let let = { .name = name, .type_name = type_name, .init = init };
	decl.let = let;

	return decl;
}

Decl decl_type_alias_create(Location location, Ident name, Ident type_name)
{
	Decl decl;
	decl.kind = DECL_TYPE_ALIAS;
	decl.location = location;
	TypeAlias type_alias = { .name = name, .type_name = type_name };
	decl.type_alias = type_alias;
	return decl;
}

typedef enum
{
	STMT_EXPR,
	STMT_DECL,
} StmtKind;

typedef struct
{
	StmtKind kind;
	Location location;

	union
	{
		// STMT_EXPR
		Expr expr;
		// STMT_DECL
		Decl decl;
	};
} Stmt;

Stmt stmt_expr_create(Location location, Expr expr)
{
	Stmt stmt;
	stmt.kind = STMT_EXPR;
	stmt.location = location;
	stmt.expr = expr;
	return stmt;
}

Stmt stmt_decl_create(Location location, Decl decl)
{
	Stmt stmt;
	stmt.kind = STMT_DECL;
	stmt.location = location;
	stmt.decl = decl;
	return stmt;
}

#define UNREACHABLE(...)                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        fprintf(stderr, "unreachable: " __VA_ARGS__);                                                                  \
        exit(1);                                                                                                       \
    } while (0)

typedef struct
{
	bool in_use;
	const char *key;
	Decl val;
} HashmapEntry;

typedef struct
{
	int cap;
	int size;
	HashmapEntry *entries;
} Hashmap;

void hm_init(Hashmap *hm)
{
	hm->cap = 2;
	hm->size = 0;
	hm->entries = calloc(hm->cap, sizeof(HashmapEntry));
}

void hm_ensure(Hashmap *hm, int min_cap)
{
	if (hm->cap >= min_cap)
	{
		return;
	}

	int prev_cap = hm->cap;
	hm->cap = prev_cap * 2;
	hm->entries = realloc(hm->entries, sizeof(HashmapEntry) * hm->cap);
	for (int i = prev_cap; i < hm->cap; i++)
	{
		hm->entries[i] = (const HashmapEntry){ 0 };
	}
}

void hm_add(Hashmap *hm, const char *key, Decl val)
{
	hm_ensure(hm, ++hm->size);

	for (int i = 0; i < hm->cap; i++)
	{
		if (!hm->entries[i].in_use)
		{
			HashmapEntry entry = { .in_use = true, .key = key, .val = val };
			hm->entries[i] = entry;
			return;
		}

		if (strcmp(hm->entries[i].key, key) == 0)
		{
			hm->entries[i].val = val;
			return;
		}
	}

	UNREACHABLE("could not insert key '%s' into hashmap\n", key);
}

bool hm_get(Hashmap *hm, const char *key, Decl *result)
{
	for (int i = 0; i < hm->cap; i++)
	{
		HashmapEntry entry = hm->entries[i];
		if (!entry.in_use)
		{
			continue;
		}

		if (strcmp(entry.key, key) == 0)
		{
			*result = entry.val;
			return true;
		}
	}

	return false;
}

bool hm_has(Hashmap *hm, const char *key)
{
	Decl dummy;
	return hm_get(hm, key, &dummy);
}

struct Scope_
{
	Scope *parent;
	Hashmap bindings;
};

void scope_init(Scope *scope, Scope *parent)
{
	scope->parent = parent;
	hm_init(&scope->bindings);
}

bool scope_get_value(Scope *s, const char *name, Decl *decl)
{
	if (hm_get(&s->bindings, name, decl))
	{
		return true;
	}

	if (s->parent != NULL)
	{
		return scope_get_value(s->parent, name, decl);
	}

	return false;
}

void scope_declare(Scope *s, const char *name, Decl decl)
{
	hm_add(&s->bindings, name, decl);
}

bool scope_is_declared(Scope *s, const char *name)
{
	Decl dummy;
	return scope_get_value(s, name, &dummy);
}

typedef struct
{
	Stmt *statements;
} Module;

typedef struct
{
	Lexer *lexer;
	Scope *scope;
	bool has_errors;
} Parser;

typedef enum
{
	PARSE_RESULT_OK,
	PARSE_RESULT_UNEXPECTED_TOK,
	PARSE_RESULT_INVALID_NUMERIC_LITERAL,
	PARSE_RESULT_CANNOT_REDECLARE,
	PARSE_RESULT_UNDECLARED,
} ParseResult;

char *parse_result_name(ParseResult res)
{
	switch (res)
	{
	case PARSE_RESULT_OK:
		return "PARSE_RESULT_OK";
	case PARSE_RESULT_UNEXPECTED_TOK:
		return "PARSE_RESULT_UNEXPECTED_TOK";
	case PARSE_RESULT_INVALID_NUMERIC_LITERAL:
		return "PARSE_RESULT_INVALID_NUMERIC_LITERAL";
	case PARSE_RESULT_CANNOT_REDECLARE:
		return "PARSE_RESULT_CANNOT_REDECLARE";
	case PARSE_RESULT_UNDECLARED:
		return "PARSE_RESULT_UNDECLARED";
	default:
		return "(unknown)";
	}
}

#define TRY_PARSE(__expr) \
    do { \
        ParseResult __res = __expr; \
        if (__res != PARSE_RESULT_OK) return __res; \
    } while (0)

Parser *parser_create(Lexer *lexer)
{
	Parser *parser = malloc(sizeof(Parser));
	parser->lexer = lexer;
	parser->has_errors = false;

	parser->scope = malloc(sizeof(Scope));
	scope_init(parser->scope, NULL);

	return parser;
}

void parser_print_error_context(Parser *parser)
{
	size_t pos = parser->lexer->pos;

	// finding the line that the error occurred on
	size_t line_start = pos;
	size_t line_end = pos;
	// first, find the start of the line, then ...
	while (parser->lexer->source[line_start - 1] != '\n' && line_start > 0)
	{
		line_start--;
	}
	// ... find the end of the line
	while (parser->lexer->source[line_end] != '\n' && line_end < parser->lexer->source_len)
	{
		line_end++;
	}

	char *current_line = substr(parser->lexer->source, line_start, line_end + 1);
	fprintf(stderr, "%s", current_line);

	size_t padding_size = pos - line_start - 1;
	char *padding = calloc(padding_size, sizeof(char));
	for (size_t i = 0; i < padding_size; i++)
	{
		padding[i] = ' ';
	}

	fprintf(stderr, "%s^ ", padding);
}

#define PARSER_ERROR(...) \
    do {                  \
        if (parser->has_errors) break; \
        parser->has_errors = true; \
        parser_print_error_context(parser); \
        fprintf(stderr, __VA_ARGS__); \
    } while (0)

bool parser_try_parse_token(Parser *parser, TokenKind kind)
{
	bool ok = parser->lexer->token != NULL && parser->lexer->token->kind == kind;
	if (ok)
	{
		lexer_scan(parser->lexer);
	}
	return ok;
}

ParseResult parser_expect_token(Parser *parser, TokenKind kind)
{
	bool ok = parser_try_parse_token(parser, kind);
	if (!ok)
	{
		PARSER_ERROR("expected a token of kind %s, got %s\n",
			token_kind_name(kind),
			token_kind_name(parser->lexer->token->kind));
		return PARSE_RESULT_UNEXPECTED_TOK;
	}
	return PARSE_RESULT_OK;
}

ParseResult parse_identifier_or_literal(Parser *parser, Expr *expr)
{
	size_t pos = parser->lexer->pos;
	Location location = { .pos = pos };
	if (parser_try_parse_token(parser, TOK_IDENT))
	{
		*expr = expr_ident_create(location, parser->lexer->prev_token->text);
		return PARSE_RESULT_OK;
	}

	if (parser_try_parse_token(parser, TOK_NUMBER))
	{
		char *end_ptr;
		double value = strtod(parser->lexer->token->text, &end_ptr);
		if (errno == ERANGE)
		{
			PARSER_ERROR("could not parse as double: %s\n", parser->lexer->token->text);
			return PARSE_RESULT_INVALID_NUMERIC_LITERAL;
		}
		*expr = expr_num_create(location, value);
		return PARSE_RESULT_OK;
	}

	if (parser_try_parse_token(parser, TOK_BOOL))
	{
		bool value = STRNCMP(parser->lexer->prev_token->text, "true");
		*expr = expr_bool_create(location, value);
		return PARSE_RESULT_OK;
	}

	PARSER_ERROR("expected identifier or a literal but got %s\n", token_kind_name(parser->lexer->token->kind));
	return PARSE_RESULT_UNEXPECTED_TOK;
}

ParseResult parse_expression(Parser *parser, Expr *expr)
{
	size_t pos = parser->lexer->pos;
	Location location = { .pos = pos };

	TRY_PARSE(parse_identifier_or_literal(parser, expr));

	if (expr->kind == EXPR_IDENT && !scope_is_declared(parser->scope, expr->ident.text))
	{
		PARSER_ERROR("cannot reference '%s' before declaration\n", expr->ident.text);
		return PARSE_RESULT_UNDECLARED;
	}

	if (expr->kind == EXPR_IDENT && parser_try_parse_token(parser, TOK_EQ))
	{
		Expr *value = malloc(sizeof(Expr));
		TRY_PARSE(parse_expression(parser, value));
		*expr = expr_assignment_create(location, expr->ident, value);
	}

	return PARSE_RESULT_OK;
}

ParseResult parse_identifier(Parser *parser, Ident *ident)
{
	Expr expr;
	TRY_PARSE(parse_identifier_or_literal(parser, &expr));
	if (expr.kind == EXPR_IDENT)
	{
		*ident = expr.ident;
		return PARSE_RESULT_OK;
	}

	PARSER_ERROR("expected identifier but got a literal?!\n");
	return PARSE_RESULT_UNEXPECTED_TOK;
}

ParseResult parse_stmt(Parser *parser, Stmt *stmt)
{
	size_t pos = parser->lexer->pos;
	Location location = { .pos = pos };

	if (parser_try_parse_token(parser, TOK_LET))
	{
		// let $name: $type_name = $expr;
		Ident name;
		TRY_PARSE(parse_identifier(parser, &name));

		if (scope_is_declared(parser->scope, name.text))
		{
			PARSER_ERROR("cannot redeclare symbol '%s'\n", name.text);
			return PARSE_RESULT_CANNOT_REDECLARE;
		}

		Ident *type_name = NULL;
		if (parser_try_parse_token(parser, TOK_COLON))
		{
			type_name = malloc(sizeof(Ident));
			TRY_PARSE(parse_identifier(parser, type_name));

			if (!scope_is_declared(parser->scope, type_name->text))
			{
				PARSER_ERROR("cannot reference type '%s' before declaration\n", type_name->text);
				return PARSE_RESULT_UNDECLARED;
			}
		}

		TRY_PARSE(parser_expect_token(parser, TOK_EQ));

		Expr init;
		TRY_PARSE(parse_expression(parser, &init));
		Decl decl = decl_let_create(location, name, type_name, init);
		*stmt = stmt_decl_create(location, decl);

		scope_declare(parser->scope, name.text, decl);
	}
	else if (parser_try_parse_token(parser, TOK_TYPE))
	{
		// type $name = $type_name;
		Ident name;
		TRY_PARSE(parse_identifier(parser, &name));

		if (scope_is_declared(parser->scope, name.text))
		{
			PARSER_ERROR("cannot redeclare symbol '%s'\n", name.text);
			return PARSE_RESULT_CANNOT_REDECLARE;
		}

		TRY_PARSE(parser_expect_token(parser, TOK_EQ));

		Ident type_name;
		TRY_PARSE(parse_identifier(parser, &type_name));

		Decl decl = decl_type_alias_create(location, name, type_name);
		*stmt = stmt_decl_create(location, decl);

		scope_declare(parser->scope, name.text, decl);
	}
	else
	{
		// $expr;
		Expr expr;
		TRY_PARSE(parse_expression(parser, &expr));
		*stmt = stmt_expr_create(location, expr);
	}

	TRY_PARSE(parser_expect_token(parser, TOK_SEMICOLON));
	return PARSE_RESULT_OK;
}

void parser_synchronize(Parser *parser)
{
	lexer_scan(parser->lexer);

	while (!lexer_has_more_chars(parser->lexer))
	{
		Token *prev_token = parser->lexer->prev_token;
		if (prev_token != NULL && prev_token->kind == TOK_SEMICOLON)
		{
			return;
		}

		switch (parser->lexer->token->kind)
		{
		case TOK_LET:
		case TOK_FUNCTION:
		case TOK_TYPE:
		case TOK_RETURN:
			return;
		default:
			break;
		}

		lexer_scan(parser->lexer);
	}
}

ParseResult parser_parse_module(Parser *parser, Module *mod)
{
	lexer_scan(parser->lexer);
	if (parser_try_parse_token(parser, TOK_END_OF_FILE))
	{
		return PARSE_RESULT_OK;
	}

	// module-level scope
	Scope scope;
	scope_init(&scope, NULL);

	ParseResult res;
	while (true)
	{
		Stmt stmt;
		res = parse_stmt(parser, &stmt);
		if (res != PARSE_RESULT_OK)
		{
			parser_synchronize(parser);
			parser->has_errors = false;
		}
		sbpush(mod->statements, stmt);

		if (parser_try_parse_token(parser, TOK_END_OF_FILE))
		{
			break;
		}
	}

	return res;
}

ParseResult parser_parse(Parser *parser, Module *module)
{
	return parser_parse_module(parser, module);
}

int main(int argc, char **argv)
{
	const char *source;
	if (argc > 1)
	{
		source = argv[1];
	}
	else
	{
		source = "let a = 1;\n"
			 "let c = 2;\n"
			 "type t = number;\n"
			 "let b: t = c = false;\n";
	}

	Parser *parser = parser_create(lexer_create(source));

	Module mod = { .statements = NULL };
	ParseResult res = parser_parse(parser, &mod);

	if (res != PARSE_RESULT_OK)
	{
		printf("failed to parse: %s\n", parse_result_name(res));
		return 1;
	}

	return 0;
}
