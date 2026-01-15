/* two template types:  STRING(t) which defines a pascal-style string
 * of element (t) [STRING(char) is the closest to the pascal string],
 * and ANCHOR(t) which defines a baseplate that a linked list can be
 * built up from.   [The linked list /must/ contain a ->next pointer
 * for linking the list together with.]
 */
#ifndef MARKDOWN_THIRD_PARTY_DISCOUNT_DISCOUNT_LITE_CSTRING_H_
#define MARKDOWN_THIRD_PARTY_DISCOUNT_DISCOUNT_LITE_CSTRING_H_

#include <stdlib.h>
#include <string.h>

#ifndef __WITHOUT_AMALLOC
// # include "amalloc.h"
#endif

/* expandable Pascal-style string.
 */
#define STRING(type) \
  struct {           \
    type* text;      \
    int size, alloc; \
  }
/* abstract anchor type that defines a list base
 * with a function that attaches an element to
 * the end of the list.
 *
 * the list base field is named .text so that the T()
 * macro will work with it.
 */
#define ANCHOR(t)  \
  struct {         \
    t *text, *end; \
  }

typedef STRING(char) Cstring;

extern void Csputc(int, Cstring*);
extern int Csprintf(Cstring*, char*, ...);
extern int Cswrite(Cstring*, char*, int);

#endif  // MARKDOWN_THIRD_PARTY_DISCOUNT_DISCOUNT_LITE_CSTRING_H_
