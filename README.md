# quadrillion
Solver for Quadrillion puzzle by SmartGames (https://www.smartgamesandpuzzles.com/quadrillion.html)

-----

The program reads one or more puzzle boards from a supplied file and generates all possible solutions.

Sample input:
```
    ....
    ...*
.*.*...*....
............
.......**...
............
    *...
    ....
 ```
 
 Sample output:
 ```
 reading pieces from 'pieces.txt'... done
reading boards from 'boards3.txt'... done
input boards: 1
board 1/1:
    ....    
    ...*    
.*.*...*....
............
.......**...
............
    *...    
    ....    

solving... done
total solutions: 600
time taken: 5.75739 seconds
board states tested: 9706531
orientations tested: 978024068
balls tested: 1290565848
```

----

The solver does a depth-first search (I ran out of memory with a breadth-first search), attempting to place each of the different puzzle pieces onto the board in different positions and orientations.

The first implementation I tried simply tested every piece in _every_ possible board position. However this was extremely slow and I didn't even let it run long enough to find a solution before I decided to try a different approach.

The second implementation (and the one in this repo) instead tries to fill the empty cells of the board, in order. Because of how Quadrillion is designed, filling every empty cell is the same thing as placing every piece. And so given any board state, if there is a single cell that cannot be filled with any of the remaining pieces, we know the board is not solvable. Searching by trying to fill cells is vastly more efficient than searching by trying to place pieces, as there are many many board configurations that have _almost_ all the pieces placed but still have un-fillable cells. A solver that searches by placing pieces will waste a lot of computation time on these "dead-end" configurations, while a solver filling cells will (in most cases) detect that it cannot make progress much sooner.

