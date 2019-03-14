/**
 * Peg solitaire
 *
 * This
 */

#include <chrono>
#include <iomanip>
#include <iostream>
#include <vector>
#include <locale>
#include <math.h>
#include <climits>
#include <sylvan_obj.hpp>


using namespace std;
using namespace std::chrono;
using namespace sylvan;


enum Square {
    NN = 0, // No square
    OO = 1, // Peg on square (on initial board)
    EE = 2, // Empty square  (on initial board)
};

#define N 7
Square BOARD[N][N] = {
    {NN,NN,OO,OO,OO,NN,NN,},
    {NN,NN,OO,OO,OO,NN,NN,},
    {OO,OO,OO,OO,OO,OO,OO,},
    {OO,OO,OO,EE,OO,OO,OO,},
    {OO,OO,OO,OO,OO,OO,OO,},
    {NN,NN,OO,OO,OO,NN,NN,},
    {NN,NN,OO,OO,OO,NN,NN,},
};


/**
 * Declare callback function for printing boards from BDD package
 */
VOID_TASK_DECL_4(printboard, void*, BDDVAR*, uint8_t*, int);
#define printboard TASK(printboard)


struct PartialRelation {
    Bdd MoveRelation;
    BddSet RelationVars; // unprimed, primed
};

class PegSolitaire {

public:
    // BDD variables:
    BddSet SourceVars;   // unprimed
    BddSet TargetVars;   // primed
    BddSet RelationVars; // unprimed, primed

    // Mapping from NxN square coordinates to resp. (unprimed) BDD variable:
    BDDVAR Coor2Var[N][N];

    // BDD variable to board coordination mapping and vice versa:
    vector<BddSet> Triples;

    // BDDs representing sets of boards:
    Bdd InitialBoard;
    Bdd WinningBoards;

    // BDD representing the move relation:
    Bdd MoveRelation;
    // BDDs representing partial (local) move relations (over a triple of squares)
    vector<PartialRelation>  Relations;

    /* Number existing squares and init the variable sets */
    PegSolitaire() {
        BDDVAR var = 0;
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                if (BOARD[i][j] == NN) // no square
                    continue;

                BDDVAR source = var++;   // unprimed (source square in relation)
                BDDVAR target = var++;   // primed   (target square in relation)

                Coor2Var[i][j] = source; // only remember source (target is +1)

                SourceVars.add(source);  // even
                TargetVars.add(target);  // odd
            }
        }

        // All relational variables:
        RelationVars.add(SourceVars);
        RelationVars.add(TargetVars);

        // Create triples of consecutive squares:
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                if (BOARD[i][j] == NN) // no square
                    continue;
                // Create triple of row consecutive squares:
                if (j+2 < N && BOARD[i][j+1] != NN && BOARD[i][j+2] != NN) {
                    BddSet Triple;
                    Triple.add(Coor2Var[i][j  ]);
                    Triple.add(Coor2Var[i][j+1]);
                    Triple.add(Coor2Var[i][j+2]);
                    Triples.push_back(Triple);
                }

                // Create triple of column consecutive squares:
                if (i+2 < N && BOARD[i+1][j] != NN && BOARD[i+2][j] != NN) {
                    BddSet Triple;
                    Triple.add(Coor2Var[i  ][j]);
                    Triple.add(Coor2Var[i+1][j]);
                    Triple.add(Coor2Var[i+2][j]);
                    Triples.push_back(Triple);
                }
            }
        }
    }

    void CreateInitialBoard() {
        // Winning voards have only one peg left
        InitialBoard = Bdd::bddOne();  // true BDD

        // iterate over all existing squares
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                if (BOARD[i][j] == NN)
                    continue;
                BDDVAR square = Coor2Var[i][j];
                if (BOARD[i][j] == EE) { // empty
                    InitialBoard = InitialBoard & !Bdd(square);
                } else {
                    InitialBoard = InitialBoard &  Bdd(square);
                }
            }
        }
    }

    void CreateWinningBoards() {
        // Winning voards have only one peg left
        WinningBoards = Bdd::bddZero(); // false BDD (empty set)

        // iterate over all existing squares
        for (BDDVAR square : SourceVars) {
            Bdd Board = Bdd::bddOne();  // true BDD

            // Put a peg on the square and no peg on all other squares:
            // TODO

            // Add the board to the Winning set:
            // WinningBoards = WinningBoards | Board;
        }
    }

    void CreateMoveRelation() {
        // Create two moves per triple of consecutive squares:
        for (BddSet &Triple : Triples) {

            vector<BDDVAR> V = Triple.toVector();

            // Add variables to partial relation
            PartialRelation Partial;
            for (BDDVAR var : V) {
                Partial.RelationVars.add(var);      // unprimed
                Partial.RelationVars.add(var + 1);  // primed
            }

            // Create BDD with all moves over these three squares
            Partial.MoveRelation = Bdd::bddZero();
            // TODO

            Relations.push_back(Partial);
        }

        // Create total move relation from partial relations
        size_t total_nodes = 0;
        MoveRelation = Bdd::bddZero(); // false BDD (empty set)
        for (PartialRelation &Partial : Relations) {
            Bdd Rel = Partial.MoveRelation; // copy

            // extend relation full variable domain setting x <==> x' for new variables:
            for (BDDVAR var : SourceVars) {
                if (Partial.RelationVars.contains(var)) // already in partial
                    continue;
                Rel &= Bdd(var).Xnor(Bdd(var + 1));
            }

            MoveRelation |= Rel;
            total_nodes += Partial.MoveRelation.NodeCount();
        }
        cout << endl << "Partial relations have a total of  "<< total_nodes <<" BDD nodes" << endl;
    }

    Bdd FixPointBFS(Bdd &Start) {
        StartTimer();

        int level = 0;
        Bdd Next = Start;       // BFS Queue
        Bdd Visited = Start;
        while (Next != Bdd::bddZero()) {

            // TODO

            PrintLevel (Visited, level++);
            // PrintBoards(Next, "Next");
            // if (level == 2) break;
        }
        return Visited;
    }

    Bdd FixPointBFS2(Bdd &Start) {
        StartTimer();

        int level = 0;
        Bdd Old = Bdd::bddZero();
        Bdd Visited = Start;
        while (Old != Visited) {

            // TODO

            PrintLevel (Visited, level++);
            // PrintBoards(Next, "Next");
            // if (level == 2) break;
        }
        return Visited;
    }

    Bdd Sweepline(Bdd &Start) {
        StartTimer();

        int level = 0;
        Bdd Old = Bdd::bddZero();
        Bdd Visited = Start;
        while (Old != Visited) {
            Old = Visited;

            // TODO

            PrintLevel (Visited, level++);
            // PrintBoards(Next, "Next");
            // if (level == 2) break;
        }
        return Visited;
    }

    Bdd FixPointBFS_Partial(Bdd &Start) {
        StartTimer();

        int level = 0;
        Bdd Next = Start;
        Bdd Visited = Bdd::bddZero();
        while (Next != Bdd::bddZero()) {

            for (PartialRelation &Rel : Relations) {
                //TODO
            }
            // TODO

            PrintLevel (Visited, level++);
            // PrintBoards(Next, "Next");
            // if (level == 2) break;
        }
        return Visited;
    }

    Bdd FixPointChaining(Bdd &Start) {
        StartTimer();

        int level = 0;
        Bdd Old = Bdd::bddZero();
        Bdd Visited = Start;
        while (Visited != Old) {
            // TODO
        }
        return Visited;
    }

    Bdd FixPointSaturationLike(Bdd &Start) {
        StartTimer();

//        int level = 0;
        Bdd Old = Bdd::bddZero();
        Bdd Visited = Start;
        //for (PartialRelation &Rel : Relations) {
        // TODO: implement saturation-like search order: repeat partial relation until closure
        //    PrintLevel (Visited, level++);
        //}
        return Visited;
    }

    Bdd FixPointSaturation(Bdd &Start) {
        StartTimer();

//        int level = 0;
        Bdd Old = Bdd::bddZero();
        Bdd Visited = Start;
        // TODO: sort relations based on ordering and implement saturation
        return Visited;
    }

    void PrintBoards(Bdd &B, string name) {
        LACE_ME;
        cout << endl <<  endl << "Printing boards: "<< name << endl;
        sylvan_enum(B.GetBDD(), SourceVars.GetBDD(), printboard, this);
    }

    system_clock::duration start;
    void StartTimer() {
        start = system_clock::now().time_since_epoch();
    }

    void PrintTimer() {
        system_clock::duration now = system_clock::now().time_since_epoch();
        double ms = duration_cast< milliseconds >(now - start).count();
        cout << "["<< fixed  << setprecision(2) << setfill(' ') << setw(8) << ms / 1000 <<"]  ";
    }

    size_t MaxNodes = 0;
private:
    void PrintLevel (const Bdd& Visited, double level) {
        PrintTimer();
        size_t NodeCount = Visited.NodeCount();
        if (MaxNodes < NodeCount)
            MaxNodes = NodeCount;
        cout << "Search Level " << setprecision (0) << setw (3) << setfill (' ') << level <<
                ": Visited has " << setprecision (0) << fixed << setfill (' ') << setw (12)
                << Visited.SatCount(SourceVars) << " boards and "
                << NodeCount << " BDD nodes" << endl;
    }
};


/**
 * Implementation of callback for printing boards from BDD package
 */
VOID_TASK_IMPL_4(printboard, void*, ctx, BDDVAR*, VA, uint8_t*, values, int, count) {
    PegSolitaire *Game = (PegSolitaire *) ctx;
    cout << "--------------------" << endl;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (BOARD[i][j] == NN) {
                cout << " , ";
            } else {
                BDDVAR var = Game->Coor2Var[i][j];
                for (int k = 0; k < count; k++) {
                    if (var == VA[k]) {
                        cout << (values[k] ? "1" : "0") <<", ";
                        break;
                    }
                }
            }
        }
        cout << endl;
    }
    cout << "--------------------" << endl;
}



int main(int argc, const char *argv[]) {

    /* Init BDD package */
    lace_init(0, 0);
    lace_startup(0, NULL, NULL);
    LACE_ME;
    sylvan_set_sizes(1ULL<<22, 1ULL<<27, 1ULL<<20, 1ULL<<26);
    sylvan_init_package();
    sylvan_init_bdd();

    int algorithm = 0;
    if (argc > 1) {
        algorithm = atoi(argv[1]);
        if (algorithm > 6) {
            cerr << "Provide a valide argument: "<< endl <<
                            "\t0 = BFS, "<< endl <<
                            "\t1 = BFS2, "<< endl <<
                            "\t2 = Sweepline"<< endl <<
                            "\t3 = BFS Partial Rel, "<< endl <<
                            "\t4 = Chaining, "<< endl <<
                            "\t5 = Saturation-like, "<< endl <<
                            "\t6 = Saturation, "<< endl;
            exit(1);
        }
    }

    /* Create peg solitaire game */
    PegSolitaire Game;

    Game.CreateInitialBoard();
    Game.PrintBoards(Game.InitialBoard, "initial board");

    Game.CreateWinningBoards();
    //Game.PrintBoards(Game.WinningBoards, "winning boards");

    Game.CreateMoveRelation();
    cout << "Total relation has "<< scientific << setprecision(2) <<
        Game.MoveRelation.SatCount(Game.RelationVars) <<" board tuples and "<<
        Game.MoveRelation.NodeCount() <<" BDD nodes"<< endl << endl;


    /* Compute Fixpoint */
    Bdd FixPoint;
    switch (algorithm) {
    case 0: FixPoint = Game.FixPointBFS(Game.InitialBoard);             break;
    case 1: FixPoint = Game.FixPointBFS2(Game.InitialBoard);            break;
    case 2: FixPoint = Game.Sweepline(Game.InitialBoard);               break;
    case 3: FixPoint = Game.FixPointBFS_Partial(Game.InitialBoard);     break;
    case 4: FixPoint = Game.FixPointChaining(Game.InitialBoard);        break;
    case 5: FixPoint = Game.FixPointSaturationLike(Game.InitialBoard);  break;
    case 6: FixPoint = Game.FixPointSaturation(Game.InitialBoard);      break;
    }


    cout << endl;
    cout << "Reachable boards count: " << setprecision (0) << fixed << setfill (' ') << setw (12)
            << FixPoint.SatCount(Game.SourceVars) << " ("<< FixPoint.NodeCount() <<" nodes)"<< endl;
    cout << "Maximum queue size: "<< Game.MaxNodes << " BDD nodes" << endl;

    /* Decide winning */
    bool winning = false; //TODO: Decide wether game is winning
    cout << endl << "This peg-solitaire board is "<<
                    (winning ? "winning!" : "losing!") << endl;

    return 0;
}
