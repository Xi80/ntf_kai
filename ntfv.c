/*
    (c) Copyright 1990, 1991  Tomoyuki Niijima
    All rights reserved
*/


#include "ntf.h"

static char *lp;
static char token[MAXLL + 1];

/* hash function for user variable */
unsigned int uvarhash(char *name)
{
  unsigned int hashval;

  for (hashval = 0; *name; name++)
    hashval = *name + 31 * hashval;
  return hashval % VARHASHN;
}

/* search user variable (VAR) in hash table */
VAR *searchvar(char *name)
{
  VAR *wp;

  for (wp = varhashtab[uvarhash(name)]; wp; wp = wp->next) {
    if (strcmp(wp->name, name) == 0)
      return wp;
  }
  return NULL;
}


/* pic up next effectibe token from expr string */
int next_token(char *buf, char *line)
{
  /* not use buf except for VARIABLE or CONSTANT */
  *buf = 0;
  /* init lp, lp keeps Last Pointer to expr */
  if (line)
    lp = line;
  while (*lp == ' ' || *lp == '\t')
    lp++;
  if (*lp == '\n' || *lp == 0)
    return -1;
  if (*lp == ')') {
    lp++;
    return CLOSEPARE;
  } else if (op('=', '=')) {
    lp += 2;
    return EQUAL;
  } else if (op('>', '=')) {
    lp += 2;
    return GEQUAL;
  } else if (op('<', '=')) {
    lp += 2;
    return LEQUAL;
  } else if (op('!', '=')) {
    lp += 2;
    return NEQUAL;
  } else if (op('/', '=')) {
    lp += 2;
    return DSTORE;
  } else if (op('*', '=')) {
    lp += 2;
    return MUSTORE;
  } else if (op('+', '=')) {
    lp += 2;
    return PSTORE;
  } else if (op('-', '=')) {
    lp += 2;
    return MISTORE;
  } else if (op('&', '&')) {
    lp += 2;
    return LAND;
  } else if (op('|', '|')) {
    lp += 2;
    return LOR;
  } else if (*lp == '(') {
    lp++;
    return OPENPARE;
  } else if (*lp == '=') {
    lp++;
    return STORE;
  } else if (*lp == '~') {
    lp++;
    return NOT;
  } else if (*lp == '!') {
    lp++;
    return LNOT;
  } else if (*lp == '>') {
    lp++;
    return GREATER;
  } else if (*lp == '<') {
    lp++;
    return LESS;
  } else if (*lp == '+') {
    lp++;
    return PLUS;
  } else if (*lp == '-') {
    lp++;
    return MINUS;
  } else if (*lp == '*') {
    lp++;
    return MULTI;
  } else if (*lp == '/') {
    lp++;
    return DIVIDE;
  } else if (*lp == '%') {
    lp++;
    return REMAIN;
  } else if (*lp == '&') {
    lp++;
    return AND;
  } else if (*lp == '|') {
    lp++;
    return OR;
  } else {
    /* if the first charactor is digit it must be constant */
    int c = 0, type = (isdigit(*lp) ? CONSTANT : VARIABLE);

    while (isalnum(*lp) || *lp == '_')
      *(buf + (c++)) = *(lp++);        /* return the figue of token */
    *(buf + c) = 0;                    /* in buf */
    if (c > 0)
      return type;
    else
      return -1;                       /* no more token in this expr */
  }
}

int getvar(char *name)
{                                      /* get variable value by name */
  VAR *wp;

  if (name == NULL)
    return 0;
  else if (strcmp(name, "pl") == 0)
    return cpage->pl;
  else if (strcmp(name, "cl") == 0)
    return cpage->plc;
  else if (strcmp(name, "hcl") == 0)
    return homepage->plc;
  else if (strcmp(name, "hll") == 0)
    return homepage->ll;
  else if (strcmp(name, "ll") == 0)
    return cpage->ll;
  else if (strcmp(name, "in") == 0)
    return cpage->in;
  else if (strcmp(name, "pn") == 0)
    return cpage->pnc;
  else if (strcmp(name, "fi") == 0)
    return (cpage->mode & FILLIN) != 0;
  else if (strcmp(name, "ju") == 0)
    return (cpage->mode & JUSTIFY) != 0;
  else if (strcmp(name, "tx") == 0)
    return (cpage->mode & TEXT) != 0;
  else if (strcmp(name, "mode") == 0)
    return cpage->mode;
  else if (strcmp(name, "it") == 0)
    return it;
  else if (strcmp(name, "debug") == 0)
    return debug;
  else if (strcmp(name, "attr") == 0)
    return cpage->nattr;
  else if (strcmp(name, "attrmask") == 0)
    return attrmask;
  else if (strcmp(name, "printer") == 0)
    return ptype;
  else if (strcmp(name, "outcode") == 0)
    return outcode;
  else if (strcmp(name, "putlc") == 0)
    return putlc;
  else if (strcmp(name, "putcc") == 0)
    return putcc;
  else if (strcmp(name, "pdfmode") == 0)
    return pdfmode;
  else if (strcmp(name, "pdfkj") == 0)
    return pdfkj;
  else if (strcmp(name, "pdf_objno") == 0)
    return pdf_objno;
  else if (strcmp(name, "kjmode") == 0)
    return fpkj;
  else if (strcmp(name, "argc") == 0) {
    int c = 0;
    char **p = csource->args;

    if (p++) {
      while (*p++)
        c++;
      return c;
    }
    return 0;
  } else if (strcmp(name, "pagec") == 0) {
    return pagec;
  } else if (strcmp(name, "pagenum") == 0) {
    return pagenum;
  } else if (*name == '_') {
    VAR *wp = cpage->lvar;

    while (wp) {
      if (strcmp(wp->name, name) == 0)
        return wp->value;
      wp = wp->next;
    }
    return 0;
  } else if (wp = searchvar(name))
    return wp->value;
  else
    return 0;
}

int setvar(char *name, int value)
{                                      /* bind value to variable */
  VAR *wp;

  if (name == NULL)
    return value;
  else if (strcmp(name, "pl") == 0)
    return cpage->pl = value;
  else if (strcmp(name, "cl") == 0)
    return cpage->plc = value;
  else if (strcmp(name, "ll") == 0)
    return cpage->ll = value;
  else if (strcmp(name, "in") == 0)
    return cpage->in = value;
  else if (strcmp(name, "pn") == 0)
    return cpage->pnc = value;
  else if (strcmp(name, "mode") == 0)
    return cpage->mode = value;
  else if (strcmp(name, "it") == 0)
    return it = value;
  else if (strcmp(name, "debug") == 0)
    return debug = value;
  else if (strcmp(name, "attr") == 0)
    return cpage->nattr = (value & ((~attrmask) | UNDERSCORE | HIGHLIGHT));
  else if (strcmp(name, "attrmask") == 0)
    return attrmask = value;
  else if (strcmp(name, "printer") == 0)
    return ptype = value;
  else if (strcmp(name, "outcode") == 0)
    return outcode = value;
  else if (strcmp(name, "putlc") == 0)
    return putlc = value;
  else if (strcmp(name, "putcc") == 0)
    return putcc = value;
  else if (strcmp(name, "pdfmode") == 0)
    return pdfmode = value;
  else if (strcmp(name, "pdfkj") == 0)
    return pdfkj = value;
  else if (strcmp(name, "pdf_objno") == 0)
    return pdf_objno = value;
  else if (strcmp(name, "kjmode") == 0)
    return fpkj = value;
  else if (*name == '_') {             /* PAGE local variable */
    wp = cpage->lvar;

    while (wp) {
      if (strcmp(wp->name, name) == 0)
        return (wp->value = value);
      wp = wp->next;
    }
    wp = (VAR *) emalloc(sizeof(VAR));
    wp->name = emalloc(strlen(name) + 1);
    strcpy(wp->name, name);
    wp->next = cpage->lvar;
    cpage->lvar = wp;
    wp->value = value;
    return value;
  } else {
    wp = searchvar(name);
    if (wp == NULL) {
      unsigned int uh = uvarhash(name);

      wp = (VAR *) emalloc(sizeof(VAR));
      wp->name = emalloc(strlen(name) + 1);
      strcpy(wp->name, name);
      if (debug && varhashtab[uh])
        fprintf(errfile, "setvar: entry chained.\n");
      wp->next = varhashtab[uh];
      varhashtab[uh] = wp;
    }
    wp->value = value;
    return value;
  }
}

int pushvn(char *name)
{                                      /* push variable to v_stack */
  if (vstkp == STACKMAX)
    fatal("evalexpr pushvn() stack overflow.", 1);
  vstk[vstkp].name = emalloc(strlen(name) + 1);
  strcpy(vstk[vstkp].name, name);
  vstk[vstkp].type = VARIABLE;
  vstkp++;
  return 0;
}

int pushv(int value)
{                                      /* push constant value to v_stack */
  if (vstkp == STACKMAX)
    fatal("evalexpr pushv() stack overflow.", 1);
  vstk[vstkp].value = value;
  vstk[vstkp].name = NULL;
  vstk[vstkp].type = CONSTANT;
  vstkp++;
  return value;
}

int popv(void)
{                                      /* pop value from v_stack */
  if (vstkp == 0) {
    emsg("evalexpr popv() stack underflow.");
    return 0;
  }
  if (vstk[vstkp - 1].type == VARIABLE) {       /* eval value if variable */
    vstk[vstkp - 1].value = getvar(vstk[vstkp - 1].name);
    efree(vstk[vstkp - 1].name);
    vstk[vstkp - 1].name = NULL;
  }
  return vstk[--vstkp].value;
}

int pusho(int opr)
{                                      /* push operator to o_stack */
  if (ostkp == STACKMAX)
    fatal("evalexpr pusho() stack overflow.", 1);
  ostk[ostkp++].type = opr;
  return 0;
}

int popo(void)
{                                      /* pop operator from o_stack */
  if (ostkp == 0) {
    emsg("evalexpr popo() stack underflow.");
    return -1;
  }
  return ostk[--ostkp].type;
}

int evalop(int opr)
{                                      /* eval operator */
  int c = 0, d = 0;

  switch (opr) {
  case MISTORE:
  case PSTORE:
  case DSTORE:
  case MUSTORE:
  case STORE:
    d = popv();
    if (vstk[vstkp - 1].type != VARIABLE) {
      emsg("can't store to CONSTANT.");
      break;
    }
    c = getvar(vstk[vstkp - 1].name);
    switch (opr) {
    case MISTORE:
      c -= d;
      break;
    case PSTORE:
      c += d;
      break;
    case DSTORE:
      if (d)
        c /= d;
      else {
        emsg("evalexpr division by zero.");
        c = 0;
      }
      break;
    case MUSTORE:
      c *= d;
      break;
    case STORE:
      c = d;
      break;
    }
    c = setvar(vstk[vstkp - 1].name, c);
    popv();
    c = pushv(c);
    break;
  case OR:
    c = popv();
    c = pushv(popv() | c);
    break;
  case AND:
    c = popv();
    c = pushv(popv() & c);
    break;
  case LOR:
    c = popv();
    c = pushv(popv() || c);
    break;
  case LAND:
    c = popv();
    c = pushv(popv() && c);
    break;
  case EQUAL:
    c = popv();
    c = pushv(popv() == c);
    break;
  case NEQUAL:
    c = popv();
    c = pushv(popv() != c);
    break;
  case GEQUAL:
    c = popv();
    c = pushv(popv() >= c);
    break;
  case LEQUAL:
    c = popv();
    c = pushv(popv() <= c);
    break;
  case GREATER:
    c = popv();
    c = pushv(popv() > c);
    break;
  case LESS:
    c = popv();
    c = pushv(popv() < c);
    break;
  case MINUS:
    c = popv();
    c = pushv(popv() - c);
    break;
  case PLUS:
    c = popv();
    c = pushv(popv() + c);
    break;
  case DIVIDE:
    c = popv();
    if (c != 0)
      c = pushv(popv() / c);
    else {
      emsg("evalexpr division by zero.");
      popv();
      c = pushv(0);
    }
    break;
  case MULTI:
    c = popv();
    c = pushv(popv() * c);
    break;
  case REMAIN:
    c = popv();
    if (c != 0)
      c = pushv(popv() % c);
    else {
      emsg("evalexpr division by zero.");
      popv();
      c = pushv(0);
    }
    break;
  case LNOT:
    c = popv();
    c = pushv(!c);
    break;
  case NOT:
    c = popv();
    c = pushv(~c);
    break;
  default:
    emsg("");
    fprintf(errfile, "invalid operation. op=%d \n", opr);
    c = 0;
    vstkp = 0;
    ostkp = 0;
    break;
  }
  return c;
}



int evalexpr(char *expr)
{
  char *p = expr;
  int tokentype, opr;

  if (debug)
    fprintf(errfile, "evalexpr:%s\n", expr);
  tokentype = next_token(token, p);
  while (1) {
    int left = (tokentype > 0);        /* left associative */
    int prec;

    prec = tokentype / 10;
    prec = (prec > 0 ? prec : -prec);  /* precedence */

    switch (tokentype) {
    case CONSTANT:
      pushv(atoi(token));
      break;
    case VARIABLE:
      pushvn(token);
      break;
    case CLOSEPARE:                   /* eval expr enclosed between () first */
      while (1) {
        opr = popo();
        if (opr == -1 || opr == OPENPARE)
          break;
        evalop(opr);
      }
      break;
    case OPENPARE:
    case MISTORE:
    case PSTORE:
    case DSTORE:
    case MUSTORE:
    case STORE:
    case OR:
    case AND:
    case LOR:
    case LAND:
    case EQUAL:
    case NEQUAL:
    case GEQUAL:
    case LEQUAL:
    case GREATER:
    case LESS:
    case MINUS:
    case PLUS:
    case DIVIDE:
    case MULTI:
    case REMAIN:
    case LNOT:
    case NOT:

      while (ostkp > 0) {
        int opprec;

        opr = popo();                  /* check operator priority */
        opprec = opr / 10;
        opprec = (opprec > 0 ? opprec : -opprec);
        if (((opprec == prec && left) ||
             (opprec > prec)) && opr != OPENPARE) {
          evalop(opr);                 /* eval it first */
        } else {
          pusho(opr);                  /* eval it later */
          break;
        }
      }
      pusho(tokentype);                /* keep it anyway */
      break;
    default:
      emsg("");
      fprintf(errfile, "evalexpr fomula error. %s", expr);
      fprintf(errfile, "         unknown token %s\n", token);
      return 0;
    }
    if ((tokentype = next_token(token, NULL)) == -1)
      break;
  }
  while (ostkp > 0) {                  /* eval expr left on stack */
    opr = popo();
    evalop(opr);
  }
  return popv();
}
