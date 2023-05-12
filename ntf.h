/*
    (c) Copyright 1990 through 2005  Tomoyuki Niijima
    All rights reserved
*/

/*
    Programme Modified By Itsuki Hashimoto
        2023-05-12:Add New Japanese Era Name "Reiwa"
*/



#define PROGNAME "ntf 2.233"

#define CMS         0                  /* C/370 */
#define KIKO        0                  /* NON-SHIFT-JIS KANJI */
#define CRLF        1                  /* print \n as \r\n on BINARYOUT mode */
#define MSC4        0                  /* MicroSoftC ver 4 */
#define Human68k    0                  /* Human68k */

#define JIS         0                  /* JIS KANJI-CODE */
                        /* You have to comment a line out from the
                           kinsokutable definition for JIS. */
#define EUC         0                  /* EUC KANJI-CODE */

#include<stdio.h>
#include<stdlib.h>
#if CMS
#include<stdefs.h>
#elif MSC4
#include<malloc.h>
#elif Human68k
#include<io.h>
#else
#include<stddef.h>
#endif
#include<string.h>
#include<ctype.h>
#include<time.h>

#ifdef iskanji
#undef iskanji
#endif

#if EUC
#define iskanji(c) (0x80 & c)
#else
#define iskanji(c) ((0x81<=(0xff&(c)) && 0x9f>=(0xff&(c))) ||\
                    (0xe0<=(0xff&(c)) && 0xfc>=(0xff&(c))))
#endif

#define NTFERR stderr

#if KIKO
/* length of KANJI-IN/OUT escape sequence string */
#if JIS
#define KINL (3)
#define KOUTL (3)
#else
#define KINL (1)
#define KOUTL (1)
#endif                                 /* JIS */
#else
/* don't specify the length */
#define KINL (0)
#define KOUTL (0)
#endif

#if CMS
/* FormFeed and LineFeed / printer control sequence if needed */
#define FF "1"
#define LF " "
/* default mode of PAGE, select from the list below */
#define DEFAULTMODE (CMSTEXT | JUSTIFY | FILLIN)
#else
/* FormFeed and LineFeed / printer control sequence if needed */
#define FF "\f"
#define LF ""
/* default mode of PAGE, select from the list below */
#define DEFAULTMODE (JUSTIFY | FILLIN)
#endif

/* mode of PAGE */
#define TEXT    (1)
#define PRINT   (2)
#define CMSTEXT (4)
#define JUSTIFY (8)
#define FILLIN  (16)
#define FULLLINE (32)
#define CONTENTS (64)
#define BINARYOUT (128)
#define PAGEBUF (256)

/* attributes */
#define UNDERSCORE  (1)
#define HIGHLIGHT   (2)
#define WIDE        (4)
#define TALL        (8)
#define SUPERSCR    (16)
#define SUBSCR      (32)
#define RULE        (64)
#define VRULE       (128)
#define SHADOW      (256)
#define ITALIC      (512)
#define REFERENCE   (32768)


#define WORD struct word
WORD {
  WORD *nextword;
  char *letter;                        /* string of WORD itself */
  int type;                            /* KANJI/ANK/SINGLEWORD */
  int attr;                            /* attribute */
  int prespace;                        /* No. of preceding space */
};

#define SOURCE struct sourcefile
SOURCE {
  SOURCE *lastsource;                  /* pointer to SOURCE used before */
  char *sourcename;                    /* name of SOURCE */
  FILE *fp;                            /* FILE pointer */
  int sourceline;                      /* read line counter */
  long ip;                             /* value of ftell (when needed) */
  char **args;                         /* macro arguments */
};

#define VAR struct variable
VAR {
  VAR *next;
  char *name;
  int value;
};

#define PAGE struct page
PAGE {
  WORD *words;                         /* chain of WORD now processing */
  WORD *lastword;                      /* last WORD of the chain */
  char *inbuf;                         /* buffer for FILE input */
  char *outbuf;                        /* buffer for output */
  int ll;                              /* current line width */
  int pl;                              /* current page length */
  int mode;                            /* current mode */
  char *hl[4];                         /* left part of headers/footers */
  char *hm[4];                         /* mid part */
  char *hr[4];                         /* right part */
  int llc;                             /* charactor counter on current line */
  int plc;                             /* line counter on current page */
  int pnc;                             /* page No. counter */
  int in;                              /* indent depth */
  int ti;                              /* temporary indent depth */
  int ri;                              /* .ri counter */
  int ce;                              /* .ce counter */
  int ju;                              /* justify direction */
  VAR *lvar;                           /* PAGE local variable */
  int attr;                            /* attribute */
  int nattr;                           /* new attribute */
};


#define MAXLL (1024)
#define MAXARGS (20)
#define MACROCASH (40000)

/* type of WORD */
#define KANJI (1)
#define HEAD  (2)
#define SINGLE (4)

#define MACRO struct macrop
MACRO {
  MACRO *nextmacro;
  char *name;
  long entry;
  long end;
};

#define MACHASHN (503)

#define INDEX struct index
#define INDEXP struct indexp
INDEX {
  INDEX *upper;
  INDEX *lower;
  INDEXP *pagep;
  char *name;
};

INDEXP {
  INDEXP *nextpageno;
  int pageno;
  int objno;
};

#define WORKBUFF struct workbuff

WORKBUFF {
  WORKBUFF *nextbuff;
  char *name;
  FILE *fp;
  char *cash;
  int cashp;
  int maxp;
  int buffno;
  int cashsize;
};

/* token of evalexpr */
/* value/10 describes precedence,
   sign means associativity  + : left to right  - : right to left
*/
#define CONSTANT  (0)
#define VARIABLE  (1)
#define CLOSEPARE (10)
#define MISTORE   (-15)
#define PSTORE    (-16)
#define DSTORE    (-17)
#define MUSTORE   (-18)
#define STORE     (-19)
#define LOR       (20)
#define LAND      (30)
#define OR        (40)
#define AND       (50)
#define EQUAL     (60)
#define NEQUAL    (61)
#define GEQUAL    (70)
#define LEQUAL    (73)
#define GREATER   (74)
#define LESS      (75)
#define MINUS     (80)
#define PLUS      (81)
#define DIVIDE    (90)
#define MULTI     (91)
#define REMAIN    (92)
#define LNOT      (-100)
#define NOT       (-101)
#define OPENPARE (110)

#define STACKMAX (100)
#define VSTACK struct tokstack

#define OSTACK struct opestack
VSTACK {
  char *name;
  int value;
  int type;
};

OSTACK {
  int type;
};

#define VARHASHN (503)

struct kinsokumoji {
  char *moji;
  int code;
};

#define op(x,y) (*lp==(x) && *(lp+1)==(y))


#if MAINPGM
#if KIKO
#if JIS
char *kin = "\x1b$@";
char *kout = "\x1b(J";
#else
char *kin = "\x0e";                    /* KANJI-IN shift code */
char *kout = "\x0f";                   /* KANJI-OUT shift code */
#endif                                 /* JIS */
#else
char *kin = "";                        /* don't define for SHIFT-JIS */
char *kout = "";
#endif                                 /* KIKO */
PAGE *cpage, *homepage;
SOURCE *csource, *homesource;
int startpageno = 1, endpageno = 0x7fff;
int commandprefix = '.';
int spacechar = ' ';
int postchar = ' ';
int indentchar = ' ';
char *KJSP = "Å@";                     /* KANJI space string including KIKO */
FILE *outfile;
FILE *errfile;
char *labell;                          /* buffer for header/footer operation */
char *labelm;
char *labelr;
char *jweek[] = {                      /* used in .dy 2 */
  "ì˙Åiì˙Åj",
  "ì˙ÅiåéÅj",
  "ì˙ÅiâŒÅj",
  "ì˙ÅiêÖÅj",
  "ì˙ÅiñÿÅj",
  "ì˙Åiã‡Åj",
  "ì˙ÅiìyÅj"
};

struct kinsokumoji kinsokutable[] = {
  /* table of kinsoku-moji */
  /* KANJI string must include KIKO shift code when KIKO mode */
  /* -1 : don't come to the end of line */
  /* 1,2  : don't come to the top of line */
  {"(", -1}, {")", 1}, {"{", -1}, {"}", 1}, {"<", -1}, {">", 1},
  {"[", -1}, {"]", 1}, {",", 2}, {".", 2}, {"?", 2}, {"!", 2},
  {":", 1}, {";", 1},
  {"Åi", -1}, {"Åj", 1}, {"Åo", -1}, {"Åp", 1},
  {"Åm", -1}, {"Ån", 1}, {"ÅÉ", -1}, {"ÅÑ", 1},
  {"Åu", -1}, {"Åv", 1},
  {"Åw", -1}, {"Åx", 1},
  {"ÅG", 1}, {"ÅF", 1}, {"Å[", 1},
  {"ÅI", 2}, {"ÅD", 2}, {"ÅC", 2}, {"ÅH", 2},
#if JIS
  {"\x1b$@!\"\x1b(J", 2},
#else
  {"ÅA", 2},                           /* comment this line out for JIS code */
#endif
  {"ÅB", 2},
  {"Åh", 2},
  {NULL, 0}                            /* end of the table, don't remove */
};

int debug = 0;
FILE *macro = NULL;
MACRO **macrohtab;
VAR **varhashtab;
INDEX *indexs = NULL;
FILE *contents = NULL;
FILE *pagebuf = NULL;
WORKBUFF *rootbuff = NULL;
int workn = 0;
int buffno = 1;
VSTACK *vstk;
int vstkp = 0;
OSTACK *ostk;
int ostkp = 0;
int barmark = '\t';                    /* joint glue between buffer */
int barchar = '|';                     /* joint charactor */
int headword = 0;
int firstpage = -1;
int it = 0;                            /* interupt line No. to exec .it macro */
char *tmppath = NULL;
int pagec = 0;
int ptype = 0;
int outcode = 0;
int putcc = 0;
int putlc = 0;
int fpkj = 0;
int attrmask = 0;
int pause = 0;
long pagenum = 0;
int pagelog = 0;
int psstr = 0;
int pdfmode = 0;
int pdf_objno = 0;
int pdfkj = -1;
char *jdatestr_heisei = "ïΩê¨%dîN%dåé%d%s\n";
char *jdatestr_reiwa = "óﬂòa%dîN%dåé%d%s\n";
char *kdectbl = "ÅZàÍìÒéOélå‹òZéµî™ã„ñúêÁïSè\\ ";
char *kdec2tbl = "óÎàÎìÛéQélåﬁòZéµî™ã„‰›ËîïSèE";
char *kdec3tbl = "ÇOÇPÇQÇRÇSÇTÇUÇVÇWÇXÇPÇPÇPÇP";
char *numwordtbl[] = {
  "zero",
  "one",
  "two",
  "three",
  "four",
  "five",
  "six",
  "seven",
  "eight",
  "nine",
  "ten",
  "eleven",
  "twelve",
  "thirteen",
  "fourteen",
  "fifteen",
  "sixteen",
  "seventeen",
  "eighteen",
  "nineteen",
  "twenty",
  "thirty",
  "forty",
  "fifty",
  "sixty",
  "seventy",
  "eighty",
  "ninety"
};
char *pplist = NULL;
#else
extern char *kin;
extern char *kout;
extern PAGE *cpage, *homepage;
extern SOURCE *csource, *homesource;
extern int startpageno, endpageno;
extern int commandprefix;
extern int spacechar;
extern int postchar;
extern int indentchar;
extern char *KJSP;
extern FILE *outfile;
extern FILE *errfile;
extern char *labell;
extern char *labelm;
extern char *labelr;
extern char *jweek[7];
extern struct kinsokumoji kinsokutable[];
extern int debug;
extern FILE *macro;
extern MACRO **macrohtab;
extern VAR **varhashtab;
extern INDEX *indexs;
extern FILE *contents;
extern FILE *pagebuf;
extern WORKBUFF *rootbuff;
extern VSTACK *vstk;
extern int vstkp;
extern OSTACK *ostk;
extern int ostkp;
extern int barmark;
extern int barchar;
extern int headword;
extern int firstpage;
extern int it;
extern char *tmppath;
extern int pagec;
extern int ptype;
extern int outcode;
extern int putcc;
extern int putlc;
extern int fpkj;
extern int attrmask;
extern int pause;
extern long pagenum;
extern int pagelog;
extern int psstr;
extern int pdfmode;
extern int pdf_objno;
extern int pdfkj;
extern char *jdatestr_heisei;
extern char *jdatestr_reiwa;
extern char *kdectbl;
extern char *kdec2tbl;
extern char *kdec3tbl;
char *numwordtbl[28];
char *pplist;
#endif

#if Human68k
#ifndef MYU_HEADERS
#define SEEK_SET (0)
#define SEEK_CUR (1)
#define SEEK_END (2)
typedef long time_t;
#endif

#define remove(x) unlink(x)
#endif


/* prototype definition of global functions */

int bputs(char *, FILE *);
char *bgets(char *, int, FILE *);
void brewind(FILE *);
int beof(FILE *);
int bflush(FILE *);
int bseek(FILE *, long, int);
long btell(FILE *);
FILE *openbuff(void);
int closebuff(FILE *);
int closeallbuff(void);
int source(SOURCE *, PAGE *);
int comm(PAGE *);
int func_wl(PAGE *, char *);
int func_ob(PAGE *, char *);
int func_cb(PAGE *, char *);
int func_wb(PAGE *, char *);
int func_ub(PAGE *, char *, int);
int func_nb(PAGE *, char *);
int func_sb(PAGE *, char *);
int func_cw(PAGE *page, char *arg);
WORKBUFF *isbuff(FILE *);
FILE *isbuffno(int);
int func_jb(PAGE *, char *);
int oneline(PAGE *, int);
INDEX *defindex(INDEX *, char *, int);
int putindex(PAGE *, INDEX *);
int func_dy(PAGE *, char *);
int func_hl(PAGE *, char *);
int func_tp(PAGE *);
int func_si(PAGE *, char *);
int setv(int *, char *);
int parselabel(PAGE *, int, char *);
unsigned int macrohash(char *);
MACRO *searchmacro(char *);
int defmacro(PAGE *, char *);
int execmacro(PAGE *, char *, char *);
int parsearg(char **, char *);
int evalarg(PAGE *, char **);
int fatal(char *, int);
char *emalloc(int);
int efree(void *);
int klen(char *, int, int);
char *evallabel(char *, char *);
int putlabel(char *, char *, char *, PAGE *);
int putbuf(PAGE *);
int getbuf(PAGE *);
int putword(WORD *, PAGE *);
int kinsokumode(WORD *);
int checkword(PAGE *, WORD *, WORD *, int);
int justify(PAGE *, WORD *, WORD *, WORD *, int);
int flush(PAGE *);
WORD *makeword(char *, int, int, int, PAGE *, int);
int modechk(PAGE *);
int putline(PAGE *);
SOURCE *opensource(char *, FILE *);
int closesource(SOURCE *, int);
PAGE *openpage(int, int, int);
int closepage(PAGE *);
unsigned int uvarhash(char *);
VAR *searchvar(char *);
int next_token(char *, char *);
int getvar(char *);
int setvar(char *, int);
int pushvn(char *);
int pushv(int);
int popv(void);
int pusho(int);
int popo(void);
int evalop(int);
int evalexpr(char *);
int expandbuf(PAGE *, char *);
int putlnl(PAGE *);
int putlf(PAGE *);
int putff(PAGE *);
char *jstrncpy(char *, char *, int);
char *roman1(int, int, int, int);
int emsg(char *);
int x2b(int);
int callmacro(PAGE *, char *, char *);
int fprintv(FILE *, char *);
int attrchk(char *);
FILE *fopene(char *, char *, char *);
FILE *searchfile(char *, char *, char *, char *);
int kdec(char *p, int v, int mode);
int kdec2(char *p, int v, int mode);
int header(PAGE *page, int head);
int getaline(char *buf, int max, FILE * fp);
int fputks(char *s, FILE * fp, int kj);
int numword(char *buf, int expr, int mode);
int ispp(char *pp, char *p);
