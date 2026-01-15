/* markdown: a C implementation of John Gruber's Markdown markup language.
 *
 * Copyright (C) 2007 David L Parsons.
 * The redistribution terms are provided in the COPYRIGHT file that must
 * be distributed with this source code.
 */
// #include "config.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cstring.h"
#include "cstring_private.h"
#include "markdown.h"

#if defined(_WIN32) || defined(_WIN64)
#define strncasecmp _strnicmp
#endif
int line_whitespace_indent(Line* t);
typedef int (*stfu)(const void*, const void*);

// static Paragraph *Pp(ParagraphRoot *, Line *, int);
static void compile(Line*, int, MMIOT*);

static int need_to_initrng = 1;

#define INITRNG(x) srand((unsigned int)x)

void mkd_init_flags(mkd_flag_t* p) {
  memset(p, 0, sizeof(*p));
}

int ___mkd_different(mkd_flag_t* dst, mkd_flag_t* src) {
  int i;
  mkd_flag_t zeroes;

  if (dst == 0 || src == 0) {
    mkd_init_flags(&zeroes);
    if (!dst)
      dst = &zeroes;
    if (!src)
      src = &zeroes;
  }

  for (i = 0; i < MKD_NR_FLAGS; i++)
    if (is_flag_set(src, i) != is_flag_set(dst, i))
      return 1;

  return 0;
}

void mkd_initialize(void) {
  if (need_to_initrng) {
    need_to_initrng = 0;
    INITRNG(time(0));
  }
}

/* trim leading characters from a line, then adjust the dle.
 */
void __mkd_trim_line(Line* p, int clip) {
  if (clip >= S(p->text)) {
    p->markdown_offset += S(p->text);
    S(p->text) = p->dle = p->white_space = 0;
    T(p->text)
    [0] = 0;
  } else if (clip > 0) {
    CLIP(p->text, 0, clip);
    p->markdown_offset += clip;
    p->dle = mkd_firstnonblank(p);
    p->white_space = line_whitespace_indent(p);
  }
}

/* case insensitive string sort for Footnote tags.
 */
int __mkd_footsort(Footnote* a, Footnote* b) {
  int i;
  char ac, bc;

  if (S(a->tag) != S(b->tag))
    return S(a->tag) - S(b->tag);

  for (i = 0; i < S(a->tag); i++) {
    ac = tolower(T(a->tag)[i]);
    bc = tolower(T(b->tag)[i]);

    if (isspace(ac) && isspace(bc))
      continue;
    if (ac != bc)
      return ac - bc;
  }
  return 0;
}

/* find the first blank character after position <i>
 */
static int nextblank(Line* t, int i) {
  while ((i < S(t->text)) && !isspace(T(t->text)[i]))
    ++i;
  return i;
}

/* find the next nonblank character after position <i>
 */
static int nextnonblank(Line* t, int i) {
  while ((i < S(t->text)) && isspace(T(t->text)[i]))
    ++i;
  return i;
}

int line_whitespace_indent(Line* t) {
  int i = 0;
  int count = 0;
  while ((i < S(t->text)) && isspace(T(t->text)[i])) {
    ++i;
    if (T(t->text)[i] == '\t') {
      count += 4;
    } else {
      count += 1;
    }
  }
  return count;
}

/* find the first nonblank character on the Line.
 */
int mkd_firstnonblank(Line* p) {
  return nextnonblank(p, 0);
}

static inline int blankline(Line* p) {
  return !(p && (S(p->text) > p->dle));
}

static Line* skipempty(Line* p) {
  while (p && (p->dle == S(p->text)))
    p = p->next;
  return p;
}

void ___mkd_tidy(Cstring* t) {
  while (S(*t) && isspace(T(*t)[S(*t) - 1]))
    --S(*t);
}

typedef struct _flo {
  Line* t;
  int i;
} FLO;

static unsigned int flogetc(FLO* f) {
  if (f && f->t) {
    if (f->i < S(f->t->text))
      return (unsigned char)T(f->t->text)[f->i++];
    f->t = f->t->next;
    f->i = 0;
    return flogetc(f);
  }
  return EOF;
}

static void splitline(Line* t, int cutpoint) {
  if (t && (cutpoint < S(t->text))) {
    Line* tmp = (Line*)(calloc(1, sizeof *tmp));

    tmp->next = t->next;
    t->next = tmp;

    SUFFIX(tmp->text, T(t->text) + cutpoint, S(t->text) - cutpoint);
    tmp->markdown_offset = t->markdown_offset + cutpoint;
    EXPAND(tmp->text) = 0;
    S(tmp->text)
    --;
    S(t->text) = cutpoint;
  }
}

#define UNCHECK(t) ((t)->is_checked = 0)

/*
 * walk a line, seeing if it's any of half a dozen interesting regular
 * types.
 */
static void checkline(Line* l, mkd_flag_t* flags) {
  int eol, i;
  int dashes = 0, spaces = 0, equals = 0, underscores = 0, stars = 0,
      tildes = 0, other = 0, backticks = 0;
  int c, first;

  l->is_checked = 1;
  l->kind = chk_text;
  l->is_fenced = 0;
  l->count = 0;

  if (l->white_space >= 4) {
    l->kind = chk_code;
    return;
  }

  for (eol = S(l->text); eol > l->dle && isspace(T(l->text)[eol - 1]); --eol)
    ;

  if (is_flag_set(flags, MKD_FENCEDCODE) && !is_flag_set(flags, MKD_STRICT)) {
    first = T(l->text)[l->dle];

    if (first == '~' || first == '`') {
      for (i = l->dle; i < eol; i++) {
        c = T(l->text)[i];

        if ((c != '~') && (c != '`') && (c != first))
          break;
        l->count++;
      }

      if (l->count > 1) {
        l->kind = (first == '`' ? chk_backtick : chk_tilde);
        l->is_fenced = 1;
        return;
      }
    }
  }

  for (i = l->dle; i < eol; i++) {
    if ((c = T(l->text)[i]) != ' ')
      l->count++;

    switch (c) {
      case '-':
        dashes = 1;
        break;
      case ' ':
        spaces = 1;
        break;
      case '=':
        equals = 1;
        break;
      case '_':
        underscores = 1;
        break;
      case '*':
        stars = 1;
        break;
      default:
        other = 1;
        break;
    }
  }

  if (dashes + equals + underscores + stars + tildes + backticks > 1)
    return;

  if (!other) {
    if (underscores || stars)
      l->kind = chk_hr;
    else if (dashes)
      l->kind = chk_dash;
    else if (equals)
      l->kind = chk_equal;
    return;
  }

  if (tildes) {
    l->kind = chk_tilde;
  } else if (backticks) {
    l->kind = chk_backtick;
  }
}

/* markdown only does special handling of comments if the comment end
 * is at the end of a line
 */
static Line* commentblock(Line* t, int* unclosed) {
  Line* ret;
  char* end;

  for (; t; t = t->next) {
    if ((end = strstr(T(t->text), "-->"))) {
      if (nextnonblank(t, 3 + (end - T(t->text))) < S(t->text))
        continue;
      /*splitline(t, 3 + (end - T(t->text)) );*/
      ret = t->next;
      t->next = 0;
      return ret;
    }
  }

  *unclosed = 1;
  return t;
}

/* footnotes look like ^<whitespace>{0,3}[stuff]: <content>$
 */
static int isfootnote(Line* t) {
  int i;

  if (((i = t->white_space) > 3) || (T(t->text)[i] != '['))
    return 0;

  for (++i; i < S(t->text); ++i) {
    if (T(t->text)[i] == '[')
      return 0;
    else if (T(t->text)[i] == ']')
      return (T(t->text)[i + 1] == ':');
  }
  return 0;
}

static inline int isquote(Line* t) {
  return (t->white_space < 4 && T(t->text)[t->dle] == '>');
}

static inline int codefence(Line* t) {
  int dle = t->dle;
  return t->text.size >= dle + 3 &&
         (t->text.text[dle] == '`' && t->text.text[dle + 1] == '`' &&
          t->text.text[dle + 2] == '`');
}

static inline int isfencecode(Line* t, Line** end, int* indent) {
  if (!codefence(t)) {
    return 0;
  }
  *indent = t->dle;
  Line* before = t;
  t = t->next;
  while (t) {
    if (codefence(t)) {
      *end = t;
      return 1;
    }
    before = t;
    t = t->next;
  }
  *end = before;
  return 1;
}

static inline int iscode(Line* t) {
  return (t->white_space >= 4);
}

static inline int ishr(Line* t, mkd_flag_t* flags) {
  if (!(t->is_checked))
    checkline(t, flags);

  if (t->count > 2)
    return t->kind == chk_hr || t->kind == chk_dash || t->kind == chk_equal;
  return 0;
}

static int issetext(Line* t, int* htyp, mkd_flag_t* flags) {
  Line* n;

  /* check for setext-style HEADER
   *                        ======
   */

  if ((n = t->next)) {
    if (!(n->is_checked))
      checkline(n, flags);

    if (n->kind == chk_dash || n->kind == chk_equal) {
      *htyp = SETEXT;
      return 1;
    }
  }
  return 0;
}

static int ishdr(Line* t, int* htyp, mkd_flag_t* flags) {
  /* ANY leading `#`'s make this into an ETX header
   */
  if ((S(t->text) > t->dle + 1) && (T(t->text)[t->dle] == '#')) {
    // check if the first character after '#'s is space
    char* text = T(t->text) + t->dle;
    char* end = T(t->text) + S(t->text);
    while (text != end) {
      if (*text != '#') {
        if (*text != ' ') {
          return 0;
        } else {
          break;
        }
      }
      text++;
    }
    if (text == end) {
      return 0;
    }
    *htyp = ETX;
    return 1;
  }

  /* And if not, maybe it's a SETEXT header instead
   */
  return issetext(t, htyp, flags);
}

static inline int end_of_block(Line* t, mkd_flag_t* flags) {
  int dummy;

  if (!t)
    return 0;

  return ((S(t->text) <= t->dle) || ishr(t, flags) || ishdr(t, &dummy, flags));
}

static Line* is_discount_dt(Line* t, int* clip, mkd_flag_t* flags) {
  if (t && t->next && (S(t->text) > 2) && (t->dle == 0) &&
      (T(t->text)[0] == '=') && (T(t->text)[S(t->text) - 1] == '=')) {
    if (t->next->dle >= 4) {
      *clip = 4;
      return t;
    } else
      return is_discount_dt(t->next, clip, flags);
  }
  return 0;
}

static int is_extra_dd(Line* t) {
  return (t->dle < 4) && (T(t->text)[t->dle] == ':') &&
         isspace(T(t->text)[t->dle + 1]);
}

static Line* is_extra_dt(Line* t, int* clip, mkd_flag_t* flags) {
  if (t && t->next && S(t->text)) {
    Line* x;

    if (iscode(t) || end_of_block(t, flags))
      return 0;

    if ((x = skipempty(t->next)) && is_extra_dd(x)) {
      *clip = x->dle + 2;
      return t;
    }

    if ((x = is_extra_dt(t->next, clip, flags)))
      return x;
  }
  return 0;
}

static int list_level(int white_space) {
  int level, extra;
  level = white_space / 4;
  extra = white_space % 4;
  if (extra >= 2) {
    level++;
  }
  return level;
}

static int islist(Line* t, int* clip, mkd_flag_t* flags, int* list_type,
                  int* ol_index, int* list_extra_level) {
  int i, j;
  char* q;

  if (end_of_block(t, flags))
    return 0;

  //  if (isdefinition(t, clip, list_type, flags))
  //    return ParagraphType::DL;

  if (strchr("*-+", T(t->text)[t->dle]) && isspace(T(t->text)[t->dle + 1])) {
    i = nextnonblank(t, t->dle + 1);
    *clip = i;
    *list_type = UL;
    *list_extra_level = list_level(t->white_space);
    return UL;
  }

  if ((j = nextblank(t, t->dle)) > t->dle + 1) {
    if (T(t->text)[j - 1] == '.' && isdigit(T(t->text)[j - 2])) {
      if (!(is_flag_set(flags, MKD_NOALPHALIST) ||
            is_flag_set(flags, MKD_STRICT)) &&
          (j == t->dle + 2) && isalpha(T(t->text)[t->dle])) {
        j = nextnonblank(t, j);
        *clip = (j > 4) ? 4 : j;
        *list_type = AL;
        *list_extra_level = list_level(t->white_space);
        return AL;
      }

      *ol_index = strtoul(T(t->text) + t->dle, &q, 10);
      if ((q > T(t->text) + t->dle) && (q == T(t->text) + (j - 1))) {
        j = nextnonblank(t, j);
        *clip = j;
        *list_type = OL;
        *list_extra_level = list_level(t->white_space);
        return AL;
      }
    }
  }
  return 0;
}

static Line* headerblock(Line* p, int htyp, MMIOT* f) {
  Line* ret = 0;
  //  Line *p = pp->text;
  int i, j;
  int hnumber;
  switch (htyp) {
    case SETEXT:
      /* p->text is header, p->next->text is -'s or ='s
       */
      hnumber = (T(p->next->text)[0] == '=') ? 1 : 2;
      f->cb->header_number(hnumber, f->cb->ud);
      ret = p->next->next;
      ___mkd_freeLine(p->next);
      p->next = 0;
      break;

    case ETX:
      /* p->text is ###header###, so we need to trim off
       * the leading and trailing `#`'s
       */
      CLIP(p->text, 0, p->dle);
      for (i = 0;
           (T(p->text)[i] == T(p->text)[0]) && (i < S(p->text) - 1) && (i < 6);
           i++)
        ;

      hnumber = i;
      f->cb->header_number(hnumber, f->cb->ud);

      while ((i < S(p->text)) && isspace(T(p->text)[i]))
        ++i;

      CLIP(p->text, 0, i);
      p->markdown_offset += i;
      UNCHECK(p);

      j = S(p->text);
      while (j && isspace(T(p->text)[j - 1]))
        --j;

      S(p->text) = j;

      ret = p->next;
      p->next = 0;
      break;
  }
  return ret;
}

static Line* codeblock(Line* t) {
  Line *o = t, *r;

  for (; t; t = r) {
    __mkd_trim_line(t, 4);
    if (!((r = skipempty(t->next)) && iscode(r))) {
      ___mkd_freeLineRange(t, r);
      t->next = 0;
      return r;
    }
  }

  return t;
}

static Line* fencecodeblock(Line* t, Line* end, int indent) {
  Line* r = skipempty(end->next);
  ___mkd_freeLineRange(end, r);
  end->next = 0;
  while (t != NULL) {
    if (t->dle > indent) {
      __mkd_trim_line(t, indent);
    } else {
      __mkd_trim_line(t, t->dle);
    }
    t = t->next;
  }
  return r;
}

static int centered(Line* first, Line* last) {
  if (first && last) {
    int len = S(last->text);

    if ((len > 2) && (strncmp(T(first->text), "->", 2) == 0) &&
        (strncmp(T(last->text) + len - 2, "<-", 2) == 0)) {
      CLIP(first->text, 0, 2);
      first->markdown_offset += 2;
      S(last->text) -= 2;
      return CENTER;
    }
  }
  return 0;
}

/* length of the id: or class: kind in a special div-not-quote block
 */
static int szmarkerclass(char* p) {
  if (strncasecmp(p, "id:", 3) == 0)
    return 3;
  if (strncasecmp(p, "class:", 6) == 0)
    return 6;
  return 0;
}

/*
 * check if the first line of a quoted block is the special div-not-quote
 * marker %[kind:]name%
 */
#define iscsschar(c) (isalpha(c) || (c == '-') || (c == '_'))

/*
 * accumulate a blockquote.
 *
 * one sick horrible thing about blockquotes is that even though
 * it just takes ^> to start a quote, following lines, if quoted,
 * assume that the prefix is ``> ''.   This means that code needs
 * to be indented *5* spaces from the leading '>', but *4* spaces
 * from the start of the line.   This does not appear to be
 * documented in the reference implementation, but it's the
 * way the markdown sample web form at Daring Fireball works.
 */
static Line* quoteblock(Line* t, mkd_flag_t* flags) {
  Line *o = t, *q;
  int qp;
  int tmp1;

  for (; t; t = q) {
    if (isquote(t)) {
      /* clip leading spaces */
      for (qp = 0; T(t->text)[qp] != '>'; qp++)
        /* assert: the first nonblank character on this line
         * will be a >
         */
        ;
      /* clip '>' */
      qp++;
      /* clip next space, if any */
      if (T(t->text)[qp] == ' ')
        qp++;
      __mkd_trim_line(t, qp);
      checkline(t, flags);
    }

    q = skipempty(t->next);

    if ((q == 0) || (q != t->next) ||
        islist(q, &tmp1, flags, &tmp1, &tmp1, &tmp1)) {
      ___mkd_freeLineRange(t, q);
      t = q;
      break;
    }
  }

  return t;
}

typedef int (*linefn)(Line*);

/*
 * pull in a list block.  A list block starts with a list marker and
 * runs until the next list marker, the next non-indented paragraph,
 * or EOF.   You do not have to indent nonblank lines after the list
 * marker, but multiple paragraphs need to start with a 4-space indent.
 */
static Line* listitem(Line* p, int indent, mkd_flag_t* flags, linefn check,
                      MMIOT* f) {
  Line *t, *q;
  int clip = indent;
  int z;
  int firstpara = 1;
  int ischeck;
#define CHECK_NOT 0
#define CHECK_NO 1
#define CHECK_YES 2

  for (t = p; t; t = q) {
    UNCHECK(t);
    if (firstpara) {}
    __mkd_trim_line(t, clip);

    if (firstpara /*&& !(is_flag_set(flags, MKD_NORMAL_LISTITEM) || is_flag_set(flags, MKD_STRICT))*/) {
      ischeck = CHECK_NOT;
      if (strncmp(T(t->text) + t->dle, "[ ]", 3) == 0)
        ischeck = CHECK_NO;
      else if (strncasecmp(T(t->text) + t->dle, "[x]", 3) == 0)
        ischeck = CHECK_YES;

      if (ischeck != CHECK_NOT) {
        __mkd_trim_line(t, 3);
        //        p->para_flags |= GITHUB_CHECK;
        if (ischeck == CHECK_YES) {
          //          p->para_flags |= IS_CHECKED;
          f->cb->list_check(1, f->cb->ud);
        } else {
          f->cb->list_check(0, f->cb->ud);
        }
      } else {
        f->cb->list_check(-1, f->cb->ud);
      }
      firstpara = 0;
    }

    if ((q = skipempty(t->next)) == 0) {
      ___mkd_freeLineRange(t, q);
      return 0;
    }

    /* after a blank line, the next block needs to start with a line
     * that's indented 4(? -- reference implementation allows a 1
     * character indent, but that has unfortunate side effects here)
     * spaces, but after that the line doesn't need any indentation
     */
    if (q != t->next) {
      if (q->white_space < indent) {
        q = t->next;
        t->next = 0;
        return q;
      }
      /* indent at least 2, and at most as
       * as far as the initial line was indented. */
      indent = clip ? clip : 2;
    }

    if ((q->white_space < indent) &&
        (ishr(q, flags) || islist(q, &z, flags, &z, &z, &z) ||
         (check && (*check)(q))) &&
        !issetext(q, &z, flags)) {
      q = t->next;
      t->next = 0;
      return q;
    }

    clip = (q->dle > indent) ? indent : q->dle;
  }
  return t;
}

static Line* enumerated_block(Line* top, int clip, MMIOT* f, int list_class,
                              int ol_index, int current_extra_level) {
  Line* p;
  Line *q = top, *text;
  int para = 0, z, extra_list_level;

  while ((text = q)) {
    f->cb->paragraph_start(LISTITEM, f->cb->ud);
    p = text;
    text = listitem(p, clip, &(f->flags), 0, f);

    compile(p, 0, f);

    if ((q = skipempty(text)) == 0 ||
        islist(q, &clip, &(f->flags), &z, &ol_index, &extra_list_level) !=
            list_class ||
        extra_list_level < current_extra_level) {
      f->cb->paragraph_end(f->cb->ud);
      break;
    }

    if ((para = (q != text))) {
      Line anchor;

      anchor.next = text;
      ___mkd_freeLineRange(&anchor, q);
    }
    f->cb->paragraph_end(f->cb->ud);
  }
  return text;
}

static int tgood(char c) {
  switch (c) {
    case '\'':
    case '"':
      return c;
    case '(':
      return ')';
  }
  return 0;
}

/*
 * eat lines for a markdown extra footnote
 */
static Line* extrablock(Line* p) {
  Line* np;

  while (p && p->next) {
    np = p->next;

    if (np->dle < 4 && np->dle < S(np->text)) {
      p->next = 0;
      return np;
    }
    __mkd_trim_line(np, 4);
    p = np;
  }
  return 0;
}

// TODO(zhouchaoying): footnote text offset needs impl
/*
 * add a new (image or link) footnote to the footnote table
 */
static Line* addfootnote(Line* p, MMIOT* f) {
  int j, i;
  int c;
  Line* np = p->next;

  Footnote* foot = &EXPAND(f->footnotes->note);

  CREATE(foot->tag);
  CREATE(foot->link);
  CREATE(foot->title);
  foot->fn_flags = foot->height = foot->width = 0;

  /* keep the footnote label */
  for (j = i = p->dle + 1; T(p->text)[j] != ']'; j++)
    EXPAND(foot->tag) = T(p->text)[j];
  EXPAND(foot->tag) = 0;
  S(foot->tag)
  --;

  /* consume the closing ]: */
  j = nextnonblank(p, j + 2);

  if (is_flag_set(&(f->flags), MKD_EXTRA_FOOTNOTE) &&
      !is_flag_set(&(f->flags), MKD_STRICT) && (T(foot->tag)[0] == '^')) {
    /* markdown extra footnote: All indented lines past this point;
     * the first line includes the footnote reference, so we need to
     * snip that out as we go.
     */
    foot->fn_flags |= EXTRA_FOOTNOTE;
    __mkd_trim_line(p, j);

    np = extrablock(p);

    compile(p, 0, f);

    return np;
  }

  while ((j < S(p->text)) && !isspace(T(p->text)[j]))
    EXPAND(foot->link) = T(p->text)[j++];
  EXPAND(foot->link) = 0;
  S(foot->link)
  --;
  j = nextnonblank(p, j);

  if (T(p->text)[j] == '=') {
    sscanf(T(p->text) + j, "=%dx%d", &foot->width, &foot->height);
    j = nextblank(p, j);
    j = nextnonblank(p, j);
  }

  if ((j >= S(p->text)) && np && np->dle && tgood(T(np->text)[np->dle])) {
    ___mkd_freeLine(p);
    p = np;
    np = p->next;
    j = p->dle;
  }

  if ((c = tgood(T(p->text)[j]))) {
    /* Try to take the rest of the line as a comment; read to
     * EOL, then shrink the string back to before the final
     * quote.
     */
    ++j; /* skip leading quote */

    while (j < S(p->text))
      EXPAND(foot->title) = T(p->text)[j++];

    while (S(foot->title) && T(foot->title)[S(foot->title) - 1] != c)
      --S(foot->title);
    if (S(foot->title)) /* skip trailing quote */
      --S(foot->title);
    EXPAND(foot->title) = 0;
    --S(foot->title);
  }

  ___mkd_freeLine(p);
  return np;
}

static Line* consume(Line* ptr, int* eaten) {
  Line* next;
  int blanks = 0;

  for (; ptr && blankline(ptr); ptr = next, blanks++) {
    next = ptr->next;
    ___mkd_freeLine(ptr);
  }
  if (ptr)
    *eaten = blanks;
  return ptr;
}

typedef ANCHOR(Line) Cache;

static void uncache(Cache* cache, /*ParagraphRoot *d,*/ MMIOT* f) {
  //  Paragraph *p;

  if (T(*cache)) {
    END(*cache)->next = 0;
    //    p = Pp(d, 0, ParagraphType::SOURCE);
    f->cb->paragraph_start(SOURCE, f->cb->ud);
    /*p->down = */ compile(T(*cache), 1, f);
    f->cb->paragraph_end(f->cb->ud);
    T(*cache) = END(*cache) = 0;
  }
}

/*
 * top-level compilation; break the document into
 * style, html, and source blocks with footnote links
 * weeded out.
 */
static void compile_document(Line* ptr, MMIOT* f) {
  //  ParagraphRoot d = {0, 0};
  Cache source = {0, 0};
  //  Paragraph *p = 0;
  struct kw* tag;
  int eaten, unclosed;
  int previous_was_break = 1;

  while (ptr) {
    checkline(ptr, &(f->flags));
    if (isfootnote(ptr)) {
      /* footnotes, like cats, sleep anywhere; pull them
       * out of the input stream and file them away for
       * later processing
       */
      ptr = consume(addfootnote(ptr, f), &eaten);
      previous_was_break = 1;
    } else {
      /* source; cache it up to wait for eof or the
       * next html/style block
       */
      ATTACH(source, ptr);
      previous_was_break = blankline(ptr);
      ptr = ptr->next;
    }
  }
  /* if there's any cached source at EOF, compile
   * it now.
   */
  uncache(&source, f);
}

static int first_nonblank_before(Line* j, int dle) {
  return (j->dle < dle) ? j->dle : dle;
}

static int actually_a_table(MMIOT* f, Line* pp, Line** end) {
  Line *r, *te = NULL;
  int j;
  int c;

  /* tables need to be turned on */
  if (is_flag_set(&(f->flags), MKD_NOTABLES) ||
      is_flag_set(&(f->flags), MKD_STRICT))
    return 0;

  /* tables need three lines */
  if (!(pp && pp->next && pp->next->next)) {
    return 0;
  }

  /* all lines must contain |'s */
  for (r = pp; r; r = r->next)
    if (!(r->has_pipechar)) {
      te = r;
      break;
    }

  /* if the header has a leading |, all lines must have leading |'s */
  if (T(pp->text)[pp->dle] == '|') {
    for (r = pp; r != te; r = r->next)
      if (T(r->text)[first_nonblank_before(r, pp->dle)] != '|') {
        te = r;
        break;
      }
  }

  /* second line must be only whitespace, -, |, or : */
  r = pp->next;

  for (j = r->dle; j < S(r->text); ++j) {
    c = T(r->text)[j];

    if (!(isspace(c) || (c == '-') || (c == ':') || (c == '|'))) {
      return 0;
    }
  }

  if (te != NULL && (te == pp || te == pp->next || te == pp->next->next)) {
    return 0;
  }

  for (r = pp; r; r = r->next) {
    if (r->next == te) {
      *end = r;
    }
  }

  return 1;
}

static int endoftextblock(Line* t, int toplevelblock, mkd_flag_t* flags) {
  int z;

  if (end_of_block(t, flags) || isquote(t))
    return 1;

  /* HORRIBLE STANDARDS KLUDGES:
   * 1. non-toplevel paragraphs absorb adjacent code blocks
   * 2. Toplevel paragraphs eat absorb adjacent list items,
   *    but sublevel blocks behave properly.
   * (What this means is that we only need to check for code
   *  blocks at toplevel, and only check for list items at
   *  nested levels.)
   */
  Line* end;
  return /*toplevelblock ? 0 :*/ (
      iscode(t) || isfencecode(t, &end, &z) || ishr(t, flags) ||
      islist(t, &z, flags, &z, &z, &z) || isquote(t) || ishdr(t, &z, flags));
}

static Line* textblock(Line* t, int toplevel, mkd_flag_t* flags) {
  Line *o = t, *next;
  int align;
  for (; t; t = next) {
    if (((next = t->next) == 0) || endoftextblock(next, toplevel, flags)) {
      align = centered(o, t);
      t->next = 0;
      return next;
    }
  }
  return t;
}

/*
 * break a collection of markdown input into
 * blocks of lists, code, html, and text to
 * be marked up.
 */
static void compile(Line* ptr, int toplevel, MMIOT* f) {
  Line* p = 0;
  Line* r;
  int para = toplevel;
  int blocks = 0;
  int hdr_type, list_type, list_class, indent, ol_index, extra_list_level,
      code_indent;

  ptr = consume(ptr, &para);
  while (ptr) {
    Line* end = NULL;
    if (isfencecode(ptr, &end, &code_indent)) {
      f->cb->paragraph_start(CODE, f->cb->ud);
      p = ptr;
      ptr = fencecodeblock(p, end, code_indent);
      f->cb->paragraph_text(p, f->cb->ud);
    } else if (ishr(ptr, &(f->flags))) {
      f->cb->paragraph_start(HR, f->cb->ud);
      p = 0;
      r = ptr;
      ptr = ptr->next;
      ___mkd_freeLine(r);
    } else if ((list_class = islist(ptr, &indent, &(f->flags), &list_type,
                                    &ol_index, &extra_list_level))) {
      if (list_class == DL) {

      } else {
        if (list_type == OL) {
          f->cb->list_index(ol_index, f->cb->ud);
        }
        f->cb->list_extra_level(extra_list_level, f->cb->ud);
        f->cb->paragraph_start(list_type, f->cb->ud);
        p = ptr;
        ptr = enumerated_block(p, indent, f, list_class, ol_index,
                               extra_list_level);
      }
    } else if (isquote(ptr)) {
      f->cb->paragraph_start(QUOTE, f->cb->ud);
      p = ptr;
      ptr = quoteblock(p, &(f->flags));
      compile(p, 1, f);
      p = 0;
    } else if (ishdr(ptr, &hdr_type, &(f->flags))) {
      p = ptr;
      ptr = headerblock(p, hdr_type, f);
      f->cb->paragraph_start(HDR, f->cb->ud);
      f->cb->paragraph_text(p, f->cb->ud);
    } else {
      p = ptr;
      ptr = textblock(p, toplevel, &(f->flags));
      Line* start = p;
      Line* tmp = p;
      Line* end = NULL;
      while (p) {
        if (actually_a_table(f, p, &end)) {
          if (p != start) {
            tmp->next = 0;
            f->cb->paragraph_start(MARKUP, f->cb->ud);
            f->cb->paragraph_text(start, f->cb->ud);
            f->cb->paragraph_end(f->cb->ud);
          }
          Line* next = end->next;
          end->next = NULL;
          f->cb->paragraph_start(TABLE, f->cb->ud);
          f->cb->paragraph_text(p, f->cb->ud);
          if (next != NULL) {
            f->cb->paragraph_end(f->cb->ud);
            f->cb->paragraph_start(MARKUP, f->cb->ud);
            f->cb->paragraph_text(next, f->cb->ud);
          }
          break;
        } else {
          tmp = p;
          p = p->next;
        }
      }
      if (!p) {
        f->cb->paragraph_start(MARKUP, f->cb->ud);
        f->cb->paragraph_text(start, f->cb->ud);
      }
    }

    blocks++;
    para = toplevel || (blocks > 1);
    ptr = consume(ptr, &para);
    f->cb->paragraph_end(f->cb->ud);
  }
}

/*
 * the guts of the markdown() function, ripped out so I can do
 * debugging.
 */

/*
 * prepare and compile `text`, returning a Paragraph tree.
 */
int mkd_compile(Document* doc, mkd_flag_t* flags) {
  if (!doc)
    return 0;

  doc->compiled = 1;
  memset(doc->ctx, 0, sizeof(MMIOT));
  doc->ctx->ref_prefix = doc->ref_prefix;
  doc->ctx->cb = &(doc->cb);
  if (flags)
    COPY_FLAGS(doc->ctx->flags, *flags);
  else
    mkd_init_flags(&doc->ctx->flags);
  CREATE(doc->ctx->in);
  doc->ctx->footnotes = malloc(sizeof doc->ctx->footnotes[0]);
  doc->ctx->footnotes->reference = 0;
  CREATE(doc->ctx->footnotes->note);

  mkd_initialize();

  /*doc->code = */ compile_document(T(doc->content), doc->ctx);
  qsort(T(doc->ctx->footnotes->note), S(doc->ctx->footnotes->note),
        sizeof T(doc->ctx->footnotes->note)[0], (stfu)__mkd_footsort);
  memset(&doc->content, 0, sizeof doc->content);
  return 1;
}

/* build a Document from any old input.
 */
typedef int (*getc_func)(void*);

Document* populate(getc_func getc, void* ctx, mkd_flag_t* flags) {
  Cstring line;
  Document* a = __mkd_new_Document();
  int c;
  int pandoc = 0;

  if (flags &&
      (is_flag_set(flags, MKD_NOHEADER) || is_flag_set(flags, MKD_STRICT)))
    pandoc = EOF;

  if (!a)
    return 0;

  if (flags &&
      (is_flag_set(flags, MKD_TABSTOP) || is_flag_set(flags, MKD_STRICT)))
    a->tabstop = 4;
  else
    a->tabstop = TABSTOP;

  CREATE(line);
  int line_offset = 0;
  while ((c = (*getc)(ctx)) != EOF) {
    if (c == '\n') {
      if (pandoc != EOF && pandoc < 3) {
        if (S(line) && (T(line)[0] == '%'))
          pandoc++;
        else
          pandoc = EOF;
      }
      __mkd_enqueue(a, &line, line_offset);
      line_offset += S(line) + 1;
      S(line) = 0;
    } else if ((c & 0x80) || isprint(c) || isspace(c)) {
      EXPAND(line) = c;
    }
  }

  if (S(line))
    __mkd_enqueue(a, &line, line_offset);

  DELETE(line);

  if (pandoc == 3) {
    /* the first three lines started with %, so we have a header.
     * clip the first three lines out of content and hang them
     * off header.
     */
    Line* headers = T(a->content);

    a->title = headers;
    __mkd_trim_line(a->title, 1);
    a->author = headers->next;
    __mkd_trim_line(a->author, 1);
    a->date = headers->next->next;
    __mkd_trim_line(a->date, 1);

    T(a->content) = headers->next->next->next;
  }

  return a;
}

Document* mkd_string(const char* buf, int len, mkd_flag_t* flags) {
  struct string_stream about;

  about.data = buf;
  about.size = len;

  return populate((getc_func)__mkd_io_strget, &about, flags);
}

/* add a line to the markdown input chain, expanding tabs and
 * noting the presence of special characters as we go.
 */
void __mkd_enqueue(Document* a, Cstring* line, int line_offset) {
  Line* p = (Line*)calloc(sizeof *p, 1);
  unsigned char c;
  int xp = 0;
  int size = S(*line);
  unsigned char* str = (unsigned char*)T(*line);
  unsigned char* str_start = (unsigned char*)T(*line);

  CREATE(p->text);
  ATTACH(a->content, p);

  while (size--) {
    c = *str++;
    if (c < ' ')
      c = ' ';  // replace control chars to space
    if (c == '|')
      p->has_pipechar = 1;
    EXPAND(p->text) = c;
  }
  EXPAND(p->text) = 0;
  S(p->text)
  --;
  p->dle = mkd_firstnonblank(p);
  p->white_space = line_whitespace_indent(p);
  p->markdown_offset = line_offset;
}

/* return a single character out of a buffer
 */
int __mkd_io_strget(struct string_stream* in) {
  if (!in->size)
    return EOF;

  --(in->size);

  return *(in->data)++;
}

/* create a new blank Document
 */
Document* __mkd_new_Document(void) {
  Document* ret = (Document*)calloc(sizeof(Document), 1);

  if (ret) {
    if (ret->ctx = (MMIOT*)calloc(sizeof(MMIOT), 1)) {
      ret->magic = VALID_DOCUMENT;
      return ret;
    }
    free(ret);
  }
  return 0;
}
