/*
    MIT License

    Copyright (c) 2020 George Prosser

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

/*
    Solver for Quadrillion puzzle by Smart Games.
    https://www.smartgamesandpuzzles.com/quadrillion.html
*/

#define FAST 1
#define WITH_STATS !FAST
#define WITH_ERROR_HANDLING !FAST

#if FAST
#define NDEBUG
#endif

#include <cstdio>
#include <cstdint>
#include <vector>
#include <ctime>
#include <cassert>
#include <string>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int32_t s32;
typedef float f32;
typedef double f64;

constexpr s32 MAX_BOARD_SIZE = 16; // max width/height of a puzzle board
constexpr s32 NUM_VALID_CELLS = 64; // number of valid cells on a puzzle board (4 * 16)
constexpr s32 NUM_PIECES = 12; // number of puzzle pieces
constexpr s32 MAX_PIECE_SIZE = 4; // max horizontal/vertical size of piece
constexpr s32 MAX_BALLS = 5; // max number of 'balls' that make up a piece
constexpr s32 NUM_ROTATIONS = 4; // 4 x 90 degree rotations

enum cell_value : u8
{
    Empty = 0u,
    Invalid,
    Blocked,
    Piece01,
    Piece02,
    Piece03,
    Piece04,
    Piece05,
    Piece06,
    Piece07,
    Piece08,
    Piece09,
    Piece10,
    Piece11,
    Piece12
};

bool IsPiece(const cell_value CellValue)
{
    return CellValue >= Piece01 && CellValue <= Piece12;
}

s32 CellValueToPieceIndex(const cell_value CellValue)
{
    return (s32) (CellValue & 0b00001111);
}

cell_value PieceIndexToCellValue(const s32 PieceIdx)
{
    return (cell_value) (cell_value::Piece01 + PieceIdx);
}

char CellValueToOutputChar(const cell_value CellValue)
{
    switch (CellValue)
    {
        case cell_value::Invalid:
            return ' ';
        case cell_value::Empty: 
            return '.';
        case cell_value::Blocked:
            return '*';
        case cell_value::Piece01:
        case cell_value::Piece02:
        case cell_value::Piece03:
        case cell_value::Piece04:
        case cell_value::Piece05:
        case cell_value::Piece06:
        case cell_value::Piece07:
        case cell_value::Piece08:
        case cell_value::Piece09:
        case cell_value::Piece10:
        case cell_value::Piece11:
        case cell_value::Piece12:
            return 'A' + CellValueToPieceIndex(CellValue);
        default:
            assert(false);
            return '?';
    }
}

cell_value InputCharToCellValue(const char InputChar)
{
    switch (InputChar)
    {
        case ' ':
            return cell_value::Invalid;
        case '.':
            return cell_value::Empty;
        case '*':
            return cell_value::Blocked;
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        case 'G':
        case 'H':
        case 'I':
        case 'J':
        case 'K':
        case 'L':
            return PieceIndexToCellValue(InputChar - 'A');
        default:
            assert(false);
            return cell_value::Invalid;
    }
}

struct piece_definition
{
    u8 Balls[MAX_PIECE_SIZE][MAX_PIECE_SIZE];
};

struct board
{
    cell_value Cells[MAX_BOARD_SIZE][MAX_BOARD_SIZE];
};

void SetCells(board& Board, const cell_value CellValue)
{
    for (s32 RowIdx = 0; RowIdx < MAX_BOARD_SIZE; RowIdx++)
    {
        for (s32 ColIdx = 0; ColIdx < MAX_BOARD_SIZE; ColIdx++)
        {
            Board.Cells[RowIdx][ColIdx] = CellValue;
        }
    }
}

void PrintBoard(const board& Board, const s32 NumRows = MAX_BOARD_SIZE, const s32 NumCols = MAX_BOARD_SIZE)
{
    for (s32 RowIdx = 0; RowIdx < NumRows; RowIdx++)
    {
        for (s32 ColIdx = 0; ColIdx < NumCols; ColIdx++)
        {
            printf("%c", CellValueToOutputChar(Board.Cells[RowIdx][ColIdx]));
        }
        printf("\n");
    }
}

void Rotate(const piece_definition& From, piece_definition& To)
{
    // rotate clockwise by 90 degrees
    for (s32 fromRowIdx = 0; fromRowIdx < MAX_PIECE_SIZE; fromRowIdx++)
    {
        for (s32 fromColIdx = 0; fromColIdx < MAX_PIECE_SIZE; fromColIdx++)
        {
            To.Balls[fromColIdx][(MAX_PIECE_SIZE - 1) - fromRowIdx] = From.Balls[fromRowIdx][fromColIdx];
        }
    }
}

void Flip(const piece_definition& From, piece_definition& To)
{
    // flip vertically
    for (s32 fromRowIdx = 0; fromRowIdx < MAX_PIECE_SIZE; fromRowIdx++)
    {
        for (s32 fromColIdx = 0; fromColIdx < MAX_PIECE_SIZE; fromColIdx++)
        {
            To.Balls[(MAX_PIECE_SIZE - 1) - fromRowIdx][fromColIdx] = From.Balls[fromRowIdx][fromColIdx];
        }
    }
}

void PushUpAndLeft(piece_definition& Definition)
{
    // find the first row and column with a set bit
    s32 MinRowIdx = MAX_PIECE_SIZE - 1;
    s32 MinColIdx = MAX_PIECE_SIZE - 1;
    for (s32 RowIdx = 0; RowIdx < MAX_PIECE_SIZE; RowIdx++)
    {
        for (s32 ColIdx = 0; ColIdx < MAX_PIECE_SIZE; ColIdx++)
        {
            if (Definition.Balls[RowIdx][ColIdx])
            {
                if (RowIdx <= MinRowIdx)
                {
                    MinRowIdx = RowIdx;
                }

                if (ColIdx <= MinColIdx)
                {
                    MinColIdx = ColIdx;
                }
            }
        }
    }

    // shift set bits up and to the left as much as possible
    for (s32 RowIdx = MinRowIdx; RowIdx < MAX_PIECE_SIZE; RowIdx++)
    {
        for (s32 ColIdx = MinColIdx; ColIdx < MAX_PIECE_SIZE; ColIdx++)
        {
            Definition.Balls[RowIdx - MinRowIdx][ColIdx - MinColIdx] = Definition.Balls[RowIdx][ColIdx];
        }
    }

    // clear remaining bits
    for (s32 RowIdx = MAX_PIECE_SIZE - MinRowIdx; RowIdx < MAX_PIECE_SIZE; RowIdx++)
    {
        for (s32 ColIdx = 0; ColIdx < MAX_PIECE_SIZE; ColIdx++)
        {
            Definition.Balls[RowIdx][ColIdx] = 0;
        }
    }

    for (s32 ColIdx = MAX_PIECE_SIZE - MinColIdx; ColIdx < MAX_PIECE_SIZE; ColIdx++)
    {
        for (s32 RowIdx = 0; RowIdx < MAX_PIECE_SIZE; RowIdx++)
        {
            Definition.Balls[RowIdx][ColIdx] = 0;
        }
    }
}

u32 ComputePackedRepresentation(piece_definition& Definition)
{
    u32 Packed = 0;
    for (s32 RowIdx = 0; RowIdx < MAX_PIECE_SIZE; RowIdx++)
    {
        for (s32 ColIdx = 0; ColIdx < MAX_PIECE_SIZE; ColIdx++)
        {
            Packed <<= 1;
            Packed |= Definition.Balls[RowIdx][ColIdx];
        }
    }
    return Packed;
}

struct solver
{
private:
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

    search_piece SearchPieces[NUM_PIECES];

public:
    void Initialize(const piece_definition (&Pieces)[NUM_PIECES]);

    void Solve(
        const board& InputBoard,
        const s32 NumRows,
        const s32 NumCols,
        std::vector<board>& OutSolutions,
        u64& OutNumBoardStatesTested,
        u64& OutNumOrientationsTested,
        u64& OutNumBallsTested) const;
};

void solver::Initialize(const piece_definition (&Pieces)[NUM_PIECES])
{
    // expand each piece out into its orientations
    struct expanded_representation
    {
        piece_definition Definition;
        u32 Packed;
    };

    expanded_representation PieceOrientations[NUM_PIECES][2 * NUM_ROTATIONS];

    for (s32 PieceIdx = 0; PieceIdx < NUM_PIECES; PieceIdx++)
    {
        // orientation 0 is unchanged
        PieceOrientations[PieceIdx][0].Definition = Pieces[PieceIdx];

        // repeatedly rotate by 90 to get orientations 1, 2, 3
        for (s32 OrientationIdx = 0; OrientationIdx < NUM_ROTATIONS - 1; OrientationIdx++)
        {
            Rotate(PieceOrientations[PieceIdx][OrientationIdx].Definition, PieceOrientations[PieceIdx][OrientationIdx + 1].Definition);
        }

        // orientation 4 is orientation 0, flipped vertically
        Flip(PieceOrientations[PieceIdx][0].Definition, PieceOrientations[PieceIdx][NUM_ROTATIONS].Definition);

        // repeatedly rotate by 90 to get orientations 5, 6, 7
        for (s32 OrientationIdx = 0; OrientationIdx < NUM_ROTATIONS - 1; OrientationIdx++)
        {
            Rotate(PieceOrientations[PieceIdx][NUM_ROTATIONS + OrientationIdx].Definition, PieceOrientations[PieceIdx][NUM_ROTATIONS + OrientationIdx + 1].Definition);
        }
    }

    // move the set bits up and to the left as far as possible
    for (s32 PieceIdx = 0; PieceIdx < NUM_PIECES; PieceIdx++)
    {
        for (s32 isoIdx = 0; isoIdx < 2 * NUM_ROTATIONS; isoIdx++)
        {
            PushUpAndLeft(PieceOrientations[PieceIdx][isoIdx].Definition);
        }
    }

    // store a packed representation of each orientation to aid de-duplication
    for (s32 PieceIdx = 0; PieceIdx < NUM_PIECES; PieceIdx++)
    {
        for (s32 OrientationIdx = 0; OrientationIdx < 2 * NUM_ROTATIONS; OrientationIdx++)
        {
            PieceOrientations[PieceIdx][OrientationIdx].Packed = ComputePackedRepresentation(PieceOrientations[PieceIdx][OrientationIdx].Definition);
        }
    }


    // convert from list of all orientations into a list of unique orientations
    struct intermediate_representation
    {
        expanded_representation Orientations[2 * NUM_ROTATIONS];
        u8 NumOrientations;
    };

    intermediate_representation IntermediatePieces[NUM_PIECES];

    for (s32 PieceIdx = 0; PieceIdx < NUM_PIECES; PieceIdx++)
    {
        intermediate_representation& IntermediatePiece = IntermediatePieces[PieceIdx];
        IntermediatePiece.NumOrientations = 0;
        for (s32 OrientationIdx = 0; OrientationIdx < 2 * NUM_ROTATIONS; OrientationIdx++)
        {
            const expanded_representation& Orientation = PieceOrientations[PieceIdx][OrientationIdx];

            bool IsOrientationUnique = true;
            for (s32 OtherOrientationIdx = 0; OtherOrientationIdx < IntermediatePiece.NumOrientations; OtherOrientationIdx++)
            {
                const expanded_representation& OtherOrientation = IntermediatePiece.Orientations[OtherOrientationIdx];
                if (Orientation.Packed == OtherOrientation.Packed)
                {
                    IsOrientationUnique = false;
                    break;
                }
            }

            if (IsOrientationUnique)
            {
                IntermediatePiece.Orientations[IntermediatePiece.NumOrientations++] = Orientation;
            }
        }
    }


    // convert from intermediate representation to the representation used for searching
    for (s32 PieceIdx = 0; PieceIdx < NUM_PIECES; PieceIdx++)
    {
        intermediate_representation& IntermediatePiece = IntermediatePieces[PieceIdx];
        search_piece& SearchPiece = SearchPieces[PieceIdx];
        SearchPiece.NumOrientations = IntermediatePiece.NumOrientations;

        for (s32 OrientationIdx = 0; OrientationIdx < IntermediatePiece.NumOrientations; OrientationIdx++)
        {
            SearchPiece.NumBalls = 0;
            piece_orientation& SearchOrientation = SearchPiece.Orientations[OrientationIdx];
            expanded_representation& IntermediateOrientation = IntermediatePiece.Orientations[OrientationIdx];
            for (s32 RowIdx = 0; RowIdx < MAX_PIECE_SIZE; RowIdx++)
            {
                for (s32 ColIdx = 0; ColIdx < MAX_PIECE_SIZE; ColIdx++)
                {
                    if (IntermediateOrientation.Definition.Balls[RowIdx][ColIdx])
                    {
                        // store the row and column of each ball that makes up this piece
                        cell_ref& CellRef = SearchOrientation.Balls[SearchPiece.NumBalls++];
                        CellRef.RowIdx = (u8)RowIdx;
                        CellRef.ColIdx = (u8)ColIdx;
                    }
                }
            }
        }
    }
}


void solver::Solve(
    const board& InputBoard,
    const s32 NumRows,
    const s32 NumCols,
    std::vector<board>& OutSolutions,
    u64& OutNumBoardStatesTested,
    u64& OutNumOrientationsTested,
    u64& OutNumBallsTested) const
{
    // pre-compute the empty cells
    cell_ref InputBoardEmptyCells[NUM_VALID_CELLS];
    s32 InputBoardNumEmptyCells = 0;
    {
        for (s32 RowIdx = 0; RowIdx < MAX_BOARD_SIZE; RowIdx++)
        {
            for (s32 ColIdx = 0; ColIdx < MAX_BOARD_SIZE; ColIdx++)
            {
                if (InputBoard.Cells[RowIdx][ColIdx] == cell_value::Empty)
                {
                    cell_ref& CellRef = InputBoardEmptyCells[InputBoardNumEmptyCells++];
                    CellRef.RowIdx = RowIdx;
                    CellRef.ColIdx = ColIdx;
                }
            }
        }
    }

    struct search_state
    {
        board Board;
        u16 RemainingPieceBitFlags : NUM_PIECES;
        u8 EmptyCellIdx;
    };

    // construct the initial search state
    search_state InitialSearchState;
    {
        InitialSearchState.Board = InputBoard;
        InitialSearchState.EmptyCellIdx = 0;

        InitialSearchState.RemainingPieceBitFlags = 0u;
        for (s32 PieceIdx = 0; PieceIdx < NUM_PIECES; PieceIdx++)
        {
            InitialSearchState.RemainingPieceBitFlags |= (1u << PieceIdx);
        }

        for (s32 RowIdx = 0; RowIdx < NumRows; RowIdx++)
        {
            for (s32 ColIdx = 0; ColIdx < NumCols; ColIdx++)
            {
                const cell_value CellValue = InputBoard.Cells[RowIdx][ColIdx];
                if (IsPiece(CellValue))
                {
                    const s32 PieceIdx = CellValueToPieceIndex(CellValue);
                    InitialSearchState.RemainingPieceBitFlags &= ~(1u << PieceIdx);
                }
            }
        }
    }

#if WITH_STATS
    OutNumOrientationsTested = 0;
    OutNumBoardStatesTested = 0;
    OutNumBallsTested = 0;
#endif

    OutSolutions.clear();
    OutSolutions.reserve(1024);

    std::vector<search_state> SearchStates;
    SearchStates.reserve(1024);
    SearchStates.push_back(InitialSearchState);

    while (!SearchStates.empty())
    {
#if WITH_STATS
        OutNumBoardStatesTested++;
#endif
        search_state SearchState = SearchStates.back();
        SearchStates.pop_back();

        // find the next empty cell on the board
        s32 EmptyCellIdx;
        s32 RowIdx, ColIdx;

        for (EmptyCellIdx = SearchState.EmptyCellIdx; EmptyCellIdx < InputBoardNumEmptyCells; EmptyCellIdx++)
        {
            cell_ref Cell = InputBoardEmptyCells[EmptyCellIdx];
            if (SearchState.Board.Cells[Cell.RowIdx][Cell.ColIdx] == cell_value::Empty)
            {
                RowIdx = (s32)Cell.RowIdx;
                ColIdx = (s32)Cell.ColIdx;
                break;
            }
        }

        assert(EmptyCellIdx < InputBoardNumEmptyCells);

        // determine which pieces are available
        s32 RemaningPieceIdxs[NUM_PIECES];
        s32 NumRemainingPieces = 0;
        for (s32 PieceIdx = 0; PieceIdx < NUM_PIECES; PieceIdx++)
        {
            if ((SearchState.RemainingPieceBitFlags >> PieceIdx) & 1u)
            {
                RemaningPieceIdxs[NumRemainingPieces++] = PieceIdx;
            }
        }

        const bool IsLastPiece = (NumRemainingPieces == 1);

        // try to fill the empty cell with every available piece...
        for (s32 RemainingIdx = 0; RemainingIdx < NumRemainingPieces; RemainingIdx++)
        {
            const s32 PieceIdx = RemaningPieceIdxs[RemainingIdx];
            const search_piece& Piece = SearchPieces[PieceIdx];

            // ... in every orientation ...
            for (s32 OrientationIdx = 0; OrientationIdx < Piece.NumOrientations; OrientationIdx++)
            {
                const piece_orientation& Orientation = Piece.Orientations[OrientationIdx];

                // ... with every ball of that piece
                for (s32 PlacedBallIdx = 0; PlacedBallIdx < Piece.NumBalls; PlacedBallIdx++)
                {
                    const s32 OffsetRowIdx = -Orientation.Balls[PlacedBallIdx].RowIdx;
                    const s32 OffsetColIdx = -Orientation.Balls[PlacedBallIdx].ColIdx;
#if WITH_STATS
                    OutNumOrientationsTested++;
#endif
                    bool CanPlace = true;
                    for (s32 TestBallIdx = 0; TestBallIdx < Piece.NumBalls; TestBallIdx++)
                    {
#if WITH_STATS
                        OutNumBallsTested++;
#endif
                        const s32 BallRowIdx = RowIdx + OffsetRowIdx + Orientation.Balls[TestBallIdx].RowIdx;
                        const s32 BallColIdx = ColIdx + OffsetColIdx + Orientation.Balls[TestBallIdx].ColIdx;
                        const bool IsValidCellIdx = (BallRowIdx >= 0) && (BallRowIdx < NumRows) && (BallColIdx >= 0) && (BallColIdx < NumCols);

                        if (!(IsValidCellIdx && SearchState.Board.Cells[BallRowIdx][BallColIdx] == cell_value::Empty))
                        {
                            CanPlace = false;
                            break;
                        }
                    }

                    if (CanPlace)
                    {
                        search_state NewSearchState = SearchState;

                        const cell_value NewCellValue = PieceIndexToCellValue(PieceIdx);
                        for (s32 BallIdx = 0; BallIdx < Piece.NumBalls; BallIdx++)
                        {
                            const s32 BallRowIdx = RowIdx + OffsetRowIdx + Orientation.Balls[BallIdx].RowIdx;
                            const s32 BallColIdx = ColIdx + OffsetColIdx + Orientation.Balls[BallIdx].ColIdx;
                            NewSearchState.Board.Cells[BallRowIdx][BallColIdx] = NewCellValue;
                        }
                        NewSearchState.RemainingPieceBitFlags &= ~(1u << PieceIdx);
                        NewSearchState.EmptyCellIdx++;

                        if (!IsLastPiece)
                        {
                            SearchStates.push_back(NewSearchState);
                        }
                        else
                        {
                            OutSolutions.push_back(NewSearchState.Board);
                        }
                    }
                }
            }
        }
    }
}

int main(int argc, char* argv[])
{
    std::string PieceInputFilename = "pieces.txt";
    std::string BoardInputFilename = "boards.txt";

    if (argc >= 2)
    {
        // override board input filename by command line
        BoardInputFilename = argv[1];
    }

    // read piece definitions from input
    piece_definition PieceDefinitions[NUM_PIECES];
    {
        printf("reading pieces from '%s'... ", PieceInputFilename.c_str());
        fflush(stdout);
        FILE* PieceInputFilePtr = fopen(PieceInputFilename.c_str(), "rb");
        assert(PieceInputFilePtr != NULL);

        for (s32 PieceIdx = 0; PieceIdx < NUM_PIECES; PieceIdx++)
        {
            for (s32 bitRowIdx = 0; bitRowIdx < MAX_PIECE_SIZE; bitRowIdx++)
            {
                for (s32 bitColIdx = 0; bitColIdx < MAX_PIECE_SIZE; bitColIdx++)
                {
                    s32 InputBit;
                    const s32 Result = fscanf(PieceInputFilePtr, "%d", &InputBit);
                    assert(Result == 1);
                    PieceDefinitions[PieceIdx].Balls[bitRowIdx][bitColIdx] = (u8)InputBit;
                }
            }
        }

        fclose(PieceInputFilePtr);
        printf("done\n");
    }

    // read in the initial board state
    // note: there is no validation on input board, puzzle pieces etc.
    std::vector<board> InputBoards;
    InputBoards.reserve(128);
    {
        printf("reading boards from '%s'... ", BoardInputFilename.c_str());
        fflush(stdout);
        FILE* BoardInputFilePtr = fopen(BoardInputFilename.c_str(), "rb");
        assert(BoardInputFilePtr != NULL);

        bool EndOfFileReached = false;
        while (!EndOfFileReached)
        {
            board InputBoard;
            SetCells(InputBoard, cell_value::Invalid);

            s32 RowIdx = 0;
            s32 ColIdx = 0;
            s32 NumValidCells = 0;

            while (1)
            {
                char InputChar;
                const s32 Result = fscanf(BoardInputFilePtr, "%c", &InputChar);
                if (!feof(BoardInputFilePtr))
                {
                    assert(Result == 1);
                }
                else
                {
                    // if this is the last line of the last board, don't require a newline
                    assert(NumValidCells == NUM_VALID_CELLS);
                    EndOfFileReached = true;
                    break;
                }

                if (InputChar == '\n')
                {
                    ColIdx = 0;
                    RowIdx++;
                    assert(RowIdx <= MAX_BOARD_SIZE);
                    if (NumValidCells >= NUM_VALID_CELLS)
                    {
                        break;
                    }
                }
                else
                {
                    assert(ColIdx < MAX_BOARD_SIZE);
                    const cell_value CellValue = InputCharToCellValue(InputChar);
                    InputBoard.Cells[RowIdx][ColIdx] = CellValue;
                    ColIdx++;

                    if (CellValue != cell_value::Invalid)
                    {
                        NumValidCells++;
                        assert(NumValidCells <= NUM_VALID_CELLS);
                    }
                }
            }

            InputBoards.push_back(InputBoard);

            if (!EndOfFileReached)
            {
                // boards deliminated with a newline
                char NewlineChar;
                const s32 Result = fscanf(BoardInputFilePtr, "%c", &NewlineChar);
                if (feof(BoardInputFilePtr))
                {
                    // allow trailing newline after final board
                    EndOfFileReached = true;
                }
                else
                {
                    assert(Result == 1);
                    assert(NewlineChar == '\n');
                }
            }
        }

        fclose(BoardInputFilePtr);
        printf("done\n");
    }

    struct stat_data
    {
        u64 NumOrientationsTested = 0;
        u64 NumBoardStatesTested = 0;
        u64 NumBallsTested = 0;
        f64 ElapsedTimeSec = 0.f;
    };

    std::vector<stat_data> StatDataArray(InputBoards.size());

    solver Solver;
    Solver.Initialize(PieceDefinitions);

    printf("input boards: %lu\n", InputBoards.size());

    for (s32 InputBoardIdx = 0; InputBoardIdx < InputBoards.size(); InputBoardIdx++)
    {
        const board& InputBoard = InputBoards[InputBoardIdx];

        // pre-compute the board size
        s32 NumRows = 0;
        s32 NumCols = 0;
        {
            for (s32 RowIdx = 0; RowIdx < MAX_BOARD_SIZE; RowIdx++)
            {
                for (s32 ColIdx = 0; ColIdx < MAX_BOARD_SIZE; ColIdx++)
                {
                    if (InputBoard.Cells[RowIdx][ColIdx] != cell_value::Invalid)
                    {
                        if (RowIdx + 1 > NumRows)
                        {
                            NumRows = RowIdx + 1;
                        }
                        if (ColIdx + 1 > NumCols)
                        {
                            NumCols = ColIdx + 1;
                        }
                    }
                }
            }
        }

        printf("board %d/%lu:\n", InputBoardIdx+1, InputBoards.size());
        PrintBoard(InputBoard, NumRows, NumCols);
        printf("\n");

        stat_data& StatData = StatDataArray[InputBoardIdx];
        std::vector<board> Solutions;

        printf("solving... ");
        fflush(stdout);
        const std::clock_t ClockStart = std::clock();

        Solver.Solve(
            InputBoard,
            NumRows,
            NumCols,
            Solutions,
            StatData.NumBoardStatesTested,
            StatData.NumOrientationsTested,
            StatData.NumBallsTested);

        const std::clock_t ClockEnd = std::clock();
        printf("done\n");

        StatData.ElapsedTimeSec = (ClockEnd - ClockStart) / (f32)CLOCKS_PER_SEC;

        constexpr bool PRINT_SOLUTIONS = false;
        if (PRINT_SOLUTIONS)
        {
            printf("solutions:\n");
            const s32 SolutionCount = Solutions.size();
            for (s32 SolIdx = 0; SolIdx < SolutionCount; SolIdx++)
            {
                //printf("solution %d/%d:\n", SolIdx+1, solutionCount);
                PrintBoard(Solutions[SolIdx], NumRows, NumCols);
                printf("\n");
            }
        }

        printf("total solutions: %lu\n", Solutions.size());
        printf("time taken: %.5f seconds\n", StatData.ElapsedTimeSec);
#if WITH_STATS
        printf("board states tested: %llu\n", StatData.NumBoardStatesTested);
        printf("orientations tested: %llu\n", StatData.NumOrientationsTested);
        printf("balls tested: %llu\n", StatData.NumBallsTested);
#endif
        printf("\n\n");
    }

#if WITH_STATS
    u64 TotalNumBoardStatesTested = 0;
    f64 TotalElapsedTimeSec = 0.f;
    for (const stat_data& StatData : StatDataArray)
    {
        TotalNumBoardStatesTested += StatData.NumBoardStatesTested;
        TotalElapsedTimeSec += StatData.ElapsedTimeSec;
    }

    constexpr f64 NANOSECONDS_PER_SECOND = 1000000000.f;
    const f64 TimePerBoardStateNS = (NANOSECONDS_PER_SECOND * TotalElapsedTimeSec) / (f64)TotalNumBoardStatesTested;
    printf("total board states tested: %llu\n", TotalNumBoardStatesTested);
    printf("total time taken: %.5f seconds\n", TotalElapsedTimeSec);
    printf("average time per board state: %.5f ns\n", TimePerBoardStateNS);
#endif

    return 0;
}