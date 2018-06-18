
#include <utilbasexx.h>

#include "pplib.h"

#define ignored_char(c) (c == 0x20 || c == 0x0A || c == 0x0D || c == 0x09 || c == 0x00)
#define newline_char(c) (c == 0x0A || c == 0x0D)
#define IGNORED_CHAR_CASE 0x20: case 0x0A: case 0x0D: case 0x09: case 0x00
#define NEWLINE_CHAR_CASE 0x0A: case 0x0D
#define DIGIT_CHAR_CASE '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9'
#define OCTAL_CHAR_CASE '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7'

#define MAX_INT_DIGITS 32

static const char * ppref_str (ppuint refnumber, ppuint refversion)
{
	static char buffer[MAX_INT_DIGITS + 1 + MAX_INT_DIGITS + 1 + 1 + 1];
	sprintf(buffer, PPUINTF " " PPUINTF " R", refnumber, refversion);
	return buffer;
}

/* name */

// pdf name delimiters: 0..32, ()<>[]{}/%
// # treated specially
// .+- are valid part of name; keep in mind names such as -| | |- .notdef ABCDEF+Font etc.
const char ppname_byte_lookup[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 1, 1, '#', 1, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

#define PPNAME_INIT (8+1)

#define ppname_flush(O, ghost, siz, flgs) \
  (iof_put(O, '\0'), \
   ghost = (_ppname *)ppheap_flush(O, &siz), \
   ghost->flags = flgs, \
   ghost->size = siz - sizeof(_ppname) - 1, \
  (ppname)(ghost + 1))

#define ppname_flush_with_ego(O, ghost, siz, flgs) \
  (iof_put(O, '\0'), \
   iof_ensure(O, sizeof(ppname *)), \
   O->pos += sizeof(ppname *), \
   ghost = (_ppname *)ppheap_flush(O, &siz), \
   ghost->flags = flgs, \
   ghost->size = siz - sizeof(_ppname) - 1 - sizeof(ppname *), \
  (ppname)(ghost + 1))

#define ppname_set_alter_ego(name, ghost, ego) (*((ppname *)(name + (ghost)->size + 1)) = ego)
#define ppname_get_alter_ego(name) (*((ppname *)(name + ppname_size(name) + 1)))

static ppname ppscan_name_or_exec (iof *I, ppheap **pheap, int flags)
{
  int c, decode;
  iof *O;
  _ppname *ghost1, *ghost2;
  ppname encoded, decoded;
  size_t size;
  const char *p, *e;
  uint8_t v1, v2;
  O = ppheap_buffer(pheap, sizeof(_ppname), PPNAME_INIT);
  for (decode = 0, c = iof_char(I); c >= 0 && ppname_byte_lookup[c]; c = iof_next(I))
  {
    if (c == '#')
      decode = 1;
    iof_put(O, c);
  }
  if (!decode)
    return ppname_flush(O, ghost1, size, flags);
  encoded = ppname_flush_with_ego(O, ghost1, size, flags|PPNAME_ENCODED);
  O = ppheap_buffer(pheap, sizeof(_ppname), PPNAME_INIT);
  for (p = encoded, e = encoded + ghost1->size; p < e; ++p)
  {
    if (*p == '#' && p + 2 < e && (v1 = base16_value(p[1])) >= 0 && (v2 = base16_value(p[2])) >= 0)
      iof_put(O, ((v1<<4)+v2));
    else
      iof_put(O, *p);
  }
  decoded = ppname_flush_with_ego(O, ghost2, size, flags|PPNAME_DECODED);
  ppname_set_alter_ego(encoded, ghost1, decoded);
  ppname_set_alter_ego(decoded, ghost2, encoded);
  return encoded;
}

#define ppscan_name(I, pheap) ppscan_name_or_exec(I, pheap, 0)
#define ppscan_exec(I, pheap) ppscan_name_or_exec(I, pheap, PPNAME_EXEC)

ppname ppname_decoded (ppname name)
{
  _ppname *ghost;
  ghost = _ppname_ghost(name);
  return (ghost->flags & PPNAME_ENCODED) ? ppname_get_alter_ego(name) : name;
}

ppname ppname_encoded (ppname name)
{
  _ppname *ghost;
  ghost = _ppname_ghost(name);
  return (ghost->flags & PPNAME_DECODED) ? ppname_get_alter_ego(name) : name;
}

/* string */

#define PPSTRING_INIT (16+1)

#define ppstring_flush(O, ghost, siz, flgs) \
  (iof_put(O, '\0'), \
   ghost = (_ppstring *)ppheap_flush(O, &siz), \
   ghost->flags = flgs, \
   ghost->size = siz - sizeof(_ppstring) - 1, \
  (ppstring)(ghost + 1))

#define ppstring_flush_with_ego(O, ghost, siz, flgs) \
  (iof_put(O, '\0'), \
   iof_ensure(O, sizeof(ppstring *)), \
   O->pos += sizeof(ppstring *), \
   ghost = (_ppstring *)ppheap_flush(O, &siz), \
   ghost->flags = flgs, \
   ghost->size = siz - sizeof(_ppstring) - 1 - sizeof(ppstring *), \
  (ppstring)(ghost + 1))

#define ppstring_set_alter_ego(string, ghost, ego) (*((ppstring *)(string + (ghost)->size + 1)) = ego)
#define ppstring_get_alter_ego(string) (*((ppstring *)(string + ppstring_size(string) + 1)))

static ppstring ppscan_string (iof *I, ppheap **pheap)
{
  int c, decode, balance;
  iof *O;
  _ppstring *ghost1, *ghost2;
  const char *p, *e;
  ppstring encoded, decoded;
  size_t size;
  O = ppheap_buffer(pheap, sizeof(_ppstring), PPSTRING_INIT);
  for (decode = 0, balance = 0, c = iof_char(I); c >= 0; )
  {
    switch (c)
    {
      case '\\':
        decode = 1; // unescaping later
        iof_put(O, '\\');
        if ((c = iof_next(I)) >= 0)
        {
          iof_put(O, c);
          c = iof_next(I);
        }
        break;
      case '(': // may be unescaped if balanced
        ++balance;
        iof_put(O, '(');
        c = iof_next(I);
        break;
      case ')':
        if (balance == 0)
        {
          c = IOFEOF;
          ++I->pos;
          break;
        }
        --balance;
        iof_put(O, ')');
        c = iof_next(I);
        break;
      default:
        iof_put(O, c);
        c = iof_next(I);
    }
  }
  if (!decode)
  {
    encoded = ppstring_flush(O, ghost1, size, 0);
    if (ghost1->size >= 2)
    { // theoretically any bytes can be there
      if (encoded[0] == 0xFE && encoded[1] == 0xFF)
        ghost1->flags |= PPSTRING_UTF16BE;
      else if (encoded[0] == 0xFF && encoded[1] == 0xFE)
        ghost1->flags |= PPSTRING_UTF16LE;
    }
    return encoded;
  }
  encoded = ppstring_flush_with_ego(O, ghost1, size, 0|PPSTRING_ENCODED);
  O = ppheap_buffer(pheap, sizeof(_ppstring), PPSTRING_INIT);
  for (p = encoded, e = encoded + ghost1->size; p < e; ++p)
  {
    if (*p == '\\' && ++p < e)
    {
      switch (*p)
      {
        case OCTAL_CHAR_CASE:
          c = *p - '0';
          if (++p < e && *p >= '0' && *p <= '7')
          {
            c = (c << 3) + c - '0';
            if (++p < e && *p >= '0' && *p <= '7')
              c = (c << 3) + c - '0';
          }
          iof_put(O, c);
          break;
        case 'n':
          iof_put(O, '\n');
          break;
        case 'r':
          iof_put(O, '\r');
          break;
        case 't':
          iof_put(O, '\t');
          break;
        case 'b':
          iof_put(O, '\b');
          break;
        case 'f':
          iof_put(O, '\f');
          break;
        case NEWLINE_CHAR_CASE: // not a part of the string, ignore (pdf spec page 55)
          break;
        case '(': case ')': case '\\':
        default: // for enything else backslash is ignored (pdf spec page 54)
          iof_put(O, *p);
          break;
      }
    }
    else
      iof_put(O, *p);
  }
  decoded = ppstring_flush_with_ego(O, ghost2, size, 0|PPSTRING_DECODED);
  if (ghost2->size >= 2)
  {
    if (decoded[0] == 0xFE && decoded[1] == 0xFF)
    {
      ghost1->flags |= PPSTRING_UTF16BE;
      ghost2->flags |= PPSTRING_UTF16BE;
    }
    else if (decoded[0] == 0xFF && decoded[1] == 0xFE)
    {
      ghost1->flags |= PPSTRING_UTF16LE;
      ghost2->flags |= PPSTRING_UTF16LE;
    }
  }
  ppstring_set_alter_ego(encoded, ghost1, decoded);
  ppstring_set_alter_ego(decoded, ghost2, encoded);
  return encoded;
}

/* Hex string may contain white characters. If odd number of digits, the last assumed to be '0' */

static ppstring ppscan_base16 (iof *I, ppheap **pheap)
{
  int c, v1, v2;
  iof *O;
  _ppstring *ghost1, *ghost2;
  size_t size;
  ppstring encoded, decoded;
  const char *p, *e;
  O = ppheap_buffer(pheap, sizeof(_ppstring), PPSTRING_INIT);
  for (c = iof_char(I); c >= 0 && (base16_digit(c) || ignored_char(c)); c = iof_next(I))
    iof_put(O, c);
  if (c == '>')
    ++I->pos;
  encoded = ppstring_flush_with_ego(O, ghost1, size, PPSTRING_BASE16|PPSTRING_ENCODED);
  O = ppheap_buffer(pheap, sizeof(_ppstring), (ghost1->size >> 1) + 1);
  for (p = encoded, e = encoded + ghost1->size; p < e; ++p)
  {
    if ((v1 = base16_value(*p)) < 0) // ignored
      continue;
    for (v2 = 0, ++p; p < e && (v2 = base16_value(*p)) < 0; ++p);
    iof_put(O, (v1<<4)|v2);
  }
  decoded = ppstring_flush_with_ego(O, ghost2, size, 0|PPSTRING_DECODED);
  if (ghost2->size >= 2)
  {
    if (decoded[0] == 0xFE && decoded[1] == 0xFF)
    {
      ghost1->flags |= PPSTRING_UTF16BE;
      ghost2->flags |= PPSTRING_UTF16BE;
    }
    else if (decoded[0] == 0xFF && decoded[1] == 0xFE)
    {
      ghost1->flags |= PPSTRING_UTF16LE;
      ghost2->flags |= PPSTRING_UTF16LE;
    }
  }
  ppstring_set_alter_ego(encoded, ghost1, decoded);
  ppstring_set_alter_ego(decoded, ghost2, encoded);
  return encoded;
}

/* PDF spec says nothing about base85 strings, but streams might be (afair no leading '<~' but resent trailing '~>') */

static ppstring ppscan_base85 (iof *I, ppheap **pheap)
{ // bawse85 alphabet is 33.117, adobe also hires 'z' and 'y' for compression
  int c;
  iof *O, B;
  _ppstring *ghost1, *ghost2;
  size_t size;
  ppstring encoded, decoded;
  O = ppheap_buffer(pheap, sizeof(_ppstring), PPSTRING_INIT);
  for (c = iof_char(I); (c >= '!' && c <= 'u') || c == 'z' || c == 'y'; c = iof_next(I))
    iof_put(O, c);
  if (c == '~')
    if ((c = iof_next(I)) == '>')
      ++I->pos;
  encoded = ppstring_flush_with_ego(O, ghost1, size, PPSTRING_BASE85|PPSTRING_ENCODED);
  iof_string_reader(&B, encoded, ghost1->size);
  O = ppheap_buffer(pheap, sizeof(_ppstring), (ghost1->size * 5 / 4) + 1);
  base85_decode(&B, O);
  decoded = ppstring_flush_with_ego(O, ghost2, size, 0|PPSTRING_DECODED);
  if (ghost2->size >= 2)
  {
    if (decoded[0] == 0xFE && decoded[1] == 0xFF)
    {
      ghost1->flags |= PPSTRING_UTF16BE;
      ghost2->flags |= PPSTRING_UTF16BE;
    }
    else if (decoded[0] == 0xFF && decoded[1] == 0xFE)
    {
      ghost1->flags |= PPSTRING_UTF16LE;
      ghost2->flags |= PPSTRING_UTF16LE;
    }
  }
  ppstring_set_alter_ego(encoded, ghost1, decoded);
  ppstring_set_alter_ego(decoded, ghost2, encoded);
  return encoded;
}

ppstring ppstring_decoded (ppstring string)
{
  _ppstring *ghost;
  ghost = _ppstring_ghost(string);
  return (ghost->flags & PPSTRING_ENCODED) ? ppstring_get_alter_ego(string) : string;
}

ppstring ppstring_encoded (ppstring string)
{
  _ppstring *ghost;
  ghost = _ppstring_ghost(string);
  return (ghost->flags & PPSTRING_DECODED) ? ppstring_get_alter_ego(string) : string;
}

/* scanner stack */

#define PPSTACK_BUFFER 512

static void ppstack_init (ppstack *stack, ppheap **pheap)
{
  stack->buf = stack->pos = (ppobj *)pp_malloc(PPSTACK_BUFFER * sizeof(ppobj));
  stack->size = 0;
  stack->space = PPSTACK_BUFFER;
  stack->pheap = pheap;
}

#define ppstack_free_buffer(stack) (pp_free((stack)->buf))

static void ppstack_resize (ppstack *stack)
{
  ppobj *newbuffer;
  stack->space <<= 1;
  newbuffer = (ppobj *)pp_malloc(stack->space * sizeof(ppobj));
  memcpy(newbuffer, stack->buf, stack->size * sizeof(ppobj));
  ppstack_free_buffer(stack);
  stack->buf = newbuffer;
  stack->pos = newbuffer + stack->size;
}

#define ppstack_push(stack) ((void)((stack)->size < (stack)->space || (ppstack_resize(stack), 0)), ++(stack)->size, (stack)->pos++)
#define ppstack_pop(stack, n) ((stack)->size -= (n), (stack)->pos -= (n))
#define ppstack_at(stack, i) ((stack)->buf + i)
#define ppstack_clear(stack) ((stack)->pos = (stack)->buf, (stack)->size = 0)

/* scanner commons */

#define ppscan_uint(I, u) iof_get_uintlw(I, u)
#define ppread_uint(s, u) string_to_uintlw((const char *)(s), u)

static ppobj * ppscan_numobj (iof *I, ppobj *obj, int negative)
{
  ppint integer;
  ppnum number;
  int exponent;
  int c;
  c = iof_char(I);
  iof_scan_integer(I, c, integer);
  switch(c)
  {
    case '.':
    {
      number = (ppnum)integer;
      c = iof_next(I);
      iof_scan_fraction(I, c, number, exponent);
      double_negative_exp10(number, exponent);
      obj->type = PPNUM, obj->number = negative ? -number : number;
      break;
    }
    default:
      obj->type = PPINT, obj->integer = negative ? -integer : integer;
      break;
  }
  return obj;
}


static int ppscan_find (iof *I)
{ // skips whitechars and comments
  int c;
  for (c = iof_char(I); ; c = iof_next(I))
  {
    switch (c)
    {
      case IGNORED_CHAR_CASE:
        break;
      case '%': {
        do {
          if ((c = iof_next(I)) < 0)
            return c;
        } while (!newline_char(c));
        break;
      }
      default:
        return c;
    }
  }
  return c; // never reached
}

static int ppscan_keyword (iof *I, const char *keyword, size_t size)
{
  size_t i;
  int c;
  if (iof_left(I) >= size)
  {
    if (memcmp(I->pos, keyword, size) != 0)
      return 0;
    I->pos += size;
    return 1;
  }
  // sticky case, we can't go back
  for (i = 0, c = iof_char(I); i < size; ++i, ++keyword, c = iof_next(I))
    if (i != c)
      return 0;
  return 1;
}

#define ppscan_key(I, literal) ppscan_keyword(I, "" literal, sizeof(literal) - 1)

/* objects parser */

static ppref * ppref_unresolved (ppheap **pheap, ppuint refnumber, ppuint refversion)
{
  ppref *ref = ppheap_take(pheap, sizeof(ppref));
  memset(ref, 0, sizeof(ppref));
  ref->object.type = PPNONE;
  ref->number = refnumber;
  ref->version = refversion;
  return ref;
}

#define PPMARK PPNONE

ppobj * ppscan_obj (iof *I, ppstack *stack, ppxref *xref)
{
  int c;
  ppobj *obj;
  size_t mark, size;
  ppuint refnumber, refversion;
  ppref *ref;

  c = iof_char(I);
  switch (c)
  {
    case DIGIT_CHAR_CASE:
      return ppscan_numobj(I, ppstack_push(stack), 0);
    case '+':
      ++I->pos;
      return ppscan_numobj(I, ppstack_push(stack), 0);
    case '-':
      ++I->pos;
      return ppscan_numobj(I, ppstack_push(stack), 1);
    case '/':
      ++I->pos;
      obj = ppstack_push(stack);
      obj->type = PPNAME;
      obj->name = ppscan_name(I, stack->pheap);
      return obj;
    case '(':
      ++I->pos;
      obj = ppstack_push(stack);
      obj->type = PPSTRING;
      obj->string = ppscan_string(I, stack->pheap);
      return obj;
    case '[':
      mark = stack->size;
      obj = ppstack_push(stack);
      obj->type = PPMARK; // ppscan_obj() checks types backward for 'R', so set the type immediatelly (reserved for PPARRAY)
      obj->any = NULL;
      ++I->pos;
      for (c = ppscan_find(I); c != ']'; c = ppscan_find(I))
      {
        if (ppscan_obj(I, stack, xref) == NULL)
        { // callers assume that NULL returns means nothing pushed
          size = stack->size - mark; // pop items AND the obj reserved for array
          ppstack_pop(stack, size);
          return NULL;
        }
      }
      ++I->pos;
      size = stack->size - mark - 1;
      obj = ppstack_at(stack, mark); // stack might have been realocated
      obj->type = PPARRAY;
      obj->array = pparray_create(ppstack_at(stack, mark + 1), size, stack->pheap);
      ppstack_pop(stack, size); // pop array items, leave the array on top
      return obj;
    case '<':
      if ((c = iof_next(I)) == '<')
      {
        mark = stack->size;
        obj = ppstack_push(stack);
        obj->type = PPMARK;
        obj->any = NULL;
        ++I->pos;
        for (c = ppscan_find(I); c != '>'; c = ppscan_find(I))
        {
          if (ppscan_obj(I, stack, xref) == NULL)
          {
            printf("FAILED DICT\n");
            size = stack->size - mark;
            ppstack_pop(stack, size);
            return NULL;
          }
        }
        if (iof_next(I) == '>')
          ++I->pos;
        size = stack->size - mark - 1;
        obj = ppstack_at(stack, mark);
        obj->type = PPDICT;
        obj->dict = ppdict_create(ppstack_at(stack, mark + 1), size, stack->pheap);
        ppstack_pop(stack, size);
        return obj;
      }
      obj = ppstack_push(stack);
      obj->type = PPSTRING;
      if (0) // c == '~' postscript only
        ++I->pos, obj->string = ppscan_base85(I, stack->pheap);
      else
        obj->string = ppscan_base16(I, stack->pheap);
      return obj;
    case 'R':
      if (stack->size >= 2 && stack->pos[-1].type == PPINT && stack->pos[-2].type == PPINT)
      {
        ++I->pos;
        obj = &stack->pos[-2];
        refnumber = (ppuint)obj->integer;
        ppstack_pop(stack, 1); // pop version number, retype obj to a reference
        if (xref == NULL || (ref = ppxref_find(xref, refnumber)) == NULL)
        { /* pdf spec page 64: unresolvable reference is not an error, should just be treated as a reference to null.
             we also need this to read trailer, where refs can't be resolved yet */
          refversion = (obj + 1)->integer;
          if (xref != NULL) // not a trailer? mess for a while
            PPMESS("unresolved reference %s\n", ppref_str(refnumber, refversion));
          ref = ppref_unresolved(stack->pheap, refnumber, refversion);
        }
        obj->type = PPREF;
        obj->ref = ref;
        return obj;
      }
      break;
    case 't':
      if (iof_next(I) == 'r' && iof_next(I) == 'u' && iof_next(I) == 'e')
      {
        ++I->pos;
        obj = ppstack_push(stack);
        obj->type = PPBOOL;
        obj->integer = 1;
        return obj;
      }
      break;
    case 'f':
      if (iof_next(I) == 'a' && iof_next(I) == 'l' && iof_next(I) == 's' && iof_next(I) == 'e')
      {
        ++I->pos;
        obj = ppstack_push(stack);
        obj->type = PPBOOL;
        obj->integer = 0;
        return obj;
      }
      break;
    case 'n':
      if (iof_next(I) == 'u' && iof_next(I) == 'l' && iof_next(I) == 'l')
      {
        ++I->pos;
        obj = ppstack_push(stack);
        obj->type = PPNULL;
        obj->any = NULL;
        return obj;
      }
      break;
  }
  return NULL;
}

/* A variant for contents streams (aka postscript); wise of operators, blind to references.
We are still PDF, so we don't care about postscript specific stuff such as radix numbers
and scientific numbers notation. */

ppobj * ppscan_psobj (iof *I, ppstack *stack)
{
  int c;
  ppobj *obj;
  size_t size, mark;
  ppname exec;

  c = iof_char(I);

  switch (c)
  {
    case DIGIT_CHAR_CASE:
      return ppscan_numobj(I, ppstack_push(stack), 0);
    case '+':
      c = iof_next(I);
      if (base10_digit(c) || c == '.') // '+.abc' is probably an executable name, but we are not in postscript
        return ppscan_numobj(I, ppstack_push(stack), 0);
      obj = ppstack_push(stack);
      obj->type = PPNAME;
      obj->name = ppscan_exec(I, stack->pheap);
      return obj;
    case '-':
      c = iof_next(I);
      if (base10_digit(c) || c == '.') // ditto, we would handle type1 '-|' '|-' operators though
        return ppscan_numobj(I, ppstack_push(stack), 1);
      obj = ppstack_push(stack);
      obj->type = PPNAME;
      obj->name = ppscan_exec(I, stack->pheap);
      return obj;
    case '/':
      ++I->pos;
      obj = ppstack_push(stack);
      obj->type = PPNAME;
      obj->name = ppscan_name(I, stack->pheap);
      return obj;
    case '(':
      ++I->pos;
      obj = ppstack_push(stack);
      obj->type = PPSTRING;
      obj->string = ppscan_string(I, stack->pheap);
      return obj;
    case '[':
      mark = stack->size;
      obj = ppstack_push(stack);
      obj->type = PPMARK;
      obj->any = NULL;
      ++I->pos;
      for (c = ppscan_find(I); c != ']'; c = ppscan_find(I))
      {
        if (ppscan_psobj(I, stack) == NULL)
        {
          size = stack->size - mark;
          ppstack_pop(stack, size);
          return NULL;
        }
      }
      ++I->pos;
      size = stack->size - mark - 1;
      obj = ppstack_at(stack, mark);
      obj->type = PPARRAY;
      obj->array = pparray_create(ppstack_at(stack, mark + 1), size, stack->pheap);
      ppstack_pop(stack, size);
      return obj;
    case '<':
      if ((c = iof_next(I)) == '<')
      {
        mark = stack->size;
        obj = ppstack_push(stack);
        obj->type = PPMARK;
        obj->any = NULL;
        ++I->pos;
        for (c = ppscan_find(I); c != '>'; c = ppscan_find(I))
        {
          if (ppscan_psobj(I, stack) == NULL)
          {
            size = stack->size - mark;
            ppstack_pop(stack, size);
            return NULL;
          }
        }
        if (iof_next(I) == '>')
          ++I->pos;
        size = stack->size - mark - 1;
        obj = ppstack_at(stack, mark);
        obj->type = PPDICT;
        obj->dict = ppdict_create(ppstack_at(stack, mark + 1), size, stack->pheap);
        ppstack_pop(stack, size);
        return obj;
      }
      obj = ppstack_push(stack);
      obj->type = PPSTRING;
      if (c == '~')
        ++I->pos, obj->string = ppscan_base85(I, stack->pheap);
      else
        obj->string = ppscan_base16(I, stack->pheap);
      return obj;
    default:
      if (c < 0)
        break;
      exec = ppscan_exec(I, stack->pheap);
      obj = ppstack_push(stack);
      switch (exec[0])
      {
        case 't':
          if (exec[1] == 'r' && exec[2] == 'u' && exec[3] == 'e' && exec[4] == '\0')
          {
            obj->type = PPBOOL;
            obj->integer = 1;
            return obj;
          }
          break;
        case 'f':
          if (exec[1] == 'a' && exec[2] == 'l' && exec[3] == 's' && exec[4] == 'e' && exec[5] == '\0')
          {
            obj->type = PPBOOL;
            obj->integer = 0;
            return obj;
          }
          break;
        case 'n':
          if (exec[1] == 'u' && exec[2] == 'l' && exec[3] == 'l' && exec[4] == '\0')
          {
            obj->type = PPNULL;
            obj->any = NULL;
            return obj;
          }
          break;
      }
      obj->type = PPNAME;
      obj->name = exec;
      return obj;
  }
  return NULL;
}

/* input reader */

/*
PDF input is a pseudo file that either keeps FILE * or data. Reader iof * is a proxy to input
that provides byte-by-byte interface. Our iof structure is capable to link iof_file *input,
but t avoid redundant checks on IOF_DATA flag, here we link iof *I directly to FILE * or mem buffer.
When reading from file we need an internal buffer, which should be kept rather small, as it is
only used to parse xrefs and objects (no streams). We allocate the buffer from a private heap
(not static) to avoid conflicts when processing >1 pdfs at once. Besides, the input buffer may be
needed after loading the document, eg. to access references raw data.
*/

#define PPDOC_BUFFER 0xFFF // keep that small, it is only used to parse body objects

static void ppdoc_reader_init (ppdoc *pdf, iof_file *input)
{
  iof *I;
  pdf->input = *input;
  input = &pdf->input;
  input->refcount = 1;
  I = &pdf->reader;
  if (input->flags & IOF_DATA)
  {
    pdf->buffer = NULL;            // input iof_file is the buffer
    iof_string_reader(I, NULL, 0); // gets IOF_DATA flag
  }
  else
  {
    pdf->buffer = ppheap_take(&pdf->heap, PPDOC_BUFFER);
    iof_setup_file_handle_reader(I, NULL, 0, iof_file_get_fh(input)); // gets IOF_FILE_HANDLE flag and FILE *
    I->space = PPDOC_BUFFER; // used on refill
  }
}

/*
Whenever we need to read the input file, we fseek the to the given offset and fread to to the private buffer.
The length we need is not always predictable, in which case PPDOC_BUFFER bytes are read (keep it small).
I->buf = I->pos is set to the beginning, I->end set to the end (end is the first byte one shouldn't read).
*/

#define PP_LENGTH_UNKNOWN ((size_t)-1)

static iof * ppdoc_reader (ppdoc *pdf, size_t offset, size_t length)
{
  iof_file *input;
  iof *I;
  input = &pdf->input;
  I = &pdf->reader;
  if (iof_file_seek(input, offset, SEEK_SET) != 0)
    return NULL;
  if (input->flags & IOF_DATA)
  {
    I->buf = I->pos = input->pos;
    I->end = (length == PP_LENGTH_UNKNOWN || I->pos + length >= input->end) ? input->end : (I->pos + length);
  }
  else
  {
    I->buf = I->pos = pdf->buffer; // ->buf is actually permanently equal pdf->buffer but we might need some tricks
    if (length == PP_LENGTH_UNKNOWN || length > PPDOC_BUFFER)
      length = PPDOC_BUFFER;
    length = fread(I->buf, 1, length, I->file);
    I->end = I->buf + length;
  }
  return I;
}

/* The position from the beginning of input
- for data buffer: (pdf->input.pos - pdf->input.buf) + (I->pos - I->buf)
  I->buf == pdf->input.pos, so this resolves to (I->pos - pdf->input.buf), independent from I->buf
- for file buffer: ftell(pdf->input.file) - (I->end - I->pos)
*/

#define ppdoc_reader_tell(pdf, I) ((size_t)(((pdf)->input.flags & IOF_DATA) ? ((I)->pos - (pdf)->input.buf) : (ftell(iof_file_get_fh(&(pdf)->input)) - ((I)->end - (I)->pos))))

/* pdf */

#define PPDOC_HEADER 10 // "%PDF-?.??\n"

static int ppdoc_header (ppdoc *pdf, uint8_t header[PPDOC_HEADER])
{
  size_t i;
  if (memcmp(header, "%PDF-", 5) != 0)
    return 0;
  for (i = 5; i < PPDOC_HEADER - 1 && !ignored_char(header[i]); ++i)
    pdf->version[i - 5] = header[i];
  pdf->version[i - 5] = '\0';
  return 1;
}

static int ppdoc_tail (ppdoc *pdf, iof_file *input, size_t *pxrefoffset)
{
  int c;
  uint8_t tail[4*10], *p, back, tailbytes;

  if (iof_file_seek(input, 0, SEEK_END) != 0)
    return 0;
  pdf->filesize = (size_t)iof_file_tell(input);
  // simple heuristic to avoif fgetc() / fseek(-2) hiccup: keep seeking back by len(startxref) + 1 == 10
  // until a letter found (assuming liberal white characters and tail length)
  for (back = 1, tailbytes = 0; ; ++back)
  {
    if (iof_file_seek(input, -10, SEEK_CUR) != 0)
      return 0;
    tailbytes += 10;
    c = iof_file_getc(input);
    tailbytes -= 1;
    switch (c)
    {
      case IGNORED_CHAR_CASE:
      case DIGIT_CHAR_CASE:
      case '%': case 'E': case 'O': case 'F':
        if (back > 4) // 2 should be enough
          return 0;
        continue;
      case 's': case 't': case 'a': case 'r': case 'x': case 'e': case 'f':
        if (iof_file_read(tail, 1, tailbytes, input) != tailbytes)
          return 0;
        tail[tailbytes] = '\0';
        for (p = &tail[0]; ; ++p)
        {
          if (*p == '\0')
            return 0;
          if ((c = base10_value(*p)) >= 0)
            break;
        }
        ppread_uint(p, pxrefoffset);
        return 1;
      default:
        return 0;
    }
  }
  return 0;
}

/* xref/body */

static int ppscan_start_entry (iof *I, ppref *ref)
{
  ppuint u;
  ppscan_find(I); if (!ppscan_uint(I, &u) || u != ref->number) return 0;
  ppscan_find(I); if (!ppscan_uint(I, &u) || u != ref->version) return 0;
  ppscan_find(I); if (!ppscan_key(I, "obj")) return 0;
  ppscan_find(I);
  return 1;
}

static int ppscan_skip_entry (iof *I)
{
  size_t u;
  ppscan_find(I); if (!ppscan_uint(I, &u)) return 0;
  ppscan_find(I); if (!ppscan_uint(I, &u)) return 0;
  ppscan_find(I); if (!ppscan_key(I, "obj")) return 0;
  ppscan_find(I);
  return 1;
}

static int ppscan_start_stream (iof *I, ppdoc *pdf, size_t *streamoffset)
{
  int c;
  ppscan_find(I);
  if (ppscan_key(I, "stream"))
  { // skip 1 or 2 whites (here we shouldn't just gobble all blanks)
    c = iof_char(I);
    if (ignored_char(c))
    {
      c = iof_next(I);
      if (ignored_char(c))
        ++I->pos;
    }
    *streamoffset = ppdoc_reader_tell(pdf, I);
    return 1;
  }
  return 0;
}

static ppxref * ppxref_load (ppdoc *pdf, size_t xrefoffset);
static int ppxref_load_prev (ppdoc *pdf, ppxref *xref);

/* Parsing xref table

  1 10               // first ref number and refs count
  0000000000 00000 n // 10-digits offset, 5 digits version, type identifier
  0000000000 00000 n // n states for normal I guess
  0000000000 00000 f // f states for free (not used)
  ...

Free entries seem to be a relic of ancient times, completelly useless for us. To avoid parsing xref table twice,
we waste some space on free entries by allocating one plane of refs for each section. Later on we slice sections,
so that effectively free entries are not involved in map.

Subsequent refs gets number, version and offset. Other fields initialized when parsing PDF body.

Having xref table loaded, we sort sections for future binary search (xref with objects count == 0 is considered invalid).

Then we have to deal with the trailer dict. In general, to load objects and resolve references we need a complete chain
of xrefs (not only the top). To load the previous xref, we need its offset, which is given in trailer. So we have to
parse the trailer ignoring references, which might be unresolvable at this point (objects parser makes a dummy check
for xref != NULL on refs resolving ppscan_obj(), which irritates me but I don't want a separate parser for trailer..).
The same applies to xref streams, in which we have parse the trailer not having xref map at all. So the procedure is:

  - load xref map, initialize references, make it ready to search
  - parse trailer ignoring references
  - get /Prev xref offset and load older xref (linked list via ->prev)
  - sort all refs in all xrefs by offset
  - parse refs in order resolving references in contained objects
  - fix trailer references

First created xref becomes a pdf->xref (top xref). We link that early to control offsets already read (insane loops?).
*/

// Every xref table item "0000000000 00000 n" is said to be terminated with 2-byte EOL but we don't like relying on whites.
#define xref_item_length (10+1+5+1+1)

static ppxref * ppxref_load_table (iof *I, ppdoc *pdf, size_t xrefoffset)
{
  ppxref *xref;
  ppxsec *xrefsection;
  ppref *ref;
  ppuint first, count, refindex;
  uint8_t buffer[xref_item_length + 1];
  const char *p;
  const ppobj *obj;

  buffer[xref_item_length] = '\0';
  xref = ppxref_create(pdf, 0, xrefoffset);
  if (pdf->xref == NULL) pdf->xref = xref;
  for (ppscan_find(I); ppscan_uint(I, &first); ppscan_find(I))
  {
    ppscan_find(I);
    if (!ppscan_uint(I, &count))
      return NULL;
    if (count == 0) // weird
      continue;
    xref->count += count;
    xrefsection = NULL;
    ref = ppheap_take(&pdf->heap, count * sizeof(ppref));
    for (refindex = 0; refindex < count; ++refindex, ++ref)
    {
      ref->xref = xref;
      ref->number = first + refindex;
      ppscan_find(I);
      iof_read(I, buffer, xref_item_length);
      switch (buffer[xref_item_length - 1])
      {
        case 'n':
          if (xrefsection == NULL)
          {
            xrefsection = ppxref_push_section(xref, &pdf->heap);
            xrefsection->first = ref->number;
            xrefsection->refs = ref;
          }
          xrefsection->last = ref->number;
          for (p = (const char *)buffer; *p == '0'; ++p);
          p = ppread_uint(p, &ref->offset);
          for ( ; *p == ' ' || *p == '0'; ++p);
          p = ppread_uint(p, &ref->version);
          ref->object.type = PPNONE; // init for sanity
          ref->object.any = NULL;
          ref->length = 0;
          break;
        case 'f':
        default:
          --ref;
          xrefsection = NULL;
          --xref->count;
      }
    }
  }
  /* sort section */
  if (!ppxref_sort(xref))
    return NULL; // xref->size == 0
  /* get trailer ignoring refs */
  if (!ppscan_key(I, "trailer"))
    return NULL;
  ppscan_find(I);
  if ((obj = ppscan_obj(I, &pdf->stack, NULL)) == NULL)
    return NULL;
  ppstack_pop(&pdf->stack, 1);
  if (obj->type != PPDICT)
    return NULL;
  xref->trailer = *obj;
  /* load older xref (recursive. some check for insane cyclic offsets?) */
  if (!ppxref_load_prev(pdf, xref))
    return NULL;
  return xref;
}

/* Parsing xref stream
First we load the trailer, ignoring references. Dict defines sections and fields lengths:

  /Size                                  % max ref number plus 1
  /Index [ first count first count ... ] % a pair of numbers for every section, defaults to [0 Size]
  /W [w1 w2 w3]                          % fields lengths, 0 states for omitted field

xref stream data is a continuous stream of binary number triplets. First number is a type:

  0 - free entry (as 'f' in xref table)
  1 - normal entry, followed by offset an version (as 'n' in xref table)
  2 - compressed entry, followed by parent object stream number and entry index

0 and 1 are handled as 'n' and 'f' entries in xref table. For type 2 we normally initialize
ref->number and ref->version (the later is implicitly 0). ref->offset is set to 0 (invalid offset),
which is recognized by objects loader.
*/

#define XREF_STREAM_MAX_FIELD 4

static ppxref * ppxref_load_stream (iof *I, ppdoc *pdf, size_t xrefoffset)
{
  ppxref *xref;
  ppxsec *xrefsection;
  ppref *ref;
  ppobj *obj;
  ppstream *xrefstream;
  size_t streamoffset;
  ppuint w1, w2, w3, w, bufferbytes;
  uint8_t buffer[3 * XREF_STREAM_MAX_FIELD], *b;
  ppuint first, count, f1, f2, f3;
  pparray *fieldwidths, *sectionindices;
  ppobj sectionmock[2], *sectionfirst, *sectioncount;
  size_t sections, sectionindex, refindex;

  if (!ppscan_skip_entry(I))
    return NULL;
  if ((obj = ppscan_obj(I, &pdf->stack, NULL)) == NULL)
    return NULL;
  ppstack_pop(&pdf->stack, 1);
  if (obj->type != PPDICT || !ppscan_start_stream(I, pdf, &streamoffset))
    return NULL;
  xrefstream = ppstream_create(pdf, obj->dict, streamoffset);
  if ((fieldwidths = ppdict_get_array(xrefstream->dict, "W")) != NULL)
  {
    if (!pparray_get_uint(fieldwidths, 0, &w1)) w1 = 0;
    if (!pparray_get_uint(fieldwidths, 1, &w2)) w2 = 0;
    if (!pparray_get_uint(fieldwidths, 2, &w3)) w3 = 0;
  }
  else
    w1 = w2 = w3 = 0;
  if (w1 > XREF_STREAM_MAX_FIELD || w2 > XREF_STREAM_MAX_FIELD || w3 > XREF_STREAM_MAX_FIELD)
    return NULL;
  bufferbytes = w1 + w2 + w3;
  if ((sectionindices = ppdict_get_array(xrefstream->dict, "Index")) != NULL)
  {
    sections = sectionindices->size >> 1;
    sectionfirst = sectionindices->data;
  }
  else
  {
    sections = 1;
    sectionmock[0].type = PPINT;
    sectionmock[0].integer = 0;
    sectionmock[1].type = PPINT;
    if (!ppdict_get_int(xrefstream->dict, "Size", &sectionmock[1].integer))
      sectionmock[1].integer = 0;
    sectionfirst = &sectionmock[0];
  }
  if ((I = ppstream_read(xrefstream, 1, 0)) == NULL)
    return NULL; // we fseek() so original I is useless anyway
  xref = ppxref_create(pdf, sections, xrefoffset);
  if (pdf->xref == NULL) pdf->xref = xref;
  xref->trailer.type = PPSTREAM;
  xref->trailer.stream = xrefstream;
  for (sectionindex = 0; sectionindex < sections; ++sectionindex, sectionfirst += 2)
  {
    sectioncount = sectionfirst + 1;
    if (!ppobj_get_uint(sectionfirst, first) || !ppobj_get_uint(sectioncount, count))
      goto xref_stream_error;
    if (count == 0)
      continue;
    xref->count += count;
    xrefsection = NULL;
    ref = ppheap_take(&pdf->heap, count * sizeof(ppref));
    for (refindex = 0; refindex < count; ++refindex, ++ref)
    {
      ref->xref = xref;
      ref->number = first + refindex;
      if (iof_read(I, buffer, bufferbytes) != bufferbytes)
        goto xref_stream_error;
      b = buffer;
      if (w1 == 0)
        f1 = 1; // default type is 1
      else
        for (f1 = 0, w = 0; w < w1; f1 = (f1 << 8)|(*b), ++w, ++b);
      for (f2 = 0, w = 0; w < w2; f2 = (f2 << 8)|(*b), ++w, ++b);
      for (f3 = 0, w = 0; w < w3; f3 = (f3 << 8)|(*b), ++w, ++b);
      switch (f1)
      {
        case 0:
          //--ref;
          xrefsection = NULL;
          --xref->count;
          break;
        case 1:
          if (xrefsection == NULL)
          {
            xrefsection = ppxref_push_section(xref, &pdf->heap);
            xrefsection->first = ref->number;
            xrefsection->refs = ref;
          }
          xrefsection->last = ref->number;
          ref->offset = f2;
          ref->version = f3;
          ref->object.type = PPNONE;
          ref->object.any = NULL;
          ref->length = 0;
          break;
        case 2:
          if (xrefsection == NULL)
          {
            xrefsection = ppxref_push_section(xref, &pdf->heap);
            xrefsection->first = ref->number;
            xrefsection->refs = ref;
          }
          xrefsection->last = ref->number;
          ref->offset = 0; // f2 is parent objstm, f3 is index in parent, both useless
          ref->version = 0; // compressed objects has implicit version == 0
          ref->object.type = PPNONE;
          ref->object.any = NULL;
          ref->length = 0;
          break;
        default:
          goto xref_stream_error;
      }
    }
  }
  /* sort sections */
  if (!ppxref_sort(xref))
    return NULL;
  /* close the stream _before_ loading prev xref */
  ppstream_done(xrefstream);
  /* load prev xref */
  if (!ppxref_load_prev(pdf, xref))
    return NULL;
  return xref;
xref_stream_error:
  ppstream_done(xrefstream);
  return NULL;
}

static int ppxref_load_prev (ppdoc *pdf, ppxref *xref)
{
  ppdict *trailer;
  ppuint xrefoffset;
  ppxref *prevxref, *nextxref;
  if ((trailer = ppxref_trailer(xref)) == NULL) // sanity
    return 0;
  if (!ppdict_get_uint(trailer, "Prev", &xrefoffset)) // XRefStm is useless
    return 1; // missing /Prev is obviously ok
  for (nextxref = pdf->xref; nextxref != NULL; nextxref = nextxref->prev)
    if (nextxref->offset == xrefoffset) // insane
      return 0;
  if ((prevxref = ppxref_load(pdf, (size_t)xrefoffset)) == NULL)
    return 0;
  xref->prev = prevxref;
  return 1;
}

static ppxref * ppxref_load (ppdoc *pdf, size_t xrefoffset)
{
  iof *I;
  if ((I = ppdoc_reader(pdf, xrefoffset, PP_LENGTH_UNKNOWN)) == NULL)
    return NULL;
  ppscan_find(I);
  if (ppscan_key(I, "xref"))
    return ppxref_load_table(I, pdf, xrefoffset);
  return ppxref_load_stream(I, pdf, xrefoffset);
  // iof_close(I) does nothing here
}

static void ppoffmap_sort (ppref **left, ppref **right)
{
  ppref **l, **r, *t;
  ppuint pivot;
  l = left, r = right;
  pivot = (*(l + ((r - l) / 2)))->offset;
  do
  { // don't read from pointer!
    while ((*l)->offset < pivot) ++l;
    while ((*r)->offset > pivot) --r;
    if (l <= r)
    {
      t = *l;
      *l = *r;
      *r = t;
      ++l, --r;
    }
  } while (l <= r);
  if (left < r)
    ppoffmap_sort(left, r);
  if (l < right)
    ppoffmap_sort(l, right);
}


static void fix_trailer_references (ppdoc *pdf)
{
  ppxref *xref;
  ppdict *trailer;
  ppname *pkey;
  ppobj *obj;
  ppref *ref;
  for (xref = pdf->xref; xref != NULL; xref = xref->prev)
  {
    if ((trailer = ppxref_trailer(xref)) == NULL)
      continue;
    for (ppdict_first(trailer, pkey, obj); *pkey != NULL; ppdict_next(pkey, obj))
    { // no need to go deeper in structs, all items in trailer except info and root must be direct refs
      if (obj->type != PPREF)
        continue;
      ref = obj->ref;
      if (ref->object.type == PPNONE) // unresolved?
        if ((ref = ppxref_find(xref, ref->number)) != NULL)
          obj->ref = ref;
    }
  }
}

/*
Here comes a procedure that loads all entries from all document bodies. We resolve references while
parsing objects and to make resolving correct, we need a complete chain of xref maps, and a knowledge
about possible linearized dict (first offset). So loading refs sorted by offsets makes sense (not sure
if it matters nowadays but we also avoid fseek() by large offsets).

Here is the proc:

  - create a list of all refs in all bodies
  - sort the list by offsets
  - for every ref from the sorted list:
    - estimate object length to avoid fread-ing more than necessary (not perfect but enough)
    - fseek() to the proper offset, fread() entry data or its part
    - parse the object with ppscan_obj(stack, xref), where xref is not necessarily top pdf->xref
    - save the actual ref->length (not sure if we need that?)
    - make a stream if a dict is followed by "stream" keyword, also save the stream offset
  - free the list

Special treatment of compressed objects: if ref->offset == 1, it means compressed (or insane) entry.
Not involved in sorted refs list, parsed when /ObjStm occurs.
*/

static int ppdoc_load_objstm (ppstream *stream, ppstack *stack, ppxref *xref);

static void ppdoc_load_entries (ppdoc *pdf)
{
  size_t objects, sectionindex, refnumber, refversion, offindex, streamoffset;
  ppnum linearized;
  ppref **offmap, **pref, *ref, *refref;
  ppxref *xref;
  ppxsec *xsec;
  iof *I;
  ppstack *stack;
  ppobj *obj;
  ppname type;
  int redundant_indirection = 0;

  if ((objects = (size_t)ppdoc_objects(pdf)) == 0) // can't happen
    return;
  pref = offmap = (ppref **)pp_malloc(objects * sizeof(ppref *));
  objects = 0; // recount refs with offset > 0
  for (xref = pdf->xref; xref != NULL; xref = xref->prev)
    for (sectionindex = 0, xsec = xref->sects; sectionindex < xref->size; ++sectionindex, ++xsec)
      for (refnumber = xsec->first, ref = xsec->refs; refnumber <= xsec->last; ++refnumber, ++ref)
        if (ref->offset > 0) // compressed or insane
          *pref++ = ref, ++objects;
  ppoffmap_sort(offmap, offmap + objects - 1);
  stack = &pdf->stack;
  for (offindex = 0, pref = offmap; offindex < objects; )
  {
    ref = *pref;
    ++pref;
    if (++offindex < objects)
      ref->length = (*pref)->offset - ref->offset;
    else
      ref->length = pdf->filesize > ref->offset ? pdf->filesize - ref->offset : 0;
    if ((I = ppdoc_reader(pdf, ref->offset, ref->length)) == NULL || !ppscan_start_entry(I, ref))
    {
      PPMESS("invalid %s offset " PPSIZEF "\n", ppref_str(ref->number, ref->version), ref->offset);
      //ref->object.type = PPNONE;
      //ref->object.any = NULL;
      continue;
    }
    xref = ref->xref; // to resolve indirects properly
    if ((obj = ppscan_obj(I, stack, xref)) == NULL)
    {
      PPMESS("invalid %s object at offset " PPSIZEF "\n", ppref_str(ref->number, ref->version), ref->offset);
      //ref->object.type = PPNONE;
      //ref->object.any = NULL;
      continue;
    }
    ref->object = *obj;
    ref->length = ppdoc_reader_tell(pdf, I) - ref->offset;
    if (obj->type == PPDICT)
    {
      /* Check if the object at first offset is linearized dict (/Linearized value is a version, default 1.0).
         We need that to resolve all references properly. */
      if (offindex == 1 && ppdict_get_num(obj->dict, "Linearized", &linearized))
        pdf->flags |= PPDOC_LINEARIZED;
      else if (ppscan_start_stream(I, pdf, &streamoffset))
      {
        ref->object.type = PPSTREAM;
        ref->object.stream = ppstream_create(pdf, obj->dict, streamoffset);
        if (xref->trailer.type == PPSTREAM && (type = ppdict_get_name(obj->dict, "Type")) != NULL && ppname_is(type, "ObjStm")) // somewhat dummy..
          if (!ppdoc_load_objstm(ref->object.stream, stack, xref))
            PPMESS("invalid objects stream %s at offset " PPSIZEF "\n", ppref_str(ref->number, ref->version), ref->offset);
      }
    }
    else if (obj->type == PPINT)
    {
      ppscan_find(I);
      if (ppscan_uint(I, &refversion) && ppscan_find(I) == 'R')
      { /* hmm.. multiple indirection? we just cut it, otherwise we would need annoying checks on every _rget.. but this is too early
           to replace ref->object with ref->object.ref->object, as the later may be PPNONE (not parsed yet). */
        redundant_indirection = 1;
        refnumber = (ppuint)obj->integer;
        if ((refref = ppxref_find(ref->xref, refnumber)) != NULL)
        {
          ref->object.type = PPREF;
          ref->object.ref = refref;
        }
        else
        {
          ref->object.type = PPNONE; // as ppref_unresolved()
          ref->object.any = NULL;
        }
      }
    }
    ppstack_pop(stack, 1);
  }
  if (redundant_indirection)
    for (offindex = 0, pref = offmap, ref = *pref; offindex < objects; ++pref, ref = *pref)
      if (ref->object.type == PPREF)
        ref->object = ref->object.ref->object; // doing for all effectively cuts all insane chains
  pp_free(offmap);
}

/* Loading entries from object stream

  /N is the number of contained entries
  /First is the offset of the first item

The stream consists of N pairs of numbers <objnum> <offset> <objnum> <offset> ...
Offsets are ascending (relative to the first), but ref numbers order is arbitrary.
PDF spec says there might be some additional data between objects, so we should obey offsets.
Which means we should basically load the stream at once (may be needed anyway to grab the stream [...]).
*/

static int ppdoc_load_objstm (ppstream *stream, ppstack *stack, ppxref *xref)
{
  ppdict *dict; // stream dict, actually still on stack
  ppref *ref;
  ppobj *obj;
  ppuint items, firstoffset, offset, objnum, i, invalid = 0;
  iof *I;
  uint8_t *firstdata, *indexdata;

  dict = stream->dict;
  if (!ppdict_rget_uint(dict, "N", &items) || !ppdict_rget_uint(dict, "First", &firstoffset))
    return 0;
  if ((I = ppstream_read(stream, 1, 1)) == NULL)
    return 0;
  firstdata = I->pos + firstoffset;
  if (firstdata >= I->end)
    goto invalid_objstm;
  for (i = 0; i < items; ++i)
  {
    ppscan_find(I);
    if (!ppscan_uint(I, &objnum))
      goto invalid_objstm;
    ppscan_find(I);
    if (!ppscan_uint(I, &offset))
      goto invalid_objstm;
    if ((ref = ppxref_find_local(xref, objnum)) == NULL || ref->object.type != PPNONE)
    {
      PPMESS("invalid compressed object number " PPUINTF " at position " PPUINTF "\n", objnum, i);
      ++invalid;
      continue;
    }
    if (firstdata + offset >= I->end)
    {
      PPMESS("invalid compressed object offset " PPUINTF " at position " PPUINTF "\n", offset, i);
      ++invalid;
      continue;
    }
    indexdata = I->pos; // save position
    I->pos = firstdata + offset; // go to the object
    ppscan_find(I);
    if ((obj = ppscan_obj(I, stack, xref)) != NULL)
    {
      ref->object = *obj;
      ppstack_pop(stack, 1);
      // nothing more needed, as obj can never be indirect ref or stream
    }
    else
    {
      ++invalid;
      PPMESS("invalid compressed object %s at stream offset " PPUINTF, ppref_str(objnum, 0), offset);
    }
    I->pos = indexdata; // restore position and read next from index
  }
  ppstream_done(stream);
  return invalid == 0;
invalid_objstm:
  ppstream_done(stream);
  return 0;
}

/* main PDF loader proc */

static ppdoc * ppdoc_read (ppdoc *pdf, iof_file *input)
{
  uint8_t header[PPDOC_HEADER];
  size_t xrefoffset;

  input = &pdf->input;
  if (iof_file_read(header, 1, PPDOC_HEADER, input) != PPDOC_HEADER || !ppdoc_header(pdf, header))
    return NULL;
  if (!ppdoc_tail(pdf, input, &xrefoffset))
    return 0;
  if (ppxref_load(pdf, xrefoffset) == NULL)
    return 0;
  ppdoc_load_entries(pdf);
  fix_trailer_references(pdf);
  return pdf;
}

static void ppdoc_pages_init (ppdoc *pdf);

static ppdoc * ppdoc_create (iof_file *input)
{
  ppdoc *pdf;
  ppheap *heap;

  heap = ppheap_new();
  pdf = ppheap_take(&heap, sizeof(ppdoc));
  pdf->flags = 0;
  pdf->heap = heap;
  pdf->xref = NULL;
  pdf->version[0] = '\0';
  ppstack_init(&pdf->stack, &pdf->heap);
  ppdoc_reader_init(pdf, input);
  ppdoc_pages_init(pdf);
  if (ppdoc_read(pdf, &pdf->input) != NULL)
    return pdf;
  ppdoc_free(pdf);
  return NULL;
}

ppdoc * ppdoc_load (const char *filename)
{
  FILE *file;
  iof_file input;
  if ((file = fopen(filename, "rb")) == NULL)
    return NULL;
  iof_file_init(&input, file);
  input.flags |= IOF_CLOSE_FILE;
  return ppdoc_create(&input);
}

ppdoc * ppdoc_mem (const void *data, size_t size)
{
	iof_file input;
	iof_file_rdata_init(&input, data, size);
	input.flags |= IOF_BUFFER_ISALLOC; // todo: 3 modes: borrow, take over, copy
	return ppdoc_create(&input);
}

void ppdoc_free (ppdoc *pdf)
{
  //iof_file_free(&pdf->input);
  iof_file_decref(&pdf->input);
  ppstack_free_buffer(&pdf->stack);
  ppheap_free(pdf->heap); // last!
}

/* pages access */

static pparray * pppage_node (ppdict *dict, ppuint *count, ppname *type)
{
  ppname *pkey, key;
  ppobj *obj;
  pparray *kids = NULL;
  *count = 0;
  *type = NULL;
  for (ppdict_first(dict, pkey, obj); (key = *pkey) != NULL; ppdict_next(pkey, obj))
  {
    switch (key[0])
    {
      case 'T':
        if (ppname_is(key, "Type"))
          *type = ppobj_get_name(obj);
        break;
      case 'C':
        if (ppname_is(key, "Count"))
          ppobj_get_uint(obj, *count);
        break;
      case 'K':
        if (ppname_is(key, "Kids"))
          kids = ppobj_rget_array(obj);
        break;
    }
  }
  return kids;
}

#define ppname_is_page(type) (type != NULL && ppname_is(type, "Page"))

ppuint ppdoc_page_count (ppdoc *pdf)
{
  ppref *ref;
  ppname type;
  ppuint count;
  if ((ref = ppxref_pages(pdf->xref)) == NULL)
    return 0;
  if (pppage_node(ref->object.dict, &count, &type) == NULL)
    return ppname_is_page(type) ? 1 : 0; // acrobat and ghostscript accept documents with root /Pages entry being a reference to a sole /Page object
  return count;
}

ppref * ppdoc_page (ppdoc *pdf, ppuint index)
{
  ppdict *dict;
  ppuint count;
  pparray *kids;
  size_t size, i;
  ppobj *r, *o;
  ppref *ref;
  ppname type;


  if ((ref = ppxref_pages(pdf->xref)) == NULL)
    return NULL;
  dict = ref->object.dict;
  if ((kids = pppage_node(dict, &count, &type)) != NULL)
  {
    if (index < 1 || index > count)
      return NULL;
  }
  else
  {
    return index == 1 && ppname_is_page(type) ? ref : NULL;
  }
scan_array:
  if (index <= count / 2)
  { // probably shorter way from the beginning
    for (i = 0, size = kids->size, r = pparray_at(kids, 0); i < size; ++i, ++r)
    {
      if (r->type != PPREF)
        return NULL;
      o = &r->ref->object;
      if (o->type != PPDICT)
        return NULL;
      dict = o->dict;
      if ((kids = pppage_node(dict, &count, &type)) != NULL)
      {
        if (index <= count)
          goto scan_array;
        index -= count;
        continue;
      }
      if (index == 1 && ppname_is_page(type))
        return r->ref;
      --index;
    }
  }
  else if ((size = kids->size) > 0) // for safe (size-1)
  { // probably shorter way from the end
    index = count - index + 1;
    for (i = 0, r = pparray_at(kids, size - 1); i < size; ++i, --r)
    {
      if (r->type != PPREF)
        return NULL;
      o = &r->ref->object;
      if (o->type != PPDICT)
        return NULL;
      dict = o->dict;
      if ((kids = pppage_node(dict, &count, &type)) != NULL)
      {
        if (index <= count)
          goto scan_array;
        index -= count;
        continue;
      }
      if (index == 1 && ppname_is_page(type))
        return r->ref;
    }
  }
  return NULL;
}

/*
Through pages iterator. Iterating over pages tree just on the base of /Kids and /Parent keys
is ineffective, as to get next pageref we need to take parent, find the pageref in /Kids,
take next (or go upper).. Annoying. We use a dedicated stack for pages iterator. This could
actually be done with pdf->stack, but some operations may clear it, so safer to keep it independent
Besides, its depth is constant (set on first use), so no need for allocs.
*/

static void ppdoc_pages_init (ppdoc *pdf)
{
  pppages *pages;
  pages = &pdf->pages;
  pages->root = pages->parent = pages->buffer;
  pages->depth = 0;
  pages->space = PPPAGES_STACK_DEPTH;
}

static ppkids * pppages_push (ppdoc *pdf, pparray *kids)
{
  ppkids *newroot, *bounds;
  pppages *pages;
  pages = &pdf->pages;
  if (pages->depth == pages->space)
  {
    pages->space <<= 1;
    newroot = ppheap_take(&pdf->heap, pages->space * sizeof(ppkids));
    memcpy(newroot, pages->root, pages->depth * sizeof(ppkids));
    pages->root = newroot;
  }
  bounds = pages->parent = &pages->root[pages->depth++];
  bounds->current = pparray_at(kids, 0);
  bounds->sentinel = pparray_at(kids, kids->size);
  return bounds;
}

#define pppages_pop(pages) (--((pages)->parent), --((pages)->depth))

static ppref * ppdoc_pages_group_first (ppdoc *pdf, ppref *ref)
{
  ppdict *dict;
  pparray *kids;
  ppuint count;
  ppname type;

  dict = ref->object.dict; // typecheck made by callers
  while ((kids = pppage_node(dict, &count, &type)) != NULL)
  {
    if ((ref = pparray_get_ref(kids, 0)) == NULL || ref->object.type != PPDICT)
      return NULL;
    pppages_push(pdf, kids);
    dict = ref->object.dict;
  }
  return ppname_is_page(type) ? ref : NULL;
}

ppref * ppdoc_first_page (ppdoc *pdf)
{
  ppref *ref;
  pppages *pages;
  if ((ref = ppdoc_pages(pdf)) == NULL)
    return NULL;
  pages = &pdf->pages;
  pages->parent = pages->root;
  pages->depth = 0;
  return ppdoc_pages_group_first(pdf, ref);
}

ppref * ppdoc_next_page (ppdoc *pdf)
{
  pppages *pages;
  ppkids *bounds;
  ppref *ref;
  ppobj *obj;
  pages = &pdf->pages;
  while (pages->depth > 0)
  {
    bounds = pages->parent;
    obj = ++bounds->current;
    if (obj < bounds->sentinel)
    {
      if (obj->type != PPREF)
        return NULL;
      ref = obj->ref;
      if (ref->object.type != PPDICT)
        return NULL;
      return ppdoc_pages_group_first(pdf, ref);
    }
    else
    { // no next node, go upper
      pppages_pop(pages);
    }
  }
  return NULL;
}

/* context */

ppcontext * ppcontext_new (void)
{
  ppheap *heap;
  ppcontext *context;
  heap = ppheap_new();
  context = (ppcontext *)pp_malloc(sizeof(ppcontext)); // not from priv heap, as we delete it on renew
  context->heap = heap;
  ppstack_init(&context->stack, &context->heap);
  return context;
}

void ppcontext_done (ppcontext *context)
{
  ppheap_renew(context->heap);
  ppstack_clear(&context->stack);
}

void ppcontext_free (ppcontext *context)
{
  ppstack_free_buffer(&context->stack);
  ppheap_free(context->heap);
  pp_free(context);
}

/* page contents streams */

//#define ppcontents_first_stream(array) pparray_rget_stream(array, 0)

static ppstream * ppcontents_first_stream (pparray *array)
{
  size_t i;
  ppobj *obj;
  ppref *ref;
  for (pparray_first(array, i, obj); i < array->size; pparray_next(i, obj))
    if ((ref = ppobj_get_ref(obj)) != NULL && ref->object.type == PPSTREAM)
      return ref->object.stream;
  return NULL;
}

static ppstream * ppcontents_next_stream (pparray *array, ppstream *stream)
{
  size_t i;
  ppobj *obj;
  ppref *ref;
  for (pparray_first(array, i, obj); i < array->size; pparray_next(i, obj))
    if ((ref = ppobj_get_ref(obj)) != NULL && ref->object.type == PPSTREAM && ref->object.stream == stream)
      if (++i < array->size && (ref = ppobj_get_ref(obj + 1)) != NULL && ref->object.type == PPSTREAM)
        return ref->object.stream;
  return NULL;
}

ppstream * ppcontents_first (ppdict *dict)
{
  ppobj *contentsobj;
  if ((contentsobj = ppdict_rget_obj(dict, "Contents")) == NULL)
    return NULL;
  switch (contentsobj->type)
  {
    case PPARRAY:
      return ppcontents_first_stream(contentsobj->array);
    case PPSTREAM:
      return contentsobj->stream;
    default:
      break;
  }
  return NULL;
}

ppstream * ppcontents_next (ppdict *dict, ppstream *stream)
{
  ppobj *contentsobj;
  if ((contentsobj = ppdict_rget_obj(dict, "Contents")) == NULL)
    return NULL;
  switch (contentsobj->type)
  {
    case PPARRAY:
      return ppcontents_next_stream(contentsobj->array, stream);
    case PPSTREAM:
      break;
    default:
      break;
  }
  return NULL;
}

static ppobj * ppcontents_op (iof *I, ppstack *stack, size_t *psize, ppname *pname)
{
  ppobj *obj;
  ppstack_clear(stack);
  do {
    if (ppscan_find(I) < 0)
      return NULL;
    if ((obj = ppscan_psobj(I, stack)) == NULL)
      return NULL;
  } while (obj->type != PPNAME || !ppname_exec(obj->name));
  *pname = obj->name;
  *psize = stack->size - 1;
  return stack->buf;
}

ppobj * ppcontents_first_op (ppcontext *context, ppstream *stream, size_t *psize, ppname *pname)
{
  iof *I;
  if ((I = ppstream_read(stream, 1, 0)) == NULL)
    return NULL;
  return ppcontents_op(I, &context->stack, psize, pname);
}

ppobj * ppcontents_next_op (ppcontext *context, ppstream *stream, size_t *psize, ppname *pname)
{
  return ppcontents_op(ppstream_iof(stream), &context->stack, psize, pname);
}

ppobj * ppcontents_parse (ppcontext *context, ppstream *stream, size_t *psize)
{
  iof *I;
  ppstack *stack;
  ppobj *obj;
  stack = &context->stack;
  ppstack_clear(stack);
  if ((I = ppstream_read(stream, 1, 0)) == NULL)
    return NULL;
  while (ppscan_find(I) >= 0)
    if ((obj = ppscan_psobj(I, stack)) == NULL)
      goto error;
  *psize = stack->size;
  ppstream_done(stream);
  return stack->buf;
error:
  ppstream_done(stream);
  return NULL;
}

/* boxes */

pprect * pparray_to_rect (pparray *array, pprect *rect)
{
  ppobj *obj;
  if (array->size != 4)
    return NULL;
  obj = pparray_at(array, 0);
  if (!ppobj_get_num(obj, rect->lx)) return NULL;
  obj = pparray_at(array, 1);
  if (!ppobj_get_num(obj, rect->ly)) return NULL;
  obj = pparray_at(array, 2);
  if (!ppobj_get_num(obj, rect->rx)) return NULL;
  obj = pparray_at(array, 3);
  if (!ppobj_get_num(obj, rect->ry)) return NULL;
  return rect;
}

pprect * ppdict_get_rect (ppdict *dict, const char *name, pprect *rect)
{
  pparray *array;
  return (array = ppdict_rget_array(dict, name)) != NULL ? pparray_to_rect(array, rect) : NULL;
}

pprect * ppdict_get_box (ppdict *dict, const char *name, pprect *rect)
{
  do {
    if (ppdict_get_rect(dict, name, rect) != NULL)
      return rect;
    dict = ppdict_rget_dict(dict, "Parent");
  } while (dict != NULL);
  return NULL;
}

ppmatrix * pparray_to_matrix (pparray *array, ppmatrix *matrix)
{
  ppobj *obj;
  if (array->size != 6)
    return NULL;
  obj = pparray_at(array, 0);
  if (!ppobj_get_num(obj, matrix->xx)) return NULL;
  obj = pparray_at(array, 1);
  if (!ppobj_get_num(obj, matrix->xy)) return NULL;
  obj = pparray_at(array, 2);
  if (!ppobj_get_num(obj, matrix->yx)) return NULL;
  obj = pparray_at(array, 3);
  if (!ppobj_get_num(obj, matrix->yy)) return NULL;
  obj = pparray_at(array, 4);
  if (!ppobj_get_num(obj, matrix->x)) return NULL;
  obj = pparray_at(array, 5);
  if (!ppobj_get_num(obj, matrix->y)) return NULL;
  return matrix;
}

ppmatrix * ppdict_get_matrix (ppdict *dict, const char *name, ppmatrix *matrix)
{
  pparray *array;
  return (array = ppdict_rget_array(dict, name)) != NULL ? pparray_to_matrix(array, matrix) : NULL;
}

/* stats and debug */

size_t ppdoc_file_size (ppdoc *pdf)
{
  return pdf->filesize;
}

ppuint ppdoc_objects (ppdoc *pdf)
{
  ppuint count;
  ppxref *xref;
  for (count = 0, xref = pdf->xref; xref != NULL; xref = xref->prev)
    count += xref->count;
  return count;
}

size_t ppdoc_memory (ppdoc *pdf, size_t *waste)
{
  size_t used;
  ppheap *heap;
  used = 0, *waste = 0;
  for (heap = pdf->heap; heap != NULL; heap = heap->prev)
  {
    used += heap->space;
    *waste += heap->size;
  }
  return used;
}
