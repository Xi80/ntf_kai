/*
    (c) Copyright 1990, 1991  Tomoyuki Niijima
    All rights reserved
*/

/*
    Programme Modified By Itsuki Hashimoto
        2023-05-12:Add New Japanese Era Name "Reiwa" And add new routine.
*/


#include "ntf.h"

int comm(PAGE *page)
{                                      /* process command line */
  char *top = NULL, *arg = NULL, *p = page->inbuf;
  int c;

  /* skip prefix space */
  for (c = 1; *(p + c) != 0 && (*(p + c) == ' ' || *(p + c) == '\t'); c++);
  if (*(p + c) == 0) {
    emsg("");
    fprintf(errfile, "invallid command%s\n", p);
    return 0;
  }
  top = p + c;                         /* pointer to command name */
  while (*(p + c) != ' ' && *(p + c) != '\t' && *(p + c) != '\n'
         && *(p + c))
    c++;
  if (*(p + c) != '\n' && *(p + c)) {
    *(p + c) = 0;                      /* mark end of command name */
    c++;
    while (*(p + c) == ' ' || *(p + c) == '\t')
      c++;
    if (*(p + c) != '\n' && *(p + c))
      arg = p + c;                     /* arguments */
  } else
    *(p + c) = 0;                      /* mark end of command name, no arg */

  if (execmacro(page, top, arg) == 0) { /* if it's macro... */
    return 0;                          /* it was macro, and already executed */
  }
  if (strncmp(top, "\\\"", 2) == 0)
    return 0;                          /* comment line */
  if (top[0] == '{')                   /* 'source' one block */
    return source(NULL, page);

  if (strlen(top) > 2) {
    emsg("");
    fprintf(errfile, "invalid command:.%s", top);
    if (arg)
      fprintf(errfile, " %s\n", arg);
    else
      fprintf(errfile, "\n");
    return -1;
  }
  /* it may be primitive */
  /* store command name into 'int' */
#define cn(a,b) (a)*0x100+(b) & 0x0ffff

  switch (cn(top[0], top[1])) {
    char *q;
    SOURCE *sp;

  case cn('l', 'l'):
    while (page->words)
      flush(page);                     /* same as .br */
    c = page->ll;
    setv(&c, arg);
    if (c > 0 && c <= MAXLL)
      page->ll = c;
    else {
      emsg("");
      fprintf(errfile, ".ll %d out of range.\n", c);
    }
    break;
  case cn('p', 'l'):
    while (page->words)
      flush(page);                     /* same as .br */
    setv(&(page->pl), arg);
    break;
  case cn('i', 'n'):
    while (page->words)
      flush(page);                     /* same as .br */
    c = page->in;
    setv(&c, arg);
    if (c >= 0 && c < page->ll)
      page->in = c;
    page->ti = -1;
    break;
  case cn('r', 'i'):
    while (page->words)
      flush(page);                     /* same as .br */
    c = 0;
    page->ri = (setv(&c, arg) >= 0 && arg ? c : 1);
    break;
  case cn('c', 'e'):
    while (page->words)
      flush(page);                     /* same as .br */
    c = 0;
    page->ce = (setv(&c, arg) >= 0 && arg ? c : 1);
    break;
  case cn('t', 'i'):
    while (page->words)
      flush(page);                     /* same as .br */
    c = page->in;
    setv(&c, arg);
    if (c != page->in && c >= 0 && c < page->ll)
      page->ti = c;
    break;
  case cn('f', 'i'):
    while (page->words)
      flush(page);                     /* same as .br */
    page->mode |= FILLIN;
    break;
  case cn('n', 'f'):
    while (page->words)
      flush(page);                     /* same as .br */
    page->mode &= ~FILLIN;
    break;
  case cn('j', 'u'):
    while (page->words)
      flush(page);                     /* same as .br */
    page->mode |= JUSTIFY;
    page->ju = -1;
    break;
  case cn('n', 'j'):
    while (page->words)
      flush(page);                     /* same as .br */
    page->mode &= ~JUSTIFY;
    page->ju = 0;
    break;
  case cn('d', 'y'):
    func_dy(page, arg);
    break;
  case cn('n', 'e'):
    if (page->mode & TEXT)
      return 0;
    c = 0;
    c = (setv(&c, arg) > 0 ? c : 1);
    while (page->words)
      flush(page);                     /* same as .br */
    if (page->pl < page->plc + 5 + c)
      while (1) {
        flush(page);
        if (page->plc == 0)
          break;
      }
    break;
  case cn('s', 'p'):
    c = 0;
    c = (setv(&c, arg) > 0 ? c : 1);
    while (page->words)
      flush(page);                     /* same as .br */
    while (c-- > 0) {
      flush(page);
      if (!(page->mode & TEXT) && page->plc == 0)
        break;
    }
    break;
  case cn('b', 'r'):
    while (page->words)
      flush(page);
    break;
  case cn('b', 'p'):
    if (page->mode & TEXT)
      break;
    c = page->pnc;
    while (page->words)
      flush(page);
    while (page->plc)
      flush(page);
    if (arg && setv(&c, arg) > 0)
      page->pnc = c;
    break;
  case cn('h', 'l'):
    func_hl(page, arg);
    break;
  case cn('s', 'o'):                  /* 'source' include file */
    q = arg;
    while (*q != ' ' && *q != '\t' && *q != '\n' && *q)
      q++;
    *q = 0;
    if (debug)
      fprintf(errfile, "try to source(%s)\n", arg);
    if ((sp = opensource(arg, NULL)) == NULL) {
      emsg("");
      fprintf(errfile, "%s can't open.\n", arg);
    } else {
      csource->ip = btell(csource->fp); /* store current */
      source(sp, page);                /* source file pointer */
      closesource(sp, -1);
    }
    break;
  case cn('o', 'h'):
  case cn('m', 'h'):
    parselabel(page, 0, arg);
    break;
  case cn('o', 'f'):
  case cn('m', 'f'):
    parselabel(page, 2, arg);
    break;
  case cn('e', 'h'):
    parselabel(page, 1, arg);
    break;
  case cn('e', 'f'):
    parselabel(page, 3, arg);
    break;
  case cn('(', 't'):
    if (contents == NULL && (contents = openbuff()) == NULL)
      fatal("temporary file can't open.", 1);
    page->mode |= CONTENTS;
    break;
  case cn(')', 't'):
    page->mode &= ~CONTENTS;
    break;
  case cn('t', 'p'):
    if (!(page->mode & CONTENTS))
      func_tp(page);
    break;
  case cn('c', 'c'):
    if (arg)
      commandprefix = *arg;
    break;
  case cn('d', 'e'):
    if (arg == NULL) {
      emsg(".de : macro name must be specified.");
      break;
    }
    if (macro == NULL) {
      if ((macro = openbuff()) == NULL)
        fatal("temporary file can't open.", 1);
      else if (MACROCASH > MAXLL * 2 + 1) {
        WORKBUFF *wp = isbuff(macro);

        efree(wp->cash);
        wp->cash = emalloc(MACROCASH);
        wp->cashsize = MACROCASH;
        *(wp->cash) = 0;
        wp->cashp = 0;
        wp->maxp = 0;
      }
    }
    defmacro(page, arg);
    break;
  case cn('i', 'x'):
    if (q = arg) {
      while (*q != '\n' && *q)
        q++;
      *q = 0;
      indexs = defindex(indexs, arg, homepage->pnc);
    }
    break;
  case cn('i', 'p'):
    while (page->words)
      flush(page);                     /* same as .br */
    putindex(page, indexs);
    break;
  case cn('e', 'x'):
    if (arg)
      evalexpr(arg);
    break;
  case cn('e', 's'):
    if (arg) {
      /* eval input string and process again
         with evaled line as input line */

      evallabel(page->outbuf, arg);
      strcpy(page->inbuf, page->outbuf);        /* copy evaled line */
      /* and process it */
      if (page->inbuf[0] == commandprefix)
        comm(page);
      else
        putline(page);
    }
    break;
  case cn('i', 'f'):
    if (arg && evalexpr(arg)) {
      oneline(page, -1);               /* process one group of line */
      oneline(page, 0);                /* ignore one group of line */
    } else {
      oneline(page, 0);
      oneline(page, -1);
    }
    break;
  case cn('w', 'l'):
    func_wl(page, arg);
    break;
  case cn('o', 'b'):
    func_ob(page, arg);
    break;
  case cn('w', 'b'):
    func_wb(page, arg);
    break;
  case cn('u', 'b'):
    func_ub(page, arg, 0);
    break;
  case cn('r', 'b'):
    func_ub(page, arg, 1);
    break;
  case cn('n', 'b'):
    func_nb(page, arg);
    break;
  case cn('s', 'b'):
    func_sb(page, arg);
    break;
  case cn('j', 'b'):
    switch (func_jb(page, arg)) {
    case -1:
      emsg("");
      fprintf(errfile, ".jb needs two buffers.%s", arg);
      break;
    case 1:
      emsg("");
      fprintf(errfile, ".jb can't open tmp buffer.%s", arg);
      break;
    }
    break;
  case cn('c', 'b'):
    func_cb(page, arg);
    break;
  case cn('c', 'j'):
    if (arg)
      barchar = *arg;
    else
      barchar = ' ';
    break;
  case cn('s', 'i'):
    if (arg)
      func_si(page, arg);
    break;
  case cn('c', 'w'):
    if (arg)
      func_cw(page, arg);
    break;
  case cn('w', 'm'):
    if (arg) {
      char *p = arg;
      while (*p && *p != '\n')
        p++;
      *p = 0;
      emsg(arg);
    } else
      emsg("");
    break;
  case cn('q', 'u'):
    fatal("quit ntf.", 1);
    break;
  case cn('a', 't'):
    if (homepage->mode & PRINT)
      page->nattr = (setv(NULL, arg) &
                     ((~attrmask) | UNDERSCORE | HIGHLIGHT | 0xffff8000));
    break;
  case cn('p', 'p'):
    if (arg) {
      p = arg;
      while (*p && *p != '\n')
        p++;
      *p = 0;
      p = emalloc(strlen(arg) + 3);
      strcpy(p, " ");
      strcat(p, arg);
      strcat(p, " ");
    } else {
      p = NULL;
    }
    while (page->words)
      flush(page);                     /* same as .br */
    if (pplist)
      free(pplist);
    pplist = p;
    break;
  default:
    emsg("");
    fprintf(errfile, "invalid command:.%s", top);
    if (arg)
      fprintf(errfile, " %s\n", arg);
    else
      fprintf(errfile, "\n");
    return -1;
  }
  return 0;
}

int func_wl(PAGE *page, char *arg)
{                                      /* while */
  if (arg) {
    char *expr = emalloc(strlen(arg) + 1);
    long ip = btell(csource->fp);
    int wline = csource->sourceline;

    strcpy(expr, arg);                 /* store expr string */
    while (evalexpr(expr)) {
      csource->sourceline = wline;
      oneline(page, -1);               /* execute while-block */
      if (bseek(csource->fp, ip, SEEK_SET)) {   /* wind up source */
        fprintf(errfile, "Sorry, but you can't use .wl for this file.\n");
        break;
      }
    }
    oneline(page, 0);                  /* jump to the end of while-block */
    efree(expr);
  } else
    oneline(page, 0);
  return 0;
}

int func_ob(PAGE *page, char *arg)
{                                      /* open buffer */
  if (arg) {
    FILE *f;
    char *p = arg;

    while (*p != ' ' && *p != '\t' && *p != '\n' && *p)
      p++;
    *p = 0;                            /* mark end of buffer name */
/*
  Given argument is used as the name of user variable, which holds
FILE * of opened buffer file as it's value.
  User will see this name of user variable as buffer name....
*/
    if ((f = isbuffno(getvar(arg))))   /* Same No. of buffer is */
      closebuff(f);                    /* already opened, close it first */
    setvar(arg, isbuff(openbuff())->buffno);
    /* store buffer No. to user variable */
  } else {
    emsg("");
    fprintf(errfile, ".ob buffer name must be specified.\n");
  }
  return 0;
}

int func_cb(PAGE *page, char *arg)
{                                      /* close buffer */
  if (arg) {
    FILE *f;
    char *p = arg;

    while (*p != ' ' && *p != '\t' && *p != '\n' && *p)
      p++;
    *p = 0;
    if ((f = isbuffno(getvar(arg)))) {
      closebuff(f);
      setvar(arg, 0);
    }
  } else {
    emsg("");
    fprintf(errfile, ".cb buffer name must be specified.\n");
  }
  return 0;
}

int func_wb(PAGE *page, char *arg)
{                                      /* write on buffer */
  if (arg) {
    FILE *f;
    char *p = arg;
    FILE *lastout = outfile;           /* keep current output file */
    int m = 0, ll = 0;

    arg = strtok(arg, " \t\n");
    if (p = strtok(NULL, " \t\n")) {
      m = evalexpr(p);
      if (p = strtok(NULL, " \t\n"))
        ll = evalexpr(p);
    }
    if (ll <= 0 || ll > MAXLL)
      ll = page->ll;

    if ((f = isbuffno(getvar(arg))) == NULL) {
      emsg("");
      fprintf(errfile, "no such buffer named %s\n", arg);
    } else if (f == outfile) {
      fatal("nested .wb on the same buffer.", 1);
    } else {
      PAGE *npage;

      npage = openpage(ll, page->pl, page->mode);
      if (m)
        npage->mode = m;
      else
        npage->mode |= (FULLLINE | TEXT);
      cpage = npage;                   /* setup new page */
      bseek(f, 0l, SEEK_END);          /* seek to the end of buffer */
      outfile = f;                     /* setup new output file */
      oneline(npage, -1);              /* process one group of line */
      while (npage->words)
        flush(npage);                  /* break */
      closepage(npage);
      outfile = lastout;               /* restore output file */
      cpage = page;                    /* restore current page */
    }
  } else {
    emsg("");
    fprintf(errfile, ".wb buffer name must be specified.\n");
  }
  return 0;
}

int func_ub(PAGE *page, char *arg, int mode)
{
  FILE *f;
  char *name, *p;
  int start, numl;
  SOURCE *so;

  if (arg && (name = strtok(arg, " \t\n"))) {

    if ((f = isbuffno(getvar(name))) == NULL) {
      emsg("");
      fprintf(errfile, "no such buffer named %s\n", name);
    } else {
      brewind(f);
      if (p = strtok(NULL, " \t\n"))
        start = (atoi(p) > 0 ? atoi(p) : 1);
      else
        start = 1;
      if (p = strtok(NULL, " \t\n"))
        numl = (atoi(p) > 0 ? atoi(p) : 0);
      else
        numl = 0;
      if (mode) {                      /* .rb command */
        int c, d, n = 0, mode = page->mode;

        page->mode &= ~FILLIN;
        while (1) {
          if (bgets(page->inbuf, MAXLL, f) == NULL)
            break;
          n++;
          if (numl && n > start + numl - 1)
            break;
          if (n >= start) {
            page->inbuf[MAXLL] = 0;
            /* replace joint mark with joint charactor */
            for (c = 0, d = 0; page->inbuf[c]; c++) {
              if (page->inbuf[c] == barmark) {
                sprintf(page->outbuf + d, "\v000000C0%c\v00000000",
                        barchar);
                d += 19;
              } else
                page->outbuf[d++] = page->inbuf[c];
            }
            page->outbuf[d] = 0;
            strcpy(page->inbuf, page->outbuf);
            page->outbuf[0] = 0;
            putline(page);             /* and put it to current page */
            while (page->words)
              flush(page);
          }
        }
        page->mode = mode;             /* restore old mode */
      } else {                         /* .ub command */
        so = opensource(name, f);
        csource->ip = btell(csource->fp);
        source(so, page);              /* simply 'source' buffer */
        closesource(so, 0);
      }
    }
  } else {
    emsg("");
    fprintf(errfile, ".ub buffer name must be specified.\n");
  }
  return 0;
}

int func_nb(PAGE *page, char *arg)
{                                      /* count up lines in buffer */
  if (arg) {
    FILE *f;
    char *p = arg;
    int c = -1;

    while (*p != ' ' && *p != '\t' && *p != '\n' && *p)
      p++;
    *p = 0;
    if ((f = isbuffno(getvar(arg)))) {
      c = 0;
      brewind(f);                      /* top of buffer */
      while (1) {                      /* count up lines */
        if (bgets(page->inbuf, MAXLL, f) == NULL)
          break;
        if (attrchk(page->inbuf) & TALL)
          c++;
        c++;
      }
    }
    setvar("rc", c);                   /* store the result to 'rc' */
  } else {
    emsg("");
    fprintf(errfile, ".nb buffer name must be specified.\n");
  }
  return 0;
}

int func_sb(PAGE *page, char *arg)
{                                      /* count up bytes in buffer */
  if (arg) {
    FILE *f;
    char *p = arg;
    int c = -1;

    while (*p != ' ' && *p != '\t' && *p != '\n' && *p)
      p++;
    *p = 0;
    if ((f = isbuffno(getvar(arg))))
      c = btell(f);
    setvar("rc", c);                   /* store the result to 'rc' */
  } else {
    emsg("");
    fprintf(errfile, ".sb buffer name must be specified.\n");
  }
  return 0;
}

/* confirm if it's realy a FILE * of buffer */

WORKBUFF *isbuff(FILE * fp)
{
  WORKBUFF *wp = rootbuff;

  if (fp == NULL)
    return NULL;
  while (wp) {                         /* scan buffer list */
    if (wp->fp == fp || wp == (WORKBUFF *) fp)
      return wp;
    wp = wp->nextbuff;
  }
  return NULL;
}

/* confirm the No. is really a buffer No. */

FILE *isbuffno(int no)
{
  WORKBUFF *wp = rootbuff;

  while (wp) {
    if (wp->buffno == no)
      return wp->fp;
    wp = wp->nextbuff;
  }
  return NULL;
}

int func_jb(PAGE *page, char *arg)
{                                      /* joint two buffers */
  FILE *fa, *fb;
  char *bufa = emalloc(MAXLL + 1);
  char *bufb = emalloc(MAXLL + 1);
  char *bufx = emalloc(MAXLL + 1);
  char *a, *b;
  int c;

  a = arg;
  while (*arg != ' ' && *arg != '\t' && *arg != '\n' && *arg)
    arg++;
  if (*arg == '\n' || *arg == 0)
    return -1;
  *(arg++) = 0;
  while (*arg == ' ' || *arg == '\t')
    *arg++;
  if (*arg == '\n' || *arg == 0)
    return -1;
  b = arg;
  while (*arg != ' ' && *arg != '\t' && *arg != '\n' && *arg)
    arg++;
  *arg = 0;
  if ((fa = isbuffno(getvar(a))) && (fb = isbuffno(getvar(b)))) {
    WORKBUFF *wo;
    int ea = -1, eb = -1;
    FILE *out;

    brewind(fa);
    brewind(fb);
    *bufa = 0;
    *bufb = 0;
    if ((out = openbuff()) == NULL)
      return 1;                        /* open new buffer */
    wo = isbuff(out);
    while (1) {
      if (ea && bgets(bufx, MAXLL, fa) == NULL) {
        ea = 0;                        /* bufferA is end */
        expandbuf(page, bufa);         /* enblank except joinmark */
      } else if (ea)
        strcpy(bufa, bufx);

      if (eb && bgets(bufx, MAXLL, fb) == NULL) {
        eb = 0;                        /* bufferB is end */
        expandbuf(page, bufb);         /* enblank except joinmark */
      } else if (eb)
        strcpy(bufb, bufx);
      /* break when both buffers are end */
      if (!ea && !eb)
        break;

      for (c = 0; bufb[c] != '\n' && bufb[c]; c++);
      bufb[c] = '\n';
      bufb[c + 1] = 0;
      for (c = 0; bufa[c] != '\n' && bufa[c]; c++);
      bufa[c] = barmark;               /* set joint mark */
      bufa[c + 1] = 0;
      strncat(bufa, bufb, MAXLL);      /* joint lines each other */
      bputs(bufa, wo->fp);             /* put it to new buffer */
      bufa[c] = '\n';
      bufa[c + 1] = 0;
    }
    setvar(a, wo->buffno);
    /* bind new buffer No. to old name */
  } else {
    efree(bufa);
    efree(bufb);
    efree(bufx);
    return -1;                         /* buffer name */
  }
  closebuff(fa);                       /* close old buffer */
  efree(bufa);                         /* efree work area */
  efree(bufb);
  efree(bufx);
  return 0;
}

int expandbuf(PAGE *page, char *buf)
{
  char *tmp = emalloc(MAXLL + 1);
  int c = 0, d = 0, attr = 0;

  while (buf[c] && buf[c] != '\n') {
#if KIKO
    if (!(page->mode & CMSTEXT)) {     /* not to count KIKO */
      if (strncmp(buf + c, kin, KINL) == 0) {   /* except in CMSTEXT mode */
        c += KINL;
        continue;
      } else if (strncmp(buf + c, kout, KOUTL) == 0) {
        c += KOUTL;
        continue;
      }
    }
#endif
    if (buf[c] == '\v') {              /* not to count attribute string */
      sscanf(buf + c + 5, "%04X", &attr);
      c += 9;
      continue;
    }
    if (buf[c] != barmark) {
      if ((attr & WIDE) && buf[c] != ' ')
        tmp[d++] = ' ';
      tmp[d++] = ' ';
    } else
      tmp[d++] = barmark;
    c++;
  }
  tmp[d] = 0;
  strcpy(buf, tmp);
  efree(tmp);
  return 0;
}

int oneline(PAGE *page, int mode)
{                                      /* process one group of line */
  int gc = 0;
  while (1) {
    gc = getbuf(page);
/* getbuf() returns -1 for EOF, 1 for '.}' end-of-block
   or end-of-macro, 0 for normal. Skip end-of-macro
   at the begining of 'oneline', so that .wb at the end of
   macro could process oneline follows the macro. */
    if (gc == -1)
      return 0;
    else if (gc == 0)
      break;
  }
  if (mode) {                          /* execute mode */
    if (debug)
      fprintf(errfile, "oneline: execute %s", page->inbuf);
    if (page->inbuf[0] == commandprefix)
      comm(page);
    else
      putline(page);
  } else {                             /* ignore mode */
    int nest = 0;

    while (1) {                        /* check block nest */
      /* and break at the end of 'oneline' */
      if (debug)
        fprintf(errfile, "oneline: ignore(%d) %s", nest, page->inbuf);
      if (page->inbuf[0] == commandprefix) {
        char *p = page->inbuf + 1;

        while (*p == ' ' || *p == '\t')
          p++;
        if (*p == '{')
          nest++;
        else if (*p == '}' && *(p + 1) != '}')
          nest--;
      }
      if (nest <= 0)
        break;                         /* end of one group of line */
      if (getbuf(page) && beof(csource->fp))
        break;
    }
  }
  return 0;
}

INDEX *defindex(INDEX *root, char *key, int pno)
/* store argument as keyword for index */
/* recursive call */
{
  INDEX *wp;
  int c;

  if (root == NULL) {                  /* new keyword */
    wp = (INDEX *) emalloc(sizeof(INDEX));
    wp->upper = NULL;
    wp->lower = NULL;
    wp->name = emalloc(strlen(key) + 1);
    strcpy(wp->name, key);
    wp->pagep = (INDEXP *) emalloc(sizeof(INDEXP));
    (wp->pagep)->nextpageno = NULL;
    (wp->pagep)->pageno = pno;
    (wp->pagep)->objno = pdf_objno;
    return wp;                         /* return new INDEX */
  }
  c = strcmp(root->name, key);
  if (c > 0)
    root->lower = defindex(root->lower, key, pno);
  else if (c < 0)
    root->upper = defindex(root->upper, key, pno);
  else {                               /* same keyword found */
    INDEXP *pp = root->pagep, *bpp;

    if (root->pagep != NULL) {
      /* add page No. to the list */
      while (pp->nextpageno)
        pp = pp->nextpageno;
      if (pp->pageno != pno) {         /* not to mark towice */
        bpp = (INDEXP *) emalloc(sizeof(INDEXP));
        bpp->nextpageno = NULL;
        bpp->pageno = pno;
        bpp->objno = pdf_objno;
        pp->nextpageno = bpp;
      }
    }
  }
  return root;                         /* return it self */
}

int putindex(PAGE *page, INDEX *root)
/* putout INDEXs */
/* also recursive call */
{
  INDEXP *wp;
  char no[40];

  if (root == NULL)                    /* an end of tree */
    return 0;
  else {
    wp = root->pagep;
    putindex(page, root->lower);       /* put lower tree first */
    sprintf(page->outbuf, "%s", root->name);
    while (wp) {                       /* put all page No.s */
      sprintf(no, " \v%04X8000%d\v00000000", wp->objno, wp->pageno);
      strcat(page->outbuf, no);
      wp = wp->nextpageno;
    }
    putbuf(page);                      /* put myself */
    putindex(page, root->upper);       /* then, upper tree */
  }
  return 0;
}


int func_dy(PAGE *page, char *arg)
{                                      /* put current date */
  struct tm *tt;
  time_t ltime;                        /* time_t may be same as long */
  int c = 0;
  char p[50];

  setv(&c, arg);
  time(&ltime);
  tt = localtime(&ltime);
  if (c == 2)                          /* Japanese style date.*/
    if(tt->tm_year <= 2019 && tt->tm_mon < 4){  /*Heisei*/
      sprintf(page->inbuf, jdatestr_heisei,
              tt->tm_year - 88, tt->tm_mon + 1,
              tt->tm_mday, jweek[tt->tm_wday]);
    } else {                                    /*Reiwa*/
      sprintf(page->inbuf, jdatestr_reiwa,
              tt->tm_year - 118, tt->tm_mon + 1,
              tt->tm_mday, jweek[tt->tm_wday]);
    }

  else if (c == 3) {
    strcpy(p, ctime(&ltime));
    *(p + 10) = 0;
    sprintf(page->inbuf, "%3.3s. %d %4.4s\n", p + 4, atoi(p + 8), p + 20);
  } else                               /* English style date */
    sprintf(page->inbuf, "%s", ctime(&ltime));
  putline(page);
  return 0;
}

int func_hl(PAGE *page, char *arg)
{                                      /* horizontal line */
  int c;

  while (page->words)
    flush(page);                       /* same as .br */
  c = 0;
  c = (setv(&c, arg) > 0 ? c : 1);
  if (page == homepage && !(page->mode & TEXT))
    c = (page->plc + c + 5 > page->pl ? page->pl - 5 - page->plc : c);
  while (c-- > 0) {
    int b, ll = page->ll - (page->ti >= 0 ? page->ti : page->in);
    for (b = 0; b < ll && b < MAXLL - 1; b++)
      *(page->inbuf + b) = '-';
    *(page->inbuf + b++) = '\n';
    *(page->inbuf + b) = 0;
    putline(page);
    while (page->words)
      flush(page);                     /* break anyway */
  }
  return 0;
}


int func_tp(PAGE *page)
{                                      /* put out contents */
  while (page->words)
    flush(page);
  bseek(contents, 0l, SEEK_SET);       /* top of contents file */
  while (1) {
    int no, obj, s, c;
    char num[50];

    if (bgets(page->inbuf, MAXLL, contents) == NULL)
      break;
    sscanf(page->inbuf, "%05d %04X", &no, &obj);
    /* copy rest part to output */
    jstrncpy(page->outbuf, page->inbuf + 10, page->ll - 5);
    *(page->outbuf + page->ll - 5) = 0;
    for (s = 0; *(page->outbuf + s) != '\n' && *(page->outbuf + s); s++);
    page->outbuf[s] = 0;
    c = page->ll - klen(page->outbuf, page->mode & CMSTEXT, 0) - 5;
    while (c-- > 0)
      page->outbuf[s++] = '.';         /* make dot line */
    page->outbuf[s] = 0;
    sprintf(num, "\v%04X8000% 5d\v00000000", obj, no);  /* add line No. */
    strcat(page->outbuf, num);
    putbuf(page);
    page->outbuf[0] = 0;
  }
  closebuff(contents);                 /* purge contents already put */
  if ((contents = openbuff()) == NULL) /* reopen contents file */
    fatal("temporary file can't open.", 1);
  return 0;
}


int func_si(PAGE *page, char *arg)
{                                      /* single word */
  WORD *wb = (WORD *) emalloc(sizeof(WORD));
  int c, d;

  /* delete post space */
  for (c = strlen(arg) - 1; c >= 0 && (arg[c] == ' ' || arg[c] == '\n');
       c--);
  if (arg[0] == '\"' && arg[c] == '\"') {
    arg[c] = 0;
    arg++;
  } else
    arg[c + 1] = 0;

  wb->letter = emalloc(c + 2);
  strcpy(wb->letter, arg);
  for (d = 0; arg[d]; d++)
    page->inbuf[d] = arg[d];
  page->inbuf[d] = '\n';
  page->inbuf[d + 1] = 0;
  if (modechk(page)) {                 /* enRIght or CEntering is in effect */
    efree(wb->letter);
    efree(wb);
    return 0;
  }
  /* create SINGLE WORD */
  if (page->words && (page->lastword->type & KANJI)) {
    wb->prespace = 0;
    wb->type = SINGLE;
  } else if (page->words) {
    wb->prespace = 1;
    wb->type = SINGLE;
  } else {
    wb->prespace = 0;
    wb->type = (SINGLE | HEAD);
  }
  wb->nextword = NULL;
  wb->attr = page->nattr;
  putword(wb, page);                   /* put it to current PAGE */
  return 0;
}


char hextbl[17] = "0123456789ABCDEF";

int x2b(int x)
{
  int c = toupper(x), d = 0;

  for (d = 0; hextbl[d] != c && hextbl[d]; d++);
  if (hextbl[d])
    return d;
  return 0;
}

int func_cw(PAGE *page, char *arg)
{
  int c = strlen(arg);
  char b[2];

  while (arg[c - 1] == '\n' || arg[c - 1] == ' ' || arg[c - 1] == '\t')
    arg[--c] = 0;

  while (*arg) {
    if (*arg == '\\') {
      if (*(arg + 1) == 'a')
        borfputs("\a", outfile);
      else if (*(arg + 1) == 'b')
        borfputs("\b", outfile);
      else if (*(arg + 1) == 'f')
        borfputs("\f", outfile);
      else if (*(arg + 1) == 'n')
        borfputs("\n", outfile);
      else if (*(arg + 1) == 'r')
        borfputs("\r", outfile);
      else if (*(arg + 1) == 't')
        borfputs("\t", outfile);
      else if (*(arg + 1) == 'v')
        borfputs("\v", outfile);
      else if (*(arg + 1) == '\\')
        borfputs("\\", outfile);
      else if (*(arg + 1) == '?')
        borfputs("?", outfile);
      else if (*(arg + 1) == '\'')
        borfputs("\'", outfile);
      else if (*(arg + 1) == '\"')
        borfputs("\"", outfile);
      else if (*(arg + 1) == '0' && isbuff(outfile) == NULL)
        fputc('\0', outfile);
      else if (*(arg + 1) == 'e')
        borfputs("\x1b", outfile);
      else if (*(arg + 1) == 'x' && isxdigit(*(arg + 2))
               && isxdigit(*(arg + 3))) {
        b[0] = ((x2b(*(arg + 2)) << 4) + x2b(*(arg + 3)));
        b[1] = 0;
        if (b[0] == 0 && isbuff(outfile) == NULL)
          fputc(b[0], outfile);
        else
          borfputs(b, outfile);
        arg += 2;
      }
      arg++;
    } else {
      b[0] = *arg;
      b[1] = 0;
      borfputs(b, outfile);
    }
    arg++;
  }
  if (isbuff(outfile) == NULL)
    fflush(outfile);
  return 0;
}

int setv(int *v, char *p)
{                                      /* eval expr and store the result */
  int c;

  if (p == NULL)
    return 0;
  if (*p == '+' || *p == '-')
    c = evalexpr(p + 1);               /* '+' and '-' are not */
  else
    c = evalexpr(p);                   /* operators for evalexpr() */

  if (v) {                             /* if pointer to value specified */
    if (*p == '+')
      *v += c;                         /* add the result */
    else if (*p == '-')
      *v -= c;                         /* substruct the result */
    else if (*p && *p != '\n')
      *v = c;
  }
  return c;
}

/* parselabel() is too complex. It should be re-written */

int parselabel(PAGE *page, int n, char *p)
/* parse label-oprand into three part */
/* !---- top charactor of the argument is the delimitor */
{
  int c = *(p++), kj = 0, s = 0;
  char **q[4], ***t = q;               /* pointer to pointer to pointer !! */

  if (debug)
    fprintf(errfile, "currentlabel: %s %s %s\n", page->hl[n]
            , page->hm[n]
            , page->hr[n]);
  if (page->hl[n])
    efree(page->hl[n]);
  if (page->hm[n])
    efree(page->hm[n]);
  if (page->hr[n])
    efree(page->hr[n]);
  q[0] = &(page->hl[n]);
  q[1] = &(page->hm[n]);
  q[2] = &(page->hr[n]);
  q[3] = NULL;
  while (*(p + s)) {
#if KIKO
    if (kj == 0 && strncmp(p + s, kin, KINL) == 0) {
      s += KINL;
#else
    if (kj == 0 && iskanji(*(p + s))) {
#endif
      kj = -1;
#if KIKO
    } else if (kj && strncmp(p + s, kout, KOUTL) == 0) {
      s += KOUTL;
#else
    } else if (kj && !iskanji(*(p + s))) {
#endif
      kj = 0;
    } else if (kj)
      s += 2;
    else if (*(p + s) == c) {          /* delimitor */
      **t = emalloc(s + 1);
      strncpy(**t, p, s);
      *(**t + s) = 0;
      p += s + 1;
      s = 0;
      t++;
      if (*t == NULL)
        break;
    } else
      s++;
  }
  if (*t != NULL)
    fprintf(errfile, "%s(%d) label error %s\n", csource->sourcename,
            csource->sourceline, p);
  if (debug)
    fprintf(errfile, "currentlabel: %s %s %s\n", page->hl[n]
            , page->hm[n]
            , page->hr[n]);
  return 0;
}
