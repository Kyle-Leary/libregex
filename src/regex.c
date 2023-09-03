#include "libregex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIN(x, y) ((x < y) ? x : y)
#define MAX(x, y) ((x > y) ? x : y)
#define IS_BETWEEN(x, min, max) ((x >= min) && (x < max))
#define CH_TO_INT(ch) (ch - 48)

#define IS_ALNUM(ch)                                                           \
  ((IS_BETWEEN(ch, 'A', 'Z') || IS_BETWEEN(ch, 'a', 'z') ||                    \
    IS_BETWEEN(ch, '0', '9')))

void re_compile(REComp *dest, const char *pattern_static) {
  // recomp constructor-type setup.
  dest->num_pairs = 0;

  // make a copy so that we don't segfault modifying a potentially static .data
  // string.
  int len = strlen(pattern_static);
  char pattern[len];
  memcpy(pattern, pattern_static, len);

  int idx = 0;
  char pat_ch = pattern[idx];

#define NEXT_CHAR()                                                            \
  {                                                                            \
    idx++;                                                                     \
    pat_ch = pattern[idx];                                                     \
  }

  while (idx < len) {
    Obj o;
    Mod m;

    // first, parse the object at the cursor.
    switch (pat_ch) {
    case '.': {
      o.type = OBJ_DOT;
      NEXT_CHAR();
    } break;

    case '[': {
      Class c = {0};

      // some sort of class.
      char class_buf[32];
      int cb_len = 0;
      NEXT_CHAR();
      while (pat_ch != ']') {
        class_buf[cb_len] = pat_ch;
        cb_len++;
        NEXT_CHAR();
      }
      NEXT_CHAR();

      // then, handle the class_buf on its own, matching against the current
      // line ch state.
      class_buf[cb_len] = '\0';

      char *_class = class_buf;

      if (_class[0] == '^') {
        // then we're actually matching against the complement of this character
        // class.
        c.is_complement = 1;
        _class++; // discard the '^' after we've acknowledged it.
      }

      char *range_buf = calloc(sizeof(char), 16);

      int i = 0;

      while (_class[0] != '\0') {
        char start = _class[0];
        char end = _class[2];
        _class += 3;

        range_buf[i] = start;
        range_buf[i + 1] = end;
        i += 2;
      }

      c.range_data.num_points = i;
      c.range_data.ranges = range_buf;

      o.type = OBJ_CLASS;
      o.data.class = c;
    } break;

    default: {
      // normal ascii character, gets generated into a simple char object.
      o.type = OBJ_CHAR;
      o.data.ch = pat_ch;
      NEXT_CHAR();
    } break;
    }

    // then, parse the modifier at the cursor.
    if (IS_ALNUM(pat_ch) || pat_ch == '[') {
      // there's an object right after the last object, which means that there's
      // no modifier.
      m.type = MOD_NONE;
    } else {
      // else, it's a real modifier and we should find out which.
      switch (pat_ch) {
      case '*': {
        m.type = MOD_STAR;
        NEXT_CHAR();
      } break;
      case '+': {
        m.type = MOD_PLUS;
        NEXT_CHAR();
      } break;
      case '?': {
        m.type = MOD_QUESTION;
        NEXT_CHAR();
      } break;

      case '{': {

        NEXT_CHAR(); // skip past the first {

        /* three cases:
         * 1) {n,m} - match anywhere from n to m instances.
         * 2) {n} - exactly n.
         * 3) {n,} - at least n.
         * */

        // no matter what, we're on n right now. parse it for the range.
        // TODO: make this proper int parsing and not just the char coercion
        // from ASCII.
        int n_num = CH_TO_INT(pat_ch);

        NEXT_CHAR();

        if (pat_ch == ',') {

          // either 1) or 3).

          NEXT_CHAR();

          if (pat_ch == '}') {
            // 3)
            m.type = MOD_N_;

            m.range_data.n = n_num;
          } else {

            // 1)
            m.type = MOD_N_M;

            int m_num = CH_TO_INT(pat_ch);

            m.range_data.n_m.n = n_num;
            m.range_data.n_m.m = m_num;

            NEXT_CHAR();
          }

        } else {
          // case 2).
          m.type = MOD_N;
          m.range_data.n = n_num;
        }

        NEXT_CHAR(); // skip past the last }
      } break;
      }
    }

    { // construct and append the pair from the parsed object and modifier.
      Pair p = {.obj = o, .mod = m};
      memcpy(&dest->pairs[dest->num_pairs], &p, sizeof(Pair));
      dest->num_pairs++;
    }

    // then, parse the next object.
  }

#undef NEXT_CHAR
}

char *_eat(Obj *o, char *line) {
  if (line == NULL)
    return NULL;

  char to_match = line[0];

  switch (o->type) {
  case OBJ_CHAR: {
    if (o->data.ch == to_match) {
      return line + 1;
    } else {
      return NULL;
    }
  } break;

  case OBJ_DOT: {
    // matches with everything except for a newline.
    if (line[0] != '\n') {
      return line + 1;
    } else {
      return NULL;
    }
  } break;

  case OBJ_CLASS: {
    if (o->data.class.is_generic) {

      // if we're using a function class matcher, then try to match the to_match
      // using that.
      class_match_fn fn = o->data.class.fn;

      if (fn) {
        if (fn(to_match)) {
          return line + 1;
        } else {
          return NULL;
        }
      } else {
        return NULL;
      }

    } else {

      char *ranges = o->data.class.range_data.ranges;
      // else, use the ranges like usual.
      for (int i = 0; i < o->data.class.range_data.num_points; i += 2) {
        if (IS_BETWEEN(to_match, ranges[i], ranges[i + 1])) {
          return line + 1;
        }
      }

      // if nothing matched, return NULL.
      return NULL;
    }

  } break;

  default: {
    return NULL;
  } break;
  }
}

// take in the point in the line, either return the new pointer to the line
// position after the successful match, or NULL for an unsuccessful match.
char *_pair(Pair *p, char *line) {
  Mod m = p->mod;
  switch (m.type) {
  case MOD_NONE: {
    return _eat(&p->obj, line);
  } break;

  case MOD_N: {
    for (int i = 0; i < m.range_data.n; i++) {
      line = _eat(&p->obj, line);
    }

    return line;
  } break;

  case MOD_N_: {
    // just n_m without the limit that it stops when we reach the upper bound.
    int num_eaten = 0;
    for (;;) {
      char *after = _eat(&p->obj, line);
      if (after) {
        line = after;
      } else {
        break;
      }

      num_eaten++;
    }

    if (num_eaten < m.range_data.n) {
      return NULL;
    }

    return line;
  } break;

    // n_m is basically the same as the star case, except we freak out and
    // return NULL if we're not in the right bounds after the loop.
  case MOD_N_M: {
    int num_eaten = 0;
    for (;;) {
      // just stop when we've reached the limit. i don't think this counts as a
      // failed match.
      if (num_eaten >= m.range_data.n_m.m) {
        break;
      }

      char *after = _eat(&p->obj, line);
      if (after) {
        line = after;
      } else {
        break;
      }

      num_eaten++;
    }

    if (num_eaten < m.range_data.n_m.n) {
      return NULL;
    }

    return line;
  } break;

  case MOD_PLUS: {
    // assert that there must be at least one match.
    char *after = _eat(&p->obj, line);
    if (!after) {
      return NULL;
    } else {
      line = after;
    }

    for (;;) {
      after = _eat(&p->obj, line);
      if (after) {
        line = after;
      } else {
        break;
      }
    }

    return line;
  } break;

  case MOD_STAR: {
    for (;;) { // _eat until we hit a NULL, then we're done.
      char *after = _eat(&p->obj, line);
      if (after) {
        line = after;
      } else {
        break;
      }
    }

    return line;
  } break;

  case MOD_QUESTION: {
    // try _eat, if it works that's good, if it doesn't that's fine too.
    char *after = _eat(&p->obj, line);
    if (after) {
      line = after;
    }

    return line;
  } break;

  default: {
    return NULL;
  } break;
  }
}

int re_get_matches(const char *line, REComp *compiled, Match *dest) {
  int line_copy_len = strlen(line);
  char line_copy[line_copy_len];
  memcpy(line_copy, line, line_copy_len);
  char *_line = line_copy;

  int num_matches = 0;

  { // handle the opening and closing ^ and $.
    if (_pattern[0] == '^') {
      if (_pattern[1] != line[0]) {
        // if the ^ doesn't immediately match, it'll never match.
        return 0;
      } else {
        // otherwise, skip the ^ and match the rest like normal.
        _pattern++;
        _pattern_len--;
      }
    }

    // basically do the same thing backwards as the ^ for the $.
    if (_pattern[_pattern_len - 1] == '$') {
      if (_pattern[_pattern_len - 2] != line[line_copy_len - 1]) {
        return 0;
      } else {
        // in this case, discarding just means null-terming the _pattern
        // early.
        _pattern[_pattern_len - 1] = '\0';
        _pattern_len--;
      }
    }
  }

  Match m;
  m.start = 0; // init by trying to match with the first possible character.

  // specifically the index into the pointer, not the buffer.
  int _pattern_idx = 0;
  int line_idx = 0;

  char *_pattern_sections[16];
  _pattern_sections[0] = strtok(_pattern, "|");
  int i = 0;
  while ((i++, _pattern_sections[i] = strtok(NULL, "|"))) {
  }

  // then, the main loop through the stable copied line.
  while (line_idx < line_copy_len) {

    // if we're at the end of the pattern, we've successfully matched with the
    // line once.
#define TRY_MATCH()                                                            \
  {                                                                            \
    if (_pattern_idx == _pattern_len) {                                        \
      m.end = line_idx;                                                        \
      memcpy(&dest[num_matches], &m, sizeof(Match));                           \
      num_matches++;                                                           \
      m.start = line_idx + 1;                                                  \
    }                                                                          \
  }

    // without the TRY_MATCH()
#define JUST_NEW_PAT_CH()                                                      \
  {                                                                            \
    _pattern_idx++;                                                            \
    pat_ch = _pattern[_pattern_idx];                                           \
  }

#define NEW_PAT_CH()                                                           \
  {                                                                            \
    _pattern_idx++;                                                            \
    pat_ch = _pattern[_pattern_idx];                                           \
    TRY_MATCH();                                                               \
  }

#define FULL_BUMP()                                                            \
  {                                                                            \
    NEW_PAT_CH();                                                              \
    NEW_LINE_CH();                                                             \
  }

    // there's been a contradiction between the rule and the line. try to match
    // from the beginning.
#define FAIL()                                                                 \
  {                                                                            \
    _pattern_idx = 0;                                                          \
    NEW_LINE_CH();                                                             \
    m.start = line_idx;                                                        \
    continue;                                                                  \
  }

    // get the next character in the pattern.
#define PAT_PEEK() (_pattern[_pattern_idx + 1])

#define NEW_LINE_CH()                                                          \
  {                                                                            \
    line_idx++;                                                                \
    line_ch = line[line_idx];                                                  \
  }

    char pat_ch = _pattern[_pattern_idx];
    char line_ch = line[line_idx];

    switch (pat_ch) {
    case '\\': {
      // escape the metacharacter.
      // literally skip the \ and treat the next char like a normal ASCII char,
      // comparing it against the target line.
      _pattern_idx++;
      pat_ch = _pattern[_pattern_idx];
      goto normal_char_compare;
    } break;

    case '.': {
      // the . matches with anything that isn't a newline.
      if (line_ch != '\n') {
        NEW_PAT_CH();
        NEW_LINE_CH();
      } else {
        FAIL();
      }
    } break;

      // parse a character class. this is simpler since char classes can't be
      // nested inside eachother.
    case '[': {
      // first, parse out all the stuff inside.
      char class_buf[32];
      int cb_len = 0;
      JUST_NEW_PAT_CH();
      while (pat_ch != ']') {
        class_buf[cb_len] = pat_ch;
        cb_len++;
        JUST_NEW_PAT_CH();
      }
      JUST_NEW_PAT_CH(); // pop past the last ] in the char class as well.

      // then, handle the class_buf on its own, matching against the current
      // line ch state.
      class_buf[cb_len] = '\0';

      char *_class = class_buf;

      int is_complement = 0;
      if (_class[0] == '^') {
        // then we're actually matching against the complement of this
        // character class.
        is_complement = 1;
        _class++; // discard the '^' after we've acknowledged it.
      }

      // check each parsed character class against the current line ch, and fail
      // if we can't match a single one.
      while (_class[0] != '\0') {
        char start = _class[0];
        char end = _class[2];
        if (IS_BETWEEN(line_ch, MIN(start, end), MAX(start, end))) {
          // we've passed.
          if (is_complement) {
            // flip the result, we haven't passed.
          } else {
            TRY_MATCH();
            NEW_LINE_CH();
            // use goto end_of_loop; since break; here would only break out of
            // the while loop we're in locally.
            goto end_of_loop; // we've bumped the pointer to the next
                              // character, and we're ready for the next loop
                              // iteration.
          }
        } else {
          if (is_complement) {
            TRY_MATCH();
            NEW_LINE_CH();
            goto end_of_loop;
          } else {
          }
        }
        _class += 3;
      }

      // if we don't pass in a single of the character classes, just fail.
      FAIL();
    } break;

    default: {
    normal_char_compare : {}

      // in ?, + and *, the metachar comes after the rule. so, we need to do
      // this right here in the normal char comparison default switch, rather
      // than in the main switch.
      switch (PAT_PEEK()) {
      case '?': {
        if (pat_ch == line_ch) {
          // eat the next ch in the line if it happens to match.
          NEW_LINE_CH();
        }
        // no matter what, X? will pass.
        NEW_PAT_CH();
        NEW_PAT_CH();
      } break;

      case '+': {
        if (pat_ch != line_ch) {
          FAIL();
        }
        while (pat_ch == line_ch) {
          NEW_LINE_CH();
        }
        NEW_PAT_CH();
        NEW_PAT_CH();
      } break;

      case '{': { // parse a numeric modifier on a character/class.

        JUST_NEW_PAT_CH(); // bump pat_ch into n, the first number of the
                           // modifier.

        /* three cases:
         * 1) {n,m} - match anywhere from n to m instances.
         * 2) {n} - exactly n.
         * 3) {n,} - at least n.
         * */

        if (PAT_PEEK() == ',') {
          // either 1) or 3).
        } else {
          // case 2).
          int to_match = CH_TO_INT(pat_ch);
          for (int i = 0; i < to_match; i++) {
          }
        }

        // then, eat the }.
        JUST_NEW_PAT_CH();
        TRY_MATCH();
        NEW_LINE_CH();

      } break;

      case '*': {
        // the same, except there's no failing mechanism like in '+'.
        while (pat_ch == line_ch) {
          // not actually moving in the pattern, just keep matching the
          // line.
          NEW_LINE_CH();
        }
        NEW_PAT_CH();
        NEW_PAT_CH();
      } break;

      default: { // if it's nothing special, then this is truly just a normal
                 // match.

        // we've matched with a non-metacharacter.
        // compare ASCII codes and handle it normally.
        if (pat_ch == line_ch) {
          FULL_BUMP();
        } else {
          FAIL();
        }

      } break;
      }

    } break;
    }

  end_of_loop : {}

#undef TRY_MATCH
#undef FULL_BUMP
#undef FAIL
#undef NEW_PAT_CH
#undef NEW_LINE_CH
#undef PAT_PEEK
  }

  return num_matches;
}

void re_debug_print(REComp *recomp) {
  if (!recomp) {
    printf("REComp is NULL!\n");
    return;
  }

  printf("REComp Debug Print Start:\n");

  for (int i = 0; i < recomp->num_pairs; i++) {
    Pair p = recomp->pairs[i];

    printf("Pair %d:\n", i);

    // Print object type
    printf("  ObjType: ");
    switch (p.obj.type) {
    case OBJ_CHAR:
      printf("OBJ_CHAR: %c\n", p.obj.data.ch);
      break;
    case OBJ_DOT:
      printf("OBJ_DOT\n");
      break;
    case OBJ_CLASS:
      printf("OBJ_CLASS\n");
      break;
    case OBJ_COUNT:
      printf("OBJ_COUNT\n");
      break;
    default:
      printf("Unknown ObjType\n");
    }

    // Print Mod type
    printf("  ModType: ");
    switch (p.mod.type) {
    case MOD_NONE:
      printf("MOD_NONE\n");
      break;
    case MOD_QUESTION:
      printf("MOD_QUESTION\n");
      break;
    case MOD_STAR:
      printf("MOD_STAR\n");
      break;
    case MOD_PLUS:
      printf("MOD_PLUS\n");
      break;
    case MOD_N_M:
      printf("MOD_N_M (n: %d, m: %d)\n", p.mod.range_data.n_m.n,
             p.mod.range_data.n_m.m);
      break;
    case MOD_N_:
      printf("MOD_N_ (n: %d)\n", p.mod.range_data.n);
      break;
    case MOD_N:
      printf("MOD_N (n: %d)\n", p.mod.range_data.n);
      break;
    case MOD_COUNT:
      printf("MOD_COUNT\n");
      break;
    default:
      printf("Unknown ModType\n");
    }

    // For OBJ_CLASS, print additional info
    if (p.obj.type == OBJ_CLASS) {
      printf("  Class Info:\n");
      printf("    is_complement: %d\n", p.obj.data.class.is_complement);
      printf("    is_generic: %d\n", p.obj.data.class.is_generic);
      if (p.obj.data.class.is_generic) {
        printf("    (Generic class function pointer)\n");
      } else {
        printf("    Num points in ranges: %d\n",
               p.obj.data.class.range_data.num_points);
        for (int k = 0; k < p.obj.data.class.range_data.num_points; k += 2) {
          char *ranges = p.obj.data.class.range_data.ranges;
          printf("      Ranges: ('%c' - '%c')\n", ranges[k], ranges[k + 1]);
        }
      }
    }

    printf("\n");
  }

  printf("REComp Debug Print End.\n");
}
