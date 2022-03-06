A sudoku solver written in C.

It uses a brute force backtracking algorithm, simply trying each digit in
each free spot, one by one, and backtracking when a collision occurs.  To
speed it up it uses some clever scheduling of the order in which it tries
the numbers and free spots, thus pruning the search tree as early as
possible.

It reads the sudoku from standard input, any non-digit character for free
spots, whitespace is ignored. Examples:
--------------
... 1.. 74.
.5. .9. .32
..6 7.. 9..

4.. 8.. ...
.2. ... .1.
... ..9 ..5

..4 ..7 3..
73. .2. .6.
.65 ..4 ...
--------------
14- -2- ---
--- --1 25-
-6- --7 -8-
-5- --9 --7
71- 8-- -2-
--- --- 8-3
--- -3- ---
--- -42 1-8
--4 --- ---
--------------

Without options it stops at the first solution. (A sudoku should normally
have only one unique solution.)
The option -a makes it continue and search for all solutions.
The option -n disables the pre-scheduling.

Apart from the solution(s) it prints the number of tests it made.
