#include "libregex.h"
#include "src/ansi.h"
#include <stdio.h>
#include <string.h>

void match(char **lines, int n_lines, char *pattern) {
  printf("\n" ANSI_BG_GREEN ANSI_BLACK "\t Matching against '%s' " ANSI_RESET
         "\n\n",
         pattern);
  for (int i = 0; i < n_lines; i++) {
    char *line = lines[i];
    int num_matches = 0;

#define PRINT_LINE(color)                                                      \
  {                                                                            \
    printf(ANSI_BG_##color ANSI_BLACK "%2d:" ANSI_RESET ANSI_##color           \
           " %s" ANSI_RESET "\n",                                              \
           i + 1, line);                                                       \
  }

    // the caller allocates their buffer based on how many matches they think
    // the string will cap out at.
    Match matches[16] = {0};
    num_matches = get_matches(line, pattern, matches);

    if (num_matches) {
      PRINT_LINE(GREEN);
      printf(ANSI_CYAN "\tMatches: \n" ANSI_RESET);
      for (int j = 0; j < num_matches; j++) {
        printf("\t\t(%d - %d)\n", matches[j].start, matches[j].end);
      }
      printf("\n");
    } else {
      PRINT_LINE(RED);
    }
  }
#undef PRINT_LINE
}

int main(int argc, char *argv[]) {

#define TESTCOMP(strlit)                                                       \
  {                                                                            \
    REComp reg;                                                                \
    re_compile(&reg, strlit);                                                  \
    re_debug_print(&reg);                                                      \
  }

  TESTCOMP("hell?o*world?");
  TESTCOMP(".*h");
  TESTCOMP("[0-9a-zA-Z]*");
  TESTCOMP("a{2,3}");
  TESTCOMP("a{2,7}");
  TESTCOMP("[0-8]{2,9}");
  TESTCOMP("a{3,8}");
  TESTCOMP("a{2,}");
  TESTCOMP("a{2}");

  // match((char *[]){"hellow", "hi", "hellowrld"}, 3, "hello");
  // match((char *[]){"", "wwhello", "hellowww", "world", "hello"}, 5,
  // "^hello$");
  //
  // // 1. Test for '^' and '$'
  // match((char *[]){"hello", "hello ", " hello", " hello "}, 4,
  // "^hello$");
  //
  // // 2. Test for '.' (dot)
  // match((char *[]){"a", "ab", "abc", "bc"}, 4, "a.c");
  //
  // // 3. Test for '*' (Kleene star)
  // match((char *[]){"aa", "aaa", "aaaa", "aaaaa"}, 4, "a*");
  //
  // // 4. Test for '+' (One or more)
  // match((char *[]){"a", "aa", "aaa", ""}, 4, "a+");
  //
  // // 5. Test for '?' (Zero or one)
  // match((char *[]){"a", "aa", "", "aaa"}, 4, "a?");
  //
  // // 6. Test for '|' (Alternation)
  // // match((char *[]){"apple", "banana", "cherry", "date"}, 4,
  // "apple|banana");
  //
  // // 7. Test for character classes [a-z], [^a-z]
  // match((char *[]){"a", "b", "8lkjalskdj", "e"}, 4, "[a-d]");
  // match((char *[]){"e", "f", "g", "h"}, 4, "[^a-d]");
  //
  // // 8. Test for back-references \\1, \\2, ...
  // // match((char *[]){"aa", "bb", "cc", "dd"}, 4, "(.)\\1");
  //
  // // 9. Test for {m,n} (min, max occurrence)
  // match((char *[]){"a", "aa", "aaa", "aaaa"}, 4, "a{2,3}");
  //
  // // 10. Test for sub-pattern capturing with (...)
  // match((char *[]){"ac", "abc", "aabc", "aaabc"}, 4, "a(bc)*");

  return 0;
}