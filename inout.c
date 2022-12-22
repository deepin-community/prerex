/*  
  inout.c  -- input/output module for interactive editor of prerequisite-chart 
              descriptions
  Copyright (c) 2005-19  R. D. Tennent   
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

element *first_node;
element *first_arrow;

PRIVATE char line[LINE_LEN];
PRIVATE char *lp;                      /* pointer into line */
PRIVATE char *cp;	               /* pointer into text fields  */

PRIVATE
FILE *pretext,	        /* whatever is in the tex_file before the chart */
     *posttext,		/* whatever is in the tex_file after the chart */
     *comments;		/* comment lines in the chart environment */

PRIVATE bool
gt (point p, point q)
/* ordering on boxes: top-to-bottom, left-to-right */
{
  return (p.y > q.y || (p.y == q.y && p.x < q.x));
}

bool
eq (point p, point q)
{
  return (p.x == q.x && p.y == q.y);
}

bool
insert_node (element * pn)
{
  element *p;
  element **pp;  

  /* ordered list */
  p = first_node;
  pp = &(first_node);
  while ((p != NULL) && gt (p->u.n.at, pn->u.n.at))
    {
      pp = &(p->next);
      p = p->next;
    }
  if ((p != NULL) && eq (p->u.n.at, pn->u.n.at))
    {
      /* node at that point already */
      return true;
    }
  *pp = pn;
  pn->next = p;
  return false;
}  /* insert_node */

element *
node_at (point p)
{
  element *pn = first_node;
  while (pn != NULL && !eq (p, pn->u.n.at))
    pn = pn->next;
  return pn;
}

bool
insert_arrow (element * pa)
{
  element *p;
  if (pa->u.a.source == NULL) return true;
  if (pa->u.a.target == NULL) return true;
  /* is the arrow already in the list? */
  p = first_arrow;
  while (p != NULL && (p->u.a.target != pa->u.a.target ||
		       p->u.a.source != pa->u.a.source))
    {
      p = p->next;
    }
  if (p != NULL) return true;
  pa->next = first_arrow;
  first_arrow = pa;
  return false;
}  /*insert_arrow */


PRIVATE coord
read_coord (void)
{
  coord c;
  while (!isdigit (*lp) && *lp != '-')
    lp++;
  c = atoi (lp);
  if (*lp == '-')
    lp++;
  while (isdigit (*lp))
    lp++;
  return c;
}

PRIVATE point
read_point (void)
{
  point pnt;
  pnt.x = read_coord ();
  pnt.y = read_coord ();
  return pnt;
}

PRIVATE void
read_textfield (char *limit)
{
  /* use recursion to allow for nested { ... }  */
  if (lp >= &line[LINE_LEN])
    error ("Missing }.");
  while (*lp != '}')
    {
      *cp = *lp; 
      if (*lp == '{')
	{			/* nested {...} */
	  cp++;
	  lp++;
	  read_textfield (limit);
	  *cp = *lp;   /* '}' */
	}
      cp++;
      lp++;
      if (lp >= &line[LINE_LEN])
	error ("Missing }.");
      if (cp >= limit)
	{
	  puts (line);
	  error ("Text-field too long.");
	}
    }
}

PRIVATE void
read_bracketed_textfield (char *limit)
{
  while (*lp != '{')
    {
      lp++;
      if (lp >= &line[LINE_LEN])
	error ("Missing {.");
    }
  lp++;				/* '{'  */
  read_textfield (limit);
  lp++;				/* '}' */
  if (cp >= limit)
    {
      puts (line);
      error ("Text-field too long.");
    }
  *cp = '\0';
}


PRIVATE void
analyze_halfcourse (bool color)
{
  element *c;
  c = (element *) malloc (sizeof (element));
  if (c == NULL) error ("Out of memory.");
  c->tag = NODE;
  c->u.n.tag = BOX;
  lp = &line[10];
  c->u.n.u.b.req_opt = NEITHER;
  c->u.n.u.b.half = true;
  c->u.n.at = read_point ();
  cp = &(c->u.n.code[0]);
  read_bracketed_textfield (&(c->u.n.code[CODE_LEN]));
  cp = &(c->u.n.u.b.title[0]);
  read_bracketed_textfield (&(c->u.n.u.b.title[TITLE_LEN]));
  cp = &(c->u.n.u.b.timetable[0]);
  read_bracketed_textfield (&(c->u.n.u.b.timetable[TIMETABLE_LEN]));
  cp = &(c->u.n.u.b.color[0]);
  if (color) 
    { 
      read_bracketed_textfield (&(c->u.n.u.b.color[COLOR_LEN]));
    }
  else 
      *cp = '\0';
  if (insert_node (c))
    {
      printf ("More than one node at %i,%i; only the first will be used.\n", c->u.n.at.x, c->u.n.at.y);
      free (c);
    }
}

PRIVATE void
analyze_reqhalfcourse (bool color)
{
  element *c;
  c = (element *) malloc (sizeof (element));
  if (c == NULL) error ("Out of memory.");
  c->tag = NODE;
  c->u.n.tag = BOX;
  lp = &line[13];
  c->u.n.u.b.req_opt = REQ;
  c->u.n.u.b.half = true;
  c->u.n.at = read_point ();
  cp = &(c->u.n.code[0]);
  read_bracketed_textfield (&(c->u.n.code[CODE_LEN]));
  cp = &(c->u.n.u.b.title[0]);
  read_bracketed_textfield (&(c->u.n.u.b.title[TITLE_LEN]));
  cp = &(c->u.n.u.b.timetable[0]);
  read_bracketed_textfield (&(c->u.n.u.b.timetable[TIMETABLE_LEN]));
  cp = &(c->u.n.u.b.color[0]);
  if (color) 
    { 
      read_bracketed_textfield (&(c->u.n.u.b.color[COLOR_LEN]));
    }
  else 
      *cp = '\0';
  if (insert_node (c))
    {
      printf ("More than one node at %i,%i; only the first will be used.\n", c->u.n.at.x, c->u.n.at.y);
      free (c);
    }
}

PRIVATE void
analyze_opthalfcourse (bool color)
{
  element *c;
  c = (element *) malloc (sizeof (element));
  if (c == NULL) error ("Out of memory.");
  c->tag = NODE;
  c->u.n.tag = BOX;
  lp = &line[13];
  c->u.n.u.b.req_opt = OPT;
  c->u.n.u.b.half = true;
  c->u.n.at = read_point ();
  cp = &(c->u.n.code[0]);
  read_bracketed_textfield (&(c->u.n.code[CODE_LEN]));
  cp = &(c->u.n.u.b.title[0]);
  read_bracketed_textfield (&(c->u.n.u.b.title[TITLE_LEN]));
  cp = &(c->u.n.u.b.timetable[0]);
  read_bracketed_textfield (&(c->u.n.u.b.timetable[TIMETABLE_LEN]));
  cp = &(c->u.n.u.b.color[0]);
  if (color) 
    { 
      read_bracketed_textfield (&(c->u.n.u.b.color[COLOR_LEN]));
    }
  else 
      *cp = '\0';
  if (insert_node (c))
    {
      printf ("More than one node at %i,%i; only the first will be used.\n", c->u.n.at.x, c->u.n.at.y);
      free (c);
    }
}

PRIVATE void
analyze_fullcourse (bool color)
{
  element *c;
  c = (element *) malloc (sizeof (element));
  if (c == NULL) error ("Out of memory.");
  c->tag = NODE;
  c->u.n.tag = BOX;
  lp = &line[10];
  c->u.n.u.b.req_opt = NEITHER;
  c->u.n.u.b.half = false;
  c->u.n.at = read_point ();
  cp = &(c->u.n.code[0]);
  read_bracketed_textfield (&(c->u.n.code[CODE_LEN]));
  cp = &(c->u.n.u.b.title[0]);
  read_bracketed_textfield (&(c->u.n.u.b.title[TITLE_LEN]));
  cp = &(c->u.n.u.b.timetable[0]);
  read_bracketed_textfield (&(c->u.n.u.b.timetable[TIMETABLE_LEN]));
  cp = &(c->u.n.u.b.color[0]);
  if (color) 
    { 
      read_bracketed_textfield (&(c->u.n.u.b.color[COLOR_LEN]));
    }
  else 
      *cp = '\0';
  if (insert_node (c))
    {
      printf ("More than one node at %i,%i; only the first will be used.\n", c->u.n.at.x, c->u.n.at.y);
      free (c);
    }
}

PRIVATE void
analyze_reqfullcourse (bool color)
{
  element *c;
  c = (element *) malloc (sizeof (element));
  if (c == NULL) error ("Out of memory.");
  c->tag = NODE;
  c->u.n.tag = BOX;
  lp = &line[13];
  c->u.n.u.b.req_opt = REQ;
  c->u.n.u.b.half = false;
  c->u.n.at = read_point ();
  cp = &(c->u.n.code[0]);
  read_bracketed_textfield (&(c->u.n.code[CODE_LEN]));
  cp = &(c->u.n.u.b.title[0]);
  read_bracketed_textfield (&(c->u.n.u.b.title[TITLE_LEN]));
  cp = &(c->u.n.u.b.timetable[0]);
  read_bracketed_textfield (&(c->u.n.u.b.timetable[TIMETABLE_LEN]));
  cp = &(c->u.n.u.b.color[0]);
  if (color) 
    { 
      read_bracketed_textfield (&(c->u.n.u.b.color[COLOR_LEN]));
    }
  else 
      *cp = '\0';
  if (insert_node (c))
    {
      printf ("More than one node at %i,%i; only the first will be used.\n", c->u.n.at.x, c->u.n.at.y);
      free (c);
    }
}


PRIVATE void
analyze_optfullcourse (bool color)
{
  element *c;
  c = (element *) malloc (sizeof (element));
  if (c == NULL) error ("Out of memory.");
  c->tag = NODE;
  c->u.n.tag = BOX;
  lp = &line[13];
  c->u.n.u.b.req_opt = OPT;
  c->u.n.u.b.half = false;
  c->u.n.at = read_point ();
  cp = &(c->u.n.code[0]);
  read_bracketed_textfield (&(c->u.n.code[CODE_LEN]));
  cp = &(c->u.n.u.b.title[0]);
  read_bracketed_textfield (&(c->u.n.u.b.title[TITLE_LEN]));
  cp = &(c->u.n.u.b.timetable[0]);
  read_bracketed_textfield (&(c->u.n.u.b.timetable[TIMETABLE_LEN]));
  cp = &(c->u.n.u.b.color[0]);
  if (color) 
    { 
      read_bracketed_textfield (&(c->u.n.u.b.color[COLOR_LEN]));
    }
  else 
      *cp = '\0';
  if (insert_node (c))
    {
      printf ("More than one node at %i,%i; only the first will be used.\n", c->u.n.at.x, c->u.n.at.y);
      free (c);
    }
}

PRIVATE void
analyze_mini (void)
{
  element *m;
  m = (element *) malloc (sizeof (element));
  if (m == NULL) error ("Out of memory.");
  m->tag = NODE;
  m->u.n.tag = MINI;
  lp = &line[4];
  m->u.n.at = read_point ();
  cp = &(m->u.n.code[0]);
  read_bracketed_textfield (&(m->u.n.code[CODE_LEN]));
  if (insert_node (m))
    {
      printf ("More than one node at %i,%i; only the first will be used.\n", m->u.n.at.x, m->u.n.at.y);
      free (m);
    }
}

PRIVATE void
analyze_text (void)
{
  element *m;
  m = (element *) malloc (sizeof (element));
  if (m == NULL) error ("Out of memory.");
  m->tag = NODE;
  m->u.n.tag = TEXT;
  lp = &line[4];
  m->u.n.at = read_point ();
  m->u.n.code[0] = '\0';
  cp = &(m->u.n.u.t.txt[0]);
  read_bracketed_textfield (&(m->u.n.u.t.txt[LINE_LEN]));
  if (insert_node (m))
    {
      printf ("More than one node at %i,%i; only the first will be used.\n", m->u.n.at.x, m->u.n.at.y);
      free (m);
    }
}

PRIVATE void
analyze_prereqc (void)
{
  element *a;
  a = (element *) malloc (sizeof (element));
  if (a == NULL) error ("Out of memory.");
  a->tag = ARROW;
  a->u.a.tag = PREREQ;
  lp = &line[7];
  a->u.a.source = node_at (read_point ());
  a->u.a.target = node_at (read_point ());
  a->u.a.DefaultCurvature = false;
  a->u.a.curvature = read_coord ();
  if (abs(a->u.a.curvature) > 100) 
  {
    puts ("Curvature too large; default assumed.");
    a->u.a.curvature = 0;
    a->u.a.DefaultCurvature = true;
  }
  if (insert_arrow (a))
  {
     puts ("Arrow source or target don't exist, or more than one arrow with the same source and target.");
     free (a);
  }
}

PRIVATE void
analyze_prereq (void)
{
  element *a;
  a = (element *) malloc (sizeof (element));
  if (a == NULL) error ("Out of memory.");
  a->tag = ARROW;
  a->u.a.tag = PREREQ;
  a->u.a.DefaultCurvature = true;
  a->u.a.curvature = 0;
  lp = &line[6];
  a->u.a.source = node_at (read_point ());
  a->u.a.target = node_at (read_point ());
  if (insert_arrow (a))
  {
     puts ("Arrow source or target don't exist, or more than one arrow with the same source and target.");
     free (a);
  }
}

PRIVATE void
analyze_coreq (void)
{
  element *a;
  a = (element *) malloc (sizeof (element));
  if (a == NULL) error ("Out of memory.");
  a->tag = ARROW;
  a->u.a.tag = COREQ;
  a->u.a.DefaultCurvature = true;
  a->u.a.curvature = 0;
  lp = &line[6];
  a->u.a.source = node_at (read_point ());
  a->u.a.target = node_at (read_point ());
  if (insert_arrow (a))
  {
     puts ("Arrow source or target don't exist, or more than one arrow with the same source and target.");
     free (a);
  }
}

PRIVATE void
analyze_coreqc (void)
{
  element *a;
  a = (element *) malloc (sizeof (element));
  if (a == NULL) error ("Out of memory.");
  a->tag = ARROW;
  a->u.a.tag = COREQ;
  lp = &line[7];
  a->u.a.source = node_at (read_point ());
  a->u.a.target = node_at (read_point ());
  a->u.a.DefaultCurvature = false;
  a->u.a.curvature = read_coord ();
  if (abs(a->u.a.curvature) > 100) 
  {
    puts ("Curvature too large; default assumed.");
    a->u.a.curvature = 0;
    a->u.a.DefaultCurvature = true;
  }
  if (insert_arrow (a))
  {
     puts ("Arrow source or target don't exist, or more than one arrow with the same source and target.");
     free (a);
  }
}

PRIVATE void
analyze_recomm (void)
{
  element *a;
  a = (element *) malloc (sizeof (element));
  if (a == NULL) error ("Out of memory.");
  a->tag = ARROW;
  a->u.a.tag = RECOMM;
  a->u.a.DefaultCurvature = true;
  a->u.a.curvature = 0;
  lp = &line[6];
  a->u.a.source = node_at (read_point ());
  a->u.a.target = node_at (read_point ());
  if (insert_arrow (a))
  {
     puts ("Arrow source or target don't exist, or more than one arrow with the same source and target.");
     free (a);
  }
}

PRIVATE void
analyze_recommc (void)
{
  element *a;
  a = (element *) malloc (sizeof (element));
  if (a == NULL) error ("Out of memory.");
  a->tag = ARROW;
  a->u.a.tag = RECOMM;
  lp = &line[7];
  a->u.a.source = node_at (read_point ());
  a->u.a.target = node_at (read_point ());
  a->u.a.DefaultCurvature = false;
  a->u.a.curvature = read_coord ();
  if (abs(a->u.a.curvature) > 100) 
  {
    puts ("Curvature too large; default assumed.");
    a->u.a.curvature = 0;
    a->u.a.DefaultCurvature = true;
  }
  if (insert_arrow (a))
  {
     puts ("Arrow source or target don't exist, or more than one arrow with the same source and target.");
     free (a);
  }
}


PRIVATE bool
analyze_tex_command (void)
{
  if (fgets (line, LINE_LEN, tex_file) == NULL)
    error ("Can't read LaTeX command.");
  if (prefix ("halfcoursec", line))
    analyze_halfcourse (true);
  else if (prefix ("reqhalfcoursec", line))
    analyze_reqhalfcourse (true);
  else if (prefix ("opthalfcoursec", line))
    analyze_opthalfcourse (true);
  else if (prefix ("fullcoursec", line))
    analyze_fullcourse (true);
  else if (prefix ("reqfullcoursec", line))
    analyze_reqfullcourse (true);
  else if (prefix ("optfullcoursec", line))
    analyze_optfullcourse (true);
  else if (prefix ("halfcourse", line))
    analyze_halfcourse (false);
  else if (prefix ("reqhalfcourse", line))
    analyze_reqhalfcourse (false);
  else if (prefix ("opthalfcourse", line))
    analyze_opthalfcourse (false);
  else if (prefix ("fullcourse", line))
    analyze_fullcourse (false);
  else if (prefix ("reqfullcourse", line))
    analyze_reqfullcourse (false);
  else if (prefix ("optfullcourse", line))
    analyze_optfullcourse (false);
  else if (prefix ("mini", line))
    analyze_mini ();
  else if (prefix ("text", line))
    analyze_text ();
  else if (prefix ("prereqc", line))
    analyze_prereqc ();
  else if (prefix ("prereq", line))
    analyze_prereq ();
  else if (prefix ("coreqc", line))
    analyze_coreqc ();
  else if (prefix ("coreq", line))
    analyze_coreq ();
  else if (prefix ("recommc", line))
    analyze_recommc ();
  else if (prefix ("recomm", line))
    analyze_recomm ();
  else if (prefix ("grid", line))
    grid = true;
  else if (prefix ("end{chart}", line))
    {
      return true;
    }
  else
    {
      char msg[LINE_LEN + 24] = {'\0'};
      char *msg_n = msg;
      append (msg, &msg_n, "Illegal command:\n ", sizeof (msg));
      append (msg, &msg_n, line, sizeof (msg));
      error (msg);
    }
  return false;
}

void
close_files (void)
{
  fclose (pretext);
  fclose (posttext);
  fclose (comments);
}

void
analyze_tex_file (void)   /* tex file -> internal representation */
{
  int ch;
  bool flag;			/* chart detected? */
  if ((pretext = tmpfile ()) == NULL) error ("Can't create temporary file.");
  if ((posttext = tmpfile ()) == NULL) error ("Can't create temporary file.");
  if ((comments = tmpfile ()) == NULL) error ("Can't create temporary file.");
  first_node = NULL;
  first_arrow = NULL;
  fflush (tex_file);
  rewind (tex_file);
  printf ("Analyzing %s.\n", chartfilename);
  ch = getc (tex_file);
  while (ch != EOF)
    {
      if (ch == '\\')
	{
	  fgets (line, LINE_LEN, tex_file);
	  flag = prefix ("begin{chart}", line);
	  putc ('\\', pretext);
	  fputs (line, pretext);
	  if (flag)
	    {
	      do
              { ch = getc (tex_file);}
	      while (isspace (ch));
	      while (true)
		{
		  if (ch == '\\')
		    {
		      if (analyze_tex_command ())
			{
			  fprintf (posttext, "%s\n", "\\end{chart}");
			  break;
			}
		    }
		  else if (ch == '%')
		    {
		      fgets (line, LINE_LEN, tex_file);
		      putc ('%', comments);
		      fputs (line, comments);
		    }
		  else
		    error
		      ("Unexpected 1st non-whitespace character in line.");
		  do
		  { ch = getc (tex_file);}
		  while (isspace (ch));
		}
	      ch = getc (tex_file);
	      while (ch != EOF)
		{
		  putc (ch, posttext);
		  ch = getc (tex_file);
		}
	    }
	}
      else
	{
	  putc (ch, pretext);
	}
      ch = getc (tex_file);
    }
}  /* analyze_tex_file */

PRIVATE void
output_node (element * p)
{
  if (p->u.n.tag == BOX)
    {
      putc ('\\', tex_file);
      if (p->u.n.u.b.req_opt == REQ)
	fprintf (tex_file, "req");
      else if (p->u.n.u.b.req_opt == OPT)
	fprintf (tex_file, "opt");
      if (p->u.n.u.b.half)
	{
	  fprintf (tex_file, "halfcourse");
	}
      else
	{
	  fprintf (tex_file, "fullcourse");
	}
      if (p->u.n.u.b.color[0] == '\0')
          fprintf (tex_file,
	       " %i,%i:{%s}{%s}{%s}\n",
	       p->u.n.at.x, p->u.n.at.y, p->u.n.code,
	       p->u.n.u.b.title, p->u.n.u.b.timetable);
      else
          fprintf (tex_file,
	       "c %i,%i:{%s}{%s}{%s}{%s}\n",
	       p->u.n.at.x, p->u.n.at.y, p->u.n.code,
	       p->u.n.u.b.title, p->u.n.u.b.timetable, p->u.n.u.b.color);
    }
  else if (p->u.n.tag == MINI)
    {
      fprintf (tex_file,
	       "\\mini %i,%i:{%s}\n", p->u.n.at.x, p->u.n.at.y, p->u.n.code);
    }
  else if (p->u.n.tag == TEXT)
    {
      fprintf (tex_file,
	       "\\text %i,%i:{%s}\n", p->u.n.at.x, p->u.n.at.y, p->u.n.u.t.txt);
    }
  else
    error ("Undefined node type.");
}

PRIVATE void
output_arrow (element * p)
{
  switch (p->u.a.tag)
    {
    case PREREQ:
      if (p->u.a.DefaultCurvature)
	{
	  fprintf (tex_file,
		   "  \\prereq %i,%i,%i,%i:\n",
		   p->u.a.source->u.n.at.x, p->u.a.source->u.n.at.y,
		   p->u.a.target->u.n.at.x, p->u.a.target->u.n.at.y);
	}
      else
	{
	  fprintf (tex_file,
		   "  \\prereqc %i,%i,%i,%i;%i:\n",
		   p->u.a.source->u.n.at.x, p->u.a.source->u.n.at.y,
		   p->u.a.target->u.n.at.x, p->u.a.target->u.n.at.y,
		   p->u.a.curvature);
	}
      break;
    case COREQ:
      if (p->u.a.DefaultCurvature)
	{
	  fprintf (tex_file,
		   "  \\coreq %i,%i,%i,%i:\n",
		   p->u.a.source->u.n.at.x, p->u.a.source->u.n.at.y,
		   p->u.a.target->u.n.at.x, p->u.a.target->u.n.at.y);
	}
      else
	{
	  fprintf (tex_file,
		   "  \\coreqc %i,%i,%i,%i;%i:\n",
		   p->u.a.source->u.n.at.x, p->u.a.source->u.n.at.y,
		   p->u.a.target->u.n.at.x, p->u.a.target->u.n.at.y,
		   p->u.a.curvature);
	}
      break;
    case RECOMM:
      if (p->u.a.DefaultCurvature)
	{
	  fprintf (tex_file,
		   "  \\recomm %i,%i,%i,%i:\n",
		   p->u.a.source->u.n.at.x, p->u.a.source->u.n.at.y,
		   p->u.a.target->u.n.at.x, p->u.a.target->u.n.at.y);
	}
      else
	{
	  fprintf (tex_file,
		   "  \\recommc %i,%i,%i,%i;%i:\n",
		   p->u.a.source->u.n.at.x, p->u.a.source->u.n.at.y,
		   p->u.a.target->u.n.at.x, p->u.a.target->u.n.at.y,
		   p->u.a.curvature);
	}
      break;
    default:
      error ("Undefined arrow kind.");
    }
}

PRIVATE void
merge_lists (void)
{
  /* 
     Output nodes in order (top-to-bottom, left-to-right).
     For each node, search for and output every arrow that targets that node, 
     provided its source node has already been output.
     After output of an arrow, remove it from the arrow list and save it on the 
     save_arrow list; the remaining arrows haven't yet been output.
     After output of all the nodes as above, process the remaining arrows, verifying 
     that their sources/targets haven't been cut or deleted.
     Then restore the saved arrows.
   */
  element *fn = first_node;
  element *fa, **ffa;		
  element *save_arrow = NULL;	/* arrows already output  */
  while (fn != NULL)
    {
      output_node (fn);
      ffa = &first_arrow;
      fa = first_arrow;
      while (fa != NULL)
	{
	  if (fa->u.a.target == fn)
	    {
	      element *ffn = first_node;
	      /* verify that a source node has already been output: */
	      while (ffn != fn && ffn != fa->u.a.source)
		{
		  ffn = ffn->next;
		}
	      if (ffn == fn)
		{
		  /* source node not yet output; defer arrow output */
		  ffa = &(fa->next);
		  fa = fa->next;
		  break;
		}
	      output_arrow (fa);
	      /* move arrow to save_arrow list: */
	      *ffa = fa->next;
	      fa->next = save_arrow;
	      save_arrow = fa;
	      fa = *ffa;
	    }
	  else
	    {
	      ffa = &(fa->next);
	      fa = fa->next;
	    }
	}
      fn = fn->next;
    }
  /* now output any remaining arrows (with source and target): */
  fa = first_arrow;
  while (fa != NULL)
    {
      /* verify that there is a source node:  */
      element *ffn = first_node;
      while (ffn != NULL && ffn != fa->u.a.source)
	{
	  ffn = ffn->next;
	}
      if (ffn == NULL)
	{
	  fa = fa->next;
	  continue;
	}
      /* verify that there is a target node:  */
      ffn = first_node;
      while (ffn != NULL && ffn != fa->u.a.target)
	{
	  ffn = ffn->next;
	}
      if (ffn == NULL)
	{
	  fa = fa->next;
	  continue;
	}
      output_arrow (fa);
      fa = fa->next;
    }
  /* restore saved arrows: */
  fa = save_arrow;
  while (fa != NULL)
    {
      save_arrow = fa->next;
      fa->next = first_arrow;
      first_arrow = fa;
      fa = save_arrow;
    }
}


void
regenerate_tex_file (void)  /* internal representation -> tex file  */
{
  printf ("Saving to %s.\n", chartfilename);
  fclose (tex_file);
  tex_file = fopen (chartfilename, "w+");
  if (tex_file == NULL) 
  {
    error("Can''t open TeX file");
  }
  copy (pretext, tex_file);
  if (grid)
    fprintf (tex_file, "\\grid\n");
  merge_lists ();
  copy (comments, tex_file);
  copy (posttext, tex_file);
  fflush(tex_file);
  rewind (tex_file);
}

