# quadrillion
Solver for Quadrillion puzzle by SmartGames (https://www.smartgamesandpuzzles.com/quadrillion.html)

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

## Search strategy

The solver does a depth-first search (I ran out of memory with a breadth-first search), attempting to place each of the different puzzle pieces onto the board in different positions and orientations.

The first implementation I tried simply tested every piece in _every_ possible board position. However this was extremely slow and I didn't even let it run long enough to find a solution before I decided to try a different approach.

The second implementation (and the one in this repo) instead tries to fill the empty cells of the board, in order. Because of how Quadrillion is designed, filling every empty cell is the same thing as placing every piece. And so given any board state, if there is a single cell that cannot be filled with any of the remaining pieces, we know the board is not solvable.

Searching by trying to fill cells is vastly more efficient than searching by trying to place pieces, as there are many many board configurations that have _almost_ all the pieces placed but still have un-fillable cells. A solver that searches by placing pieces will waste a lot of computation time on these "dead-end" configurations, while a solver filling cells will (in most cases) detect that it cannot make progress much sooner.

The solver in this repo just tries to fill cells left-to-right, top-to-bottom. A potential improvement (that I didn't try) would be to prioritize cells which are "harder" to fill - ie. cells with fewer unoccupied neighbor cells - as this may help us find an un-fillable cell sooner. However the order left-to-right, top-to-bottom, appears to work well enough. Perhaps because this implicitly enforces an upper bound on how many neighboring cells can be unoccupied: there cannot be more than 2 as the direct neighbors above and to the left will have already been filled, as they come first in the order.

## Optimization

Almost all the optimization comes down to: using pre-computation to reduce repeated work and remove branches from inner loops.

First, we pre-compute all the orientations of the puzzle pieces. Each piece has 4 rotations, and can also be flipped over, resulting in 8 orientations. Not all of these orientations are unique however, and the pre-computation step also strips out these duplicates.

Rather than representing the puzzle piece orientations using a 2d array (as they are inputted to the program) we store an array of the row and column indices of the "balls" that make up the piece (a kind of packed version of the 2d array). When testing whether a piece orientation can be placed on the board, this allows us to test all of the balls of the piece by just iterating through this array, avoiding the branching that would be necessary with a 2d array (where we would condition each test on whether the 2d array entry represented a ball).

````C++
struct cell_ref
{
    u8 RowIdx, ColIdx;
};

struct piece_orientation
{
    cell_ref Balls[MAX_BALLS];
};

struct search_piece
{
    piece_orientation Orientations[2 * NUM_ROTATIONS];
    s32 NumOrientations;
    s32 NumBalls;
};
````

A similar approach is used for the empty cells of the board (pre-computed and put into a single array before searching begins), and also the remaining pieces (pre-computed and put into a single array for each board state that is tested).


One source of branching that I attempted to reduce was lines 564-566 (code below). I got rid of the need for a valid index test by "padding" the board with additional rows/columns containing only invalid cells. I also removed the `break` and replaced the conditional with `CanPlace &=`. Although these changes presumably reduced branching, they actually resulted in a slowdown. I didn't investigate much further. Perhaps any savings were offset by the additional cost of the `SearchState.Board.Cells` lookup, or maybe the branch here wasn't as bad as I'd first thought (we have to branch on `CanPlace` immediately after this loop, so maybe the compiler had optimized the branching somehow)?
````C++
const bool IsValidCellIdx = (BallRowIdx >= 0) && (BallRowIdx < NumRows) && (BallColIdx >= 0) && (BallColIdx < NumCols);

if (!(IsValidCellIdx && SearchState.Board.Cells[BallRowIdx][BallColIdx] == cell_value::Empty))
{
    CanPlace = false;
    break;
````


I had a few other ideas for speeding the solver up that I didn't explore.

First: **caching**. If the set of empty cells and set of remaining pieces for any two boards is the same, these remaining pieces can be placed exactly the same way for both boards. By comparing a board state to a state whose solutions have already been found, we can use this observation to rapidly eliminate a state with no solutions, or to quickly identify all its possible solutions.

Not only can we compare states with the same input board, we can also compare states with different input boards, since this approach doesn't distinguish between cells which are occupied by a piece and those which are blocked/invalid. This would suggest that we may wish to use a single cache across all the boards we want to solve. Comparing board states may potentially be done quite efficiently using some kind of packed representation.

Second: **parallelization**. I think the problem lends itself quite well to parallelization, as we tend to do a lot more testing of pieces than we do adding/removing search states. That means we could potentially distribute the work across multiple threads without much contention on the array of search states.
