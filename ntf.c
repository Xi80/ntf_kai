/*
    (c) Copyright 1990 trough 1994  Tomoyuki Niijima
    All rights reserved
*/


/*
    Programme Modified By Itsuki Hashimoto
        2023-05-12:Add Initialisation of Variable.
*/

#define MAINPGM 1

#include "ntf.h"
#include <signal.h>

static int buffc = 0, buffpc = 0, filec = 0;
static int debugc = 0;

int buffcount()
{
  WORKBUFF *pp = rootbuff;
  int mc = 0, fc = 0;

  while (pp) {
    if (pp->buffno > 0)
      fc++;
    mc++;
    pp = pp->nextbuff;
  }
  buffpc = (buffpc < mc ? mc : buffpc);
  buffc = (buffc < fc ? fc : buffc);
  return 0;
}

int attrchk(char *s)
{
  int c = 0, d;

  while (*s) {
    if (*s == '\v') {
      sscanf(s + 5, "%04X", &d);
      c |= d;
      s += 9;
    } else
      s++;
  }
  return c;
}

int ecallmacro(FILE * fp, PAGE *page, char *command, char *arg)
{
  char bb[100];

  if (outcode & 4 && cpage->mode & PRINT && psstr) {
    borfputs((psstr == 1 ? ") s\n" : "> s\n"), fp);
    psstr = 0;
  } else if (outcode & 8 && cpage->mode & PRINT && pdfmode >= 1) {
    if (pdfmode >= 2) {
      borfputs((pdfmode == 2 ? ") Tj\n" : "> Tj %%1\n"), fp);
      pdfmode = 1;
    }
    if (pdfmode >= 1)
      borfputs("ET\n", fp);
    pdfmode = 0;
    pdfkj = -1;
  }
  return callmacro(page, command, arg);
}

static int outattr = 0;

int attrmacro(FILE * fp, int c, int ref)
{
  char t[60];

  if (outattr == c)
    return 0;
  sprintf(t, "%d %d %d %d", c, (c & ~outattr), (~c & outattr), ref);
  ecallmacro(fp, cpage, "attr", t);
  outattr = c;
  return 0;
}

unsigned int tojis(char *s)
{
  unsigned int u = (0xff & (*s)), l = (0xff & (*(s + 1)));

#if EUC
  u &= 0x07f;
  l &= 0x07f;
#else
  if (u >= 0xe0)
    u = (u - 0xc1) * 2 + 0x21;
  else
    u = (u - 0x81) * 2 + 0x21;

  if (l >= 0x80)
    --l;
  if ((l -= 0x40) >= 94) {
    ++u;
    l -= 94;
  }
  l += 0x21;
#endif
  *s = u;
  *(s + 1) = l;
  return (u << 8) + l;
}

int borfputs(char *s, FILE * fp)
{
  WORKBUFF *wp;

  wp = isbuff(fp);
  if (wp) {
    if (wp->cash)
      bputs(s, fp);
    else
      fputs(s, wp->fp);
  } else {
    fputs(s, fp);
  }
}

int sheetnum = 1;

int fputks(char *s, FILE * fp, int kj)
{
  char bb[10];

  if (kj) {
    char b[4];

    if (fpkj == 0) {
#if KIKO
      if ((outcode & (4 | 8)) && (cpage->mode & PRINT)) {
        if (pdfmode == 0) {
          borfputs("BT %%1\n", fp);
          pdfmode = 1;
          pdfkj = -1;
        }
        if (pdfkj < 1)
          callmacro(cpage, "KI", "1");
        pdfkj = 1;
      } else
        borfputs(kin, fp);
#else
      if ((outcode & 8) && (cpage->mode & PRINT)) {
        if (pdfmode == 0) {
          borfputs("BT %%2\n", fp);
          pdfmode = 1;
          pdfkj = -1;
        } else if (pdfmode >= 2) {
          borfputs((pdfmode == 2 ? ") Tj\n" : "> Tj %%2\n"), fp);
          pdfmode = 1;
        }
        if (pdfkj < 1) {
          callmacro(cpage, "KI", "2");
        }
        pdfkj = 1;
      } else if (!(outcode & 2) && (cpage->mode & PRINT)) {
        ecallmacro(fp, cpage, "KI", "3");
      }
#endif
    }
    fpkj = -1;
    b[0] = s[0];
    b[1] = s[1];
    b[2] = 0;
#if KIKO
#else
    if ((outcode & 1) && (cpage->mode & PRINT))
      tojis(b);
#endif
    if (debug)
      fprintf(errfile, "fputks<%2.2s> kj=%d\n", b, kj);
    if (outcode & 4 && cpage->mode & PRINT) {
      if (psstr == 0) {
        callmacro(cpage, "TM", NULL);
        borfputs("<", fp);
        psstr = 2;
      }
      sprintf(bb, "%02.2x%02.2x", 0x0ff & b[0], 0x0ff & b[1]);
      borfputs(bb, fp);
    } else if (outcode & 8 && cpage->mode & PRINT) {
      if (pdfmode == 0) {
        borfputs("BT %%3\n", fp);
        pdfmode = 1;
        pdfkj = -1;
      }
      if (pdfkj < 1) {
        callmacro(cpage, "KI", "4");
        pdfkj = 1;
      }
      if (pdfmode <= 1) {
        callmacro(cpage, "TM", NULL);
        borfputs("<", fp);
        pdfmode = 3;
      }
      sprintf(bb, "%02.2x%02.2x", 0x0ff & b[0], 0x0ff & b[1]);
      borfputs(bb, fp);
    } else {
      sprintf(bb, "%2.2s", b);
      borfputs(bb, fp);
    }
    putcc += 2;
    return 2;
  } else {
    if (fpkj) {
#if KIKO
      if ((outcode & 8) && (cpage->mode & PRINT)) {
        if (pdfmode == 0) {
          borfputs("BT %%4\n", fp);
          pdfmode = 1;
          pdfkj = -1;
        }
        callmacro(fp, cpage, "KO", "1");
        pdfkj = 0;
      } else
        borfputs(kout, fp);
#else
      if ((outcode & 8) && (cpage->mode & PRINT)) {
        if (pdfmode == 0) {
          borfputs("BT %%5\n", fp);
          pdfmode = 1;
          pdfkj = -1;
        }
        if (pdfmode >= 2) {
          borfputs((pdfmode == 2 ? ") Tj\n" : "> Tj %%2\n"), fp);
          pdfmode = 1;
          pdfkj = 0;
          callmacro(cpage, "KO", "2");
        }
        pdfkj = -1;
      } else if (outcode & 4 && cpage->mode & PRINT && psstr) {
        borfputs((psstr == 1 ? ") s\n" : "> s\n"), fp);
        psstr = 0;
        callmacro(cpage, "KO", "3");
      } else if (!(outcode & 2) && (cpage->mode & PRINT))
        callmacro(cpage, "KO", "3");
#endif
    }
    fpkj = 0;
    if (debug)
      fprintf(errfile, "fputks<%c> kj=%d\n", *s, kj);
    if (isprint(*s) && (outcode & 2) && (cpage->mode & PRINT))
      if (!isbuff(fp))
        fputc('\0', fp);
#if CRLF
    if ((cpage->mode & BINARYOUT) && *s == '\n' && *(s + 1) == 0)
      borfputs("\r", fp);
#endif
    if (outcode & 4 && cpage->mode & PRINT) {
      if (*s == '\n') {
        if (psstr)
          borfputs((psstr == 1 ? ") s " : "> s "), fp);
        psstr = 0;
        borfputs("LF\n", fp);
      } else if (*s == '\r') {
        if (psstr)
          borfputs((psstr == 1 ? ") s " : "> s "), fp);
        psstr = 0;
        borfputs("CR\n", fp);
      } else if (*s == '\f') {
        if (psstr)
          borfputs((psstr == 1 ? ") s " : "> s "), fp);
        psstr = 0;
        borfputs("showpage\n", fp);
        sheetnum++;
        sprintf(bb, "%%%%Page: %d %d\n", sheetnum, sheetnum);
        borfputs(bb, fp);
        sprintf(bb, "LEFTMARGIN TOPMARGIN moveto SB\n");
        borfputs(bb, fp);
        if (getvar("PSROTATE"))
          fprintf(fp, "90 rotate\n");
      } else {
        if (psstr == 0) {
          borfputs("(", fp);
          psstr = 1;
        }
        if (*s == '(' || *s == ')' || *s == '\\')
          sprintf(bb, "\\%c", *s);
        else
          sprintf(bb, "%c", *s);
        borfputs(bb, fp);
      }
    } else if (outcode & 8 && cpage->mode & PRINT) {
      if (*s == '\n') {
        if (pdfmode >= 2) {
          borfputs((pdfmode == 2 ? ") Tj\n" : "> Tj %%3\n"), fp);
          pdfmode = 1;
          pdfkj = 0;
        } else if (pdfmode == 0) {
          borfputs("BT %%6\n", fp);
          pdfmode = 1;
          pdfkj = -1;
        }
        pdfmode = 1;
/*        callmacro(cpage, "LF", NULL); */
      } else if (*s == '\r') {
        if (pdfmode >= 2)
          borfputs((pdfmode == 2 ? ") Tj\n" : "> Tj %%4\n"), fp);
        else if (pdfmode == 0) {
          borfputs("BT %%7\n", fp);
          pdfkj = -1;
        }
        pdfmode = 1;
        callmacro(cpage, "CR", NULL);
      } else if (*s == '\f') {
        if (pdfmode >= 2)
          borfputs((pdfmode == 2 ? ") Tj\n" : "> Tj %%5\n"), fp);
        if (pdfmode >= 1)
          borfputs("ET\n", fp);
        pdfmode = 0;
        pdfkj = -1;
        sheetnum++;
      } else {
        if (pdfmode == 0) {
          borfputs("BT %%8\n", fp);
          pdfmode = 1;
          pdfkj = -1;
        }
        if (pdfkj != 0) {
          callmacro(cpage, "KO", "4");
          pdfkj = 0;
        }
        if (pdfmode <= 1) {
          callmacro(cpage, "TM", NULL);
          borfputs("(", fp);
        }
        pdfmode = 2;
        pdfkj = 0;
        if (*s == '(' || *s == ')' || *s == '\\')
          sprintf(bb, "\\%c", *s);
        else
          sprintf(bb, "%c", *s);
        borfputs(bb, fp);
      }
    } else {
      sprintf(bb, "%c", *s);
      borfputs(bb, fp);
    }
    if (*s == '\r' || *s == '\n') {
      putcc = 0;
      if (*s == '\n')
        putlc++;
    } else if (*s == '\f') {
      putlc = 0;
    } else
      putcc++;
    return 1;
  }
}


char *emwide(char *s, FILE * fp, int mode, int kj)
{
  if (kj) {
    if (mode) {
      fputks("<", fp, 0);
      fputks(s, fp, kj);
      fputks(">", fp, 0);
    } else {
      fputks("<", fp, 0);
      fputks(" ", fp, 0);
      fputks(" ", fp, 0);
      fputks(">", fp, 0);
    }
    s += 2;
  } else if (*s == ' ')
    s += fputks(s, fp, kj);
  else {
    if (mode) {
      fputks(s, fp, 0);
      fputks(">", fp, 0);
    } else {
      fputks("<", fp, 0);
      fputks(">", fp, 0);
    }
    s++;
  }
  return s;
}

int putattr(char *s, FILE * fp, int mode)
{
  int c = 0, d, kj = 0, talled = 0, ref;

  if (debug) {
    fprintf(errfile, "putattr mode=%04x : ", mode);
    fprintv(errfile, s);
  }
  if (*s != '\n' && (outcode & 2) && (cpage->mode & PRINT))
    ecallmacro(fp, cpage, "KI", NULL);
  while (*s && *s != '\n') {
    WORKBUFF *wp;

    wp = isbuff(fp);
    if (!wp && ferror(fp))
      return EOF;
    if (*s == '\v') {
      sscanf(s + 1, "%04X%04X", &ref, &c);
      s += 9;
      if (cpage->mode & PRINT) {
        if (mode & TALL)
          attrmacro(fp, c & ~(UNDERSCORE | SUBSCR | SUPERSCR | attrmask),
                    ref);
        else if (mode == 0 && (ptype & 2))
          attrmacro(fp, c & ~attrmask, ref);
        else if (mode == 0)
          attrmacro(fp, c & ~(TALL | attrmask), ref);
      }
      continue;
    }
#if KIKO
    if (strncmp(s, kin, KINL) == 0) {
      s += KINL;
      kj = -1;
      continue;
    } else if (strncmp(s, kout, KOUTL) == 0) {
      s += KOUTL;
      kj = 0;
      continue;
    }
#else
    if (iskanji(*s))
      kj = -1;
    else if (!iskanji(*s))
      kj = 0;
#endif
    if (c & WIDE) {
      if (*s == ' ') {
        if (mode & TALL)
          attrmacro(fp, c & ~(UNDERSCORE | SUPERSCR | SUBSCR | WIDE), ref);
        else if (mode == 0)
          attrmacro(fp, c & ~(TALL | WIDE), ref);
      } else {
        if (mode & TALL)
          attrmacro(fp, c & ~(UNDERSCORE | SUPERSCR | SUBSCR), ref);
        else if (mode == 0 && (ptype & 2))
          attrmacro(fp, c, ref);
        else if (mode == 0)
          attrmacro(fp, c & ~TALL, ref);
      }
    }
    if (mode & (RULE | VRULE)) {
      if (!(mode & RULE) && (c & VRULE)) {
        if (cpage->mode & PRINT) {
          if (isalpha(*s)) {
            ecallmacro(fp, cpage, "vrule", NULL);
            putcc++;
          } else
            fputks(" ", fp, 0);
        } else
          fputks(s, fp, 0);
        s++;
      } else if ((mode & RULE) && (c & mode)) {
        if (ptype && (cpage->mode & PRINT)) {
          char t[8];

          if (isalpha(*s) || *s == '_') {
            strcpy(t, "rule_ ");
            t[5] = *s;
            ecallmacro(fp, cpage, t, NULL) == 0;
            putcc++;
          } else if (*s == ' ') {
            fputks(s, fp, 0);
          } else {
            t[0] = *s;
            t[1] = 0;
            ecallmacro(fp, cpage, "rule", t);
          }
        } else {
          fputks(s, fp, 0);
        }
        s++;
      } else {
        if (kj)
          d = ((c & WIDE) ? 4 : 2);
        else
          d = (((c & WIDE) && *s != ' ') ? 2 : 1);
        for (; d > 0; d--)
          fputks(" ", fp, 0);
        s += 1 - kj;
      }
    } else if (mode & UNDERSCORE) {
      if (kj)
        d = (c & WIDE ? 4 : 2);
      else
        d = ((c & WIDE) && *s != ' ' ? 2 : 1);
      for (; d > 0; d--) {
        if (c & UNDERSCORE)
          fputks("_", fp, 0);
        else
          fputks(" ", fp, 0);
      }
      s += 1 - kj;
    } else if (mode & HIGHLIGHT) {
      if (kj)
        d = (c & WIDE ? 4 : 2);
      else
        d = ((c & WIDE) && *s != ' ' ? 2 : 1);
      if ((c & WIDE) || !(c & HIGHLIGHT)) {
        for (; d > 0; d--)
          fputks(" ", fp, 0);
        s += 1 - kj;
      } else {
        s += fputks(s, fp, kj);
      }
    } else if (mode & TALL) {
      if (ptype && (cpage->mode & PRINT)) {
        if (((ptype & 1) ||
             ((ptype & 2) && kj) || (ptype & ~3)) && (c & TALL)) {
          d = fputks(s, fp, kj);
          s += d;
          talled = -1;
          if (c & WIDE && *s != ' ')
            putcc += d;
        } else if ((ptype & 1) && (c & VRULE)) {
          attrmacro(fp, c | TALL, ref);
          ecallmacro(fp, cpage, "vrule", NULL);
          attrmacro(fp, c & ~(UNDERSCORE | SUBSCR | SUPERSCR | attrmask),
                    ref);
          s++;
          putcc++;
        } else if (outcode & 8 && c & VRULE) {
          ecallmacro(fp, cpage, "vrule", NULL);
          s++;
          putcc++;
        } else {
          s += fputks(" ", fp, 0);
          if (c & WIDE && *s != ' ')
            putcc++;
        }
      } else {
        if (c & WIDE) {
          if (c & TALL)
            s = emwide(s, fp, 1, kj);
          else {
            if (kj) {
              fputks(" ", fp, 0);
              fputks(" ", fp, 0);
              s++;
            }
            fputks(" ", fp, 0);
            fputks(" ", fp, 0);
            s++;
          }
        } else {
          if (c & TALL)
            s += fputks(s, fp, kj);
          else
            s += fputks(" ", fp, 0);
        }
      }
    } else {
      if (ptype && (cpage->mode & PRINT)) {
        if (((ptype & 1) ||
             ((ptype & 2) && kj) || (ptype & ~3)) && (c & TALL)) {
          if (kj)
            s += fputks(" ", fp, 0);
          s += fputks(" ", fp, 0);
          if (c & WIDE && *s != ' ')
            putcc += 1 - kj;
        } else if (c & (RULE | VRULE)) {
          char t[8];

          if (isalpha(*s) || *s == '_') {
            strcpy(t, "rule_ ");
            t[5] = *s;
            ecallmacro(fp, cpage, t, NULL);
            putcc++;
          } else if (*s == ' ') {
            fputks(s, fp, kj);
          } else {
            t[0] = *s;
            t[1] = 0;
            ecallmacro(fp, cpage, "rule", t);
          }
          s++;
        } else {
          s += fputks(s, fp, kj);
          if (c & WIDE && *s != ' ')
            putcc += 1 - kj;
        }
      } else {
        if (c & TALL) {
          if (c & WIDE)
            s = emwide(s, fp, 0, kj);
          else
            s += fputks(" ", fp, 0);
        } else {
          if (c & WIDE)
            s = emwide(s, fp, 1, kj);
          else {
            s += fputks(s, fp, kj);
          }
        }
      }
    }
  }
#if CMS
  if ((!mode && *s == '\n') || mode)
    fputks("\n", fp, 0);
  if (mode)
    fputks("+", fp, 0);
#else
  if (mode == TALL && (ptype & 2) && !talled)
    fputks("\n", fp, 0);               /* for ESC/P */
  else if (mode == VRULE)
    fputks("\n", fp, 0);
  else if (mode)
    fputks("\r", fp, 0);
  else if (mode == 0 && *s == '\n')
    fputks("\n", fp, 0);
#endif
  return 0;
}


static halfed = 0;

int eputs(char *s, FILE * fp)
{
  int c, mode;

  mode = attrchk(s);
  if (cpage->mode & 2 && !(outcode & 8)) {
    if (ptype && (mode & VRULE)) {
      ecallmacro(fp, cpage, "halflf", "1");
      halfed = -1;
    } else if (halfed) {
      ecallmacro(fp, cpage, "halflf", "0");
      halfed = 0;
    }
  }
  if (mode & TALL) {
    if ((ptype & 1) && (mode & VRULE)) {        /* for NM9900 and PCPR201 */
      ecallmacro(fp, cpage, "halflf", "2");
      putattr(s, fp, TALL);
      ecallmacro(fp, cpage, "halflf", "1");
    } else {
      if (mode & (VRULE | RULE) && !(outcode & 8))
        putattr(s, fp, (VRULE | RULE));
      if (outcode & (4 | 8))
        fputks("\n", fp, 0);
      putattr(s, fp, TALL);
      if (!(ptype & 3) && !(outcode & (4 | 8)))
        ecallmacro(fp, cpage, "talllf", NULL);  /* for IBM5575 */
    }
  }
  if ((ptype == 0 || (attrmask & UNDERSCORE && cpage->mode & PRINT)) &&
      mode & UNDERSCORE)
    putattr(s, fp, UNDERSCORE);
  if ((ptype == 0 || (attrmask & HIGHLIGHT && cpage->mode & PRINT)) &&
      mode & HIGHLIGHT)
    for (c = 0; c < 4; c++)
      putattr(s, fp, HIGHLIGHT);
  putattr(s, fp, 0);
  if (ptype && (cpage->mode & 2) && (mode & VRULE) && !(outcode & 8)) {
    putattr(s, fp, VRULE);
  }
  return 0;
}

int bputs(char *s, FILE * fp)
{
  if (s && *s) {
    WORKBUFF *wp = isbuff(fp);
    int c;

    if (wp == NULL) {
      if (cpage == homepage && cpage->mode & PAGEBUF) { /* for PDF */
        if (pagebuf == NULL && (pagebuf = openbuff()) == NULL) {
          fatal("Can't open pagebuf.\n", 1);
        }
        fp = outfile;
        outfile = pagebuf;
        wp = isbuff(pagebuf);
        setvar("pagebuf", wp->buffno);
        cpage->mode &= ~PAGEBUF;
        eputs(s, outfile);
        cpage->mode |= PAGEBUF;
        outfile = fp;
        return;
      } else if (!(cpage->mode & CMSTEXT)) {
        return eputs(s, fp);
      } else if (wp == NULL) {
        while (*s) {
          if (*s == '\v')
            s += 9;
          else if (fputc(*s++, fp) == EOF)
            return EOF;
        }
        return 0;
      }
    } else if (wp->cash == NULL)
      return fputs(s, fp);

    if (debug) {
      fprintf(errfile, "wirte on buffer:");
      fprintv(errfile, s);
    }
    c = strlen(s);
    if (wp->cashp + c <= wp->cashsize) {
      strcpy(wp->cash + wp->cashp, s);
      wp->cashp += c;
      wp->maxp = (wp->maxp < wp->cashp ? wp->cashp : wp->maxp);
      return 0;
    } else {
      if (wp->fp == (FILE *) wp) {
        if ((wp->fp = fopen(wp->name, "w+b")) == NULL) {
          fprintf(errfile, "buffer file %s can't open.\n", wp->name);
          fatal("buffer file can't open.", 1);
        }
        if (fp == contents)
          contents = wp->fp;
        else if (fp == macro)
          macro = wp->fp;
        else if (fp == pagebuf)
          pagebuf = wp->fp;
        if (fp == outfile)
          outfile = wp->fp;
        buffcount();
        filec++;
      }
      fputs(wp->cash, wp->fp);
      fputs(s, wp->fp);
      efree(wp->cash);
      wp->cash = NULL;
      wp->cashp = 0;
      wp->maxp = 0;
      return 0;
    }
  } else
    return 0;
}

char *bgets(char *s, int l, FILE * fp)
{
  WORKBUFF *wp = isbuff(fp);

  if (wp == NULL)
    return fgets(s, l, fp);
  if (wp->cash) {
    char *sb = s;
    int p = wp->cashp;

    if (*(wp->cash + p)) {
      while (*(wp->cash + p) != '\n' && *(wp->cash + p))
        *s++ = *(wp->cash + p++);
      if (*(wp->cash + p))
        *s++ = *(wp->cash + p++);
      *s = 0;
      wp->cashp = p;
      return sb;
    }
    return NULL;
  }
  return fgets(s, l, fp);
}

void brewind(FILE * fp)
{
  WORKBUFF *wp = isbuff(fp);

  if (wp && wp->cash)
    wp->cashp = 0;
  else
    rewind(fp);
}

int beof(FILE * fp)
{
  WORKBUFF *wp = isbuff(fp);

  if (wp && wp->cash) {
    if (*(wp->cash + wp->cashp))
      return 0;
    else
      return EOF;
  } else
    return feof(fp);
}

int bflush(FILE * fp)
{
  WORKBUFF *wp = isbuff(fp);

  if (wp && wp->cash) {
    if (wp->fp == (FILE *) wp) {
      if ((wp->fp = fopen(wp->name, "w+b")) == NULL) {
        fprintf(errfile, "buffer file %s can't open.\n", wp->name);
        fatal("buffer file can't open.", 1);
      }
      if (fp == contents)
        contents = wp->fp;
      else if (fp == macro)
        macro = wp->fp;
      else if (fp == outfile)
        outfile = wp->fp;
      buffcount();
      filec++;
    }
    if (*(wp->cash))
      fputs(wp->cash, wp->fp);
    efree(wp->cash);
    wp->cash = NULL;
    wp->cashp = 0;
    wp->maxp = 0;
  }
  return fflush(wp->fp);
}

int bseek(FILE * fp, long l, int o)
{
  WORKBUFF *wp = isbuff(fp);

  if (wp && wp->cash) {
    switch (o) {
    case SEEK_SET:
      wp->cashp = (int) l;
      break;
    case SEEK_CUR:
      wp->cashp += (int) l;
      break;
    case SEEK_END:
      wp->cashp = wp->maxp;
      break;
    }
    return 0;
  } else {
    clearerr(fp);
    return fseek(fp, l, o);
  }
}

long btell(FILE * fp)
{
  WORKBUFF *wp = isbuff(fp);

  if (wp && wp->cash)
    return (long) wp->cashp;
  else
    return ftell(fp);
}

WORKBUFF *buffname()
{
  char *tmp, *name;
  int pathd = '\\', c;
  WORKBUFF *wp = rootbuff;


  while (wp) {                         /* get free WORKBUFF */
    if (wp->buffno == -1)
      break;
    wp = wp->nextbuff;
  }
  if (wp == NULL) {
    wp = (WORKBUFF *) emalloc(sizeof(WORKBUFF));
    wp->cash = emalloc(MAXLL * 2 + 1);
    wp->cashsize = MAXLL * 2 + 1;
    wp->fp = NULL;
    wp->nextbuff = NULL;

#if Human68k
    if (tmp = (tmppath ? tmppath : getenv("temp"))) {
#else
    if (tmp = (tmppath ? tmppath : getenv("TMP"))) {
#endif
      name = emalloc(strlen(tmp) + 20);
      for (c = 0; tmp[c]; c++) {       /* check path delimiter */
        if (tmp[c] == '\\' || tmp[c] == '/') {
          pathd = tmp[c];
          break;
        }
      }
      while (tmp[c])
        c++;                           /* point to the end of string */
      if (tmp[c - 1] == pathd)         /* ended with path delimiter? */
        sprintf(name, "%sntfwork.%03x", tmp, workn);
      else
        sprintf(name, "%s%cntfwork.%03x", tmp, pathd, workn);
    } else {
      name = emalloc(20);
      sprintf(name, "ntfwork.%03x", workn);
    }
    wp->name = name;                   /* give a new name */
    workn = (workn < 0x7ff ? workn + 1 : 0);
  }
  wp->buffno = buffno;                 /* give a new number */
  buffno = (buffno < 0x7fff ? buffno + 1 : 1);
  while (isbuffno(buffno)) {
    if (++buffno == 0x7fff)
      fatal("Too many buffers.", 1);
  }
  *(wp->cash) = 0;
  wp->cashp = 0;
  wp->maxp = 0;
  return wp;
}


/*
 open tmp file, user don't know the name but FILE*
*/

FILE *openbuff(void)
{
  WORKBUFF *wp;
  FILE *fp;

  wp = buffname();
  if (wp->fp == NULL) {
    wp->fp = (FILE *) wp;
    wp->nextbuff = rootbuff;
    rootbuff = wp;
  }
  buffcount();
  return wp->fp;
}

int closebuff(FILE * fp)
{                                      /* close and remove tmp file */
  WORKBUFF *wp = rootbuff, **bp = &rootbuff;

  while (wp) {
    if (wp->fp == fp) {
      if (wp->cash == NULL) {          /* This file has been written. */
        fclose(wp->fp);                /* Remove it */
        if (remove(wp->name)) {
          fprintf(errfile, "filename: %s\n", wp->name);
          fatal("file can't remove.", 1);
        }
        efree(wp->name);
        *bp = wp->nextbuff;
        efree(wp);
        break;
      } else {
        wp->buffno = -1;               /* This file has never been */
        break;                         /* written. So keep it */
      }
    } else {
      bp = &(wp->nextbuff);
      wp = wp->nextbuff;
    }
  }
  return 0;
}

/*
 close and remove tmp file still open
*/
int closeallbuff(void)
{
  WORKBUFF *wp = rootbuff;

  while (wp) {
    WORKBUFF *bp = wp;

    if (wp->fp != (FILE *) wp) {
      fclose(wp->fp);
      remove(wp->name);
    }
    efree(wp->name);
    wp = wp->nextbuff;
    efree(bp);
  }
  rootbuff = NULL;
  return 0;
}


/* this is very important. source() is called from '.so', '.{'
   and execmacro(), and return when file, block and macro ended.
   getbuf() returns true or back the SOURCE pointer chain in
   these cases. */

int source(SOURCE *sp, PAGE *page)
{
  PAGE *mypage, *lastpage = cpage;
  SOURCE *onsource;
  int block = 0;

/*  when PAGE is not specified, open new page anyway.
    line width (ll), page length (pl) and
    page mode are given from that of CurrentPAGE (cpage) */

  if (page == NULL)
    mypage = openpage(cpage->ll, cpage->pl, cpage->mode);
  else {
    mypage = page;
  }
  cpage = mypage;                      /* mypage is current from now on */

  if (sp) {
    if (debug) {
      fprintf(errfile, "source %s(%d:", csource->sourcename,
              csource->sourceline);
      fprintf(errfile, "ip=%lx):", csource->ip);
      fprintf(errfile, " source into %s\n", sp->sourcename);
    }
    sp->lastsource = csource;          /* add to the SOURCE chain */
    csource = sp;
  } else
    block = 1;
  onsource = csource;
  while (1) {
    /* !---- getbuf() back the SOURCE chain */
    int c = 0;  /*FIX:Add Initialisation*/

    if ((onsource != csource) || (c = getbuf(mypage))) {
      if (c == 2 && !block) {
        emsg(".{ was not found for .}");
        continue;
      } else {
        if (page == NULL && cpage != homepage)
          closepage(mypage);
        cpage = lastpage;
        return 0;
      }
    }
    /* like SoftWare tools.... */
    if (mypage->inbuf[0] == commandprefix)
      comm(mypage);                    /* it's command */
    else
      putline(mypage);                 /* it's text */
  }
}

void errhandler()
{
  fatal("sigsegv error.", 9);
}

void ctrl_c_handler()
{
  if (debug == 0)
    closeallbuff();
  fatal("user abort.", 1);
}

#define FILES struct files
FILES {
  FILES *next;
  char *name;
} *froot = NULL;

void pmarg(int argc, char **argv, PAGE *page)
{
  FILE *fp;
  FILES **filep = &froot;
  int c, option = -1;

  for (c = 1; c < argc; c++) {
    if (*argv[c] == '-' && strcmp(argv[c], "-x") && *(argv[c] + 1)) {
      if (strcmp(argv[c], "-d") == 0) {
        debug = -1;
      } else if (strcmp(argv[c], "-v") == 0) {
        fprintf(errfile, "This is %s.\n", PROGNAME);
        exit(0);
      } else if (strcmp(argv[c], "-c") == 0) {
        pause = -1;
      } else if (strcmp(argv[c], "-o") == 0) {
        if (argc > c + 1 && outfile == stdout) {
          if ((fp = fopen(argv[++c], "w")) == NULL)
            fprintf(errfile, "%s can't open.\n", argv[c]);
          else
            outfile = fp;
        } else if (argc > c + 1)
          c++;
      } else if (strcmp(argv[c], "-O") == 0) {
        if (argc > c + 1 && outfile == stdout) {
          if ((fp = fopen(argv[++c], "wb")) == NULL)
            fprintf(errfile, "%s can't open.\n", argv[c]);
          else {
            outfile = fp;
            page->mode |= BINARYOUT;
          }
        } else if (argc > c + 1)
          c++;
      } else if (strcmp(argv[c], "-r") == 0) {
        if (argc > c + 1 && errfile == NTFERR) {
          if ((fp = fopen(argv[++c], "w")) == NULL)
            fprintf(errfile, "%s can't open.\n", argv[c + 1]);
          else {
            errfile = fp;
            pagelog = -1;
          }
        } else if (argc > c + 1)
          c++;
      } else if (strcmp(argv[c], "-b") == 0) {
        if (argc > c + 1)
          startpageno = atoi(argv[++c]);
      } else if (strcmp(argv[c], "-e") == 0) {
        if (argc > c + 1)
          endpageno = atoi(argv[++c]);
      } else if (strcmp(argv[c], "-ju") == 0) {
        page->mode |= JUSTIFY;
        page->ju = -1;
      } else if (strcmp(argv[c], "-p") == 0) {
        page->mode |= PRINT;
        page->mode &= ~CMSTEXT;
      } else if (strcmp(argv[c], "-t") == 0) {
        page->mode |= TEXT;
      } else if (strncmp(argv[c], "-n", 2) == 0 && argv[c][3] == 0) {
        switch (argv[c][2]) {
        case 't':
          page->mode &= ~TEXT;
          break;
        case 'j':
          page->mode &= ~JUSTIFY;
          page->ju = 0;
          break;
        case 'f':
          page->mode &= ~FILLIN;
          break;
        case 'p':
          page->mode &= ~PRINT;
          break;
        }
      } else if (strcmp(argv[c], "-ll") == 0) {
        if (argc > c + 1) {
          int ll = atoi(argv[++c]);

          if (ll > 0)
            page->ll = ll;
          else
            fprintf(errfile, "error: -ll %s\n", argv[c]);
        }
      } else if (strcmp(argv[c], "-pl") == 0) {
        if (argc > c + 1) {
          int pl = atoi(argv[++c]);

          if (pl > 0)
            page->pl = pl;
          else
            fprintf(errfile, "error: -pl %s\n", argv[c]);
        }
      } else if (strcmp(argv[c], "-wk") == 0) {
        if (argc > c + 1 && tmppath == NULL) {
          tmppath = emalloc(strlen(argv[++c]) + 1);
          strcpy(tmppath, argv[c]);
        } else if (argc > c + 1)
          c++;
      } else {
        fprintf(errfile, "error: no such flag %s\n", argv[c]);
        fatal("", 1);
      }
    } else {
      *filep = (FILES *) emalloc(sizeof(FILES));
      (*filep)->next = NULL;
      if (strcmp(argv[c], "-x") == 0) {
        if (argc > c + 1) {
          (*filep)->name = emalloc(strlen(argv[c + 1]) + 5);
          sprintf((*filep)->name, "-x %s\n", argv[c + 1]);
          c++;
        } else {
          fatal("no text for -x.", 1);
        }
      } else {
        (*filep)->name = emalloc(strlen(argv[c]) + 1);
        strcpy((*filep)->name, argv[c]);
      }
      filep = &(*filep)->next;
    }
  }
}

int main(int argc, char **argv)
{
  int c, option = -1;
  SOURCE *sp;
  FILE *fp;
  PAGE *page;
  struct tm *tt;
  time_t ltime;

#ifndef SIG_ERR
#define SIG_ERR ((void (*)()) -1)
#endif

#if SIGSEGV && !Human68k && 0
  if (signal(SIGSEGV, errhandler) == SIG_ERR)
    fatal("can't handle SIGSEGV\n", 1);
#endif

#if SIGINT && !CMS
  if (signal(SIGINT, ctrl_c_handler) == SIG_ERR)
    fatal("can't handle SIGINT\n", 1);
#endif

  macrohtab = (MACRO **) emalloc(sizeof(MACRO *) * MACHASHN);
  for (c = 0; c < MACHASHN; c++)
    macrohtab[c] = (MACRO *) NULL;
  varhashtab = (VAR **) emalloc(sizeof(VAR *) * VARHASHN);
  for (c = 0; c < VARHASHN; c++)
    varhashtab[c] = (VAR *) NULL;
  vstk = (VSTACK *) emalloc(sizeof(VSTACK) * STACKMAX);
  ostk = (OSTACK *) emalloc(sizeof(OSTACK) * STACKMAX);
  labell = emalloc(MAXLL + 1);
  labelm = emalloc(MAXLL + 1);
  labelr = emalloc(MAXLL + 1);

#if !CMS
  setvar("CODE", 1);                   /* code=ANSI */
#endif
  setvar("LANGUAGE", 1);               /* language=JAPANESE */

  time(&ltime);
  tt = localtime(&ltime);
  setvar("year", tt->tm_year);
  setvar("month", tt->tm_mon + 1);
  setvar("day", tt->tm_mday);
  setvar("wday", tt->tm_wday);
  homesource = csource = opensource("unnamed", stdin);
  outfile = stdout;
  errfile = NTFERR;
  cpage = homepage = page = openpage(72, 66, DEFAULTMODE);
  pmarg(argc, argv, page);
  if (froot) {
    while (froot) {

      if (strncmp(froot->name, "-x ", 3) == 0) {
        strcpy(cpage->inbuf, froot->name + 3);
        /* like SoftWare tools.... */
        if (cpage->inbuf[0] == commandprefix)
          comm(cpage);                 /* it's command */
        else
          putline(cpage);              /* it's text */
      } else if ((sp = opensource(froot->name, NULL)) == NULL)
        fprintf(errfile, "%s can't open.\n", froot->name);
      else {
        source(sp, page);
        closesource(sp, -1);
      }
      froot = froot->next;
    }
  } else {
    /* when no input file name specified */
    if (pause)
      fprintf(errfile, "You must specify input file when you use -c.\n");
    else
      source(csource, page);
  }
  closepage(page);
  if (debug) {
    fprintf(errfile,
            "ntf: active=%d total=%d disk=%d workn=%d buffno=%d\n",
            buffc, buffpc, filec, workn, buffno);
    exit(0);
  }
  closeallbuff();
  exit(0);
  return 0;
}
