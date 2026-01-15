/* 
 * Copyright (C) 2007 David L Parsons.
 * The redistribution terms are provided in the COPYRIGHT file that must
 * be distributed with this source code.
 */
#ifndef MARKDOWN_THIRD_PARTY_DISCOUNT_DISCOUNT_LITE_CSTRING_PRIVATE_H_
#define MARKDOWN_THIRD_PARTY_DISCOUNT_DISCOUNT_LITE_CSTRING_PRIVATE_H_

#define CREATE(x) ((T(x) = 0), (S(x) = (x).alloc = 0))
#define EXPAND(x)                                                            \
  (S(x)++)[(S(x) < (x).alloc)                                                \
               ? (T(x))                                                      \
               : (T(x) = T(x) ? realloc(T(x),                                \
                                        sizeof T(x)[0] * ((x).alloc += 100)) \
                              : malloc(sizeof T(x)[0] * ((x).alloc += 100)))]

#define DELETE(x) ALLOCATED(x) ? (free(T(x)), S(x) = (x).alloc = 0) : (S(x) = 0)
#define CLIP(t, i, sz)                                             \
  S(t) -= (((i) >= 0) && ((sz) > 0) && (((i) + (sz)) <= S(t)))     \
              ? (memmove(&T(t)[i], &T(t)[i + sz],                  \
                         (S(t) - (i + sz) + 1) * sizeof(T(t)[0])), \
                 (sz))                                             \
              : 0

#define RESERVE(x, sz)                                                                        \
  T(x) = ((x).alloc > S(x) + (sz) ? T(x)                                                      \
          : T(x)                  ? realloc(T(x),                                             \
                                            sizeof T(x)[0] * ((x).alloc = 100 + (sz) + S(x))) \
                 : malloc(sizeof T(x)[0] * ((x).alloc = 100 + (sz) + S(x))))
#define SUFFIX(t, p, sz)                                                      \
  memcpy(((S(t) += (sz)) - (sz)) +                                            \
             (T(t) = T(t) ? realloc(T(t), sizeof T(t)[0] * ((t).alloc += sz)) \
                          : malloc(sizeof T(t)[0] * ((t).alloc += sz))),      \
         (p), sizeof(T(t)[0]) * (sz))

#define PREFIX(t, p, sz)              \
  RESERVE((t), (sz));                 \
  if (S(t)) {                         \
    memmove(T(t) + (sz), T(t), S(t)); \
  }                                   \
  memcpy(T(t), (p), (sz));            \
  S(t) += (sz)

/* reference-style links (and images) are stored in an array
 */
#define T(x) (x).text
#define S(x) (x).size
#define ALLOCATED(x) (x).alloc

#define END(t) ((t).end)

#define ATTACH(t, p) \
  (T(t) ? ((END(t)->next = (p)), (END(t) = (p))) : ((T(t) = END(t) = (p))))

#endif  // MARKDOWN_THIRD_PARTY_DISCOUNT_DISCOUNT_LITE_CSTRING_PRIVATE_H_
