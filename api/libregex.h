#pragma once

typedef enum ObjType {
  OBJ_CHAR,
  OBJ_DOT,
  OBJ_CLASS,

  // entire regexes can be used as objects. think about this as:
  // (a+b*){2-3}
  // where the {2-3} modifier has been applied to the (a+b*) object.
  OBJ_SUBREGEX,

  OBJ_COUNT,
} ObjType;

// returns whether the character is inside of the class.
typedef int (*class_match_fn)(char to_match);

typedef struct Class {
  int is_complement;
  int is_generic; // does this use the function?
  union {
    class_match_fn fn; // a generic function representing a preset class.
    struct {
      char *ranges; // or, a list of ranges to dynamically run through.
      int num_points;
    } range_data;
  };
} Class;

typedef struct REComp REComp;

typedef struct Obj {
  ObjType type;
  union {
    char ch;
    Class class;
    REComp *sub_regex;
  } data;
} Obj;

// all the possible regex modifiers.
typedef enum ModType {
  MOD_NONE, // there's an option for there to be nothing special, and it just
            // matches the character once by default.

  MOD_QUESTION,
  MOD_STAR,
  MOD_PLUS,

  MOD_N_M, // between N and M.
  MOD_N_,  // at least N times.
  MOD_N,   // exactly N.

  MOD_COUNT,
} ModType;

typedef struct Mod {
  ModType type;
  union {
    struct {
      int n;
      int m;
    } n_m;
    int n;
  } range_data;
} Mod;

typedef struct Pair {
  Obj obj;
  Mod mod;
} Pair;

#define MAX_PAIRS 128

// representing a fully compiled regex pattern that can be directly run through
// text.
typedef struct REComp {
  Pair pairs[MAX_PAIRS];
  int num_pairs;
  int has_caret;  // ^ at the beginning of the pattern.
  int has_dollar; // $ at the end of the pattern.
} REComp;

typedef struct Match {
  // both indices.
  int start;
  int end;
} Match;

// return the number of matches found. the *dest array will be filled with
// descriptions of the matches.
int re_get_matches(const char *line, REComp *compiled, Match *dest);

REComp *re_compile(const char *pattern_static);
void re_debug_print(REComp *recomp);
void re_free(REComp *r);
