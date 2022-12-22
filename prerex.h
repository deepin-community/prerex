/*  
  prerex.h  -- header file for an interactive editor of prerequisite-chart 
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

# include "config.h"
# include <stdlib.h>
# include <stdio.h>
# include <math.h>
# include <ctype.h>
# include <limits.h>
# include <string.h>
# ifdef HAVE_LIBREADLINE
# include <readline/readline.h>
# include <readline/history.h>
# else
# include <editline/readline.h>
# include <histedit.h>
# endif
# include <unistd.h>
# include <signal.h>
# include <unistd.h>
# include <sys/types.h>
# include <assert.h> 
# include <getopt.h>

# define bool    _Bool
# define true    1
# define false   0

# define LINE_LEN 1023
# define FILE_LEN 127
# define COMMAND_LEN 63
# define CODE_LEN 31
# define TITLE_LEN 127
# define TIMETABLE_LEN 31
# define COLOR_LEN 31

# define PRIVATE static

typedef int coord;

typedef struct point
{
  coord x, y;
} point;

typedef enum req_opt_type
{ NEITHER, REQ, OPT} req_opt_type;

typedef struct box
{
  bool half;
  req_opt_type req_opt;
  char title[TITLE_LEN + 1];
  char timetable[TIMETABLE_LEN + 1];
  char color[COLOR_LEN +1];  /* null string for default color */
} box;

typedef enum triv_type
{ TRIV } triv_type;

typedef struct mini
{
  triv_type t;
} mini;

typedef struct text
{
  char txt[LINE_LEN+1];
} text;

typedef char CourseCode[CODE_LEN + 1];
typedef enum node_type
{ BOX, MINI, TEXT } node_type;
typedef struct node
{
  point at;
  CourseCode code;
  node_type tag;
  union
  {
    box b;
    mini m;
    text t;
  } u;
} node;

typedef struct element *element_ptr;
typedef enum arrow_type
{ PREREQ, COREQ, RECOMM } arrow_type;
typedef struct arrow
{
  element_ptr source, target;
  bool DefaultCurvature;
  coord curvature;		
  arrow_type tag;
} arrow;

typedef enum element_type
{ NODE, ARROW } element_type;
typedef struct element
{
  element_ptr next;		/* to next element of linked list */
  element_type tag;
  union
  {
    node n;
    arrow a;
  } u;
} element;


typedef struct done *done_ptr;  /* declarations to support undo commands */
typedef enum done_type
{ 
  NODE_CREATE, NODE_EDIT, NODE_DELETE, NODE_SHIFT, NODE_RAISE,
  ARROW_CREATE, ARROW_EDIT, ARROW_DELETE,
  SHIFT_ALL, RAISE_ALL, CUT, PASTE,
  COMPOSITE 
} done_type;
typedef struct done
{
  done_ptr next;
  done_type tag;
  int i;
  node n;
  arrow a;
  point p;
  element_ptr e;
} done;
  

/******************************************************************************************/
/*                                                                                        */
/* defined in utils.c:                                                                    */
/*                                                                                        */
/******************************************************************************************/

extern size_t append (char *dst, char **offset, const char *src, size_t n);
/*  Copies src to *offset and updates *offset accordingly (if possible). 
 *  Assumes *offset is dst if offset == NULL.
 *  The src string must be null-terminated.
 *  Execution aborts unless **offset == '\0'. 
 *  Returns (original offset - dst) + strlen(src);  if >= n, the string was truncated.
 */

extern bool prefix (const char *cs, const char *ct); /* is string cs[] a prefix of ct[]?  */
extern bool suffix (const char *cs, const char *ct); /* is string cs[] a suffix of ct[]?  */

extern void error (char msg[]);	/* abort with stderr message msg */

extern void copy (FILE * f, FILE * g);

extern bool inrange ( coord x, coord x0, coord x1);
/* is coordinate x in the range x0:x1? */

/******************************************************************************************/
/*                                                                                        */
/* defined in prerex.c:                                                                   */
/*                                                                                        */
/******************************************************************************************/

# ifdef HAVE_LIBEDIT
extern EditLine *el;
extern History *hist;
extern HistEvent ev;
# endif

extern bool grid;       /* should the background grid be enabled? */
extern bool reprocess;  /* should the tex file be re-processed after the current command? */

extern char chartfilename[FILE_LEN];
extern char *chartfilename_n;
extern char backup_filename[FILE_LEN + 1];
extern char *backup_filename_n;

extern FILE *tex_file;
extern FILE *backup_tex_file;

extern void regenerate_and_process (void); /* internal rep. -> tex file -> pdf file  */

/******************************************************************************************/
/*                                                                                        */
/* defined in inout.c:                                                                    */
/*                                                                                        */
/******************************************************************************************/

extern element *first_node;
extern bool insert_node (element * pc);

extern element *first_arrow;
extern bool insert_arrow (element * pa);

extern bool eq (point p, point q);  /* p == q ? */

extern element *node_at (point p);

extern void analyze_tex_file (void);       /* tex file -> internal representation  */
extern void regenerate_tex_file (void);    /* internal representation -> tex file  */

extern void close_files (void);

/******************************************************************************************/
/*                                                                                        */
/* defined in edit.c:                                                                     */
/*                                                                                        */
/******************************************************************************************/

extern char deftext[LINE_LEN];		  /* buffer for default input */
# ifdef HAVE_LIBREADLINE
extern int set_deftext (void); /* used by readline to output defaults */
# else
extern char * prompt_f (void);            /* used to output prompts */
# endif
extern char * Readline ( char * str );

extern void process_commands (void);      /* command-processing loop  */
