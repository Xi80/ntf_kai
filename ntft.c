/*
    (c) Copyright 1990 trough 1994  Tomoyuki Niijima
    All rights reserved
*/


#include "ntf.h"


int emsg(char *msg)
{
  SOURCE *wp = csource;
  int c = 0, d;

  if (csource) {
    fprintf(errfile, "%s: %s(%d)", PROGNAME, csource->sourcename,
            csource->sourceline);
    fprintv(errfile, msg);
    while (wp->lastsource && wp->lastsource != homesource) {
      wp = wp->lastsource;
      c++;
      for (d = 0; d < c; d++)
        fprintf(errfile, ">");
      fprintf(errfile, " %s(%d)\n", wp->sourcename, wp->sourceline);
    }
  } else {
    fprintf(errfile, "%s: ", PROGNAME);
    fprintv(errfile, msg);
  }
  return 0;
}


int fatal(char *msg, int no)
/* display error message and exit with error code */
{
  emsg(msg);
  if (cpage) {
    fprintf(errfile, "input buffer: ");
    fprintv(errfile, cpage->inbuf);
  }
  exit(no);
  return 0;
}

static long allocedmemory = 0;
static long allocc = 0;

char *emalloc(int size)
{                                      /* malloc and exit if error */
  unsigned int asize;
  char *m;

  if (size > (~0x0f & size))
    asize = (unsigned) ((size >> 4) + 1) << 4;
  else
    asize = (unsigned) size;
  m = malloc(asize);
  if (m) {
    if (debug)
      fprintf(errfile, "%d bytes allocated. %d times.\n", size, allocc);
    allocedmemory += size;
    allocc++;
    return m;
  } else {
    fprintf(errfile, "total %ld bytes allocated.\n", allocedmemory);
    fatal("no more memory.", 1);
  }
}

int efree(void *p)
{
  free(p);
  allocc--;
  return 0;
}

char *jstrncpy(char *p, char *q, int n)
{
  int c = 0, kj = 0;
  char *t = p;

  while (*q) {
    if (kj == 0 && c == n)
      return t;
#if KIKO
    if (kj && c >= n - 1 - KOUTL) {
      strcpy(p, kout);
      p += KOUTL;
      c += KOUTL;
#else
    if (kj && c >= n - 1) {
#endif
      while (c++ < n)
        *(p++) = ' ';
      return t;
    }
#if KIKO
    if (kj && strncmp(q, kout, KOUTL) == 0) {
      kj = 0;
      strcpy(p, kout);
      p += KOUTL;
      q += KOUTL;
      c += KOUTL;
    }
#else
    if (kj && !iskanji(*q))
      kj = 0;
#endif
    else if (kj) {
      *(p++) = *(q++);
      *(p++) = *(q++);
      c += 2;
#if KIKO
    } else if (strncmp(q, kin, KINL) == 0) {
      if (c >= n - 2 - KINL - KOUTL) {
#else
    } else if (iskanji(*q)) {
      if (c >= n - 2) {
#endif
        while (c++ < n)
          *(p++) = ' ';
        return t;
      }
      kj = -1;
#if KIKO
      strcpy(p, kin);
      p += KINL;
      q += KINL;
      c += KINL;
#endif
    } else {
      *(p++) = *(q++);
      c++;
    }
  }
  *p = 0;
  return t;
}

/* KanjistringLEN */
/* if mode is NON-ZORO count KIKO shift code in */

int klen(char *p, int mode, int attr)
{
  int c, l = 0, w, kj = 0;

  if (p == NULL)
    return 0;
  if (attr & WIDE)
    w = 2;
  else
    w = 1;
  for (c = 0; *(p + c) && *(p + c) != '\n';) {  /* not to count LF */
    if (*(p + c) == '\v') {
      char *s = p + c;

      c += 9;
      sscanf(s + 5, "%04X", &attr);
      if (attr & WIDE)
        w = 2;
      else
        w = 1;
      continue;
    }
#if KIKO
    if (strncmp(p + c, kin, KINL) == 0) {
      c += KINL;
      l += (mode ? KINL : 0);
      kj = -1;
    } else if (strncmp(p + c, kout, KOUTL) == 0) {
      c += KOUTL;
      l += (mode ? KOUTL : 0);
      kj = 0;
    } else
      continue;
#else
    if (iskanji(*(p + c)))
      kj = -1;
    else
      kj = 0;
#endif
    if (kj) {
      if (attr & (SUPERSCR | SUBSCR))
        l++;
      else
        l += 2 * w;
      c += 2;
    } else {
      if (*(p + c) == ' ' || (attr & (SUPERSCR | SUBSCR)))
        l++;
      else
        l += w;
      c++;
    }
  }
  return l;
}


char roman1buf[20];
char *alphalbuf = "0abcdefghijklmnopqrstuvwxyz";
char *alphaubuf = "0ABCDEFGHIJKLMNOPQRSTUVWXYZ";

char *roman1(int v, int c1, int c5, int c10)
{
  int c = 0;

  roman1buf[0] = 0;
  v %= 10;
  if (v >= 1 && v <= 3)
    roman1buf[c++] = c1;
  if (v >= 2 && v <= 3)
    roman1buf[c++] = c1;
  if (v >= 3 && v <= 4)
    roman1buf[c++] = c1;
  if (v >= 4 && v <= 8)
    roman1buf[c++] = c5;
  if (v >= 6 && v <= 8)
    roman1buf[c++] = c1;
  if (v >= 7 && v <= 8)
    roman1buf[c++] = c1;
  if (v >= 8 && v <= 9)
    roman1buf[c++] = c1;
  if (v == 9)
    roman1buf[c++] = c10;
  roman1buf[c++] = 0;
  return roman1buf;
}



/* "\ " at the end of kdectbl is to produce right string
even if you forget to specify -J option. I might be able
to input the KANJI character with hex codes but I didn't,
as I don't like to write codes which depends on some 
character code set. */

int kdec(char *p, int v, int mode)
{
  char q[50];
  int c, d = 0;
  char *kv;

  sprintf(q, "%d", v);
  if (mode)
    kv = kdec3tbl;
  else
    kv = kdectbl;
#if KIKO
  strncpy(p, kin, KINL);
  d += KINL;
#endif
  for (c = 0; c < 50; c++) {
    char t[2];

    if (*(q + c) == 0)
      break;
    *t = *(q + c);
    *(t + 1) = 0;
    strncpy(p + d, kv + KINL + atoi(t) * 2, 2);
    d += 2;
  }
#if KIKO
  strncpy(p + d, kout, KOUTL);
  d += KOUTL;
#endif
  return d;
}


int kdec2(char *p, int v, int mode)
{
  int c, d = 0, e, t = 10000;
  char *kv;

  if (mode)
    kv = kdec2tbl;
  else
    kv = kdectbl;
  if (v <= 0)
    return 0;
#if KIKO
  strncpy(p, kin, KINL);
  d += KINL;
#endif
  for (e = 0; e < 5; e++) {
    if (c = v / t) {
      if (c > 9)
        c = 9;
      if (c > 1 || e == 0 || e == 4) {
        sprintf(p + d, kv + KINL + c * 2);
        d += 2;
      }
      if (e < 4) {
        sprintf(p + d, kv + KINL + 20 + e * 2, 2);
        d += 2;
      }
    }
    v %= t;
    t /= 10;
  }
#if KIKO
  strncpy(p + d, kout, KOUTL);
  d += KOUTL;
#endif
  return d;
}


int numword(char *buf, int expr, int mode)
{
  char tmp[60];

  if (expr <= 20) {
    strcpy(tmp, *(numwordtbl + expr));
    if (mode)
      *tmp = toupper(*tmp);
  } else if (expr <= 99) {
    char *p;

    strcpy(tmp, *(numwordtbl + 18 + (expr / 10)));
    if (mode)
      *tmp = toupper(*tmp);
    if (expr % 10) {
      strcat(tmp, "-");
      p = tmp + strlen(tmp);
      strcat(tmp, *(numwordtbl + (expr % 10)));
      if (mode)
        *p = toupper(*p);
    }
  } else
    *tmp = 0;
  strcpy(buf, tmp);
  return strlen(tmp);
}

/*
   eval header/footer label string
   store result string to buf
   this function also used in .es primitive
 */
char *evallabel(char *buf, char *label)
{
  int c = 0, b = 0, esc = 0;

  *buf = 0;
  if (label == NULL)
    return buf;

  while (*(label + c)) {
    /* escape KANJI charactors it can't be %() sequence */
#if KIKO
    if (strncmp(label + c, kin, KINL) == 0) {
#else
    if (esc == 0 && iskanji(*(label + c))) {
#endif
      esc = -1;
#if KIKO
      strncpy(buf + b, label + c, KINL);
      c += KINL;
      b += KINL;
    } else if (strncmp(label + c, kout, KOUTL) == 0) {
#else
    } else if (esc && !iskanji(*(label + c))) {
#endif
      esc = 0;
#if KIKO
      strncpy(buf + b, label + c, KOUTL);
      c += KOUTL;
      b += KOUTL;
#endif
      /* replace "\\" with "\", this is a quoted one */
    } else if (!esc && strncmp(label + c, "\\\\", 2) == 0) {
      *(buf + b++) = '\\';
      c += 2;
      /* replace "\%" with "%", this is a quoted one */
    } else if (!esc && strncmp(label + c, "\\%", 2) == 0) {
      *(buf + b++) = '%';
      c += 2;
    } else if (!esc && *(label + c) == '%') {   /* % sequence */
      char *tmp;
      int p = 0, ne = 0, strc = *(label + c + 2), type = 0, exp = 0, len =
          0;

      if (*(label + c + 1) == '(') {   /* %(expr) */
        p = c + 1;
        type = 1;
      } else if (isdigit(*(label + c + 1)) && *(label + c + 2) == '(') {
        /* %{len}(expr) */
        len = atoi(label + c + 1);
        if (len == 0)
          len = 10;
        p = c + 2;
        c++;
        type = 1;
      } else if (*(label + c + 1) == 's' && *(label + c + 3) == '('
                 && *(label + c + 2)) {
        /* %s?(expr) */
        p = c + 3;
        c += 2;
        type = 2;
      } else if (*(label + c + 1) == 'r' && *(label + c + 2) == '(') {
        p = c + 2;                     /* %r(expr) */
        c++;
        type = 3;
      } else if (*(label + c + 1) == 'R' && *(label + c + 2) == '(') {
        p = c + 2;                     /* %R(expr) */
        c++;
        type = 4;
      } else if (*(label + c + 1) == 'a' && *(label + c + 2) == '(') {
        p = c + 2;                     /* %a(expr) */
        c++;
        type = 5;
      } else if (*(label + c + 1) == 'A' && *(label + c + 2) == '(') {
        p = c + 2;                     /* %A(expr) */
        c++;
        type = 6;
      } else if (*(label + c + 1) == 't' && *(label + c + 2) == '(') {
        p = c + 2;                     /* %t(expr) */
        c++;
        type = 7;
      } else if (*(label + c + 1) == 'x' && *(label + c + 2) == '(') {
        p = c + 2;                     /* %x(expr) */
        c++;
        type = 8;
      } else if (*(label + c + 1) == 'X' && *(label + c + 2) == '(') {
        p = c + 2;                     /* %X(expr) */
        c++;
        type = 9;
      } else if (*(label + c + 1) == 'k' && *(label + c + 2) == '(') {
        p = c + 2;                     /* %k(expr) */
        c++;
        type = 10;
      } else if (*(label + c + 1) == 'K' && *(label + c + 2) == '(') {
        p = c + 2;                     /* %K(expr) */
        c++;
        type = 11;
      } else if (*(label + c + 1) == 'l' && *(label + c + 2) == '(') {
        p = c + 2;                     /* %l(expr) */
        c++;
        type = 12;
      } else if (*(label + c + 1) == 'L' && *(label + c + 2) == '(') {
        p = c + 2;                     /* %L(expr) */
        c++;
        type = 13;
      } else if (*(label + c + 1) == 'w' && *(label + c + 2) == '(') {
        p = c + 2;                     /* %w(expr) */
        c++;
        type = 14;
      } else if (*(label + c + 1) == 'W' && *(label + c + 2) == '(') {
        p = c + 2;                     /* %W(expr) */
        c++;
        type = 15;
      }

      if (p > 0) {                     /* %(expr) sequence */
        for (; *(label + p); p++) {
          switch (*(label + p)) {      /* check blanket nesting */
          case '(':
            ne++;
            break;
          case ')':
            ne--;
            break;
          }
          if (ne == 0)
            break;                     /* all blankets are closed */
        }
        p++;
        tmp = emalloc(p - c);
        strncpy(tmp, label + c + 1, p - c - 1); /* copy expr string */
        *(tmp + p - c - 1) = 0;
        c = p;
        exp = evalexpr(tmp);
        switch (type) {
        case 1:                       /* %(expr) */
          if (len == 0)
            b += sprintf(buf + b, "%d", exp);   /* copy reslut */
          else {
            char nb[20];

            sprintf(nb, "%010d", exp);
            strcpy(buf + b, nb + 10 - len);
            b += len;
          }
          break;
        case 2:                       /* %s?(expr) */
          while (exp-- > 0 && b < MAXLL)
            *(buf + b++) = strc;
          *(buf + b) = 0;
          break;
        case 3:                       /* %r(expr) */
        case 4:                       /* %R(expr) */
          *(buf + b) = 0;
          if (exp >= 1000) {
            if (type == 3)
              strcat(buf, roman1(exp / 1000, 'm', '?', '?'));
            else
              strcat(buf, roman1(exp / 1000, 'M', '?', '?'));
            exp %= 1000;
          }
          if (exp >= 100) {
            if (type == 3)
              strcat(buf, roman1(exp / 100, 'c', 'd', 'm'));
            else
              strcat(buf, roman1(exp / 100, 'C', 'D', 'M'));
            exp %= 100;
          }
          if (exp >= 10) {
            if (type == 3)
              strcat(buf, roman1(exp / 10, 'x', 'l', 'c'));
            else
              strcat(buf, roman1(exp / 10, 'X', 'L', 'C'));
            exp %= 10;
          }
          if (exp >= 1) {
            if (type == 3)
              strcat(buf, roman1(exp, 'i', 'v', 'x'));
            else
              strcat(buf, roman1(exp, 'I', 'V', 'X'));
          }
          b = strlen(buf);
          break;
        case 5:                       /* %a(expr) */
        case 6:                       /* %A(expr) */
          if (exp > 0 && exp < 27)
            *(buf + b++) = (type == 5 ? *(alphalbuf + exp) :
                            *(alphaubuf + exp));
          *(buf + b) = 0;
          break;
        case 7:                       /* %t(expr) */
          if (homepage->mode & PRINT) {
            exp &= (~attrmask | UNDERSCORE | HIGHLIGHT | 0xffff8000);
            sprintf(buf + b, "\v%08X", exp);
            b += 9;
          }
          break;
        case 8:                       /* %x(expr) */
          sprintf(buf + b, "%02x", exp);
          b += 2;
          break;
        case 9:                       /* %X(expr) */
          sprintf(buf + b, "%02X", exp);
          b += 2;
          break;
        case 10:
          b += kdec(buf + b, exp, 0);
          break;
        case 11:
          b += kdec2(buf + b, exp, 0);
          break;
        case 12:
          b += kdec2(buf + b, exp, 1);
          break;
        case 13:
          b += kdec(buf + b, exp, 1);
          break;
        case 14:
          b += numword(buf + b, exp, 0);
          break;
        case 15:
          b += numword(buf + b, exp, 1);
          break;
        default:
          break;
        }
        efree(tmp);
      } else {                         /* simple % */
        b += sprintf(buf + b, "%d", cpage->pnc);
        c++;
      }
    } else {                           /* normal string */
      if (esc)
        *(buf + b++) = *(label + c++);
      *(buf + b++) = *(label + c++);
    }
  }
  *(buf + b) = 0;
  if (debug)
    fprintf(errfile, "evallabel: %s -> %s\n", label, buf);
  return buf;
}

int putlnl(PAGE *page)
{
  if (page->mode & FULLLINE) {
    int c = page->ll;

    while (c-- > 0)
      bputs(" ", outfile);
  }
  bputs("\n", outfile);
  return 0;
}

int putlf(PAGE *page)
{
  char *p = LF;

  if (page == homepage && (page->mode & PRINT))
    while (*p)
      bputs(p++, outfile);
  return 0;
}

int putff(PAGE *page)
{
  char *p = FF;

  if (page == homepage && (page->mode & PRINT)) {
#if !CMS
    if (pause) {
      fflush(outfile);
      fprintf(stderr, "hit return.");
      while (fgetc(stdin) != '\n');
    } else {
#endif
      if (firstpage)
        firstpage = 0;
      else
        while (*p)
          bputs(p++, outfile);
      if (pagelog) {
        fflush(outfile);
        fprintf(errfile, "### %ld %ld\n", pagenum++, ftell(outfile));
      }
#if !CMS
    }
#endif
  }
  return 0;
}

int putlabel(char *l, char *m, char *r, PAGE *page)
/* putout header/footer lines */
{
  int a, b, c, s1, s2;

  /* eval left part */
  a = klen(evallabel(labell, l), page->mode & CMSTEXT, 0);
  /* middle part */
  b = klen(evallabel(labelm, m), page->mode & CMSTEXT, 0);
  /* right part */
  c = klen(evallabel(labelr, r), page->mode & CMSTEXT, 0);
  s1 = (page->ll - b) / 2 - a;         /* space between left part and middle */
  s2 = page->ll - a - s1 - b - c;      /* space between middle part and right */
  if (s1 >= 0 && s2 >= 0) {            /* every parts dosn't overlap */
    char *p;

    putlnl(page);
    putlf(page);
    putlnl(page);
    putlf(page);
    p = labell + strlen(labell);
    while ((s1--) > 0)
      *p++ = ' ';
    *p = 0;
    strcat(labell, labelm);
    p = labell + strlen(labell);
    while ((s2--) > 0)
      *p++ = ' ';
    *p = 0;
    strcat(labell, labelr);
    strcat(labell, "\n");
    bputs(labell, outfile);
    putlf(page);
    putlnl(page);
    putlf(page);
    putlnl(page);
  } else {
    emsg("Header/Footer is too long.");
    if (s1 < 0 && s2 < 0 && page->ll >= a) {    /* can't but left part only */
      char *p;

      s2 = page->ll - a;
      putlnl(page);
      putlf(page);
      putlnl(page);
      putlf(page);
      p = labell + strlen(labell);
      if (page->mode & FULLLINE)
        while ((s2--) > 0)
          *p++ = postchar;
      *p = 0;
      strcat(labell, "\n");
      bputs(labell, outfile);
      putlf(page);
      putlnl(page);
      putlf(page);
      putlnl(page);
    } else if (page->ll >= a + b) {    /* left and middle part only */
      char *p;

      s1 = page->ll - a - b;
      putlnl(page);
      putlf(page);
      putlnl(page);
      putlf(page);
      p = labell + strlen(labell);
      while ((s1--) > 0)
        *p++ = ' ';
      *p = 0;
      strcat(labell, labelr);
      strcat(labell, "\n");
      bputs(labell, outfile);
      putlf(page);
      putlnl(page);
      putlf(page);
      putlnl(page);
    }
  }
  return 0;
}

int fprintv(FILE * fp, char *p)
{
  int kj = 0, c;

  fprintf(fp, "(%d)<", strlen(p));
  while (*p) {
#if KIKO
    if (strncmp(p, kin, KINL) == 0) {
      kj = -1;
      for (c = 0; c < KINL; c++)
        fputc(*p++, fp);
      continue;
    }
    if (strncmp(p, kout, KOUTL) == 0) {
      kj = 0;
      for (c = 0; c < KOUTL; c++)
        fputc(*p++, fp);
      continue;
    }
#else
    if (iskanji(*p)) {
      fprintf(fp, "%2.2s", p);
      p += 2;
      continue;
    }
#endif

    if (!kj && !isprint(*p))
      fprintf(fp, "\\x%02X", (*p & 0xff));
    else
      fprintf(fp, "%c", *p);
    p++;
  }
  fprintf(fp, ">\n");
  return 0;
}

int header(PAGE *page, int head)
{
  int b;

  if (head) {
    putff(page);
    b = 0;
  } else {
    putlf(page);
    b = 2;
  }
  if ((page->pnc % 2) || page->hl[b + 1] == NULL)
    putlabel(page->hl[b], page->hm[b]
             , page->hr[b], page);
  else
    putlabel(page->hl[b + 1], page->hm[b + 1]
             , page->hr[b + 1], page);
  return 0;
}

static lsc = 0;

int putbuf(PAGE *page)
{
  /* command line option -b and -e can specify start and
     end page. and page No. limit control is effective
     just for HOMEPAGE
   */
  int c, ls = 1, spline = 0;

  /* tate-baikaku adjustment */
  if (!(page->mode & (CMSTEXT | TEXT)) && (attrchk(page->outbuf) & TALL)) {
    int h = 180 / getvar("LPI");

    ls = 2;
    lsc++;
    if (lsc * (h - 24) > h) {
      page->plc--;
      lsc = 0;
    }
  }

  c = 0;
  while (page->outbuf[c]) {
    if (page->outbuf[c] == '\n')
      break;
    else if (page->outbuf[c] == '\v')
      c += 9;
    else if (page->outbuf[c] == ' ')
      c++;
    else
      break;
  }
  if (page->outbuf[c] == '\n' || page->outbuf[c] == 0)
    spline = -1;

  if (!(page->mode & TEXT) && page == homepage
      && (page->pnc < startpageno || page->pnc > endpageno)) {
    if (page->plc == 0)
      page->plc = 5 + ls;
    else if ((page->plc + ls) == page->pl - 5) {        /* page change */
      page->plc = 0;
      page->pnc++;
    } else if ((page->plc + ls) > page->pl - 5) {       /* TALL line */
      page->plc = 7;
      page->pnc++;
    } else
      page->plc += ls;
    if (page->plc == 0) {
      setvar("_sp", 0);
      /* tate-baikaku adjustment counter reset */
      lsc = 0;
    } else {
      if (spline)
        setvar("_sp", getvar("_sp") + 1);
      else
        setvar("_sp", 0);
    }
  } else {
    /* top of the page */
    if (!(page->mode & TEXT)) {
      if (page->plc + ls > page->pl - 5) {
        if (bputs("\n", outfile) == EOF)
          fatal("file write error.", 1);
        header(page, 0);               /* output footer */
        if (page == homepage && outcode & 8 && page->mode & PRINT) {
          page->mode &= ~PAGEBUF;
          bputs("ET\n", pagebuf);
          page->mode |= PAGEBUF;
          pdfmode = 0;
          pdfkj = -1;
        }

        if (page == homepage && page->mode & PAGEBUF) {
          page->mode &= ~PAGEBUF;
          callmacro(page, "FF", NULL);
          page->mode |= PAGEBUF;
          closebuff(pagebuf);
          pagebuf = NULL;
        }
        page->plc = 0;
        page->pnc++;
        /* tate-baikaku adjustment counter reset */
        lsc = 0;
      }
      if (page->plc == 0) {
        header(page, 1);               /* output header */
        page->plc = 5;
        setvar("_sp", 0);
      }
    }

    /* normal line */
    if (debug) {
      fprintf(errfile, "putbuf");
      fprintv(errfile, page->outbuf);
    }
    if (!(page->mode & TEXT) && page->plc == 0)
      setvar("_sp", 0);
    else {
      if (spline)
        setvar("_sp", getvar("_sp") + 1);
      else
        setvar("_sp", 0);
    }

    if (!(page->mode & TEXT))
      page->plc += ls;
    strcat(page->outbuf, "\n");
    if (putlf(page) == EOF || bputs(page->outbuf, outfile) == EOF)
      fatal("file write error.", 1);

    /* bottom of the page */
    if (!(page->mode & TEXT) && page->plc == page->pl - 5) {
      header(page, 0);
      if (page == homepage && outcode & 8 && page->mode & PRINT) {
        page->mode &= ~PAGEBUF;
        bputs("ET\n", pagebuf);
        page->mode |= PAGEBUF;
        pdfmode = 0;
        pdfkj = -1;
      }
      if (page == homepage && page->mode & PAGEBUF) {
        page->mode &= ~PAGEBUF;
        callmacro(page, "FF", NULL);
        page->mode |= PAGEBUF;
        closebuff(pagebuf);
        pagebuf = NULL;
      }
      page->pnc++;
      page->plc = 0;
      setvar("_sp", 0);
    }
  }
  if (page == homepage && it == page->plc && it > 0) {
    int oldbar = barchar;

    callmacro(page, "it", NULL);
    barchar = oldbar;
  }
  return 0;
}

int callmacro(PAGE *page, char *com, char *arg)
{
  int cc = commandprefix, hw = headword;
  char *p = emalloc(MAXLL + 1);
  char *q = emalloc(MAXLL + 1);
  WORD *tmpw = page->words, *tmplw = page->lastword;
  int inb, tib, ceb, rib, llcb, c, rc, attr, nattr;

  page->words = NULL;
  page->lastword = NULL;
  memcpy(p, page->inbuf, MAXLL);
  memcpy(q, page->outbuf, MAXLL);
  commandprefix = '.';
  inb = page->in;
  page->in = 0;
  tib = page->ti;
  page->ti = -1;
  ceb = page->ce;
  page->ce = 0;
  rib = page->ri;
  page->ri = 0;
  llcb = page->llc;
  page->llc = 0;
  attr = page->attr;
  page->attr = 0;
  nattr = page->nattr;
  page->nattr = 0;
  rc = execmacro(page, com, arg);
  page->in = inb;
  page->ti = tib;
  page->ce = ceb;
  page->ri = rib;
  page->llc = llcb;
  page->attr = attr;
  page->nattr = nattr;
  commandprefix = cc;
  memcpy(page->inbuf, p, MAXLL);
  memcpy(page->outbuf, q, MAXLL);
  page->words = tmpw;
  page->lastword = tmplw;
  headword = hw;
  efree(p);
  efree(q);
  return rc;
}

int getaline(char *buf, int max, FILE * fp)
{
  int c = 0, kj = 0, commline = 0;
  char *p = buf;

  if (bgets(p, max, fp) == NULL)
    return -1;
  if (*buf != commandprefix)
    return 0;

  c++;
  while (p[c] == ' ')
    c++;

  while (p[c] && p[c] != '\n') {
#if KIKO
    if (!kj && strncmp(p + c, kin, KINL) == 0) {
      kj = -1;
      c++;
    } else if (kj && strncmp(p + c, kout, KOUTL) == 0) {
      kj = 0;
      c++;
    }
#else
    if (!kj && iskanji(p[c])) {
      kj = -1;
      c += 2;
    } else if (kj && !iskanji(p[c])) {
      kj = 0;
    }
#endif
    else if (kj)
      c += 2;
    else if (!commline && p[c] == ' ')
      commline = -1;
    else {
      int cc = p[c], cn = p[c + 1];

      if (commline && cc == '\\' && (cn == '\n' || cn == 0)) {
        p += c;
        max -= c;
        c = 0;
        if (max < 0)
          fatal("Too long line.", 1);
        if (bgets(p, max, fp) == NULL)
          return -1;
        if (p != buf)
          csource->sourceline++;
      } else
        c++;
    }
  }
  return 0;
}


int getbuf(PAGE *page)
{                                      /* input a line from currnt SOURCE */
  if (getaline(page->inbuf, MAXLL, csource->fp)) {
    /* end of current SOURCE */
    if (debug)
      fprintf(errfile, "getbuf %s(%d): source back to (%s:ip=%lx)\n",
              csource->sourcename, csource->sourceline,
              csource->lastsource->sourcename, csource->lastsource->ip);
    if (csource->lastsource) {         /* back the SOURCE chain */
      csource = csource->lastsource;
      /* reset file pointer of old SOURCE, if needed */
      if (csource->fp == macro || isbuff(csource->fp))
        bseek(csource->fp, csource->ip, SEEK_SET);
    }
    return -1;
  }
  csource->sourceline++;
  /* increment sourceline. sourceline is for error message */
  if (page->inbuf[0] == commandprefix) {
    /* check '.}' or '.}}' end of block or macro */

    char *p = page->inbuf + 1;
    while (*p && (*p == ' ' || *p == '\t'))
      p++;
    if (*p == '}') {
      if (debug) {
        fprintf(errfile, "getbuf(%s(%d)):", csource->sourcename,
                csource->sourceline);
        fprintv(errfile, page->inbuf);
      }
      if (*(p + 1) == '}' && csource->fp == macro && csource->lastsource) {
        /* end of macro. back SOURCE chain */

        if (debug)
          fprintf(errfile,
                  "getbuf %s(%d): source back to (%s:ip=%lx)\n",
                  csource->sourcename, csource->sourceline,
                  csource->lastsource->sourcename,
                  csource->lastsource->ip);
        csource = csource->lastsource;
        if (csource->fp == macro || isbuff(csource->fp))
          bseek(csource->fp, csource->ip, SEEK_SET);
        return 1;                      /* end of macro */
      }
      return 2;                        /* end of block */
    }
  }
  if (debug) {
    fprintf(errfile, "getbuf(%s(%d)):", csource->sourcename,
            csource->sourceline);
    fprintv(errfile, page->inbuf);
  }
  if (csource->fp == macro) {
    /* replace $n sequence in macro statements with it's args */
    evalarg(page, csource->args);
    if (debug) {
      fprintf(errfile, "getbuf/evalarg(%s(%d)):",
              csource->sourcename, csource->sourceline);
      fprintv(errfile, page->inbuf);
    }
  }
  return 0;
}


/*  put a WORD into current PAGE. PAGE will hold them until enough WORDs
    to output one line put, or some primitive occurs line BReak */

int putword(WORD *word, PAGE *page)
{
  int wl;

  if (debug) {
    fprintf(errfile, "<%s>%02x type=%02x attr=%04x prespace=%d\n",
            word->letter, *(word->letter), word->type, word->attr,
            word->prespace);
  }
  /* add WORD to the end of words chain */
  if (page->words == NULL) {
    page->words = word;
  } else {
    page->lastword->nextword = word;
  }
  page->lastword = word;
  /* normaly WORD doesn't hold KANJI and non-KANJI at the same time
     except SINGLE word created by .si command */
/* to be deleted.
    if (word->type & SINGLE)
        wl=klen(word->letter,page->mode & CMSTEXT,wp->attr);
    else wl=strlen(word->letter);
    if (page->llc+wl > page->ll+4 )
        flush(page);
    else page->llc+=wl;
*/
  return 0;
}

/* check a WORD is kinsokumoji or not */

int kinsokumode(WORD *word)
{
  /* and return kinsokumode */
  char p[10];
  struct kinsokumoji *q = kinsokutable;
  *p = 0;

  /* SUPERSCR or SUBSCR word with -l prespace length
     should be treated as kinsokumoji
   */
  if ((word->attr & (SUPERSCR | SUBSCR)) && word->prespace < 0)
    return 1;

  /* kinsokumoji must be a single KANJI or non-KANJI charactor.
     For expamle, ".cc" is not a kinsokumoji
     even if '.' is a kinsokumoji.
   */
  if (word->type & KANJI) {
    if (strlen(word->letter) > 2)
      return 0;
    /* add KIKO shift code to compare with the charactors
       in kinsokutable */
    strcat(p, kin);
    strcat(p, word->letter);
    strcat(p, kout);
  } else {
    if (strlen(word->letter) > 1)
      return 0;
    *p = *(word->letter);
    *(p + 1) = 0;
  }
  while (q->moji) {
    if (strcmp(q->moji, p) == 0)
      return q->code;                  /* return kinsokumode */
    q++;
  }
  return 0;
}

/* check a WORD in WORD chain, if the length between the WORD (wp) and
   the WORD before (lwp) can be increased */

int checkword(PAGE *page, WORD *lwp, WORD *wp, int sp)
{
  /* In CMSTEXT mode, KIKO shift code needs space equal to
     one single bytes charactor on screen. So it must be
     very careful when you separete one KANJI string into
     two strings.
   */

  /* prespace=0 means that space can be added when needed.
     This policy has been changed to be limited as with
     the case when the word follows KUTOUTEN charactor.
   */

  if ((wp->prespace > 0 && wp->prespace != 1) ||
      (wp->prespace == 1 &&
       !(lwp->letter[1] == 0 &&
         (lwp->letter[0] == '.' ||
          lwp->letter[0] == '?' || lwp->letter[0] == '!')
       )
      ) ||
      (wp->prespace == 0 &&
       (lwp->type & KANJI) && kinsokumode(lwp) > 1 &&
       !(page->mode & CMSTEXT))
      ) {
    wp->prespace++;
    sp--;
  }
  if (wp->prespace == 0 && (wp->type & KANJI) && (page->mode & CMSTEXT)) {
    if (sp >= 3) {
      /* 3 means KO code, one blank and KI code */
      wp->prespace = 1;
      sp -= 3;
    } else if (sp >= 2) {
      char *p;

      p = emalloc(strlen(wp->letter) + 3);
      strncpy(p, KJSP + KINL, 2);
      strcpy(p + 2, wp->letter);
      efree(wp->letter);
      wp->letter = p;
      sp -= 2;
    }
  }
  return sp;
}

/* justify() check the WORDs in the chain one by one, and
   add some spaces between them to adjust output line length
   equal to ll (LineLength) of current PAGE.
   PAGE->ju holds the direction of this operation.
   justify() is used recursively.
*/

int justify(PAGE *page, WORD *lwp, WORD *wp, WORD *ewp, int sp)
{
  if (sp <= 0 || wp == NULL)
    return sp;
  if (page->ju > 0) {                  /* forward direction */
    if (wp->prespace >= 0)
      sp = checkword(page, lwp, wp, sp);
    if (wp == ewp)
      return sp;
    else
      return justify(page, wp, wp->nextword, ewp, sp);
  } else {                             /* backward */
    if (wp->nextword != NULL && wp != ewp)
      sp = justify(page, wp, wp->nextword, ewp, sp);
    if (sp > 0 && wp->prespace >= 0)
      return checkword(page, lwp, wp, sp);
    return sp;
  }
}

int setattr(PAGE *page, int llc, int attr, int prespace)
{
  int c = prespace;

  if (page->mode & CMSTEXT) {
    for (; c > 0; c--)
      page->outbuf[llc++] = spacechar;
    return llc;
  }
  if ((page->attr == attr) && (page->attr & (VRULE | RULE))) {
    sprintf(page->outbuf + llc, "\v%08X", page->attr & ~(RULE | VRULE));
    llc += 9;
    for (; c > 0; c--)
      page->outbuf[llc++] = spacechar;
    sprintf(page->outbuf + llc, "\v%08X", attr);
    llc += 9;
  } else if (page->attr == attr)
    for (; c > 0; c--)
      page->outbuf[llc++] = spacechar;
  else if ((page->attr & attr) == attr) {
    sprintf(page->outbuf + llc, "\v%08X", attr);
    llc += 9;
    for (; c > 0; c--)
      page->outbuf[llc++] = spacechar;
  } else if ((page->attr & attr) == page->attr) {
    for (; c > 0; c--)
      page->outbuf[llc++] = spacechar;
    sprintf(page->outbuf + llc, "\v%08X", attr);
    llc += 9;
  } else {
    sprintf(page->outbuf + llc, "\v%08X", (page->attr & attr));
    llc += 9;
    for (; c > 0; c--)
      page->outbuf[llc++] = spacechar;
    sprintf(page->outbuf + llc, "\v%08X", attr);
    llc += 9;
  }
  page->attr = attr;
  return llc;
}

int ispp(char *pp, char *p)
{
  char *buf;
  int rc;

  if (!pplist)
    return 0;
  buf = emalloc(strlen(pp) + strlen(p) + 3);
  strcpy(buf, " ");
  strcat(buf, pp);
  strcat(buf, p);
  strcat(buf, " ");
  rc = strstr(pplist, buf) ? 1 : 0;
  free(buf);
  return rc;
}

int flush(PAGE *page)
{                                      /* flush out one line from PAGE */
  WORD *wp = page->words, *wpb = page->words, *wpbb = page->words, *bwp;
  int kj = 0, llc = 0, llo = 0, EOL = 0, kil = 0, kol = 0, llcb =
      0, llcbb = 0, ll, in;
  int c, period = 0;

  if (page->mode & CMSTEXT) {          /* shift code length. default 0 */
    kil = KINL;
    kol = KOUTL;
  }
  if (wp == NULL) {                    /* PAGE is empty */
    llc = 0;
    if (page->mode & FULLLINE) {
      llc = setattr(page, llc, page->nattr, 0);
      for (c = 0; c < page->ll; c++)
        page->outbuf[llc++] = postchar;
      llc = setattr(page, llc, 0, 0);
    }
    page->outbuf[llc] = 0;
    putbuf(page);
    page->ti = -1;                     /* reset temporary indent mark */
    return 0;
  }
  if (page->ti >= 0) {                 /* temporary indent */
    ll = page->ll - page->ti;
    in = page->ti;
    page->ti = -1;
  } else {
    ll = page->ll - page->in;
    in = page->in;
  }

  /* left justify if the WORD is not a HEAD of paragraph */
  if (!(wp->type & HEAD))
    wp->prespace = -1;

  bwp = wp;
  while (wp) {                         /* count up line length exactly */
    int ckj = (wp->type & KANJI);
    int t;

    if (wp->type & SINGLE)
      t = klen(wp->letter, page->mode & CMSTEXT, wp->attr);
    /* SINGLE WORD sometimes holds KIKO code in it */
    else {
      t = strlen(wp->letter);
      if (wp->attr & (SUPERSCR | SUBSCR)) {
        if (wp->type & KANJI)
          t /= 2;
      } else if (wp->attr & WIDE)
        t *= 2;
    }
    llc += t;
    llo += t;

    if (wp->prespace > 0) {
      if (wp->prespace > 0 &&
          page->words != wp && !(wp->attr & (RULE | VRULE)) &&
          (page->mode & FILLIN)) {
        if (period) {
          llc += 2;
          wp->prespace = 2;
        } else
          llc += 1;
      } else
        llc += wp->prespace;
      llo += wp->prespace;
    }
    if (*(wp->letter + 1) == 0 &&
        (*(wp->letter) == '.' ||
         *(wp->letter) == '!' ||
         *(wp->letter) == '?') && !ispp(bwp->letter, wp->letter)) {
      period = -1;
    } else
      period = 0;
    if (kj && wp->prespace > 0)
      kj = 0;
    if (!kj && ckj) {
      llc += kil + kol;
      llo += kil + kol;
    }
    kj = ckj;
    if ((llc > ll) || (!(page->ju) && llo > ll))
      break;
    if (wp->nextword == NULL ||
        (kinsokumode(wp) >= 0 &&
         wp->nextword && kinsokumode(wp->nextword) <= 0)) {
      wpbb = wpb;
      wpb = wp;
      llcbb = llcb;
      llcb = llc;
    }
    bwp = wp;
    wp = wp->nextword;
  }

  if (page->ju && llo > ll && page->words != wpb) {
    /* llo>ll means there's following WORD in PAGE.
       if not, this is the end line of a paragraph.
       So, need not to be justified. */
    int sp = ll - llcb, spb;

    wp = page->words->nextword;
    bwp = wp;
    if (wp != NULL) {
      int period = 0;

      while (wp) {                     /* remove meaning-less blank */
        if (wp->prespace > 1 && !period)
          wp->prespace = 1;
        if (wp == wpb)
          break;
        if (*(wp->letter + 1) == 0 &&
            (*(wp->letter) == '.' ||
             *(wp->letter) == '?' ||
             *(wp->letter) == '!') && !ispp(bwp->letter, wp->letter))
          period = -1;
        else
          period = 0;
        bwp = wp;
        wp = wp->nextword;
      }
      wp = page->words->nextword;
      while (sp > 0) {                 /* right-justify */
        spb = justify(page, page->words, wp, wpb, sp);
        if (debug)
          fprintf(errfile, "justify(%d)->%d\n", sp, spb);
        if (sp == spb)
          break;
        sp = spb;
        page->ju *= -1;
      }
    }
  } else if (page->ju && page->words == wpb &&
             wpb->prespace + klen(wpb->letter, page->mode & CMSTEXT,
                                  0) > ll) {
    wpb->prespace = ll - klen(wpb->letter, page->mode & CMSTEXT, 0);
    /* there is only a WORD but too many blank at the top */
    if (wpb->type & KANJI)
      wpb->prespace -= kil;
  }

  /* construct output string from the WORD chain */
  kj = 0;
  llc = 0;
  if (in > 0)
    for (c = 0; c < in; c++)
      *(page->outbuf + (llc++)) = indentchar;

  wp = page->words;
  while (!EOL) {
    int ckj = (wp->type & KANJI);
    int c;

    if (kj && (wp->prespace > 0 ||
               (!(page->mode & CMSTEXT) && wp->attr != page->attr))) {
      kj = 0;
      for (c = 0; kout[c]; c++)
        page->outbuf[llc++] = kout[c];
    }
    if (!kj && ckj) {
      llc = setattr(page, llc, wp->attr, wp->prespace);
      for (c = 0; kin[c]; c++)
        page->outbuf[llc++] = kin[c];
      for (c = 0; wp->letter[c]; c++)
        page->outbuf[llc++] = wp->letter[c];
    } else if (kj && !ckj) {
      for (c = 0; kout[c]; c++)
        page->outbuf[llc++] = kout[c];
      llc = setattr(page, llc, wp->attr, wp->prespace);
      for (c = 0; wp->letter[c]; c++)
        page->outbuf[llc++] = wp->letter[c];
    } else {
      llc = setattr(page, llc, wp->attr, wp->prespace);
      for (c = 0; wp->letter[c]; c++)
        page->outbuf[llc++] = wp->letter[c];
    }
    if (wp == wpb) {
      EOL = -1;
      if (ckj) {
        strncpy(page->outbuf + llc, kout, KOUTL);
        llc += KOUTL;
      }
    }
    page->outbuf[llc] = 0;
    wpbb = wp;
    wp = wp->nextword;
    efree(wpbb->letter);
    efree(wpbb);
    page->words = wp;
    kj = ckj;
  }
  if (page->mode & FULLLINE) {
    int kl = klen(page->outbuf, page->mode & CMSTEXT, 0);

    while ((kl++) < page->ll) {
      page->outbuf[llc++] = postchar;
    }
    page->outbuf[llc] = 0;
  }
  llc = setattr(page, llc, 0, 0);
  page->outbuf[llc] = 0;
  if (debug) {
    fprintf(errfile, "wrap: ");
    fprintv(errfile, page->outbuf);
  }
  /* output the line */
  putbuf(page);
  /* count up rough length of the WORDs left in PAGE */
  page->llc = 0;
  page->lastword = page->words;
  for (wp = page->words; wp != NULL; wp = wp->nextword) {
    page->lastword = wp;
/* to be deleted.
        if (wp->type & SINGLE)
            page->llc+=klen(wp->letter,page->mode & CMSTEXT,0);
        else
            page->llc+=strlen(wp->letter);
*/
  }
}


/* cut out a string from inbuf and create a WORD */
WORD *makeword(char *p, int c, int b, int prespace, PAGE *page, int attr)
{
  WORD *word;

  word = (WORD *) emalloc(sizeof(WORD));
  word->letter = emalloc(c - b + 1);
  word->type = 0;
  word->attr = attr;
  if (headword) {
    int t = 0;

    while (page->inbuf[t]) {
      if (page->inbuf[t] == '\v')
        t += 9;
      else
        break;
    }
    if (b && b != t) {
      word->type |= HEAD;
      while (page->words)
        flush(page);
    } else if (page->words) {
      if ((page->lastword->attr & (RULE | VRULE)) &&
          (attr & (RULE | VRULE)) && prespace == 0)
        prespace = -1;
      else if (page->lastword->type & KANJI)
        prespace = 0;
      else
        prespace = 1;
    } else
      prespace = 1;
    headword = 0;
  }
  word->nextword = NULL;
  if ((word->attr & (SUPERSCR | SUBSCR)) && (page->lastword) &&
      !(page->lastword->attr & (SUPERSCR | SUBSCR)))
    word->prespace = -1;
  else
    word->prespace = prespace;
  strncpy(word->letter, p + b, c - b);
  *(word->letter + c - b) = 0;
  return word;
}

int modechk(PAGE *page)
{
  if (page->mode & CONTENTS) {         /* copy a line as CONTENTS infomation */
    char *p = page->outbuf;

    sprintf(page->outbuf, "%05d %04X %s", homepage->pnc, pdf_objno,
            page->inbuf);
    while (*p != '\n' && *p)
      p++;
    *p = 0;
    strcat(page->outbuf, " \n");
    if (bputs(page->outbuf, contents) == EOF)
      fatal("contents file write error.", 1);
  }
  if (!(page->mode & FILLIN)) {        /* break the line not to fill in */
    while (page->words)
      flush(page);
  }
  if (page->ri > 0) {                  /* right justify this line */
    int t, r;

    r = page->ll - klen(page->inbuf, page->mode & CMSTEXT, page->nattr);
    while (page->words)
      flush(page);                     /* break the line first */
    page->ri--;
    if (r >= 0) {                      /* adjust the leading space and output */
      int il = strlen(page->inbuf) - 1;

      page->inbuf[il] = 0;
      for (t = 0; t < r; t++)
        *(page->outbuf + t) = ' ';
      sprintf(page->outbuf + r, "\v%08X", page->nattr);
      strcpy(page->outbuf + 9 + r, page->inbuf);
      sprintf(page->outbuf + 9 + r + il, "\v%08X", 0);
      page->attr = 0;
      putbuf(page);
      return -1;
    }
  } else if (page->ce > 0) {           /* centering this line */
    int t, r;
    int l;

    if (page->ti >= 0)
      t = page->ti;
    else
      t = page->in;
    page->ti = -1;
    r = page->ll - t - klen(page->inbuf, page->mode & CMSTEXT,
                            page->nattr);
    l = r / 2;
    r = r - l + t;
    while (page->words)
      flush(page);                     /* line break */
    page->ce--;
    if (r >= 0) {
      int il = strlen(page->inbuf) - 1;

      for (t = 0; t < r; t++)
        *(page->outbuf + t) = ' ';     /* leading space */
      sprintf(page->outbuf + r, "\v%08X", page->nattr); /* set attr */
      page->inbuf[il] = 0;             /* delete LF code */
      strcpy(page->outbuf + 9 + r, page->inbuf);
      if (page->mode & FULLLINE) {     /* post spaces if needed */
        char *p = page->outbuf + 9 + r + il;

        for (; l > 0; l--) {
          *p = ' ';
          p++;
        }
        *p = 0;
      }
      strcat(page->outbuf, "\v00000000");       /* reset attr */
      page->attr = 0;
      putbuf(page);
      return -1;
    }
  }
  return 0;
}

/* put a line into PAGE. separate a line into WORDs and put them
into WORD chain of the PAGE */

int putline(PAGE *page)
{
  int c = 0, kj = 0, b = 0, prespace = 0, attr = page->nattr, nosp = 0;
  char *p = page->inbuf;
  WORD *word;

  headword = -1;

  c = 0;
  kj = 0;
  while (*(p + c)) {                   /* replace '\t' with ' ' */
    if (kj) {
#if KIKO
      if (strncmp(p + c, kout, KOUTL) == 0) {
        kj = 0;
        c += KOUTL;
#else
      if (!iskanji(*(p + c))) {
        kj = 0;
#endif
      } else {
        c += 2;
      }
    } else {
#if KIKO
      if (strncmp(p + c, kin, KINL) == 0) {
        kj = -1;
        c += KINL;
#else
      if (iskanji(*(p + c))) {
        kj = -1;
#endif
      } else if (*(p + c) == '\t')
        *(p + c) = ' ';
      else
        c++;
    }
  }

  if (modechk(page))
    return 0;

  kj = 0;
  c = 0;
  while (*(p + c)) {                   /* normal case */
    int cc = *(p + c);

    if (kj) {
#if KIKO
      if (strncmp(p + c, kout, KOUTL) == 0) {
        if (strncmp(p + c + KOUTL, kin, KINL) == 0) {
          c += KOUTL + KINL;
          b = c;
        } else {
          kj = 0;
          c += KOUTL;
          b = c;
          if (prespace < 0)
            prespace = 0;
        }
        continue;
      }
#else
      if (!iskanji(cc)) {
        kj = 0;
        if (prespace < 0)
          prespace = 0;
        continue;
      }
#endif
      else if (*(p + c + 1) == 0)
        break;                         /* Broken KANJI character */
      else {                           /* KANJI WORD */
        if (!(page->mode & CMSTEXT) && strncmp(p + c, KJSP + KINL, 2) == 0) {
          c += 2;
          b = c;
          if (prespace > 0)
            prespace += 2;
          else
            prespace = 2;
          continue;
        }
        word = (WORD *) emalloc(sizeof(WORD));
        word->letter = emalloc(3);
        word->type = 0;
        word->type |= KANJI;
        if ((c > KINL && headword) ||
            (c == KINL && strncmp(p + c, KJSP + KINL, 2) == 0)) {
          word->type |= HEAD;
          while (page->words)
            flush(page);
        } else if (headword &&
                   page->lastword != NULL &&
                   (page->lastword->type & KANJI) &&
                   kinsokumode(page->lastword) < 2)
          prespace = -1;

        word->nextword = NULL;
        *(word->letter) = *(p + (c++));
        *(word->letter + 1) = *(p + (c++));
        *(word->letter + 2) = 0;
        word->attr = attr;
        if (kinsokumode(word) > 0 && prespace == 0)
          prespace = -1;
        word->prespace = prespace;
        putword(word, page);
        nosp = -1;
        headword = 0;
        b = c;
        if (kinsokumode(word) > 1)
          prespace = 0;
        else
          prespace = -1;
      }
    } else {
#if KIKO
      if (strncmp(p + c, kin, KINL) == 0) {
        if (b != c) {
          putword(makeword(p, c, b, prespace, page, attr), page);
          prespace = 0;
          nosp = -1;
        }
        kj = -1;
        c += KINL;
        b = c;
        continue;
      }
#else
      if (iskanji(cc)) {
        if (b != c) {
          putword(makeword(p, c, b, prespace, page, attr), page);
          prespace = 0;
          nosp = -1;
        }
        kj = -1;
        b = c;
        continue;
      }
#endif
      else {
        switch (cc) {
        case '(':                     /* delimiters */
        case ')':
        case '{':
        case '}':
        case '[':
        case ']':
        case '<':
        case '>':
        case ',':
        case '.':
        case '!':
        case '?':
        case ' ':
        case '\n':
        case '\v':
          if (cc == '\v') {            /* attribute set */
            if (b != c) {
              putword(makeword(p, c, b, prespace, page, attr), page);
              b = c;
              prespace = 0;
              nosp = -1;
            }
            sscanf(p + c + 1, "%08X", &attr);
            c += 9;
            b = c;
          } else if ((cc == ' ') && b == c) {
            prespace++;
            c++;
            b = c;
          } else {
            if (b != c) {
              putword(makeword(p, c, b, prespace, page, attr), page);
              b = c;
              prespace = 0;
              nosp = -1;
            }
            if (cc != ' ' && cc != '\n') {
              c++;
              if ((cc == ')' || cc == '}' ||
                   cc == ']' || cc == '>' ||
                   cc == '.' || cc == ',' ||
                   cc == ';' || cc == ':' ||
                   cc == '?' || cc == '!') && prespace == 0)
                prespace = -1;
              putword(makeword(p, c, b, prespace, page, attr), page);
              b = c;
              prespace = 0;
              nosp = -1;
            }
            if (cc == '\n' && nosp == 0) {
              while (page->words)
                flush(page);
              flush(page);
              return 0;
            }
            if (cc == '\n')
              b = ++c;
          }
          break;
        default:
          c++;
          break;
        }
      }
    }
  }
}

SOURCE *opensource(char *sourcename, FILE * fp)
{
  SOURCE *so;
  FILE *sp;

  if (strcmp(sourcename, "-") == 0)
    fp = stdin;
  if (fp == NULL) {
#if CMS
    if (*sourcename == '@')
      sourcename++;
#else
    if (*sourcename == '@') {
      if ((sp = fopene(sourcename + 1, "r", NULL)) == NULL) {
        fprintf(errfile, "file %s can't find.\n", sourcename + 1);
        fatal("can't open file.", 1);
      }
    } else
#endif
    if ((sp = fopen(sourcename, "r")) == NULL) {
      fprintf(errfile, "file %s can't open.\n", sourcename);
      fatal("can't open file.", 1);
    }
  } else
    sp = fp;
  so = (SOURCE *) emalloc(sizeof(SOURCE));
  so->sourcename = emalloc(strlen(sourcename) + 1);
  strcpy(so->sourcename, sourcename);
  so->sourceline = 0;
  so->fp = sp;
  so->ip = btell(so->fp);
  so->args = NULL;
  so->lastsource = NULL;
  return so;
}

int closesource(SOURCE *sp, int mode)
{
  if (mode)
    fclose(sp->fp);
  efree(sp->sourcename);
  efree(sp);
  return 0;
}


PAGE *openpage(int ll, int pl, int mode)
{
  PAGE *page;

  page = (PAGE *) emalloc(sizeof(PAGE));
  page->inbuf = emalloc(MAXLL + 1);
  page->outbuf = emalloc(MAXLL + 1);
  page->inbuf[0] = 0;
  page->outbuf[0] = 0;
  page->words = NULL;
  page->lastword = NULL;
  page->ll = ll;
  page->pl = pl;
  page->mode = mode;
  page->hl[0] = NULL;
  page->hm[0] = NULL;
  page->hr[0] = NULL;
  page->hl[1] = NULL;
  page->hm[1] = NULL;
  page->hr[1] = NULL;
  page->hl[2] = NULL;
  page->hm[2] = NULL;
  page->hr[2] = NULL;
  page->hl[3] = NULL;
  page->hm[3] = NULL;
  page->hr[3] = NULL;
  page->llc = 0;
  page->plc = 0;
  page->pnc = 1;
  page->in = 0;
  page->ti = -1;
  page->ri = 0;
  page->ce = 0;
  if (mode & JUSTIFY)
    page->ju = -1;
  else
    page->ju = 0;
  page->lvar = (VAR *) NULL;
  page->attr = 0;
  page->nattr = 0;
  pagec++;
  return page;
}

int closepage(PAGE *page)
{
  VAR *wp = page->lvar;
  int c = 0;

  while (page->words)
    flush(page);
  if (!(page->mode & TEXT) && page->plc != 0)
    while (page->plc)
      flush(page);
  if (page == homepage && (page->mode & PRINT))
    callmacro(page, "RESET", NULL);
  efree(page->inbuf);
  efree(page->outbuf);
  while (wp) {                         /* free local variable area */
    VAR *p = wp->next;

    free(wp->name);
    free(wp);
    wp = p;
  }
  efree(page);
  pagec--;
  return 0;
}


#if CMS
#else
/* Lattice-C "fopene()" compatible function */


FILE *fopene(char *name, char *action, char *opath)
{
  char *path, ext[4];
  FILE *fp;
  int c;

  fp = fopen(name, action);
  if (name[0] == '\\' || name[0] == '.' || name[0] == '/' ||
      name[1] == ':' || fp != NULL)
    return fp;

  ext[0] = 0x00;
  for (c = strlen(name) - 1; c > 0; c--) {
    if (name[c] == '.') {
      int d;
      path = &name[c + 1];
      for (d = 0; d < 3; d++) {
        if (path[d] == 0x00) {
          ext[d] = 0x00;
          break;
        } else
          ext[d] = toupper(path[d]);
      }
      ext[3] = 0x00;
      break;
    } else if (name[c] == '\\')
      break;
  }

  if (*ext && (path = getenv(ext)) != NULL)
    if ((fp = searchfile(path, name, action, opath)) != NULL)
      return fp;
#if Human68k
  if ((path = getenv("path")) != NULL)
#else
  if ((path = getenv("PATH")) != NULL)
#endif
    return searchfile(path, name, action, opath);
  else
    return NULL;
}

FILE *searchfile(char *path, char *name, char *action, char *opath)
{
  char buf[256];
  int c, lc = 0, cc, nl = strlen(name) + 2;
  unsigned int pathd = '/';
  FILE *fp;

  buf[0] = 0;
  for (c = 0; 1; c++) {
    switch (path[c]) {
    case ':':
      if (lc == 1 && *buf != '/' && *buf != '\\' && isalpha(*buf)) {
        buf[lc++] = path[c];
        break;                         /* this is a drive char, ex. a: */
      }
    case ';':
    case 0x00:
      buf[lc] = 0x00;
      if (lc == 0) {                   /* path name is empty */
        if (path[c])
          break;                       /* and this is the end of list */
        if (opath)
          *opath = 0;
        return NULL;
      }
      if (buf[lc - 1] != pathd) {
        buf[lc++] = pathd;
        buf[lc] = 0x00;
      }
      strcat(buf, name);
      for (cc = 0; buf[cc]; cc++)
        if (buf[cc] == '/' || buf[cc] == '\\')
          buf[cc] = pathd;
      if ((fp = fopen(buf, action)) != NULL) {
        if (opath != NULL) {
          buf[lc - 1] = 0x00;
          strcpy(opath, buf);
        }
        return fp;
      }
      if (path[c] == 0) {
        if (opath)
          *opath = 0;
        return NULL;
      }
      lc = 0;
      break;
    case '\\':
    case '/':
      pathd = path[c];
    default:
      buf[lc++] = path[c];
      if (lc + nl > 255) {
        if (opath)
          *opath = 0;
        return NULL;
      }
    }
  }
}
#endif
