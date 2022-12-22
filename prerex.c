/*  
  prerex.c  -- interactive editor of prerequisite-chart descriptions
  Copyright (C) 2005 - 2019  R. D. Tennent   
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

/* 

The key functions in this program are as follows:

  analyze_tex_file():  reads a TeX file and builds an internal 
    representation of the boxes and arrows for the chart in linked lists 
    accessed through global pointer variables first_box and first_arrow, 
    respectively.  Non-chart elements of the TeX file are preserved in 
    temporary files pretext, posttext, and comments. 

  regenerate_tex_file():  re-constructs a TeX file from the temporary files
    pretext, posttext and comments, and the linked lists of nodes and arrows;
    merge_lists() implements the output-ordering strategy.

  process_tex_file():  uses system calls to process the TeX file using
    pdflatex.

  analyze_user_command(): processes editing and other commands entered at 
    the interactive prompt. The libedit (editline) library is used to 
    generate prompts with default responses and allow editing of the 
    responses.

The current stack of "cuts" is accessed through pointer variable
first_cut. A stack of "done" records to support subsequent undo commands
is accessed through pointer variable first_done.

The bool variables grid and reprocess record whether a background
coordinate grid should be displayed, and whether the tex file should be
reprocessed after an editing operation.


*/

# include "prerex.h"


extern int optind;              /* defined in unistd.h  */

bool grid;			/* true if coordinate grid should be shown in background */
bool reprocess = true;          /* false if regenerate_and_process should do nothing */ 

# ifdef HAVE_LIBEDIT
EditLine *el;
History *hist;
HistEvent ev;
# endif

PRIVATE char *command_line;

PRIVATE char basefilename[FILE_LEN];
PRIVATE char *basefilename_n = basefilename;
char chartfilename[FILE_LEN];
char *chartfilename_n = chartfilename;
char backup_filename[FILE_LEN + 1];
char *backup_filename_n = backup_filename;

FILE *tex_file, *backup_tex_file;

PRIVATE void
usage (void)
{
  puts ("Usage: prerex [-v | --version | -h | --help] [basefile[.tex] [chartfile[.tex]]]");
}



PRIVATE void
sigproc (int i)  /* signal handler */
{
# ifdef HAVE_LIBEDIT
  history_end (hist);
  el_end (el);
# endif
  i++;  /* to avoid spurious compiler warning */
  grid = false;
  puts("Turning off coordinate grid.");
  regenerate_tex_file(); 
  fclose(tex_file);
  exit (0);
}


PRIVATE void
create_blank_tex_file (void)
{
  tex_file = fopen (chartfilename, "w+");
  if (tex_file == NULL)
  {
    error ("Opening new file fails.");
  }
  fprintf (tex_file, "%s\n", "\\documentclass{article}");
  fprintf (tex_file, "%s\n", "\\usepackage{geometry}");
  fprintf (tex_file, "%s\n", "\\geometry{noheadfoot, margin=0.5in}");
  fprintf (tex_file, "%s\n", "\\usepackage{prerex}");
  fprintf (tex_file, "%s\n", "\\begin{document}");
  fprintf (tex_file, "%s\n", "\\thispagestyle{empty}");
  fprintf (tex_file, "%s\n", "\\begin{chart}");
  fprintf (tex_file, "%s\n", "\\end{chart}");
  fprintf (tex_file, "%s\n", "\\end{document}");
  fflush (tex_file);
  rewind (tex_file);
}


PRIVATE void
open_tex_file (void)  /* initial opening  */
{
  tex_file = fopen (chartfilename, "r+");
  if (tex_file == NULL)
    {
      printf ("Creating new prerex chart file %s.\n", chartfilename);
      create_blank_tex_file ();
    }
  printf ("Chart file %s opened.\n", chartfilename);
}

PRIVATE void
reopen_tex_file (void)
{
  tex_file = fopen (chartfilename, "r+");
  if (tex_file == NULL)
     error ("Can't re-open the chart file.");
} 

PRIVATE void
create_backup (void)
{
  backup_filename[0] = '\0';
  backup_filename_n = backup_filename;
  append (backup_filename, &backup_filename_n, ".", sizeof (backup_filename));
  append (backup_filename, &backup_filename_n, chartfilename, sizeof (backup_filename));
  backup_tex_file = fopen (backup_filename, "w+");
  if (backup_tex_file == NULL)
    {
      puts ("Can't open backup file.");
      return;
    }
  copy (tex_file, backup_tex_file);
  fclose (backup_tex_file);
}

PRIVATE bool
process_tex_file (void)
{ /*  generates system calls to make file.pdf */
  /*  returns true if processing fails */
  char command[LINE_LEN] = {'\0'};
  char *command_n = command;
  char filename_root[FILE_LEN + 3] = {'\0'};
  char *filename_root_n = filename_root;
  append (filename_root, &filename_root_n, basefilename, sizeof (filename_root));
  filename_root[strlen(filename_root) - 4] = '\0'; /*   truncate ".tex"     */
  printf ("Processing %s.\n", basefilename);
  append (command, &command_n, "pdflatex ", sizeof (command));
  append (command, &command_n,
           "-halt-on-error -interaction batchmode ", sizeof (command));
  append (command, &command_n, filename_root, sizeof (command));
  if (fclose(tex_file) == EOF) error ("Chart file not closed.");
  if (system (command))
    { 
     printf ("Processing %s fails.\n", basefilename);
     return true;
    }
  reopen_tex_file ();
  return false;  /* success */
}


void
regenerate_and_process (void)
{
  if (!reprocess) return;
  regenerate_tex_file ();
  process_tex_file ();  /* ignore failure; allows user to recover by editing the tex file */
}

int
main (int argc, char *argv[])
{
  int ch;
  const char version[] = "6.8.0, 2019-11-15";
# define NOPTS 3
  struct option longopts[NOPTS] =
  {  { "help", 0, NULL, 'h'},
     { "version", 0, NULL, 'v'},
     { NULL, 0, NULL, 0}
  };


  printf ("This is prerex, version %s.\n", version);
  ch = getopt_long (argc, argv, "hv", longopts, NULL);
  while (ch != -1)
    {
      switch (ch)
	{
	case 'h':
	  usage ();
          puts ( "Please report bugs to rdt@cs.queensu.ca." );
	  exit (0);
	case 'v':
	  exit (0);
	case '?':
	  exit (EXIT_FAILURE);
	default:
	  printf ("Function getopt returned character code 0%o.\n",
		  (unsigned int) ch);
	  exit (EXIT_FAILURE);
	}
      ch = getopt_long (argc, argv, "hv", longopts, NULL);
    }
  puts ( "Copyright (C) 2005 - 2014  R. D. Tennent" );
  puts ( "School of Computing, Queen's University, rdt@cs.queensu.ca" );
  puts ( "License GNU GPL version 2 or later <http://gnu.org/licences/gpl.html>." );
  puts ( "There is NO WARRANTY, to the extent permitted by law." );
  puts ( "" );
# ifdef HAVE_LIBEDIT
  el = el_init (argv[0], stdin, stdout, stderr);
  hist = history_init();
  history(hist, &ev, H_SETSIZE, 800);
  el_set (el, EL_HIST, history, hist);
  el_set (el, EL_PROMPT, &prompt_f);
  el_set (el, EL_EDITOR, "emacs");
# endif

  basefilename[0] = '\0';
  basefilename_n = basefilename;
  if (optind >= argc)
    {
      puts ("");
      do
	{
      deftext[0] = '\0';
	  command_line = Readline ("Please enter a file name: ");
	  sscanf (command_line, "%127s", basefilename);
          while (*basefilename_n)  
            basefilename_n++;
	}
      while (basefilename[0] == '\0');
    }
  else
    append (basefilename, &basefilename_n, argv[optind], sizeof (basefilename));
  if (!suffix (".tex", basefilename))
    append (basefilename, &basefilename_n, ".tex", sizeof (basefilename));
  optind++;
  chartfilename[0] = '\0';
  chartfilename_n = chartfilename;
  if (optind < argc)  /* user-provided chartfilename */
    {
      append (chartfilename, &chartfilename_n, argv[optind], sizeof (chartfilename));
      if (!suffix (".tex", chartfilename))
        append (chartfilename, &chartfilename_n, ".tex", sizeof (chartfilename));
    }
  else /* use basefile as chartfile */
    append (chartfilename, &chartfilename_n, basefilename, sizeof (chartfilename));

  open_tex_file ();

  if (process_tex_file ()) /* bail if initial processing fails */
  {
    exit(EXIT_FAILURE);
  }

  analyze_tex_file ();

  /* catch signals in order to regenerate tex file, restore write access, etc. */
  signal (SIGINT, &sigproc); 
  signal (SIGILL, &sigproc);
  signal (SIGSEGV, &sigproc);
  signal (SIGTERM, &sigproc);  

  grid = true;	
  puts("Turning on coordinate grid.");
  regenerate_and_process (); 

  create_backup ();  

  process_commands();

  return 0;
}
