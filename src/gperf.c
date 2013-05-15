/* ANSI-C code produced by gperf version 3.0.4 */
/* Command-line: gperf include/gperf-input.txt  */
/* Computed positions: -k'' */

#line 1 "include/gperf-input.txt"

#include "gperf.h"
#include "irc.h"
#line 13 "include/gperf-input.txt"
struct function_list;
#include <string.h>

#define TOTAL_KEYWORDS 2
#define MIN_WORD_LENGTH 3
#define MAX_WORD_LENGTH 7
#define MIN_HASH_VALUE 3
#define MAX_HASH_VALUE 7
/* maximum key range = 5, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
/*ARGSUSED*/
// static unsigned int
// hash (register const char *str, register unsigned int len)
// {
//   return len;
// }

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
      {"bot", (void *) &bot},
#line 15 "include/gperf-input.txt"
      {"PRIVMSG", (void *) &irc_privmsg}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = len;

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
              case 4:
                if (len == 7)
                  {
                    resword = &wordlist[1];
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
