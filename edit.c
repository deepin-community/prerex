/*  
  edit.c  -- editing module for an interactive editor of prerequisite-chart 
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

PRIVATE element *first_cut;            /* stack of cut nodes */
PRIVATE done *first_done;               /* stack of commands done */


char deftext[LINE_LEN];            /* buffer for default input */

# ifdef HAVE_LIBREADLINE
int
set_deftext (void)            /* used by readline to output defaults */
{
  rl_insert_text (deftext);
  deftext[0] = '\0';
  return 0;
}
# else
PRIVATE char prompt[LINE_LEN];
PRIVATE char *prompt_n;

char *
prompt_f (void)                   /* used to output prompts */
{ return prompt; }
# endif

char *
Readline ( char * str )
/* Uses str as the prompt and deftext as the "default" */
{
# ifdef HAVE_LIBREADLINE
return readline (str);
# else
  char *line;
  int line_n;
  prompt[0] = '\0';
  prompt_n = prompt;
  append (prompt, &prompt_n, str, sizeof (prompt));
  el_push (el, deftext);
  line = (char *) el_gets (el, &line_n);
  if (line == NULL) error("Readline fails.");
  line[line_n - 1] = '\0';    /* replace '\n' by '\0' */ 
  deftext[0] = '\0';
  return line;
# endif
}

PRIVATE char *command_line;
PRIVATE char *clp;

PRIVATE void
free_elements (element ** pb)
{
  if (*pb != NULL)
    {
      element *pbb = *pb;
      free_elements (&(pbb->next));
      free (pbb);
      *pb = NULL;
    }
}

PRIVATE void
free_lists (void)
{
  free_elements (&first_node);
  free_elements (&first_arrow);
  free_elements (&first_cut);
}

PRIVATE void
skip_command (void)
{
  while (isspace (*clp)) clp++;
  while (isalpha (*clp)) clp++;  /* command */
  while (isspace (*clp)) clp++;
}

PRIVATE void
execute_shell_command (void)
{
  fflush (tex_file);
  if (fclose (tex_file) == EOF) error (".tex file closure failed.");
  if (system (clp)) puts("System call failed.");
  tex_file = fopen (chartfilename, "r+");
  if (tex_file == NULL) error ("Can't re-open the .tex file.");
  close_files(); 
  free_lists();
  analyze_tex_file ();
  regenerate_and_process ();
}

PRIVATE void
shiftAllNodes ( element *pb, coord c )
{
  while (pb != NULL)
    {
      pb->u.n.at.x += c;
      pb = pb->next;
    }
}

PRIVATE void
shift (void)
{
  coord c;
  int n_shifts = 0;
  done *this;
  coord x;
  element *pb = NULL;
  if (sscanf (command_line, "%*s %i", &c) != 1) {
      puts ("Can't read shift amount.");
      return;
    }
  clp = command_line;
  skip_command ();
  if (*clp == '-' || *clp == '+') clp++;
  while (isdigit (*clp)) clp++;   /* shift amount */
  if (sscanf (clp, "%i", &x) != 1) 
  { /* shift all nodes */
    shiftAllNodes(first_node, c);
    /* arrows refer to source and target nodes and will be shifted automatically.  */
    this = (done *) malloc (sizeof (done)); 
    if (this == NULL) error ("Out of memory");
    this->tag = SHIFT_ALL;
    this->next = first_done;
    this->i = c;
    first_done = this;
    regenerate_and_process ();
    return;
  }
  printf("Nodes shifted to: ");
  while (true) /* process coordinate pairs/ranges */
  {
    point p0, p1;
    if (sscanf (clp, "%i", &p0.x) != 1) {
      break;
    }
    while (isspace (*clp)) clp++;
    if (*clp == '-' || *clp == '+') clp++;
    while (isdigit(*clp)) clp++;
    while (isspace (*clp)) clp++;
    if (*clp != ',') {
      printf("\nCan't analyze coordinates.");
      break;
    }
    clp++;  /* ','  */
    if (sscanf (clp, "%i", &p0.y) != 1) {
      printf("\nCan't analyze coordinates.");
      break;
    }
    while (isspace (*clp)) clp++;
    if (*clp == '-' || *clp == '+') clp++;
    while (isdigit(*clp)) clp++;
    while (isspace (*clp)) clp++;
    if (*clp != ':') 
    { /* shift this node */
      pb = node_at (p0);
      if (pb == NULL) {
         printf("\nNo node at coordinates %i,%i.\n", p0.x, p0.y);
         break;
      }
      this = (done *) malloc (sizeof (done)); 
      if (this == NULL) error ("Out of memory");
      this->tag = NODE_SHIFT;
      this->next = first_done;
      this->i = c;  /* record shift amount and pointer to shifted node element  */
      this->e = pb;
      first_done = this;
      pb->u.n.at.x += c;
      printf(" %i,%i", p0.x + c, p0.y);
      n_shifts++;
      continue;
    }
    clp++; /* ':' */
    if (sscanf (clp, "%i", &p1.x) != 1) {
      printf("\nCan't analyze coordinates.");
      break;
    }
    while (isspace (*clp)) clp++;
    if (*clp == '-' || *clp == '+') clp++;
    while (isdigit(*clp)) clp++;
    while (isspace (*clp)) clp++;
    if (*clp != ',') {
      printf("\nCan't analyze coordinates.");
      break;
    }
    clp++;  /* ','  */
    if (sscanf (clp, "%i", &p1.y) != 1) {
      printf("\nCan't analyze coordinates.");
      break;
    }
    while (isspace (*clp)) clp++;
    if (*clp == '-' || *clp == '+') clp++;
    while (isdigit(*clp)) clp++;
    while (isspace (*clp)) clp++;
    /* shift all nodes in box p0:p1 */
    {
      pb = first_node;
      while (pb != NULL) {
        if (inrange(pb->u.n.at.x, p0.x, p1.x) && inrange(pb->u.n.at.y, p0.y, p1.y)) 
        /* shift this node */
        {
          pb->u.n.at.x += c;
          printf(" %i,%i", pb->u.n.at.x, pb->u.n.at.y);
          this = (done *) malloc (sizeof (done)); 
          if (this == NULL) error ("Out of memory");
          this->tag = NODE_SHIFT;
          this->next = first_done;
          this->i = c;  /* record shift amount and pointer to shifted node element  */
          this->e = pb;
          first_done = this;
          n_shifts++;
        }
        pb = pb->next;
      }
    }
  } /* while */
  if (n_shifts > 0) 
  {
    puts("");
    this = (done *) malloc (sizeof (done)); 
    if (this == NULL) error ("Out of memory");
    this->tag = COMPOSITE;
    this->next = first_done;
    this->i = n_shifts;  /* record number of shifts to undo */
    first_done = this;
  }
  regenerate_and_process ();
}

PRIVATE void
raiseAllNodes ( element *pb, coord c )
{
  while (pb != NULL) {
      pb->u.n.at.y += c;
      pb = pb->next;
    }
}

PRIVATE void
Raise (void)  /* "Raise" to avoid conflicting with signal.h function raise */
{
  coord c;
  int n_raises = 0;
  done *this;
  coord x;
  element *pb = NULL;
  if (sscanf (command_line, "%*s %i", &c) != 1)
    {
      puts ("Can't read raise amount.");
      return;
    }
  clp = command_line;
  skip_command ();
  if (*clp == '-' || *clp == '+') clp++;
  while (isdigit (*clp)) clp++;   /* raise amount */
  if (sscanf (clp, "%i", &x) != 1) 
  { /* raise all nodes */
    raiseAllNodes(first_node, c);
    /* arrows refer to source and target nodes and will be raised automatically.  */
    this = (done *) malloc (sizeof (done)); 
    if (this == NULL) error ("Out of memory");
    this->tag = RAISE_ALL;
    this->next = first_done;
    this->i = c;
    first_done = this;
    regenerate_and_process ();
    return;
  }
  printf("Nodes raised to: ");
  while (true) /* process coordinate pairs */
  {
    point p0, p1;
    if (sscanf (clp, "%i", &p0.x) != 1) break;
    while (isspace (*clp)) clp++;
    if (*clp == '-' || *clp == '+') clp++;
    while (isdigit(*clp)) clp++;
    while (isspace (*clp)) clp++;
    if (*clp != ',') {
      printf("\nCan't analyze coordinates.");
      break;
    }
    clp++;  /* ','  */
    if (sscanf (clp, "%i", &p0.y) != 1) {
      printf("\nCan't analyze coordinates.");
      break;
    }
    while (isspace (*clp)) clp++;
    if (*clp == '-' || *clp == '+') clp++;
    while (isdigit(*clp)) clp++;
    while (isspace (*clp)) clp++;
    if (*clp != ':')
    { /* raise this node */
      pb = node_at (p0);
      if (pb == NULL) 
      {
         printf("No node at coordinates %i,%i.\n", p0.x, p0.y);
         break;
      }
      this = (done *) malloc (sizeof (done)); 
      if (this == NULL) error ("Out of memory");
      this->tag = NODE_RAISE;
      this->next = first_done;
      this->i = c;  /* record raise amount and pointer to raised node element  */
      this->e = pb;
      first_done = this;
      pb->u.n.at.y += c;
      printf(" %i,%i", p0.x, p0.y + c);
      n_raises++;
      continue;
    }
    clp++; /* ':' */
    if (sscanf (clp, "%i", &p1.x) != 1) {
      printf("\nCan't analyze coordinates.");
      break;
    }
    while (isspace (*clp)) clp++;
    if (*clp == '-' || *clp == '+') clp++;
    while (isdigit(*clp)) clp++;
    while (isspace (*clp)) clp++;
    if (*clp != ',') {
      printf("\nCan't analyze coordinates.");
      break;
    }
    clp++;  /* ','  */
    if (sscanf (clp, "%i", &p1.y) != 1) {
      printf("\nCan't analyze coordinates.");
      break;
    }
    while (isspace (*clp)) clp++;
    if (*clp == '-' || *clp == '+') clp++;
    while (isdigit(*clp)) clp++;
    while (isspace (*clp)) clp++;
    /* raise all nodes in box p0:p1 */
    {
      done *this;
      pb = first_node;
      while (pb != NULL) {
        if (inrange(pb->u.n.at.x, p0.x, p1.x) && inrange(pb->u.n.at.y, p0.y, p1.y)) 
        /* raise this node */
        {
          pb->u.n.at.y += c;
          printf(" %i,%i", pb->u.n.at.x, pb->u.n.at.y);
          this = (done *) malloc (sizeof (done)); 
          if (this == NULL) error ("Out of memory");
          this->tag = NODE_RAISE;
          this->next = first_done;
          this->i = c;  /* record raise amount and pointer to shifted node element  */
          this->e = pb;
          first_done = this;
          n_raises++;
        }
        pb = pb->next;
      }
    }
  } /* while */
  if (n_raises > 0) {
    puts("");
    this = (done *) malloc (sizeof (done)); 
    if (this == NULL) error ("Out of memory");
    this->tag = COMPOSITE;
    this->next = first_done;
    this->i = n_raises;  /* record the number of raises to undo */
    first_done = this;
  }
  regenerate_and_process ();
}

PRIVATE void
cut_node (point p)
{
  element *pn, **ppn;
  done *this;
  ppn = &first_node;  /* used if the first node is removed from the list */
  pn = first_node;
  while (pn != NULL && !eq (p, pn->u.n.at))
    {
      ppn = &(pn->next);
      pn = pn->next;
    }
  if (pn == NULL)
    {
      puts ("No course box, mini or text at this location.");
      return;
    }
  *ppn = pn->next;  /* removes pn node from the list */
  /* move node to cut list */
  pn->next = first_cut;
  first_cut = pn;
  this = (done *) malloc (sizeof (done)); 
  if (this == NULL) error ("Out of memory");
  this->tag = CUT;
  this->next = first_done;
  first_done = this;
}

PRIVATE void
delete_node (point p)
{
  element *pn, **ppn;
  done *this;
  ppn = &first_node;
  pn = first_node;
  while (pn != NULL && !eq (p, pn->u.n.at))
    {
      ppn = &(pn->next);
      pn = pn->next;
    }
  if (pn == NULL)
    {
      puts ("No course box, mini or text at this location.");
      return;
    }
  *ppn = pn->next;
  this = (done *) malloc (sizeof (done)); 
  if (this == NULL) error ("Out of memory");
  this->tag = NODE_DELETE;
  this->next = first_done;
  this->e = pn;
  first_done = this;
  printf("Node at %i,%i deleted.\n", p.x, p.y);
}

PRIVATE void
delete_arrow (point p0, point p1)
{
  done *this;
  element *n0 = node_at (p0);
  element *n1 = node_at (p1);
  element *pa;
  element **ppa; 
  if (n0 == NULL)
    {
      puts ("No course box, mini or text at the source location.");
      return;
    }
  if (n1 == NULL)
    {
      puts ("No course box, mini or text at the target location.");
      return;
    }
  pa = first_arrow;
  ppa = &first_arrow; /* used if the first arrow is removed */          
  while (pa != NULL && (pa->u.a.source != n0 || pa->u.a.target != n1))
    {
      ppa = &(pa->next);
      pa = pa->next;
    }
  if (pa == NULL)
    {
      puts ("No such arrow.");
      return;
    }
  *ppa = pa->next; /* removes pa arrow from the list */
  this = (done *) malloc (sizeof (done)); 
  if (this == NULL) error ("Out of memory");
  this->tag = ARROW_DELETE;
  this->next = first_done;
  this->e = pa;
  first_done = this;
  printf("Arrow from %i,%i to %i,%i deleted.\n", p0.x, p0.y, p1.x, p1.y);
}

PRIVATE void
analyze_cut_command (void)
{
  clp = command_line;
  skip_command ();
  while (true) /* process coordinate pairs */
  {
    point p;
    if (sscanf (clp, "%i", &p.x) != 1) break;
    while (isspace (*clp)) clp++;
    if (*clp == '-' || *clp == '+') clp++;
    while (isdigit(*clp)) clp++;
    while (isspace (*clp)) clp++;
    if (*clp != ',') {
      puts("Can't analyze coordinates.");
      return;
    }
    clp++;  /* ','  */
    if (sscanf (clp, "%i", &p.y) != 1) {
      puts("Can't analyze coordinates.");
      return;
    }
    while (isspace (*clp)) clp++;
    if (*clp == '-' || *clp == '+') clp++;
    while (isdigit(*clp)) clp++;
    cut_node (p);
  }
}

PRIVATE void
analyze_delete_command (void)
{
  int n_deletions = 0;
  done *this;
  clp = command_line;
  skip_command ();
  while (true) /* process coordinate pairs/ranges/4-tuples */
  {
    point p0, p1; 
    bool colon;  
    if (sscanf (clp, "%i", &p0.x) != 1) break;
    while (isspace (*clp)) clp++;
    if (*clp == '-' || *clp == '+') clp++;
    while (isdigit(*clp)) clp++;
    while (isspace (*clp)) clp++;
    if (*clp != ',') {
      puts("Can't analyze coordinates.");
      break;
    }
    clp++;  /* ','  */
    if (sscanf (clp, "%i", &p0.y) != 1) {
      puts("Can't analyze coordinates.");
      break;
    }
    while (isspace (*clp)) clp++;
    if (*clp == '-' || *clp == '+') clp++;
    while (isdigit(*clp)) clp++;
    while (isspace (*clp)) clp++;
    if (*clp != ',' && *clp != ':') 
    {
      delete_node (p0);
      n_deletions++;
      continue;
    }
    colon = *clp == ':'; 
    clp++;  /* ',' or ':' */
    if (sscanf (clp, "%i", &p1.x) != 1) {
      puts("Can't analyze coordinates.");
      break;
    }
    while (isspace (*clp)) clp++;
    if (*clp == '-' || *clp == '+') clp++;
    while (isdigit(*clp)) clp++;
    while (isspace (*clp)) clp++;
    if (*clp != ',') {
      puts("Can't analyze coordinates.");
      break;
    }
    clp++;  /* ','  */
    if (sscanf (clp, "%i", &p1.y) != 1) {
      puts("Can't analyze coordinates.");
      break;
    }
    if (*clp == '-' || *clp == '+') clp++;
    while (isdigit(*clp)) clp++;
    if (!colon)  /* arrow from p0 to p1 */
    {
      delete_arrow (p0, p1);
      n_deletions++;
      continue;
    }
    /* delete all nodes in box p0:p1 */
    {
      element *pn, **ppn;
      ppn = &(first_node);
      pn = first_node;
      while (pn != NULL) {
        if (inrange(pn->u.n.at.x, p0.x, p1.x) && inrange(pn->u.n.at.y, p0.y, p1.y)) 
        /* delete this node */
        {
          *ppn = pn->next;
          printf("Node at %i,%i deleted.\n", pn->u.n.at.x, pn->u.n.at.y);
          this = (done *) malloc (sizeof (done)); 
          if (this == NULL) error ("Out of memory");
          this->tag = NODE_DELETE;
          this->next = first_done;
          this->e = pn;
          first_done = this;
          n_deletions++;
        }
        else  ppn = &(pn->next);
        pn = pn->next;
      }
    }
  } /*  while  */
  this = (done *) malloc (sizeof (done)); 
  if (this == NULL) error ("Out of memory");
  this->tag = COMPOSITE;
  this->next = first_done;
  this->i = n_deletions;  /* record the number of deletions to undo */
  first_done = this;
  regenerate_and_process ();
}

PRIVATE void
undo (void)
{
  done *this = first_done;
  if (this == NULL)
    {
      puts ("Nothing more to undo.");
      return;
    }
  first_done = this->next;
  if (this->tag == NODE_DELETE)
    { 
      element *pn = this->e;
      if (insert_node (pn))
        {
        puts ("There is now another course box, mini or text at that location.");
          return;
        }
      printf ("Course box, mini or text at %i,%i restored.\n",
            pn->u.n.at.x, pn->u.n.at.y);
    } 
  else if (this->tag == ARROW_DELETE)
    { 
      element *pn = this->e;
      if (insert_arrow (pn))
      {
        puts
          ("Can't undelete arrow: source and/or target is/are now missing.");
        return;
      }
      printf ("Arrow from %i,%i to %i,%i restored.\n",
            pn->u.a.source->u.n.at.x, pn->u.a.source->u.n.at.y,
            pn->u.a.target->u.n.at.x, pn->u.a.target->u.n.at.y);
    }
  else if (this->tag == NODE_CREATE)
    {
      element *pn, **ppn;
      point p = this->p;
      ppn = &(first_node);
      pn = first_node;
      while (pn != NULL && !eq (p, pn->u.n.at))
        {
          ppn = &(pn->next);
          pn = pn->next;
        }
      if (pn == NULL)
        {
          puts ("No course box, mini or text at this location.");
          return;
        }
      *ppn = pn->next;
      printf("Course box, mini or text at %i,%i deleted.\n", p.x, p.y);
    }
  else if (this->tag == ARROW_CREATE)
    {
      element *n0, *n1, *pa, **ppa;
      n0 = this->a.source;
      n1 = this->a.target;
      pa = first_arrow;
      ppa = &(first_arrow); 
      while (pa != NULL && (pa->u.a.source != n0 || pa->u.a.target != n1))
        {
          ppa = &(pa->next);
          pa = pa->next;
        }
      if (pa == NULL)
        {
          printf("There is no arrow from %i,%i to %i,%i.\n", n0->u.n.at.x, n0->u.n.at.y, n1->u.n.at.x, n1->u.n.at.y);
          return;
        }
      *ppa = pa->next;
      printf("Arrow from %i,%i to %i,%i deleted.\n", n0->u.n.at.x, n0->u.n.at.y, n1->u.n.at.x, n1->u.n.at.y);
    }
  else if (this->tag == NODE_EDIT)
    {
      element *pn = this->e;
      pn->u.n = this->n; /* restore old node */
      printf("Course box, mini or text at %i,%i restored.\n", pn->u.n.at.x, pn->u.n.at.y);
    }
  else if (this->tag == ARROW_EDIT)
    {
       element *pa = this->e;
       pa->u.a = this->a;  /* restore old arrow */
       printf("Arrow from %i,%i to %i,%i restored.\n", pa->u.a.source->u.n.at.x, pa->u.a.source->u.n.at.y, pa->u.a.target->u.n.at.x, pa->u.a.target->u.n.at.y);
    }
  else if (this->tag == NODE_SHIFT)
    {
      element *pb = this->e;
      int c = this->i;
      pb->u.n.at.x -= c;
      printf("Course box, mini or text unshifted %i units to %i,%i.\n", c, pb->u.n.at.x, pb->u.n.at.y);
    }
  else if (this->tag == NODE_RAISE)
    {
      element *pb = this->e;
      int c = this->i;
      pb->u.n.at.y -= c;
      printf("Course box, mini or text unraised %i units to %i,%i.\n", c, pb->u.n.at.x, pb->u.n.at.y);
    }
  else if (this->tag == SHIFT_ALL)
    {
      int c = this->i;  
      shiftAllNodes(first_node, -c);
      printf("All nodes unshifted %i units.\n", c);
    }
  else if (this->tag == RAISE_ALL)
    {
      int c = this->i;  
      raiseAllNodes(first_node, -c);
      printf("All nodes unraised %i units.\n", c);
    }
  else if (this->tag == CUT)
    {
      element *pn = first_cut;
      first_cut = pn->next;
      if (insert_node (pn))
      {
        printf ("There is already a course box, mini, or text at %i,%i.\n", pn->u.n.at.x, pn->u.n.at.y);
        return;
      }
      else
        printf ("Course box, mini or text restored at %i,%i.\n", pn->u.n.at.x, pn->u.n.at.y);
    }
  else if (this->tag == PASTE)
    {
      element *pn, **ppn;
      element *pe = this->e;
      ppn = &(first_node);
      pn = first_node;
      while (pn != NULL && pn != pe)
        {
          ppn = &(pn->next);
          pn = pn->next;
        }
      if (pn == NULL)
        {
          puts ("Unable to find pasted node.");
          return;
        }
      *ppn = pn->next;
      pn->next = first_cut;
      printf ("Course box, mini or text unpasted from %i,%i.\n", pn->u.n.at.x, pn->u.n.at.y);
      pn->u.n.at = this->p;
      first_cut = pn; 
    }
  else if (this->tag == COMPOSITE)
    /* undo this->i steps */
    { int j;
      for (j = 0; j < this->i; j++) undo();
    }
}

PRIVATE void
edit_mini (element * pm)
{
  append (deftext, NULL, pm->u.n.code, sizeof (deftext));
  command_line = Readline ("course code: ");
  pm->u.n.code[0] = '\0';
  if (append (pm->u.n.code, NULL, command_line, sizeof (pm->u.n.code))
      >= sizeof (pm->u.n.code))
    puts ("Warning: course code too long, truncated.");
}

PRIVATE void
edit_text (element * pm)
{
  append (deftext, NULL, pm->u.n.u.t.txt, sizeof (deftext));
  command_line = Readline ("text: ");
  pm->u.n.u.t.txt[0] = '\0';
  if (append (pm->u.n.u.t.txt, NULL, command_line, sizeof (pm->u.n.u.t.txt))
      >= sizeof (pm->u.n.u.t.txt))
    puts ("Warning: text too long, truncated.");
}

PRIVATE void
edit_box (element * pb)
{
  char code[8];
  append (deftext, NULL, pb->u.n.code, sizeof (deftext));
  command_line = Readline ("course code: ");
  pb->u.n.code[0] = '\0';
  if (append (pb->u.n.code, NULL, command_line, sizeof (pb->u.n.code))
      >= sizeof (pb->u.n.code))
    puts ("Warning: course code too long, truncated.");
  if (pb->u.n.u.b.half)
    append (deftext, NULL, "y", sizeof (deftext));
  else
    append (deftext, NULL, "n", sizeof (deftext));
  command_line = Readline ("half course (y/n)? ");
  sscanf (command_line, "%1s", code);
  if (code[0] == 'y')
    pb->u.n.u.b.half = true;
  else if (code[0] == 'n')
    pb->u.n.u.b.half = false;
  else
    {
      puts ("Response not recognized; 'y' assumed.");
      pb->u.n.u.b.half = true;
    }
  if (pb->u.n.u.b.req_opt == REQ)
    append (deftext, NULL, "r", sizeof (deftext));
  else if (pb->u.n.u.b.req_opt == OPT)
    append (deftext, NULL, "o", sizeof (deftext));
  else
    append (deftext, NULL, "n", sizeof (deftext));
  command_line = Readline ("required (r) optional (o) or neither (n)? ");
  sscanf (command_line, "%7s", code);
  if (code[0] == 'r')
    pb->u.n.u.b.req_opt = REQ;
  else if (code[0] == 'o')
    pb->u.n.u.b.req_opt = OPT;
  else if (code[0] == 'n')
    pb->u.n.u.b.req_opt = NEITHER;
  else
    {
      puts ("Response not recognized; 'n' assumed.");
      pb->u.n.u.b.req_opt = NEITHER;
    }
  append (deftext, NULL, pb->u.n.u.b.title, sizeof (deftext));
  command_line = Readline ("course title: ");
  pb->u.n.u.b.title[0] = '\0';
  if (append (pb->u.n.u.b.title, NULL, command_line, sizeof (pb->u.n.u.b.title))
      >= sizeof (pb->u.n.u.b.title))
    puts ("Warning: course title too long, truncated.");
  append (deftext, NULL, pb->u.n.u.b.timetable, sizeof (deftext));
  command_line = Readline ("course timetable: ");
  pb->u.n.u.b.timetable[0] = '\0';
  if (append
      (pb->u.n.u.b.timetable, NULL, command_line,
       sizeof (pb->u.n.u.b.timetable)) >= sizeof (pb->u.n.u.b.timetable))
    puts ("Warning: course timetable too long, truncated.");
  append (deftext, NULL, pb->u.n.u.b.color, sizeof (deftext));
  command_line = Readline ("background color (empty string for default): ");
  pb->u.n.u.b.color[0] = '\0';
  if (append
      (pb->u.n.u.b.color, NULL, command_line,
       sizeof (pb->u.n.u.b.color)) >= sizeof (pb->u.n.u.b.color))
    puts ("Warning: color string too long, truncated.");
  if (pb->u.n.u.b.color[0] == '\n') pb->u.n.u.b.color[0] = '\0';
}

PRIVATE void
set_curvature (element * pa)
{
  char code[16];
  if (pa->u.a.DefaultCurvature) 
    {
      append (deftext, NULL, "d", sizeof (deftext));
    }
  else
    {
      sprintf (deftext, "%i", pa->u.a.curvature);
    }
  command_line = Readline ("curvature, default (d) or int value? ");
  sscanf (command_line, "%15s", code);
  if ((code[0] == '-' && isdigit (code[1]) ) || isdigit (code[0]))
    {
      sscanf (code, "%i", &(pa->u.a.curvature));
      if (abs(pa->u.a.curvature) > 100) {
        puts ("Curvature too large; default curvature used.");
        pa->u.a.DefaultCurvature = true;
        pa->u.a.curvature = 0;
      }
      else pa->u.a.DefaultCurvature = false;
    }
  else if (code[0] == 'd')
  {
    pa->u.a.DefaultCurvature = true;   
    pa->u.a.curvature = 0;
  }
  else
    {
      puts ("Response not recognized; default assumed.");
      pa->u.a.DefaultCurvature = true;
      pa->u.a.curvature = 0;
    }
}

PRIVATE void
edit_arrow (element * pa)
{
  char code[16];
  switch (pa->u.a.tag)
    {
    case PREREQ:
      append (deftext, NULL, "p", sizeof (deftext));
      break;
    case COREQ:
      append (deftext, NULL, "c", sizeof (deftext));
      break;
    case RECOMM:
      append (deftext, NULL, "r", sizeof (deftext));
      break;
    default:;
    }
  command_line =
    Readline ("prerequisite (p), corequisite (c), or recommended (r)? ");
  sscanf (command_line, "%15s", code);
  switch (code[0])
    {
    case 'p':
      pa->u.a.tag = PREREQ;
      break;
    case 'c':
      pa->u.a.tag = COREQ;
      break;
    case 'r':
      pa->u.a.tag = RECOMM;
      break;
    default:
      puts ("Response not recognized; prerequisite assumed.");
      pa->u.a.tag = PREREQ;
    }
  set_curvature (pa);
}

PRIVATE void
paste_node (point p)
{
  point oldp;
  element *pn = first_cut;
  done *this;
  if (pn == NULL)
    {
      puts ("No cut box, mini or text to paste.");
      return;
    }
  first_cut = pn->next;
  oldp = pn->u.n.at;
  pn->u.n.at = p;
  if (insert_node (pn))
    {
      printf("There is a course box, mini or text already at %i,%i.\n", pn->u.n.at.x, pn->u.n.at.y);
      pn->next = first_cut;
      first_cut = pn;
      return;
    }
  if (first_cut == NULL)
    regenerate_and_process ();
  this = (done *) malloc (sizeof (done)); 
  if (this == NULL) error ("Out of memory");
  this->tag = PASTE;
  this->e = pn;  
  this->p = oldp;
  this->next = first_done;
  first_done = this;
}

PRIVATE void
analyze_paste_command (void)
{ point p;
  if (sscanf (command_line, "%*s %i,%i", &p.x, &p.y) != 2) {
    puts("Can't analyze coordinates.");
    return;
  }
  paste_node (p);
}

PRIVATE void
analyze_xchange_command (void)
{
  point p0, p1;
  done *this;
  clp = command_line;
  skip_command ();
  if (sscanf (clp, "%i", &p0.x) != 1) {
    puts("Can't analyze coordinates.");
    return;
  }
  while (isspace (*clp)) clp++;
  if (*clp == '-' || *clp == '+') clp++;
  while (isdigit(*clp)) clp++;
  while (isspace (*clp)) clp++;
  if (*clp != ',') {
    puts("Can't analyze coordinates.");
    return;
  }
  clp++;  /* ','  */
  if (sscanf (clp, "%i", &p0.y) != 1) {
    puts("Can't analyze coordinates.");
    return;
  }
  while (isspace (*clp)) clp++;
  if (*clp == '-' || *clp == '+') clp++;
  while (isdigit(*clp)) clp++;
  cut_node (p0);

  if (sscanf (clp, "%i", &p1.x) != 1) {
    puts("Can't analyze coordinates.");
    return;
  }
  while (isspace (*clp)) clp++;
  if (*clp == '-' || *clp == '+') clp++;
  while (isdigit(*clp)) clp++;
  while (isspace (*clp)) clp++;
  if (*clp != ',') {
    puts("Can't analyze coordinates.");
    return;
  }
  clp++;  /* ','  */
  if (sscanf (clp, "%i", &p1.y) != 1) {
    puts("Can't analyze coordinates.");
    return;
  }
  while (isspace (*clp)) clp++;
  if (*clp == '-' || *clp == '+') clp++;
  while (isdigit(*clp)) clp++;
  cut_node (p1);

  paste_node (p0);
  paste_node (p1);
  
  this = (done *) malloc (sizeof (done)); 
  if (this == NULL) error ("Out of memory");
  this->tag = COMPOSITE;
  this->i = 4;   /* four steps to undo */
  this->next = first_done;
  first_done = this;
}

PRIVATE void
analyze_box_command (void)
{
  point p;
  element *pb;
  done *this;
  char code[8];
  if (sscanf (command_line, "%*s %i,%i", &p.x, &p.y) != 2)
    {
      puts ("Can't analyze box command.");
      return;
    }
  this = (done *) malloc (sizeof (done)); 
  if (this == NULL) error ("Out of memory");
  pb = node_at (p);
  if (pb != NULL)
    {
      this->tag = NODE_EDIT;
      this->next = first_done;
      this->n = pb->u.n;  /* record the old node */
      this->e = pb;
      first_done = this;
      edit_box (pb);
      regenerate_and_process ();
      return;
    }
  this->tag = NODE_CREATE;
  append (deftext, NULL, "y", sizeof (deftext));
  command_line = Readline ("create new course box (y/n)? ");
  sscanf (command_line, "%7s", code);
  if (code[0] == 'y')
    {
      pb = (element *) malloc (sizeof (element));
      if (pb == NULL) error ("Out of memory");
      pb->tag = NODE;
      pb->u.n.tag = BOX;
      pb->u.n.at = p;
      pb->u.n.u.b.req_opt = NEITHER;
      pb->u.n.u.b.half = true;
      pb->u.n.code[0] = '\0';
      pb->u.n.u.b.title[0] = '\0';
      pb->u.n.u.b.timetable[0] = '\0';
      if (insert_node (pb))
      {
        puts ("There is already a course box, mini, or text at that location.");
          free (pb);
          free (this);
        return;
      }
      this->next = first_done;
      this->p = p; /* record coordinates of new node */
      first_done = this;
      edit_box (pb);
      regenerate_and_process ();
      return;
    }
  else if (code[0] == 'n')
    {
       free (this);
    }
  else
    {
      puts ("Response not recognized; 'n' assumed.");
      free (this);
    }
}

PRIVATE void
analyze_mini_command (void)
{
  point p;
  element *pm;
  done *this;
  char code[8];
  if (sscanf (command_line, "%*s %i,%i", &p.x, &p.y) != 2)
    {
      puts ("Can't analyze mini command.");
      return;
    }
  this = (done *) malloc (sizeof (done)); 
  if (this == NULL) error ("Out of memory");
  pm = node_at (p);
  if (pm != NULL)
    {
      this->tag = NODE_EDIT;
      this->next = first_done;
      this->n = pm->u.n;  /* record the old node */
      this->e = pm;
      first_done = this;
      edit_mini (pm);
      regenerate_and_process ();
      return;
    }
  this->tag = NODE_CREATE;
  append (deftext, NULL, "y", sizeof (deftext));
  command_line = Readline ("create new mini (y/n)? ");
  sscanf (command_line, "%7s", code);
  if (code[0] == 'y')
    {
      pm = (element *) malloc (sizeof (element));
      if (pm == NULL) error ("Out of memory");
      pm->tag = NODE;
      pm->u.n.tag = MINI;
      pm->u.n.at = p;
      pm->u.n.code[0] = '\0';
      if (insert_node (pm))
      {
        puts ("There is already a course box, mini, or text at that location.");
          free (pm);
          free (this);
        return;
      }
      this->next = first_done;
      this->p = p; /* record coordinates of new node */
      first_done = this;
      edit_mini (pm);
      regenerate_and_process ();
    }
  else if (code[0] == 'n')
    {
      free (this);
    }
  else
    {
      puts ("Response not recognized; 'n' assumed.");
      free (this);
    }
}

PRIVATE void
analyze_text_command (void)
{
  point p;
  element *pm;
  done *this;
  char code[8];
  if (sscanf (command_line, "%*s %i,%i", &p.x, &p.y) != 2)
    {
      puts ("Can't analyze text command.");
      return;
    }
  this = (done *) malloc (sizeof (done)); 
  if (this == NULL) error ("Out of memory");
  pm = node_at (p);
  if (pm != NULL)
    {
      this->tag = NODE_EDIT;
      this->next = first_done;
      this->n = pm->u.n;  /* record the old node */
      this->e = pm;
      first_done = this;
      edit_text (pm);
      regenerate_and_process ();
      return;
    }
  this->tag = NODE_CREATE;
  append (deftext, NULL, "y", sizeof (deftext));
  command_line = Readline ("create new text (y/n)? ");
  sscanf (command_line, "%7s", code);
  if (code[0] == 'y')
    {
      pm = (element *) malloc (sizeof (element));
      if (pm == NULL) error ("Out of memory");
      pm->tag = NODE;
      pm->u.n.tag = TEXT;
      pm->u.n.at = p;
      pm->u.n.code[0] = '\0';
      if (insert_node (pm))
      {
        puts ("There is already a course box, mini, or text at that location.");
          free (pm);
          free (this);
        return;
      }
      this->next = first_done;
      this->p = p; /* record coordinates of new node */
      first_done = this;
      edit_text (pm);
      regenerate_and_process ();
    }
  else if (code[0] == 'n')
    {
      free (this);
    }
  else
    {
      puts ("Response not recognized; 'n' assumed.");
      free (this);
    }
}

PRIVATE void
analyze_arrow_command (void)
{
  point p0, p1;
  element *n0, *n1;
  element *pa;
  done *this;
  if (sscanf (command_line, "%*s %i,%i,%i,%i", &p0.x, &p0.y, &p1.x, &p1.y) !=
      4)
    {
      puts ("Can't analyze arrow command.");
      return;
    }
  n0 = node_at (p0);
  if (n0 == NULL)
    {
      puts ("There is no course box, mini, or text at the source location.");
      return;
    }
  n1 = node_at (p1);
  if (n1 == NULL)
    {
      puts ("There is no course box, mini, or text at the target location.");
      return;
    }
  this = (done *) malloc (sizeof (done)); 
  if (this == NULL) error ("Out of memory");
  pa = first_arrow;
  while (pa != NULL && !(pa->u.a.source == n0 && pa->u.a.target == n1))
    pa = pa->next;
  if (pa != NULL)
    {
      this->tag = ARROW_EDIT;
      this->next = first_done;
      this->a = pa->u.a;  /* record the old arrow  */
      this->e = pa;
      first_done = this;
      edit_arrow (pa);
      regenerate_and_process ();
    }
  else
    {
      char code[8];
      this->tag = ARROW_CREATE;
      append (deftext, NULL, "y", sizeof (deftext));
      command_line = Readline ("create new arrow (y/n)? ");
      sscanf (command_line, "%7s", code);
      if (code[0] == 'y')
      {
        pa = (element *) malloc (sizeof (element));
        if (pa == NULL) error ("Out of memory");
        pa->tag = ARROW;
        pa->u.a.source = n0;
        pa->u.a.target = n1;
        pa->u.a.tag = PREREQ;
        pa->u.a.DefaultCurvature = true;
        pa->u.a.curvature = 0;   
        if (insert_arrow (pa))
        {
          /*  shouldn't happen! */
          free (pa);
          free (this);
          return;
        }
        this->next = first_done;
        this->a = pa->u.a; /* record arrow */
        first_done = this;
        edit_arrow (pa);
        regenerate_and_process ();
      }
      else if (code[0] == 'n')
        {
        free (this);
        }
      else
      {
        puts ("Response not recognized; 'n' assumed.");
        free (this);
      }
    }
}

PRIVATE void
analyze_grid_command (void)
{
  char code[8];
  if (sscanf (command_line, "%*s %7s", code) != 1)
    {
      if (grid)
           append (deftext, NULL, "n", sizeof (deftext));
      else
           append (deftext, NULL, "y", sizeof (deftext));
      command_line = Readline ("grid (y/n)? ");
      sscanf (command_line, "%7s", code);
    }
  if (code[0] == 'y')
    grid = true;
  else if (code[0] == 'n')
    grid = false;
  else
    {
      puts ("Response not recognized; 'y' assumed.");
      grid = true;
    }
  regenerate_and_process ();
}

PRIVATE void
analyze_file_command (void)
{
  puts ("This command is no longer supported.");
}

PRIVATE void
backup (void)
{
  char code[8];
  append (deftext, NULL, "y", sizeof (deftext));
  command_line = Readline ("Overwrite current backup file (y/n)? ");
  sscanf (command_line, "%7s", code);
  if (code[0] != 'y')
    return;
  backup_tex_file = fopen (backup_filename, "w");
  if (backup_tex_file == NULL)
    {
      puts ("Can't open backup file.");
      return;
    }
  copy (tex_file, backup_tex_file);
  fclose (backup_tex_file);
  printf ("Backed up to %s.\n", backup_filename);
}

PRIVATE void
restore (void)
{
  char code[8];
  backup_tex_file = fopen (backup_filename, "r");
  if (backup_tex_file == NULL)
    {
      puts ("Can't open backup file.");
      return;
    }
  append (deftext, NULL, "y", sizeof (deftext));
  command_line =
    Readline ("Delete current TeX file and restore from backup (y/n)? ");
  sscanf (command_line, "%7s", code);
  if (code[0] != 'y')
    return;
  fclose (tex_file);
  tex_file = fopen (chartfilename, "w+");
  rewind (tex_file); 
  copy (backup_tex_file, tex_file);
  fflush (tex_file);
  fclose (backup_tex_file);
  close_files();
  free_lists();
  grid = false;
  analyze_tex_file ();
  regenerate_and_process ();
}


PRIVATE void
quit (void)
{
  char code[8];
  if (first_cut)
    {
      append (deftext, NULL, "y", sizeof (deftext));
      command_line =
        Readline ("Warning: there are unpasted cuts; continue (y/n)? ");
      sscanf (command_line, "%7s", code);
      if (code[0] != 'y')
        return;
    }
# ifdef HAVE_LIBEDIT
  history_end (hist);
  el_end (el);
# endif
  grid = false;
  puts("Turning off coordinate grid.");
  regenerate_and_process ();
  close_files ();
  exit (0);
}

PRIVATE void
help (void)
{
  puts ("");
  puts ("        command:                    effect:");
  puts ("");
  puts (" file> box    x,y                   [create and] edit course box at x,y");
  puts (" file> mini   x,y                   [create and] edit mini course at x,y");
  puts (" file> text   x,y                   [create and] edit text centered at x,y");
  puts (" file> arrow  x0,y0,x1,y1           [create and] edit arrow from x0,y0 to x1,y1");
  puts (" file> cut    x,y ...               (temporarily) remove box, mini, or text at x,y"); 
  puts (" file> paste  x,y                   re-insert last cut box, mini, or text at x,y");
  puts (" file> xchange x0,y0 x1,y1          exchange elements at x0,y0 and x1,y1");
  puts (" file> delete [x,y | x0,y0:x1,y1 | x0,y0,x1,y1] ...  remove specified elements/arrows");
  puts (" file> undo                         undo most recent editing command");
  puts (" file> shift [-]n x0,y0[:x1,y1] ... move [specified] elements n units right [left]");
  puts (" file> raise [-]n x0,y0[:x1,y1] ... move [specified] elements n units up [down]");
  puts (" file> ! | write                    save to file.tex and process (with pdflatex)");
  puts (" file> quit | exit | x | ^D         turn off grid, save, process, and exit");
  puts (" file> !cmd                         execute shell command cmd, then re-load");
  puts (" file> Backup                       copy file.tex to .file.tex");
  puts (" file> Restore                      restore from .file.tex");
  puts (" file> grid   [y/n]                 turn on/off coordinate-grid background");
  puts (" file> help | ?                     print this summary");
  puts ("");
  puts ("Append \";\" to commands to suppress usual save-and-process.");
  puts("");
}


PRIVATE void
analyze_user_command (void)
{
  char command[COMMAND_LEN + 1] = {'\0'};
  sscanf (command_line, "%63s", command);

  if (prefix (command, "write"))
    {
      reprocess = true;
      regenerate_and_process ();
    }
  else if (prefix (command, "shift"))
    shift ();
  else if (prefix (command, "raise"))
    Raise ();
  else if (prefix (command, "cut"))
    analyze_cut_command ();
  else if (prefix (command, "delete"))
    analyze_delete_command ();
  else if (prefix (command, "undo"))
  {
    undo ();
    regenerate_and_process ();
  }
  else if (prefix (command, "paste"))
    analyze_paste_command ();
  else if (prefix (command, "xchange"))
    analyze_xchange_command ();
  else if (prefix (command, "box"))
    analyze_box_command ();
  else if (prefix (command, "mini"))
    analyze_mini_command ();
  else if (prefix (command, "text"))
    analyze_text_command ();
  else if (prefix (command, "arrow"))
    analyze_arrow_command ();
  else if (prefix (command, "quit")
         || prefix (command, "exit") || prefix (command, "x"))
    {
      quit ();
    }
  else if (prefix (command, "grid"))
    analyze_grid_command ();
  else if (prefix (command, "file"))
    analyze_file_command ();
  else if (command[0] == '!')
    {
      clp = command_line;
      while (isspace (*clp))
      clp++;
      clp++;                  /* ! */
      while (isspace (*clp))
      clp++;
      if (*clp == '\0')
      {
          reprocess = true;
        regenerate_and_process ();
      }
      else
      {
        execute_shell_command ();
      }
    }
  else if (prefix (command, "Backup"))
    backup ();
  else if (prefix (command, "Restore"))
    restore ();
  else if (prefix (command, "help"))
    help ();
  else if (command[0] == '?')
    help ();
  else
    {
      puts ("Command not recognized.");
      help ();
    }
}

void process_commands(void)
{
# ifdef HAVE_LIBREADLINE
  char prompt[FILE_LEN + 8] = {'\0'};
  char *prompt_n = prompt;
  rl_startup_hook = set_deftext;
  append (prompt, &prompt_n, chartfilename, sizeof (prompt));
  prompt_n = prompt_n - 4; *prompt_n = '\0'; /* suppress ".tex"  */
  append (prompt, &prompt_n, "> ", sizeof (prompt));
# else
  el_set (el, EL_PROMPT, &prompt_f);
  deftext[0] = '\0';
# endif
  help ();
  do
    {
# ifdef HAVE_LIBREADLINE
      command_line = Readline (prompt);
# else
      int command_line_n;  
      prompt[0] = '\0';
      prompt_n = prompt;
      append (prompt, &prompt_n, chartfilename, sizeof (prompt));
      prompt_n = prompt_n - 4; *prompt_n = '\0'; /* suppress ".tex"  */
      append (prompt, &prompt_n, "> ", sizeof (prompt));
      command_line = (char *) el_gets (el, &command_line_n);
# endif
      if (command_line == NULL)    /* EOF */
      { putchar ('\n'); quit(); }
# ifdef HAVE_LIBREADLINE
      if (command_line[0] != '\0')
# else
      if (command_line[0] != '\n')
# endif
      {
# ifdef HAVE_LIBREADLINE
        add_history (command_line);
# else
        history(hist, &ev, H_ENTER, command_line);
        command_line_n--;
        command_line[command_line_n] = '\0';
# endif
        reprocess = true;
        if (suffix(";", command_line))
        { /* suppress possible regeneration and processing after this command */
            reprocess = false;
# ifdef HAVE_LIBREADLINE
            command_line[strlen(command_line) - 1] = '\0';
# else
            command_line[command_line_n - 1] = '\0';
# endif
          }
        analyze_user_command ();
      }
     }
  while (true);         /* exit via call to exit or error()  */
}
