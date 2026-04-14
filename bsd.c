#include <ctype.h>

int strcasecmp(s,p)
     char	*s,*p;
{
  while (*s && *p) {
    if (*s==*p) {
      s++; p++;
      continue;
    }
    if (isalpha(*s) && isalpha(*p)) {
      int	c1, c2;
      c1 = isupper(*s)?*s:toupper(*s);
      c2 = isupper(*p)?*p:toupper(*p);
      s++; p++;
      if (c1==c2)
	continue;
      else
	return (c1-c2);
    }
    break;
  }
  return *s-*p;
}
