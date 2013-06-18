/* ANSI-C code produced by gperf version 3.0.4 */
/* Command-line: gperf include/gperf-input.txt  */
/* Computed positions: -k'1' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gnu-gperf@gnu.org>."
#endif

#line 1 "include/gperf-input.txt"

#include "gperf.h"
#line 12 "include/gperf-input.txt"
struct function_list;
#include <string.h>

#define TOTAL_KEYWORDS 12
#define MIN_WORD_LENGTH 3
#define MAX_WORD_LENGTH 10
#define MIN_HASH_VALUE 3
#define MAX_HASH_VALUE 21
/* maximum key range = 19, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
hash (register const char *str, register unsigned int len)
{
  static const unsigned char asso_values[] =
    {
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 15, 22,
       0, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
       5, 22, 15, 10, 10, 22, 22, 22,  5,  5,
      22, 22,  0, 22, 22, 22,  0,  0, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
      22, 22, 22, 22, 22, 22
    };
  return len + asso_values[(unsigned char)str[0]];
}

#ifdef __GNUC__
__inline
#if defined __GNUC_STDC_INLINE__ || defined __GNUC_GNU_INLINE__
__attribute__ ((__gnu_inline__))
#endif
#endif
const struct function_list *
function_lookup (register const char *str, register unsigned int len)
{
  static const struct function_list wordlist[] =
    {
#line 16 "include/gperf-input.txt"
      {"url", url},
#line 23 "include/gperf-input.txt"
      {"ping", ping},
#line 25 "include/gperf-input.txt"
      {"uptime", uptime},
#line 14 "include/gperf-input.txt"
      {"PRIVMSG", irc_privmsg},
#line 24 "include/gperf-input.txt"
      {"dns", dns},
#line 19 "include/gperf-input.txt"
      {"list", list},
#line 22 "include/gperf-input.txt"
      {"traceroute", traceroute},
#line 17 "include/gperf-input.txt"
      {"mumble", mumble},
#line 20 "include/gperf-input.txt"
      {"help", list},
#line 21 "include/gperf-input.txt"
      {"github", github},
#line 18 "include/gperf-input.txt"
      {"fail", bot_fail},
#line 15 "include/gperf-input.txt"
      {"NOTICE", irc_notice}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= MIN_HASH_VALUE)
        {
          register const struct function_list *resword;

          switch (key - 3)
            {
              case 0:
                if (len == 3)
                  {
                    resword = &wordlist[0];
                    goto compare;
                  }
                break;
              case 1:
                if (len == 4)
                  {
                    resword = &wordlist[1];
                    goto compare;
                  }
                break;
              case 3:
                if (len == 6)
                  {
                    resword = &wordlist[2];
                    goto compare;
                  }
                break;
              case 4:
                if (len == 7)
                  {
                    resword = &wordlist[3];
                    goto compare;
                  }
                break;
              case 5:
                if (len == 3)
                  {
                    resword = &wordlist[4];
                    goto compare;
                  }
                break;
              case 6:
                if (len == 4)
                  {
                    resword = &wordlist[5];
                    goto compare;
                  }
                break;
              case 7:
                if (len == 10)
                  {
                    resword = &wordlist[6];
                    goto compare;
                  }
                break;
              case 8:
                if (len == 6)
                  {
                    resword = &wordlist[7];
                    goto compare;
                  }
                break;
              case 11:
                if (len == 4)
                  {
                    resword = &wordlist[8];
                    goto compare;
                  }
                break;
              case 13:
                if (len == 6)
                  {
                    resword = &wordlist[9];
                    goto compare;
                  }
                break;
              case 16:
                if (len == 4)
                  {
                    resword = &wordlist[10];
                    goto compare;
                  }
                break;
              case 18:
                if (len == 6)
                  {
                    resword = &wordlist[11];
                    goto compare;
                  }
                break;
            }
          return 0;
        compare:
          {
            register const char *s = resword->command;

            if (*str == *s && !memcmp (str + 1, s + 1, len - 1))
              return resword;
          }
        }
    }
  return 0;
}
