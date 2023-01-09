#include <iostream>
#include <vector>
#include <climits>
#include <cstdlib>
#include <ctime>
#include <cmath>

// Prefix g_ means that this variable is globally accessible
const unsigned int FIELD_DIM = 8;
int g_field[FIELD_DIM][FIELD_DIM] = { 0 };
const int EMPTY = 0;

enum GameStates {
    Placement,
    Replacement
};
int g_gameState = Placement;

struct Coord2d {
    int x;
    int y;
};

const Coord2d INVALID_COORD = { -1, -1 };

bool coordsEqual(Coord2d c1, Coord2d c2) {
    return c1.x == c2.x && c1.y == c2.y;
}

struct Vec2d {
    int x;
    int y;
};

// Coordinate system is like this:
// 0 -------> x
// |
// |
// |
// V
// y
const int POSSIBLE_MOVES_COUNT = 8;
const Vec2d possibleMoves[POSSIBLE_MOVES_COUNT] = {
    {  0,  1 }, // down
    {  1,  0 }, // right
    { -1,  0 }, // left
    {  0, -1 }, // up
    {  1,  1 }, // down and right
    { -1,  1 }, // down and left
    { -1, -1 }, // up and left
    {  1, -1 }  // up and right
};

const int LINE_ORTHS_COUNT = 4;
const Vec2d lineOrths[LINE_ORTHS_COUNT] = {
    { 1,  0 },
    { 1,  1 },
    { 0,  1 },
    { 1, -1 },
};

enum Side {
    White = 1,
    Black = 2,
};

const int DESIRED_ROW_LEN = 5;

const int CHECKER_COUNT = 32;
struct PlayerInfo {
    PlayerInfo(Side s) : side(s) {}
    Coord2d checkers[CHECKER_COUNT];
    int placedCount = 0;
    Side side;
    int score = 0;
};

PlayerInfo g_player(White);
PlayerInfo g_ai(Black);

int calculateRowLengthInGivenDestination(Coord2d c, Vec2d dest);
int calculateRowLengthInLine(Coord2d c, Vec2d line) {
    return calculateRowLengthInGivenDestination(c, line) + calculateRowLengthInGivenDestination(c, { -line.x, -line.y }) - 1;
}

bool playerWon(PlayerInfo &pinfo) {
    for (int i = 0; i < pinfo.placedCount; i++) {
        for (int l = 0; l < LINE_ORTHS_COUNT; l++) {
            if (calculateRowLengthInLine(pinfo.checkers[i], lineOrths[l]) >= DESIRED_ROW_LEN) {
                return true;
            }
        }
    }

    return false;
}

bool isGameOver() {
    if (playerWon(g_player) || playerWon(g_ai))
        return true;

    return false;
}

bool isCellOccupied(Coord2d c) {
    return g_field[c.x][c.y] != EMPTY;
}

// Best moves for player - with positive value,
// for AI - with negative values
int grade(PlayerInfo &pl, PlayerInfo &ai) {
    int gr = 0;
    int minDiff = DESIRED_ROW_LEN;
    // Searching for longest row on the player's side
    for (int i = 0; i < pl.placedCount; i++) {
        for (int l = 0; l < LINE_ORTHS_COUNT; l++) {
            const int rowLen = calculateRowLengthInLine(pl.checkers[i], lineOrths[l]);
            gr += rowLen;
            if (DESIRED_ROW_LEN - rowLen < minDiff) {
                minDiff = DESIRED_ROW_LEN - rowLen;
            }
        }
    }

    // The same, but for AI side
    for (int i = 0; i < ai.placedCount; i++) {
        for (int l = 0; l < LINE_ORTHS_COUNT; l++) {
            const int rowLen = calculateRowLengthInLine(ai.checkers[i], lineOrths[l]);
            gr -= rowLen;
        }
    }

    return gr;
}

int calculateRowLengthInGivenDestination(Coord2d c, Vec2d dest) {
    const auto curSide = g_field[c.x][c.y];
    Vec2d curDest = dest;
    int rowLen = 1; // initial value is 1 because actual function does not count given checker itself
    while (g_field[c.x + curDest.x][c.y + curDest.y] == curSide) {
        rowLen++;
        curDest.x += dest.x;
        curDest.y += dest.y;
    }

   return rowLen;
}

void registerCheckerForPlayer(PlayerInfo &pinfo, Coord2d c) {
    pinfo.checkers[pinfo.placedCount++] = c;
}

void unregisterCheckerForPlayer(PlayerInfo &pinfo, Coord2d c) {
    for (int i = 0; i < pinfo.placedCount; i++) {
        if (coordsEqual(c, pinfo.checkers[i])) {
            for (int j = i; j < pinfo.placedCount - 1; j++) {
                pinfo.checkers[j] = pinfo.checkers[j + 1];
            }
            pinfo.checkers[pinfo.placedCount] = INVALID_COORD;
            pinfo.placedCount--;
            break;
        }
    }
}

void placeChecker(PlayerInfo &pinfo, Coord2d c) {
    g_field[c.x][c.y] = pinfo.side;
    registerCheckerForPlayer(pinfo, c);
}

void clearCell(Coord2d c);
void removeChecker(Coord2d c) {
    const auto checkerSide = g_field[c.x][c.y];
    auto &pinfo = checkerSide == g_player.side ? g_player : g_ai;
    unregisterCheckerForPlayer(pinfo, c);
    clearCell(c);
}

void replaceChecker(PlayerInfo &whose, Coord2d from, Coord2d to) {
    removeChecker(from);
    placeChecker(whose, to);
}

// ???
void virtualPlacement(Coord2d c, Side side) {
    g_field[c.x][c.y] = side;
}

struct ReplacementInfo {
    Coord2d from;
    Coord2d to;
    Side side;
};

ReplacementInfo virtualReplacement(PlayerInfo &whose, Coord2d from, Coord2d to) {
    replaceChecker(whose, from, to);

    return { from, to, whose.side };
}

void clearCell(Coord2d c) {
    g_field[c.x][c.y] = EMPTY;
}

void printField() {
    std::cout << " x 0 1 2 3 4 5 6 7\ny\n";
    for (int j = 0; j < FIELD_DIM; j++) {
        std::cout << j << "  ";
        for (int i = 0; i < FIELD_DIM; i++) {
            if (g_field[i][j] == White)
                std::cout << "O ";
            else if (g_field[i][j] == Black)
                std::cout << "0 ";
            else
                std::cout << "# ";
        }
        std::cout << '\n';
    }
}

bool moveOutOfField(Coord2d c, Vec2d v) {
    return 0 > c.x + v.x || FIELD_DIM <= c.x + v.x ||
        0 > c.y + v.y || FIELD_DIM <= c.y + v.y;
}

auto availableMoves(PlayerInfo &pinfo) -> std::vector<Coord2d> {
    std::vector<Coord2d> available;
    for (int i = 0; i < pinfo.placedCount; i++) {
        for (int m = 0; m < POSSIBLE_MOVES_COUNT; m++) {
            if (!moveOutOfField(pinfo.checkers[i], possibleMoves[m]) &&
                    g_field[pinfo.checkers[i].x + possibleMoves[m].x][pinfo.checkers[i].y + possibleMoves[m].y] == EMPTY) {
                available.push_back({ pinfo.checkers[i].x + possibleMoves[m].x, pinfo.checkers[i].y + possibleMoves[m].y });
            }
        }
    }

    return available;
}

// What should it do actually?
// [ ] it should use it's own heuristic function
// [ ] it should first determine which checker can be took off the board
// with minimal losses
// [ ] then it should predict where to put this checker to gain max profit
// [ ] then it should determine which checker it's opponent can take off
// the board for replacement
// [ ] and then place it's opponent's checker somewhere where it provides his opponent
// with max profit
// etc... until recursion depth run out
int minmaxReplacement(Side side, int depth, int alpha, int beta) {
    return 0;
}

const int DEPTH_LEVEL = 3;
int minmaxPlacement(Side side, int depth, Coord2d &bestMove, int alpha, int beta) {
    if (depth >= DEPTH_LEVEL) {
        return grade(g_player, g_ai);
    }
    int mm = side == g_player.side ? INT_MIN : INT_MAX;
    int test = 0;
    bestMove = INVALID_COORD;

    const std::vector<Coord2d> avail = availableMoves(side == g_player.side ? g_player : g_ai);
    for (const auto am : avail) {
        virtualPlacement(am, side);

        test = minmaxPlacement(side == g_player.side ? g_ai.side : g_player.side, depth + 1, bestMove, alpha, beta);

        if ((test >= mm && side == White) ||
            (test <= mm && side == Black)) {
            mm = test;
            bestMove = am;
        }

        if (side == g_player.side) {
            alpha = std::max(test, alpha);
        }
        else {
            beta = std::min(test, beta);
        }

        if (beta < alpha) {
            clearCell(am);
            break;
        }

        clearCell(am);
    }

    if (coordsEqual(bestMove, INVALID_COORD))
        return grade(g_player, g_ai);

    return mm;
}

int main() {
    int turn = 0;
    bool firstTurn = true;
    while (!isGameOver() && g_player.placedCount != CHECKER_COUNT && g_ai.placedCount != CHECKER_COUNT) {
        printField();
//        if (g_player.placedCount == CHECKER_COUNT && g_ai.placedCount == CHECKER_COUNT) {
//            g_gameState = GameStates::Replacement;
//        }
        if (turn == 0) {
            bool firstTry = true;
//            if (g_gameState == Placement) {
                std::cout << "Your turn, type X and Y of your next checker placement: ";
                int x, y;
                do {
                    if (!firstTry) {
                        std::cout << "Cell is occupied, try different coordinates: ";
                    }
                    std::cin >> x >> y;
                    firstTry = false;
                } while (isCellOccupied({ x, y }));
                placeChecker(g_player, { x, y });
//           }
//            else {
//                std::cout << "Your turn; replacement phase;\nType X and Y of the checker to be moved: ";
//                int xToClear, yToClear;
//                std::cin >> xToClear >> yToClear;
//                std::cout << "Type X and Y of your checker placement: ";
//                int x, y;
//                std::cin >> x >> y;
//                replaceChecker(g_player, { xToClear, yToClear }, { x, y });
//            }
            turn = 1;
        }
        else {
            std::cout << "Computer is making it's move...\n";
//            if (g_gameState == Placement) {
                if (!firstTurn) {
                    Coord2d bestMove;
                    int mm = minmaxPlacement(g_ai.side, 0, bestMove, INT_MIN, INT_MAX);
                    placeChecker(g_ai, bestMove);
                }
                else {
                    srand(time(0));
                    const auto nearOpponent = availableMoves(g_player);
                    const Coord2d randPosition = nearOpponent[rand() % nearOpponent.size()];
                    placeChecker(g_ai, randPosition);
                    firstTurn = false;
                }
//            }
//            else {
//                Coord2d bestMove;
//                int mm = minmax(g_ai.side, 0, bestMove, INT_MIN, INT_MAX);
//                placeChecker(g_ai, bestMove);
//            }
            turn = 0;
        }
    }
    printField();
    if (g_player.score != 0) {
        std::cout << "You win!\n";
    }
    else std::cout << "AI wins\n";

    return 0;
}
