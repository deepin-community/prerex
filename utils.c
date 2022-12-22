
/*  
  utils.c  -- utilities for interactive editor of prerequisite-chart descriptions
  Copyright (C) 2005 - 2014  R. D. Tennent   
  School of Computing, Queen's University, rdt@cs.queensu.ca 

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/


# include "prerex.h"

void
error (char msg[])		/* abort with stderr message msg */
{
  fprintf (stderr, "Error: %s\n", msg);
# ifdef HAVE_LIBEDIT
  history_end (hist);
  el_end (el);
# endif
  close_files ();
  exit (EXIT_FAILURE);
}


size_t
append (char *dst, char **offset, const char *src, size_t n)
/*  Copies src to *offset and updates *offset accordingly (if possible). 
 *  Assumes *offset is dst if offset == NULL.
 *  Execution aborts if **offset != '\0'. The src string must be null-terminated. 
 *  Returns (original offset - dst) + strlen(src);  if >= n, the string was truncated.
 */
{
  const char *s = src;
  char *d;
  char *off_orig = ( offset != NULL ? *offset : dst );
  d = off_orig;
  if (*d != '\0')
  {
    char msg[LINE_LEN + 24] = {'\0'};
    char *msg_n = msg;
    append (msg, &msg_n, "\nNull character expected at offset specified for string ", sizeof (msg));
    append (msg, &msg_n, src, sizeof (msg));
    error (msg);
  }
  n = n - 1 - (off_orig - dst); /* number of available slots (leaving room for \0)  */
  while (n > 0 && *s != '\0')
    {
      *d = *s;                  /* copy non-null character */
      d++; s++; n--;
    }
  *d = '\0';                    /* null-terminate */
  if (offset != NULL)
    *offset = d;                /* update *offset (if possible) */
  while (*s != '\0')            /* traverse rest of src */
    s++;                        
  return (off_orig - dst) + (s - src);  /* length of the untruncated string */
}

bool
prefix (const char *cs, const char *ct)
/* is string cs[] a prefix of ct[]?   */
{
  const char *pcs = cs;
  const char *pct = ct;
  while (*pcs != '\0')
    {
      if (*pcs != *pct) return false;
      pcs++; pct++;
    }
  return true;
}

bool
suffix (const char *cs, const char *ct)
/* is string cs[] a suffix of ct[]?  */
{
  const char *pcs = cs;
  const char *pct = ct + strlen (ct) - strlen (cs);
  while (*pcs != '\0')
    {
      if (*pct != *pcs) return false;
      pcs++; pct++;
    }
  return true;
}


void
copy (FILE * f, FILE * g)
{
  int c;
  fflush (f);
  rewind (f);
  c = getc (f);
  while (c != EOF)
    {
      putc (c, g);
      c = getc (f);
    }
}

bool
inrange (coord x, coord x0, coord x1)
/* is coordinate x in the range x0:x1? */
{
# define min(x,y) (x < y ? x : y) 
# define max(x,y) (x < y ? y : x)
  return x >= min(x0, x1) && x <= max(x0, x1) ;
}

