#include "hashmap.h"
#include <assert.h>
#include <corecrt.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define int long long

#define Extend(type, size, max, ptr)                                           \
  if (size == max) {                                                           \
    max *= 2;                                                                  \
    ptr = (type *)realloc(ptr, sizeof(type) * max);                            \
  }

// Extendable sequence

#define NewSeq(type, name)                                                     \
  size_t name##_max = 8, name##_size = 0;                                      \
  type *name##_val = (type *)malloc(sizeof(type) * name##_max)

#define FreeSeq(name) free(name##_val)

#define SeqNth(name, n) (name##_val[n])

#define SeqAppend(type, name, val)                                             \
  Extend(type, name##_size, name##_max, name##_val);                           \
  name##_val[name##_size++] = val

#define SeqSize(name) (name##_size)

const size_t CHAR_SIZE = sizeof(char);
const size_t HASH_MOD = 131, HASH_MAX = 1024;

size_t str_hash(char *str) {
  assert(str);
  size_t hash = 0;
  for (char *i = str; *i; i++) {
    hash *= HASH_MOD;
    hash += *i;
  }
  return hash % HASH_MAX;
}

char *str_clone(char *str) {
  char *new = (char *)malloc((strlen(str) + 1) * CHAR_SIZE);
  strcpy(new, str);
  return new;
}

bool contains(char *str, char ch) {
  for (char *i = str; *i; i++) {
    if (*i == ch)
      return true;
  }
  return false;
}

// RunSignal

typedef enum {
  Return,
  Break,
  Continue,
  NoSignal,
} Signal;

typedef struct {
  Signal signal;
  int ret_val;
} RunSignal;

// Lines

typedef struct Lines {
  char **lines;
  size_t *pairs;
  size_t line_cnt;
} Lines;

// Func

RunSignal run_block(Lines lines, size_t start, size_t end);

typedef struct Func {
  Lines lines;
  size_t start_line, end_line;
  char **params;
  size_t params_cnt;
} Func;

void Func_free(Func func) {
  for (size_t i = 0; i < func.params_cnt; i++) {
    free(func.params[i]);
  }
  free(func.params);
}

// HashMap

void int_free(int i) {}

// 只有全局作用域
// VimScript：你叫我？
NewHashMapType(int, HashMap, int_free);
NewHashMapType(Func, FuncMap, Func_free);

// Eval expr

HashMap global_scope;
FuncMap global_func_scope;

int eval_expr(char *expr);
void _skip(char **expr);
void _eat(char **expr, char *pat);
char *_get_id(char **expr);
bool _match(char *expr, char *pat);
int _eval_boolean(char **expr);
int _eval_compare(char **expr);
int _eval_addsub(char **expr);
int _eval_term(char **expr);
int _eval_factor(char **expr);

void _skip(char **expr) {
  while (contains(" \n\t", **expr)) {
    (*expr)++;
  }
}

void _eat(char **expr, char *pat) {
  _skip(expr);
  if (!_match(*expr, pat)) {
    printf("Expected '%s'.", pat);
    exit(-1);
  }
  (*expr) += strlen(pat);
}

char *_get_id(char **expr) {
  if (!isalpha(**expr) && **expr != '_') {
    printf("Expected a identifier");
    exit(-1);
  }
  NewSeq(char, id);
  while (**expr && (isalnum(**expr) || **expr == '_')) {
    SeqAppend(char, id, **expr);
    (*expr)++;
  }
  id_val[id_size] = '\0';
  return id_val;
}

bool _match(char *expr, char *pat) {
  for (size_t i = 0; pat[i]; i++) {
    if (!expr[i]) {
      return false;
    }
    if (pat[i] != expr[i]) {
      return false;
    }
  }
  return true;
}

int eval_expr(char *expr) {
  char *cpy = expr;
  return _eval_boolean(&cpy);
}

int _eval_boolean(char **expr) {
  int res = _eval_compare(expr);
  _skip(expr);
  while (contains("&|", **expr)) {
    if (**expr == '&') {
      (*expr)++;
      res &= _eval_compare(expr);
    } else {
      (*expr)++;
      res |= _eval_compare(expr);
    }
    _skip(expr);
  }
  return res;
}

int _eval_compare(char **expr) {
  int res = _eval_addsub(expr);
  _skip(expr);
  while (_match(*expr, "==") || _match(*expr, "!=") || _match(*expr, ">=") ||
         _match(*expr, "<=") || contains("><", **expr)) {
    if (_match(*expr, "==")) {
      (*expr) += 2;
      res = res == _eval_addsub(expr);
    } else if (_match(*expr, "!=")) {
      (*expr) += 2;
      res = res != _eval_addsub(expr);
    } else if (_match(*expr, ">=")) {
      (*expr) += 2;
      res = res >= _eval_addsub(expr);
    } else if (_match(*expr, "<=")) {
      (*expr) += 2;
      res = res <= _eval_addsub(expr);
    } else if (**expr == '>') {
      (*expr)++;
      res = res > _eval_addsub(expr);
    } else {
      (*expr)++;
      res = res < _eval_addsub(expr);
    }
    _skip(expr);
  }
  return res;
}

int _eval_addsub(char **expr) {
  int res = _eval_term(expr);
  _skip(expr);
  while (contains("+-", **expr)) {
    // printf("addsub %lld\n", res);
    if (**expr == '+') {
      (*expr)++;
      res += _eval_term(expr);
    } else {
      (*expr)++;
      res -= _eval_term(expr);
    }
    _skip(expr);
  }
  return res;
}

int _eval_term(char **expr) {
  int res = _eval_factor(expr);
  _skip(expr);
  while (contains("*/%", **expr)) {
    // printf("term %lld\n", res);
    if (**expr == '*') {
      (*expr)++;
      res *= _eval_factor(expr);
    } else if (**expr == '/') {
      (*expr)++;
      res /= _eval_factor(expr);
    } else {
      (*expr)++;
      res %= _eval_factor(expr);
    }
    _skip(expr);
  }
  return res;
}

int _eval_factor(char **expr) {
  _skip(expr);
  if (isdigit(**expr)) {
    int res = 0;
    while (**expr && isdigit(**expr)) {
      res *= 10;
      res += *(*expr)++ - '0';
    }
    return res;
  } else if (isalpha(**expr) || **expr == '_') {
    char *id = _get_id(expr);
    _skip(expr);
    int res;
    if (**expr == '(') { // Func call
      (*expr)++;
      _skip(expr);
      NewSeq(int, args);
      if (**expr != ')') {
        SeqAppend(int, args, _eval_boolean(expr));
        _skip(expr);
        while (**expr == ',') {
          (*expr)++;
          SeqAppend(int, args, _eval_boolean(expr));
          _skip(expr);
        }
      }
      _eat(expr, ")");
      Func *func = FuncMap_find(&global_func_scope, id);
      assert(func);
      if (args_size != func->params_cnt) {
        printf("Wrong function call.");
        exit(-1);
      }
      for (size_t i = 0; i < func->params_cnt; i++) {
        HashMap_insert(&global_scope, func->params[i], args_val[i]);
      }
      RunSignal ret = run_block(func->lines, func->start_line, func->end_line);
      if (ret.signal == Return)
        return ret.ret_val;
      return 0;
    } else { // Variable
      // printf("'%s'\n", id);
      int *resp = HashMap_find(&global_scope, id);
      assert(resp);
      res = *resp;
    }
    free(id);
    return res;
  } else if (**expr == '(') {
    (*expr)++;
    int res = _eval_boolean(expr);
    _eat(expr, ")");
    return res;
  } else {
    printf("Unexpected character %c.\n", **expr);
    exit(-1);
  }
}

// Parse

// 不得直接传递字符串字面量给code，code的生命周期必须长于或等于res
Lines split_lines(char *code) {
  NewSeq(char *, res);
  char *last_line = code;
  for (char *i = code; *i; i++) {
    if (*i == '\n' || *i == '\r' || *i == '\0') {
      if (*last_line && *last_line != '\n' && *last_line != '\r') {
        SeqAppend(char *, res, last_line);
      }
      *i++ = '\0';
      while (*i && (*i == '\n' || *i == '\r'))
        i++;
      last_line = i;
    }
  }
  if (*last_line && *last_line != '\n' && *last_line != '\r') {
    SeqAppend(char *, res, last_line);
  }
  // pairs在parse中填充
  return (Lines){res_val, NULL, res_size};
}

Lines parse_code(Lines lines) {
  for (size_t i = 0; i < lines.line_cnt; i++) {
    // 去除前部空格
    while (contains(" \n\t", *lines.lines[i])) {
      lines.lines[i]++;
    }
    // 去除后部空格和注释
    size_t j = 0;
    while (lines.lines[i][j] && lines.lines[i][j] != '#') {
      j++;
    }
    while (j > 0 && contains(" \n\t#", lines.lines[i][j - 1]))
      j--;
    lines.lines[i][j] = '\0';
  }
  // 填充pairs
  size_t *pairs = (size_t *)malloc(sizeof(size_t) * lines.line_cnt);
  NewSeq(size_t, stack);
  for (size_t i = 0; i < lines.line_cnt; i++) {
    if (_match(lines.lines[i], "while ") || _match(lines.lines[i], "if ") ||
        _match(lines.lines[i], "func ")) {
      SeqAppend(size_t, stack, i);
    } else if (_match(lines.lines[i], "end")) {
      pairs[i] = stack_val[stack_size - 1];
      pairs[stack_val[--stack_size]] = i;
    }
  }
  lines.pairs = pairs;
  return lines;
}

// Run code

RunSignal run_statement(Lines lines, size_t *linenum);

RunSignal run_block(Lines lines, size_t start, size_t end) {
  for (size_t i = start + 1; i < end; i++) {
    RunSignal ret = run_statement(lines, &i);
    if (ret.signal != NoSignal)
      return ret;
  }
  return (RunSignal){NoSignal};
}

RunSignal run_statement(Lines lines, size_t *linenum) {
  size_t i = *linenum;
  if (_match(lines.lines[i], "while ")) {
    char *cond = lines.lines[i] + 6;
    while (eval_expr(cond)) {
      RunSignal ret = run_block(lines, i, lines.pairs[i]);
      if (ret.signal == Return) {
        *linenum = lines.pairs[i];
        return ret;
      }
      if (ret.signal == Break)
        break;
    }
    *linenum = lines.pairs[i];
  } else if (_match(lines.lines[i], "if ")) {
    char *cond = lines.lines[i] + 3;
    RunSignal ret = {NoSignal};
    if (eval_expr(cond)) {
      ret = run_block(lines, i, lines.pairs[i]);
    }
    *linenum = lines.pairs[i];
    return ret;
  } else if (_match(lines.lines[i], "return ")) {
    char *res = lines.lines[i] + 6;
    return (RunSignal){Return, eval_expr(res)};
  } else if (_match(lines.lines[i], "break")) {
    return (RunSignal){Break};
  } else if (_match(lines.lines[i], "continue")) {
    return (RunSignal){Continue};
  } else if (_match(lines.lines[i], "print ")) {
    char *expr = lines.lines[i] + 6;
    printf("%lld\n", eval_expr(expr));
  } else if (_match(lines.lines[i], "let ")) {
    char *p = lines.lines[i] + 4;
    char *id = _get_id(&p);
    _eat(&p, "=");
    _skip(&p);
    // printf("'%s' '%s'\n", id, p);
    HashMap_insert(&global_scope, id, eval_expr(p));
    free(id);
  } else if (_match(lines.lines[i], "func ")) {
    char *p = lines.lines[i] + 5;
    _skip(&p);
    char *name = _get_id(&p);
    _eat(&p, "(");
    _skip(&p);
    NewSeq(char *, params);
    if (*p != ')') {
      SeqAppend(char *, params, _get_id(&p));
      _skip(&p);
      while (*p == ',') {
        p++;
        _skip(&p);
        SeqAppend(char *, params, _get_id(&p));
        _skip(&p);
      }
    }
    _eat(&p, ")");
    FuncMap_insert(&global_func_scope, name,
                   (Func){lines, i, lines.pairs[i], params_val, params_size});
    free(name);
    *linenum = lines.pairs[i];
  } else if (strlen(lines.lines[i]) == 0)
    ;
  else {
    printf("Unknown statement '%s' at line %lld.", lines.lines[i], i + 1);
    exit(-1);
  }
  return (RunSignal){NoSignal};
}

void run_program(Lines lines) {
  for (size_t i = 0; i < lines.line_cnt; i++) {
    run_statement(lines, &i);
  }
}

// Init

void init() {
  global_scope = HashMap_new();
  global_func_scope = FuncMap_new();
}

// Tests

void test_expr() {
  char *expr = "12 + 24 * 36";
  printf("%lld", eval_expr(expr));
}

void test_seq() {
  NewSeq(int, a);
  SeqAppend(int, a, 1);
  SeqAppend(int, a, 2);
  SeqAppend(int, a, 3);
  SeqAppend(int, a, 4);
  SeqAppend(int, a, 5);
  for (size_t i = 0; i < SeqSize(a); i++)
    printf("%lld ", SeqNth(a, i));
  puts("");
  SeqAppend(int, a, 6);
  SeqAppend(int, a, 7);
  SeqAppend(int, a, 8);
  SeqAppend(int, a, 9);
  SeqAppend(int, a, 10);
  for (size_t i = 0; i < SeqSize(a); i++)
    printf("%lld ", SeqNth(a, i));
  puts("");
  FreeSeq(a);
}

void test_split_lines() {
  char *code = str_clone("while 1 # While Loop\n"
                         "  puts Hello, world!\n"
                         "end");
  Lines lines = split_lines(code);
  for (size_t i = 0; i < lines.line_cnt; i++)
    puts(lines.lines[i]);
  free(code);
}

void test(char *filename) {
  FILE *f = fopen(filename, "r");
  char *code = calloc(1024, CHAR_SIZE);
  char ch = fgetc(f);
  for (size_t i = 0; ch != -1; i++) {
    code[i] = ch;
    ch = fgetc(f);
  }
  fclose(f);
  // puts(code);
  Lines lines = split_lines(code);
  lines = parse_code(lines);
  run_program(lines);
}

#undef int
int main(int argc, char **argv) {
#define int long long
  init();
  // test("test5.z0");
  HashMap_free(global_scope);
  FuncMap_free(global_func_scope);
  return 0;
}
// 528行，致敬rswier/c4
