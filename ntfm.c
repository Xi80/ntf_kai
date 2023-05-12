/*
    (c) Copyright 1990, 1991  Tomoyuki Niijima
    All rights reserved
*/


#include "ntf.h"

unsigned int macrohash(char *name)
{                                      /* hash function for macro */
  unsigned int hashval;

  for (hashval = 0; *name; name++)
    hashval = *name + 31 * hashval;
  return hashval % MACHASHN;
}


MACRO *searchmacro(char *name)
{                                      /* search macro in hash table */
  MACRO *wp;

  for (wp = macrohtab[macrohash(name)]; wp; wp = wp->nextmacro) {
    if (strcmp(name, wp->name) == 0)
      return wp;
  }
  return NULL;
}


int defmacro(PAGE *page, char *arg)
{                                      /* define macro */
  MACRO *wp;
  SOURCE *macsource = csource;
  int c;
  long ip = btell(macro);

  /* get first token, this is macro name */
  for (c = 0; arg[c] != ' ' && arg[c] != '\t'
       && arg[c] != '\n' && arg[c]; c++);
  arg[c] = 0;
  if ((wp = searchmacro(arg)) == NULL) {        /* new macro */
    unsigned int macn = macrohash(arg);

    wp = (MACRO *) emalloc(sizeof(MACRO));
    wp->name = emalloc(strlen(arg) + 1);
    strcpy(wp->name, arg);
    if (debug && macrohtab[macn])
      fprintf(errfile, "defmacro: entry is chained.\n");
    wp->nextmacro = macrohtab[macn];
    macrohtab[macn] = wp;
  }

  bseek(macro, 0l, SEEK_END);          /* seek file pointer to the end */
  wp->entry = btell(macro);            /* store file_end as entry point */

  /* append defined macro text to macro buffer */
  while (1) {
    if (getbuf(page) && beof(macsource->fp)) {
      emsg("");
      fprintf(errfile, "incomplete macro definition. macro(%s)\n",
              wp->name);
      break;
    }
    if (*(page->inbuf) == commandprefix) {
      char *p = page->inbuf + 1;
      while (*p == ' ' || *p == '\t')
        p++;
      if (strncmp(p, "ed", 2) == 0)
        break;                         /* check .ed */
    }
    bputs(page->inbuf, macro);         /* write out the line */
  }                                    /* to macro buffer */
  /* .}} is end_mark of a macro */
  /* getbuf() will check it and exit macro */
  bputs(".}}\n", macro);
  wp->end = btell(macro);
  bseek(macro, ip, SEEK_SET);          /* store current file pointer as end */
  return 0;                            /* back file pointer */
}

int execmacro(PAGE *page, char *name, char *arg)
{                                      /* execute specified macro */
  MACRO *wp;
  SOURCE *sp;
  char **args;
  char *msname;

  if ((wp = searchmacro(name)) == NULL)
    return -1;
  csource->ip = btell(csource->fp);    /* keep current source pointer */
  args = (char **) emalloc(sizeof(char *) * MAXARGS);
  parsearg(args, arg);                 /* parse macro arguments $0 $1 $2... */
  bseek(macro, wp->entry, SEEK_SET);   /* seek macro file to entry point */

  /* make unique source name from the pointer to current SOURCE */
  /* source name will be like "macro(macroname:2f00)". */
  msname = emalloc(strlen(wp->name) + 10 + sizeof(int) * 2);
  sprintf(msname, "macro(%s:%x)", wp->name, pagec);

  sp = opensource(msname, macro);
  sp->args = args;                     /* keep macro arguments in SOURCE */
  source(sp, page);                    /* 'source' macro buffer */
  closesource(sp, 0);
  efree(args[0]);                      /* efree parsed macro arguments */
  if (args[1])
    efree(args[1]);
  efree(args);
  efree(msname);
  return 0;
}

int parsearg(char **args, char *arg)
{                                      /* parse macro arguments */
  int kj = 0, c = 0, ac = 1;
  char *temp, *arg0;
/*  arguments will be parsed like below
    $0--!
        arg0........................ ..0

    $1--! $2-------!  $4----!
        temp .....0..0.....0....0... ..0
              $3------'
*/
  temp = emalloc(MAXLL + 1);
  for (ac = 0; ac < MAXARGS; ac++)
    args[ac] = 0;
  ac = 1;
  if (arg) {
    int cc = 0;

    kj = 0;
    arg0 = emalloc(strlen(arg) + 1);
    c = 0;
    while (arg[c] != '\n' && arg[c]) {
#if KIKO
      if (!kj && strncmp(arg + c, kin, KINL) == 0) {
        kj = -1;
        strncpy(arg0 + cc, arg + c, KINL);
        cc += KINL;
        c += KINL;
        continue;
      } else if (kj && strncmp(arg + c, kout, KOUTL) == 0) {
        kj = 0;
        strncpy(arg0 + cc, arg + c, KOUTL);
        cc += KOUTL;
        c += KOUTL;
        continue;
      }
#else
      if (!kj && iskanji(arg[c]))
        kj = -1;
      else if (kj && !iskanji(arg[c]))
        kj = 0;
#endif
      if (!kj && arg[c] == '\\' && arg[c + 1] == ' ') {
        arg0[cc++] = ' ';
        c += 2;
      } else {
        if (kj)
          arg0[cc++] = arg[c++];
        arg0[cc++] = arg[c++];
      }
    }
    arg0[cc] = 0;
  } else {
    arg0 = emalloc(2);
    arg0[0] = 0;
  }
  c = 0;
  kj = 0;
  args[0] = arg0;
  args[1] = temp;
  temp[0] = 0;
  if (arg) {
    while (*arg) {
#if KIKO
      if (kj == 0 && strncmp(arg, kin, KINL) == 0) {
        strncpy(temp + c, arg, KINL);  /* entering KANJI mode */
        c += KINL;
        arg += KINL;
#else
      if (kj == 0 && iskanji(*arg)) {
#endif
        kj = -1;
#if KIKO
      } else if (kj && strncmp(arg, kout, KOUTL) == 0) {
        strncpy(temp + c, arg, KOUTL); /* exit KANJI mode */
        c += KOUTL;
        arg += KOUTL;
#else
      } else if (kj && !iskanji(*arg)) {
#endif
        kj = 0;
      } else if (kj) {
        strncpy(temp + c, arg, 2);
        c += 2;
        arg += 2;
      } else if (!kj && strncmp(arg, "\\ ", 2) == 0) {
        *(temp + c) = ' ';             /* insert quoted space charactor */
        c++;
        arg += 2;
      } else if (!kj && *arg != ' ' && *arg != '\t' && *arg != '\n') {
        *(temp + c) = *arg;
        c++;
        arg++;
      } else if (!kj && ac < MAXARGS - 1
                 && (*arg == ' ' || *arg == '\t' || *arg == '\n'
                     || *arg == 0)) {
        /* delimiter, this is the end of one argument */
        *(temp + c) = 0;
        c++;
        arg++;
        while (*arg == ' ' || *arg == '\t')
          arg++;                       /* skip space */
        args[++ac] = temp + c;
      } else if (!kj && (*arg == ' ' || *arg == '\t')) {
        *(temp + c) = ' ';             /* ac>=MAXARGS */
        c++;
        arg++;
      } else
        arg++;
    }
    *(temp + c) = 0;
  }
  for (ac = 1; ac < MAXARGS; ac++) {
    if (args[ac] && args[ac][0] == 0)
      break;
  }
  while (ac < MAXARGS)
    args[ac++] = NULL;
  if (args[1] == NULL)
    efree(temp);
  return 0;
}

/* eval macro args called from getbuf() */

int evalarg(PAGE *page, char **args)
{
  int c = 0, ano, kj = 0;
  char *a = page->inbuf, *p = emalloc(MAXLL * 2);

  while (*a) {
    if (c > MAXLL)
      break;
#if KIKO
    if (!kj && strncmp(a, kin, KINL) == 0) {
      strncpy(p + c, a, KINL);
      c += KINL;
      a += KINL;
#else
    if (!kj && iskanji(*a)) {
#endif
      kj = -1;
#if KIKO
    } else if (kj && strncmp(a, kout, KOUTL) == 0) {
      strncpy(p + c, a, KOUTL);
      c += KOUTL;
      a += KOUTL;
#else
    } else if (kj && !iskanji(*a)) {
#endif
      kj = 0;
    } else if (kj) {
      strncpy(p + c, a, 2);
      c += 2;
      a += 2;
    } else if (!kj && strncmp(a, "\\$", 2) == 0) {
      *(p + c) = '$';                  /* insert quoted $ */
      c++;
      a += 2;
    } else if (!kj && *a == '$') {
      ano = atoi(a + 1);
      if (ano < MAXARGS && isdigit(*(a + 1))) {
        *(p + c) = 0;
        if (args[ano]) {               /* replace $n to argument */
          if (debug)
            fprintf(errfile, "evalargs: $%d -> %s\n", ano, args[ano]);
          strncat(p, args[ano], MAXLL);
          c += strlen(args[ano]);
        }
        for (a++; isdigit(*a); a++);
      } else {                         /* escape meaning less $ */
        a++;
        if (*a == ' ')
          a++;                         /* "$ " is special escape sequence */
      }
    } else
      *(p + (c++)) = *(a++);
  }
  *(p + c) = 0;
  strncpy(page->inbuf, p, MAXLL);      /* replace input with evaled string */
  efree(p);
  return 0;
}
