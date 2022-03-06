/* sudoku.c
**
** pem 2005, 2022
**
*/

#include <stdlib.h>
#include <stdio.h>

#define ROWI 0
#define COLI 1

#define IS_COMMENT(C) ((C) == '#' || (C) == '%' || (C) == ';')
#define IS_WS(C)      ((C) <= ' ' || (C) == 127)
#define IS_EOL(C)     ((C) == '\n' || (C) == '\r')
#define IS_PDIGIT(C)   ('1' <= (C) && (C) <= '9')

/* The sudoku matrix and it's shadow (during scheduling) */
static int sudoku[9][9];
static int shadow[9][9];

/* For the schedule calculation */
static int row_count[9];
static int col_count[9];
static int sqr_count[3][3];
static int cell_count[9][9];

/* Slot schedule */
static int schedule[81][2];
static int schedidx = 0;

/* Outside-inside mappings, for digit scheduling
   We use digit as indices here, so [0] is not used. */
static int out2in_dig_map[10];
static int in2out_dig_map[10];	/* Used for counting digits as well */

/* Instead of doing modulo arithmetic when checking cells */
static int cellmap[9][9][2] =
  {
    { {0,0},{0,0},{0,0}, {0,3},{0,3},{0,3}, {0,6},{0,6},{0,6} },
    { {0,0},{0,0},{0,0}, {0,3},{0,3},{0,3}, {0,6},{0,6},{0,6} },
    { {0,0},{0,0},{0,0}, {0,3},{0,3},{0,3}, {0,6},{0,6},{0,6} },
    { {3,0},{3,0},{3,0}, {3,3},{3,3},{3,3}, {3,6},{3,6},{3,6} },
    { {3,0},{3,0},{3,0}, {3,3},{3,3},{3,3}, {3,6},{3,6},{3,6} },
    { {3,0},{3,0},{3,0}, {3,3},{3,3},{3,3}, {3,6},{3,6},{3,6} },
    { {6,0},{6,0},{6,0}, {6,3},{6,3},{6,3}, {6,6},{6,6},{6,6} },
    { {6,0},{6,0},{6,0}, {6,3},{6,3},{6,3}, {6,6},{6,6},{6,6} },
    { {6,0},{6,0},{6,0}, {6,3},{6,3},{6,3}, {6,6},{6,6},{6,6} }
  };

static void
read_matrix(void)
{
  int row = 0, col = 0;
  enum { state_ws, state_comment, state_data } state = state_ws;
  int c = getchar();

  while (c != EOF)
  {
    switch (state)
    {
    case state_ws:
      if (IS_COMMENT(c))
      {
	state = state_comment;
	c = getchar();
      }
      else if (IS_WS(c))
	c = getchar();
      else
	state = state_data;
      break;
    case state_comment:
      if (IS_EOL(c))
	state = state_ws;
      c = getchar();
      break;
    case state_data:
      if (IS_COMMENT(c))
	state = state_comment;
      else if (IS_WS(c))
	state = state_ws;
      else
      {
	if (row < 9 && col < 9)
	{
	  if (IS_PDIGIT(c))
	    sudoku[row][col] = shadow[row][col] = c - '0';
	}
	else
	{
	  fputs("sudoku: Too many rows or columns\n", stderr);
	  exit(1);
	}
	col += 1;
	if (col == 9)
	{
	  col = 0;
	  row += 1;
	}
      }
      c = getchar();
      break;
    }
  }

  if (row < 9 && col < 9)
  {
    fputs("sudoku: Unexpected end-of-file\n", stderr);
    exit(1);
  }
}

void
usage(void)
{
  fputs("Usage: sudoku [-a|-n]\n", stderr);
  exit(1);
}

int
main(int argc, char **argv)
{
  int row, col;
  int all = 0, naive = 0;

  if (argc > 1)
  {
    if (argv[1][0] != '-')
      usage();
    switch (argv[1][1])
    {
    case 'a':
      all = 1;
      break;
    case 'n':
      naive = 1;
      break;
    default:
      usage();
    }
  }

  /* Reset arrays */
  for (row = 0 ; row < 9 ; row++)
    for (col = 0 ; col < 9 ; col++)
      sudoku[row][col] = shadow[row][col] = 0;

  /* Read matrix */
  read_matrix();

  /* Zero counter arrays */
  for (row = 0 ; row < 9 ; row++)
  {
    row_count[row] = 0;
    for (col = 0 ; col < 9 ; col++)
      col_count[col] = sqr_count[row/3][col/3] = 0;
  }
  if (naive)
  {
    /* Naive mode, no "clever" scheduling of the slots */
    for (row = 0 ; row < 9 ; row++)
      for (col = 0 ; col < 9 ; col++)
	if (shadow[row][col] == 0)
	{
	  schedule[schedidx][ROWI] = row;
	  schedule[schedidx++][COLI] = col;
	  in2out_dig_map[shadow[row][col]] += 1;
	}
  }
  else
  {
    /* Count numbers in rows, columns and squares */
    for (row = 0 ; row < 9 ; row++)
      for (col = 0 ; col < 9 ; col++)
      {
	if (shadow[row][col] > 0)
	{
	  row_count[row] += 1;
	  col_count[col] += 1;
	  sqr_count[row/3][col/3] += 1;
	  in2out_dig_map[shadow[row][col]] += 1;
	}
      }

    /* Make a schedule for cells */
    while (1)
    {
      int max, max_row, max_col;

      /* Count rows, columns and squares affected by each empty cell */
      for (row = 0 ; row < 9 ; row++)
	for (col = 0 ; col < 9 ; col++)
	  if (shadow[row][col] > 0)
	    cell_count[row][col] = 0;
	  else
	    cell_count[row][col] =
	      row_count[row] + col_count[col] + sqr_count[row/3][col/3];

      /* Find the maximum cell count */
      max = max_row = max_col = 0;
      for (row = 0 ; row < 9 ; row++)
	for (col = 0 ; col < 9 ; col++)
	  if (cell_count[row][col] > max)
	  {
	    max = cell_count[row][col];
	    max_row = row;
	    max_col = col;
	  }
      if (max == 0)
	break;			/* No more, we're done with the schedule */
      /* Add cell to shedule */
      schedule[schedidx][ROWI] = max_row;
      schedule[schedidx++][COLI] = max_col;
      /* Simulate the cell used, and update the counts */
      shadow[max_row][max_col] = 10;
      row_count[max_row] += 1;
      col_count[max_col] += 1;
      sqr_count[max_row/3][max_col/3] += 1;
    }
  }
  if (schedidx == 0)
  {
    /* A special case, we have an empty grid!
       Just fill the schedule row-wise... */
    for (row = 0 ; row < 9 ; row++)
    {
      out2in_dig_map[row+1] = in2out_dig_map[row+1] = row+1; /* 1-1 */
      for (col = 0 ; col < 9 ; col++)
      {
	schedule[schedidx][ROWI] = row;
	schedule[schedidx++][COLI] = col;
      }
    }
  }
  else
  {
    /*
      Create the digit scheduling maps.
      This affects the result a great deal. Simply trying the digits
      in the order 1,2,3... etc is one of the best ways. Most other
      orderings will give a worse result. However, it turns out that
      going from the digits with the LEAST occurences to the ones with
      MORE occurences, but maintaining the numerical order when equal,
      is better for most puzzles.
    */
    int i;

    for (i = 1 ; i < 10 ; i++)
    {
      int j, mini, min;

      mini = 1;
      while ((min = in2out_dig_map[mini]) < 0)
	mini += 1;
      for (j = 2 ; j < 10 ; j++)
	if (in2out_dig_map[j] >= 0 && in2out_dig_map[j] < min)
	{
	  mini = j;
	  min = in2out_dig_map[j];
	}
      out2in_dig_map[i] = mini;
      in2out_dig_map[mini] = -1;
    }
    /* Update digit map */
    for (i = 1 ; i < 10 ; i++)
      in2out_dig_map[out2in_dig_map[i]] = i;
  }

  /* Remap the sudoku matrix */
  for (row = 0 ; row < 9 ; row++)
    for (col = 0 ; col < 9 ; col++)
    {
      int dig = sudoku[row][col];

      if (dig > 0)
	sudoku[row][col] = out2in_dig_map[dig];
    }

  printf("%u given digits\n\n", 81-schedidx);
  /* Start the real work... */
  {
    unsigned long icounter = 0;
    unsigned long scounter = 0;
    int ok;
    int i = 0;
    int val;

    while (1)
    {
      while (i < schedidx)
      {
	int r = schedule[i][ROWI];
	int c = schedule[i][COLI];

	switch (sudoku[r][c])
	{
	case 0:
	  /* New slot */
	  sudoku[r][c] = 1;
	  break;
	case 9:
	  /* Failed in this slot */
	  if (i > 0)
	  {			/* Back */
	    sudoku[r][c] = 0;
	    i -= 1;
	    continue;
	  }
	  else
	  {			/* Last slot */
	    if (all)
	      printf("%lu solution%s\n", scounter, (scounter == 1 ? "" : "s"));
	    else
	      fputs("Failed\n", stdout);
	    exit(0);
	  }
	  break;
	default:
	  /* Try next digit in this slot */
	  sudoku[r][c] += 1;
	  break;
	}
#ifdef DEBUG
	{
	  int n;

	  for (n = 0 ; n <= i ; n++)
	    putchar('#');
	  putchar('\n');
	}
#endif
	icounter += 1;
	ok = 1;
	val = sudoku[r][c];
	/* Check row */
	row = r;
	while (row--)
	  if (sudoku[row][c] == val)
	  {
	    ok = 0;
	    break;
	  }
	if (!ok)
	  continue;
	row = r;
	while (++row < 9)
	  if (sudoku[row][c] == val)
	  {
	    ok = 0;
	    break;
	  }
	if (!ok)
	  continue;
	/* Check column */
	col = c;
	while (col--)
	  if (sudoku[r][col] == val)
	  {
	    ok = 0;
	    break;
	  }
	if (!ok)
	  continue;
	col = c;
	while (++col < 9)
	  if (sudoku[r][col] == val)
	  {
	    ok = 0;
	    break;
	  }
	if (!ok)
	  continue;
	/* Check square */
	for (row = cellmap[r][c][ROWI] ;
	     row < cellmap[r][c][ROWI]+3 ;
	     row++)
	  for (col = cellmap[r][c][COLI] ;
	       col < cellmap[r][c][COLI]+3 ;
	       col++)
	    if (row != r && col != c && sudoku[row][col] == val)
	    {
	      ok = 0;
	      break;
	    }
	if (ok)
	  i += 1;
      }

      /* Print the result */
      for (row = 0 ; row < 9 ; row++)
      {
	if (row > 0 && row%3 == 0)
	  putchar('\n');
	for (col = 0 ; col < 9 ; col++)
	{
	  if (col > 0 && col%3 == 0)
	    putchar(' ');
	  putchar('0' + in2out_dig_map[sudoku[row][col]]);
	}
	putchar('\n');
      }

      printf("\n%lu tests\n\n", icounter);
      scounter += 1;

      if (all && i > 0)
	i -= 1;
      else
	break;
    }
  }

  exit(0);
}
