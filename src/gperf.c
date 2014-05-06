/* ANSI-C code produced by gperf version 3.0.4 */
/* Command-line: gperf include/gperf.txt  */
/* Computed positions: -k'1-2' */

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

#line 1 "include/gperf.txt"

#include "gperf.h"
#line 13 "include/gperf.txt"
struct command_entry;
#include <string.h>

#define TOTAL_KEYWORDS 23
#define MIN_WORD_LENGTH 3
#define MAX_WORD_LENGTH 10
#define MIN_HASH_VALUE 3
#define MAX_HASH_VALUE 47
/* maximum key range = 45, duplicates = 0 */

#ifndef GPERF_DOWNCASE
#define GPERF_DOWNCASE 1
static unsigned char gperf_downcase[256] =
  {
      0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
     15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,
     30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,
     45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,
     60,  61,  62,  63,  64,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106,
    107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121,
    122,  91,  92,  93,  94,  95,  96,  97,  98,  99, 100, 101, 102, 103, 104,
    105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
    120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134,
    135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149,
    150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164,
    165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
    180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194,
    195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209,
    210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224,
    225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
    240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254,
    255
  };
#endif

#ifndef GPERF_CASE_MEMCMP
#define GPERF_CASE_MEMCMP 1
static int
gperf_case_memcmp (register const char *s1, register const char *s2, register unsigned int n)
{
  for (; n > 0;)
    {
      unsigned char c1 = gperf_downcase[(unsigned char)*s1++];
      unsigned char c2 = gperf_downcase[(unsigned char)*s2++];
      if (c1 == c2)
        {
          n--;
          continue;
        }
      return (int)c1 - (int)c2;
    }
  return 0;
}
#endif

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
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 10, 48,  5, 10, 15,
      30,  0, 15, 25, 48, 10,  0,  5,  0, 10,
       0, 48,  0,  5,  0,  0, 48,  0, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 10, 48,  5,
      10, 15, 30,  0, 15, 25, 48, 10,  0,  5,
       0, 10,  0, 48,  0,  5,  0,  0, 48,  0,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
      48, 48, 48, 48, 48, 48
    };
  return len + asso_values[(unsigned char)str[1]] + asso_values[(unsigned char)str[0]];
}

#ifdef __GNUC__
__inline
#if defined __GNUC_STDC_INLINE__ || defined __GNUC_GNU_INLINE__
__attribute__ ((__gnu_inline__))
#endif
#endif
const struct command_entry *
command_lookup (register const char *str, register unsigned int len)
{
  static const struct command_entry wordlist[] =
    {
#line 21 "include/gperf.txt"
      {"url", bot_url},
#line 27 "include/gperf.txt"
      {"play", bot_play},
#line 36 "include/gperf.txt"
      {"tweet", bot_tweet},
#line 26 "include/gperf.txt"
      {"uptime", bot_uptime},
#line 15 "include/gperf.txt"
      {"PRIVMSG", irc_privmsg},
#line 28 "include/gperf.txt"
      {"playlist", bot_playlist},
#line 32 "include/gperf.txt"
      {"stop", bot_stop},
#line 25 "include/gperf.txt"
      {"traceroute", bot_traceroute},
#line 20 "include/gperf.txt"
      {"mumble", bot_mumble},
#line 30 "include/gperf.txt"
      {"current", bot_current},
#line 24 "include/gperf.txt"
      {"dns", bot_dns},
#line 33 "include/gperf.txt"
      {"roll", bot_roll},
#line 16 "include/gperf.txt"
      {"NOTICE", irc_notice},
#line 35 "include/gperf.txt"
      {"announce", bot_announce},
#line 31 "include/gperf.txt"
      {"next", bot_next},
#line 37 "include/gperf.txt"
      {"marker", bot_marker},
#line 34 "include/gperf.txt"
      {"seek", bot_seek},
#line 23 "include/gperf.txt"
      {"ping", bot_ping},
#line 22 "include/gperf.txt"
      {"github", bot_github},
#line 18 "include/gperf.txt"
      {"help", bot_help},
#line 17 "include/gperf.txt"
      {"KICK", irc_kick},
#line 19 "include/gperf.txt"
      {"fail", bot_fail},
#line 29 "include/gperf.txt"
      {"history", bot_history}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= MIN_HASH_VALUE)
        {
          register const struct command_entry *resword;

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
              case 2:
                if (len == 5)
                  {
                    resword = &wordlist[2];
                    goto compare;
                  }
                break;
              case 3:
                if (len == 6)
                  {
                    resword = &wordlist[3];
                    goto compare;
                  }
                break;
              case 4:
                if (len == 7)
                  {
                    resword = &wordlist[4];
                    goto compare;
                  }
                break;
              case 5:
                if (len == 8)
                  {
                    resword = &wordlist[5];
                    goto compare;
                  }
                break;
              case 6:
                if (len == 4)
                  {
                    resword = &wordlist[6];
                    goto compare;
                  }
                break;
              case 7:
                if (len == 10)
                  {
                    resword = &wordlist[7];
                    goto compare;
                  }
                break;
              case 8:
                if (len == 6)
                  {
                    resword = &wordlist[8];
                    goto compare;
                  }
                break;
              case 9:
                if (len == 7)
                  {
                    resword = &wordlist[9];
                    goto compare;
                  }
                break;
              case 10:
                if (len == 3)
                  {
                    resword = &wordlist[10];
                    goto compare;
                  }
                break;
              case 11:
                if (len == 4)
                  {
                    resword = &wordlist[11];
                    goto compare;
                  }
                break;
              case 13:
                if (len == 6)
                  {
                    resword = &wordlist[12];
                    goto compare;
                  }
                break;
              case 15:
                if (len == 8)
                  {
                    resword = &wordlist[13];
                    goto compare;
                  }
                break;
              case 16:
                if (len == 4)
                  {
                    resword = &wordlist[14];
                    goto compare;
                  }
                break;
              case 18:
                if (len == 6)
                  {
                    resword = &wordlist[15];
                    goto compare;
                  }
                break;
              case 21:
                if (len == 4)
                  {
                    resword = &wordlist[16];
                    goto compare;
                  }
                break;
              case 26:
                if (len == 4)
                  {
                    resword = &wordlist[17];
                    goto compare;
                  }
                break;
              case 28:
                if (len == 6)
                  {
                    resword = &wordlist[18];
                    goto compare;
                  }
                break;
              case 31:
                if (len == 4)
                  {
                    resword = &wordlist[19];
                    goto compare;
                  }
                break;
              case 36:
                if (len == 4)
                  {
                    resword = &wordlist[20];
                    goto compare;
                  }
                break;
              case 41:
                if (len == 4)
                  {
                    resword = &wordlist[21];
                    goto compare;
                  }
                break;
              case 44:
                if (len == 7)
                  {
                    resword = &wordlist[22];
                    goto compare;
                  }
                break;
            }
          return 0;
        compare:
          {
            register const char *s = resword->name;

            if ((((unsigned char)*str ^ (unsigned char)*s) & ~32) == 0 && !gperf_case_memcmp (str, s, len))
              return resword;
          }
        }
    }
  return 0;
}
