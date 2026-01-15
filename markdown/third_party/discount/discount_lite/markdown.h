/* markdown: a C implementation of John Gruber's Markdown markup language.
 *
 * Copyright (C) 2007 David L Parsons.
 * The redistribution terms are provided in the COPYRIGHT file that must
 * be distributed with this source code.
 */
#ifndef MARKDOWN_THIRD_PARTY_DISCOUNT_DISCOUNT_LITE_MARKDOWN_H_
#define MARKDOWN_THIRD_PARTY_DISCOUNT_DISCOUNT_LITE_MARKDOWN_H_
// #include "paragraph.h"
// #include "config.h"
#include "cstring.h"
#include "stdio.h"
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#elif HAVE_STDINT_H
#include <stdint.h>
#endif
#define TABSTOP 4
/* flags, captured into a named type
 */
enum {
  MKD_NOLINKS = 0,     /* don't do link processing, block <a> tags  */
  MKD_NOIMAGE,         /* don't do image processing, block <img> */
  MKD_NOPANTS,         /* don't run smartypants() */
  MKD_NOHTML,          /* don't allow raw html through AT ALL */
  MKD_NORMAL_LISTITEM, /* disable github-style checkbox lists */
  MKD_TAGTEXT,         /* process text inside an html tag */
  MKD_NO_EXT,          /* don't allow pseudo-protocols */
#define MKD_NOEXT MKD_NO_EXT
  MKD_EXPLICITLIST,  /* don't combine numbered/bulletted lists */
  MKD_CDATA,         /* generate code for xml ![CDATA[...]] */
  MKD_NOSUPERSCRIPT, /* no A^B */
  MKD_STRICT,   /* conform to Markdown standard as implemented in Markdown.pl */
  MKD_NOTABLES, /* disallow tables */
  MKD_NOSTRIKETHROUGH,  /* forbid ~~strikethrough~~ */
  MKD_1_COMPAT,         /* compatibility with MarkdownTest_1.0 */
  MKD_TOC,              /* do table-of-contents processing */
  MKD_AUTOLINK,         /* make http://foo.com link even without <>s */
  MKD_NOHEADER,         /* don't process header blocks */
  MKD_TABSTOP,          /* expand tabs to 4 spaces */
  MKD_SAFELINK,         /* paranoid check for link protocol */
  MKD_NODIVQUOTE,       /* forbid >%class% blocks */
  MKD_NOALPHALIST,      /* forbid alphabetic lists */
  MKD_EXTRA_FOOTNOTE,   /* enable markdown extra-style footnotes */
  MKD_NOSTYLE,          /* don't extract <style> blocks */
  MKD_DLDISCOUNT,       /* enable discount-style definition lists */
  MKD_DLEXTRA,          /* enable extra-style definition lists */
  MKD_FENCEDCODE,       /* enabled fenced code blocks */
  MKD_IDANCHOR,         /* use id= anchors for TOC links */
  MKD_GITHUBTAGS,       /* allow dash and underscore in element names */
  MKD_URLENCODEDANCHOR, /* urlencode non-identifier chars instead of replacing
                           with dots */
  MKD_LATEX,            /* handle embedded LaTeX escapes */
  MKD_ALT_AS_TITLE,     /* use alt text as the title if no title is listed */
  /* end of user flags */
  IS_LABEL,
  MKD_NR_FLAGS
};

typedef struct {
  char bit[MKD_NR_FLAGS];
} mkd_flag_t;

void mkd_init_flags(mkd_flag_t* p);

#define is_flag_set(flags, item) ((flags)->bit[item])
#define set_mkd_flag(flags, item) ((flags)->bit[item] = 1)
#define clear_mkd_flag(flags, item) ((flags)->bit[item] = 0)

#define COPY_FLAGS(dst, src) memcpy(&dst, &src, sizeof dst)

void ___mkd_or_flags(mkd_flag_t* dst, mkd_flag_t* src);
int ___mkd_different(mkd_flag_t* dst, mkd_flag_t* src);
int ___mkd_any_flags(mkd_flag_t* dst, mkd_flag_t* src);

#define ADD_FLAGS(dst, src) ___mkd_or_flags(dst, src)
#define DIFFERENT(dst, src) ___mkd_different(dst, src)
#define ANY_FLAGS(dst, src) ___mkd_any_flags(dst, src)

/* each input line is read into a Line, which contains the line,
 * the offset of the first non-space character [this assumes
 * that all tabs will be expanded to spaces!], and a pointer to
 * the next line.
 */
typedef enum {
  chk_text,
  chk_code,
  chk_hr,
  chk_dash,
  chk_tilde,
  chk_backtick,
  chk_equal
} line_type;
typedef struct line {
  Cstring text;
  struct line* next;
  int dle;         /* leading indent on the line */
  int white_space; /* white space count, '\t' = 4 white space ' ' = 1 white space*/
  int has_pipechar; /* line contains a | */
  int is_checked;
  line_type kind;
  int is_fenced;     /* line inside a fenced code block (ick) */
  char* fence_class; /* fenced code class (ick) */
  int count;
  int markdown_offset;
} Line;

enum ParagraphType {
  WHITESPACE = 0,
  CODE,
  QUOTE,
  MARKUP,
  HTML,
  STYLE,
  DL,
  UL,
  OL,
  AL,
  LISTITEM,
  HDR,
  HR,
  TABLE,
  SOURCE
};

enum AlignType { IMPLICIT = 0, PARA, CENTER };

enum { ETX, SETEXT }; /* header types */

/* reference-style links (and images) are stored in an array
 * of footnotes.
 */
typedef struct footnote {
  Cstring tag;   /* the tag for the reference link */
  Cstring link;  /* what this footnote points to */
  Cstring title; /* what it's called (TITLE= attribute) */
                 //  Paragraph *text; /* EXTRA_FOOTNOTE content */

  int height, width; /* dimensions (for image link) */
  int dealloc;       /* deallocation needed? */
  int refnumber;
  int fn_flags;
#define EXTRA_FOOTNOTE 0x01
#define REFERENCED 0x02
} Footnote;

enum BlockType { bTEXT, bSTAR, bUNDER };
typedef struct block {
  int b_type;
  int b_count;
  char b_char;
  Cstring b_text;
  Cstring b_post;
} block;

typedef STRING(block) Qblock;

typedef void (*OnParagraphStart)(int type, void* ud);
typedef void (*OnParagraphText)(struct line* line, void* ud);
typedef void (*OnHeaderNumber)(int hn, void* ud);
typedef void (*OnParagraphAlign)(int align_type, void* ud);
typedef void (*OnListCheck)(int checked, void* ud);
typedef void (*OnListIndex)(int index, void* ud);
typedef void (*OnListExtraLevel)(int level, void* ud);
typedef void (*OnParagraphEnd)(void* ud);
typedef struct callback_data {

  void* ud;
  OnParagraphStart paragraph_start;
  OnParagraphEnd paragraph_end;
  OnParagraphText paragraph_text;
  OnHeaderNumber header_number;
  OnParagraphAlign align;
  OnListCheck list_check;
  OnListIndex list_index;
  OnListExtraLevel list_extra_level;
} Callback_data;

struct escaped {
  char* text;
  struct escaped* up;
};

struct footnote_list {
  int reference;
  STRING(Footnote)
  note;
};

/* a magic markdown io thing holds all the data structures needed to
 * do the backend processing of a markdown document
 */
typedef struct mmiot {
  Cstring out;
  Cstring in;
  Qblock Q;
  char last; /* last text character added to out */
  int isp;
  struct escaped* esc;
  char* ref_prefix;
  struct footnote_list* footnotes;
  mkd_flag_t flags;

  Callback_data* cb;
} MMIOT;

#define MKD_EOLN '\r'

/*
 * the mkdio text input functions return a document structure,
 * which contains a header (retrieved from the document if
 * markdown was configured * with the * --enable-pandoc-header
 * and the document begins with a pandoc-style header) and the
 * root of the linked list of Lines.
 */
typedef struct document {
  int magic; /* "I AM VALID" magic number */
#define VALID_DOCUMENT 0x19600731
  Line* title;
  Line* author;
  Line* date;
  ANCHOR(Line)
  content; /* uncompiled text, not valid after compile() */
           //  Paragraph *code; /* intermediate code generated by compile() */
  int compiled; /* set after mkd_compile() */
  int dirty;    /* flags or callbacks changed */
  int html;     /* set after (internal) htmlify() */
  int tabstop;  /* for properly expanding tabs (ick) */
  char* ref_prefix;
  MMIOT* ctx;       /* backend buffers, flags, and structures */
  Callback_data cb; /* callback functions & private data */
} Document;

/*
 * economy FILE-type structure for pulling characters out of a
 * fixed-length string.
 */
struct string_stream {
  const char* data; /* the unread data */
  int size;         /* and how much is there? */
};

/* the published interface (plus a few local functions that I need to fix)
 */
extern int mkd_firstnonblank(Line*);
extern int mkd_compile(Document*, mkd_flag_t*);
extern int mkd_document(Document*, char**);
extern int mkd_generatehtml(Document*, FILE*);
extern int mkd_css(Document*, char**);
extern int mkd_generatecss(Document*, FILE*);
#define mkd_style mkd_generatecss
extern int mkd_xml(char*, int, char**);
extern int mkd_generatexml(char*, int, FILE*);
extern void mkd_cleanup(Document*);
extern int mkd_line(char*, int, char**, mkd_flag_t*);
extern int mkd_generateline(char*, int, FILE*, mkd_flag_t*);
#define mkd_text mkd_generateline
extern void mkd_basename(Document*, char*);

typedef int (*mkd_sta_function_t)(const int, const void*);
extern void mkd_string_to_anchor(char*, int, mkd_sta_function_t, void*, int,
                                 MMIOT*);

extern Document* mkd_in(FILE*, mkd_flag_t*);
extern Document* mkd_string(const char*, int, mkd_flag_t*);

extern Document* gfm_in(FILE*, mkd_flag_t*);
extern Document* gfm_string(const char*, int, mkd_flag_t*);

extern void mkd_initialize(void);
extern void mkd_shlib_destructor(void);

extern void mkd_ref_prefix(Document*, char*);

/* internal resource handling functions.
 */
extern void ___mkd_freeLine(Line*);
extern void ___mkd_freeLines(Line*);
// extern void ___mkd_freeParagraph(Paragraph *);
extern void ___mkd_freefootnote(Footnote*);
extern void ___mkd_freefootnotes(MMIOT*);
extern void ___mkd_initmmiot(MMIOT*, void*);
extern void ___mkd_freemmiot(MMIOT*, void*);
extern void ___mkd_freeLineRange(Line*, Line*);
extern void ___mkd_xml(char*, int, FILE*);
extern void ___mkd_reparse(char*, int, mkd_flag_t*, MMIOT*, char*);
extern void ___mkd_emblock(MMIOT*);
extern void ___mkd_tidy(Cstring*);

extern Document* __mkd_new_Document(void);
extern void __mkd_enqueue(Document*, Cstring*, int);
extern void __mkd_trim_line(Line*, int);

extern int __mkd_io_strget(struct string_stream*);

/* utility function to do some operation and exit the current function
 * if it fails
 */
#define DO_OR_DIE(op) \
  if ((op) == EOF)    \
    return EOF;       \
  else                \
    1

#endif  // MARKDOWN_THIRD_PARTY_DISCOUNT_DISCOUNT_LITE_MARKDOWN_H_
