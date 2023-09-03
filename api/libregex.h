#pragma once

typedef enum ObjType {
  OBJ_CHAR,
  OBJ_DOT,
  OBJ_CLASS,

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

typedef struct Obj {
  ObjType type;
  union {
    char ch;
    Class class;
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
} REComp;

typedef struct Match {
  // both indices.
  int start;
  int end;
} Match;

// return the number of matches found. the *dest array will be filled with
// descriptions of the matches.
int re_get_matches(const char *line, REComp *compiled, Match *dest);

void re_compile(REComp *dest, const char *pattern_static);
void re_debug_print(REComp *recomp);
