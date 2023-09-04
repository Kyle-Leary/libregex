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

// we need to allocate this since we'll call this function recursively.
// we can't have the caller allocate all the recursive REComps ahead of time,
// that would be over-complicated. just calloc each REComp as we make it.
REComp *re_compile(const char *pattern_static) {
  REComp *dest = calloc(1, sizeof(REComp));

  // make a copy so that we don't segfault modifying a potentially static .data
  // string.
  int len = strlen(pattern_static);
  char pattern_copied[len];
  memcpy(pattern_copied, pattern_static, len);
  char *pattern = pattern_copied;

  // handle the opening and closing ^ and $.
  dest->has_caret = (pattern[0] == '^');
  dest->has_dollar = (pattern[len - 1] == '$');

  // ignore these characters in the compilation if they're in the regex pattern.
  if (dest->has_dollar) {
    pattern[len - 1] = '\0';
    len--;
  }
  if (dest->has_caret) {
    pattern++;
    len--;
  }

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

      // wrap the expression in () to make a regex as a subobject.
    case '(': {
      o.type = OBJ_SUBREGEX;

      // some sort of class.
      char subobj_buf[128];
      int so_len = 0;
      NEXT_CHAR();
      while (pat_ch != ']') {
        subobj_buf[so_len] = pat_ch;
        so_len++;
        NEXT_CHAR();
      }
      NEXT_CHAR();

      subobj_buf[so_len] = '\0';

      o.data.sub_regex = re_compile(subobj_buf);
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

    default: {
      m.type = MOD_NONE;
    } break;
    }

    { // construct and append the pair from the parsed object and modifier.
      Pair p = {.obj = o, .mod = m};
      memcpy(&dest->pairs[dest->num_pairs], &p, sizeof(Pair));
      dest->num_pairs++;
    }

    // then, parse the next object.
  }

#undef NEXT_CHAR

  return dest;
}

const char *_eat(Obj *o, const char *line) {
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

  case OBJ_SUBREGEX: {
    // ?? we don't have the parent's matches.
    // re_get_matches(line, o->data.sub_regex, );
    return NULL;
  } break;

  default: {
    return NULL;
  } break;
  }
}

// take in the point in the line, either return the new pointer to the line
// position after the successful match, or NULL for an unsuccessful match.
const char *_pair(Pair *p, const char *line) {
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
      const char *after = _eat(&p->obj, line);
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

      const char *after = _eat(&p->obj, line);
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
    const char *after = _eat(&p->obj, line);
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
      const char *after = _eat(&p->obj, line);
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
    const char *after = _eat(&p->obj, line);
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
  int line_len = strlen(line);
  int num_matches = 0;

  Match m;
  m.start = 0; // init by trying to match with the first possible character.

  int up_to = line_len;
  if (compiled->has_caret) {
    // only try to match the first pass.
    up_to = 1;
  }

  // try to match starting with each character in the string.
  for (int c = 0; c < up_to; c++) {
    const char *_line = &line[c];

    for (int i = 0; i < compiled->num_pairs; i++) {
      if (_line >= line + line_len) {
        // we've run out of space to keep matching in the pattern. this is not a
        // match.
        goto fail;
      }

      Pair *p = &compiled->pairs[i];
      _line = _pair(p, _line);

      if (_line == NULL) {
        goto fail;
      }
    }

    goto succeed;

  fail : {
    m.start = c + 1;
    continue;
  }

  succeed : {
    // if we get all the way through the iterator, then match.
    m.end = _line - line - 1;
    // skip this if the end of the match isn't at the end of the line.
    if (compiled->has_dollar && !(m.end == line_len - 1)) {
      continue;
    }
    memcpy(&dest[num_matches], &m, sizeof(Match));
    num_matches++;
    // the next index we're going through on the next attempted match.
    m.start = c + 1;
    continue;
  }
  }

full_break : {}

  // this is kind of a hack, but we can filter out any matches after the fact
  // that don't line up with the $ expectation.
  if (compiled->has_dollar) {
    Match buf[16];
    int buf_len = 0;
    for (int i = 0; i < num_matches; i++) {
      Match *m = &dest[i];
      if (m->end == line_len - 1) {
        memcpy(&buf[buf_len], m, sizeof(Match));
        buf_len++;
      }
    }

    // then, copy in the temp filter match list into the real Match *dest one.
    num_matches = buf_len;
    memcpy(dest, buf, sizeof(Match) * buf_len);
  }

  return num_matches;
}

void re_debug_print(REComp *recomp) {
  if (!recomp) {
    printf("REComp is NULL!\n");
    return;
  }

  printf("REComp Debug Print Start:\n");
  printf("\tHas dollar: %d\n\tHas caret: %d\n", recomp->has_dollar,
         recomp->has_caret);

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

void re_free(REComp *r) {
  for (int i = 0; i < r->num_pairs; i++) {
    Pair *p = &r->pairs[i];
    if (p->obj.type == OBJ_SUBREGEX) {
      re_free(p->obj.data.sub_regex);
    }
  }

  free(r);
}
