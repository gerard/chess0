/* 
   This file is part of Chess0, a computer chess program based on Winglet chess
   by Stef Luijten.

   Copyright (C) 2017 Claudio M. Camacho

   Chess0 is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Chess0 is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Foobar. If not, see <http://www.gnu.org/licenses/>.
   */



// @file app.cpp
//
// This file contains the code for creating and starting the actual program
// (Application). The application interacts with the user to accept input
// (commands and moves) and perform an action based on the given input.
//
// The application can run in two different modes: XBOARD and CLI. The XBOARD
// mode makes the app to work quietly and interact with a graphical user
// interface (GUI) using the Winboard/Xboard protocol. The CLI mode makes the
// program run on the system command line and interact with the user by
// accepting text-based commands and moves.
//
// In summary, the application (or app) is responsible for taking the input and
// performing different actions, based on user (or GUI) commands. Once the app
// terminates, it returns a symbolic value and the whole program is terminated.
#include <iostream>
#include <iomanip>
#include <sstream>
#include <map>
#include <set>
#include <iterator>
#include <algorithm>
#include <string>
#include <cctype>
#include <cstring>
#include "app.h"
#include "board.h"
#include "move.h"
#include "book.h"
#include "functions.h"
#include "globals.h"
#include "cmd.h"



using namespace std;



// Initialize the application, game and board status.
unsigned numberOfMove = 1;
unsigned cursor = 0;
string lastErrorMessage = "";
ColorType gameMate = COLOR_TYPE_NONE;
int playMode = HUMAN_CPU;
int wPlayer = PLAYER_TYPE_HUMAN;
int bPlayer = PLAYER_TYPE_COMPUTER;
vector<string> listOfCommands;
vector<string> history;
bool useBook = true;
bool usePersonalBook = false;
char sanMove[12];
unsigned char movingSide = WHITE_MOVE;
map<string, string> validMoves;
int i, j, number;
char command[80];
char userinput[80];
string userMove = "";

bool useCache = false;
bool verbose = true;
bool LMR = true;
Cache cache;
EndType gameEnd = END_TYPE_NOEND;



// startApp()
//
// Start the whole application and main CLI loop.
int startApp(int mode)
{
    // Welcome the user with the program name and a prompt, which is ready to
    // start taking moves (or commands) and play a game.
    if (mode == APP_MODE_CLI)
        cout << "Welcome to " << PROGRAM_NAME << "!" << endl;

    // XBoard protocol goes here
    else
    {
        // TODO
    }


    // Reset all variables before starting a new game.
    string input = "";
    curPlayerType = wPlayer;
    Move myMove;
    string moveIsFromBook = "";
    clock_t start, end;
    InputType itype;


    // Initialize the board, the list of commands and the openings book.
    dataInit();
    board.init();
    initListOfCommands();
    initBook();


    // Application's main loop:
    // - Show the prompt and ask for an input (move or command)
    // - Read the input and check if it is a move or a command
    // - If invalid command or move, show error and return to previous prompt
    // - If valid command, execute and return to previous prompt
    // - If valid move, perform the move.
    //
    // This loop is exited by entering Ctrl+D or the commands "quit" or "exit",
    // which call terminateApp().
    while (1)
    {
        // Check for game end (e.g., resign).
        if (gameEnd)
            dealEnd();


        // Clear both the input and output buffer, to make sure the new input
        // isn't altered from something entered during the program's output.
        cout.flush();
        input = "";


        // Show the prompt and ask the user for an input (move or command).
        prompt(numberOfMove);


        // Current player is Human, ask for input
        if (curPlayerType == PLAYER_TYPE_HUMAN)
        {
            input = getInput();
        }
        else if (curPlayerType == PLAYER_TYPE_COMPUTER)
        {
            // first, we try to get a reply from the openings book.
            moveIsFromBook = "";

            if (useBook)
            {
                // try to get a move from the opening book
                // Note: the sequence is assumed to be consistent with the game
                string bookSequence = getGameSequence();
                if (bookSequence == "")
                    bookSequence = "%";
                input = getReplyTo(bookSequence);
            }


            // If no move from the book, enter search mode.
            if (input.empty())
            {
                start = clock();
                cout << endl << "Thinking..." << endl << endl;

                // store who's moving right now for netScore in search
                // statistics
                if (board.nextMove)
                    movingSide = BLACK_MOVE;
                else
                    movingSide = WHITE_MOVE;


                // search the move
                myMove = board.think();

                // set the input from computer to the SAN move
                toSan(myMove, sanMove);
                input = string(sanMove);

                end   = clock();
            }

            // if the move was form the book, show it with a (%) symbol
            else if (useBook)
            {
                moveIsFromBook = " [book] ";
            }
        }


        // clean-up the input from '+' symbols
        string addSymbol = "";
        size_t pos = input.rfind("#");
        if (pos != std::string::npos)
            addSymbol = "#";
        pos = input.rfind("+");
        if (pos != std::string::npos)
            addSymbol = "+";
        input.erase(std::remove(input.begin(), input.end(), '+'), input.end());
        input.erase(std::remove(input.begin(), input.end(), '#'), input.end());


        // Check what type of input was entered: move, null move, command, etc.
        itype = getInputType(input);


        // If the input was a move (and the game is still running), make the
        // move -- thus updating the board and other variables --, display the
        // updated board, change side to move and go to the beginning of the
        // loop again.
        if (itype == INPUT_TYPE_MOVE)
        {
            // make a list of current valid moves
            string tmpStr = "";
            string s1, s2;
            board.moveBufLen[0] = 0;
            board.moveBufLen[1] = movegen(board.moveBufLen[0]);
            validMoves.clear();
            for (auto i = board.moveBufLen[0]; i < board.moveBufLen[1]; i++)
            {
                makeMove(board.moveBuffer[i]);
                if (isOtherKingAttacked())
                {
                    unmakeMove(board.moveBuffer[i]);
                }
                else
                {
                    unmakeMove(board.moveBuffer[i]);
                    toSan(board.moveBuffer[i], sanMove);

                    string moveStr(sanMove);
                    s1 = SQUARENAME[(board.moveBuffer[i]).getFrom()];
                    s2 = SQUARENAME[(board.moveBuffer[i]).getTosq()];
                    tmpStr = s1 + s2;
                    moveStr.erase(std::remove(moveStr.begin(), moveStr.end(), '+'), moveStr.end());
                    moveStr.erase(std::remove(moveStr.begin(), moveStr.end(), '#'), moveStr.end());

                    // if promo, append promo piece
                    if ((board.moveBuffer[i]).isPromo())
                    {
                        string tc(PIECECHARS[(board.moveBuffer[i]).getPromo()]);
                        tmpStr += tc;
                        transform(tmpStr.begin(), tmpStr.end(), tmpStr.begin(), ::tolower);
                    }

                    validMoves[moveStr] = tmpStr;
                }
            }


            // if valid move, make the move
            auto t = validMoves.find(input);
            if (t != validMoves.end())
            {
                userMove = t->second;
                sscanf(userMove.c_str(), "%s", userinput);

                // generate the pseudo-legal move list
                board.moveBufLen[0] = 0;
                board.moveBufLen[1] = movegen(board.moveBufLen[0]);

                // check to see if the user move is also found in the pseudo-legal move list
                if (isValidTextMove(userinput, myMove))
                {
                    makeMove(myMove);

                    // post-move check to see if we are leaving our king in check
                    if (isOtherKingAttacked())
                    {
                        unmakeMove(myMove);
                        cout << "    invalid move, leaving king in check: " << userinput << endl;
                    }
                    else
                    {
                        board.endOfGame++;
                        board.endOfSearch = board.endOfGame;
                    }
                }
                else
                {
                    cout << "    move is invalid or not recognized: " << userinput << endl;
                }
            }

            // not a valid move
            else
            {
                cout << "Invalid move '" << input << "'" << endl;
                continue;
            }


            if (curPlayerType == PLAYER_TYPE_COMPUTER)
            {
                // measure how long it takes to think the move
                float timeUsed = (end - start) / (CLOCKS_PER_SEC / 1000);

                // display the move chosen by the computer and other info
                cout << endl << endl << "\aMy move: ";
                if (board.nextMove)
                    cout << to_string(numberOfMove) << ". ";
                else
                    cout << to_string(numberOfMove) << "... ";
                input += addSymbol;
                cout << input;


                // show if the move is from the book
                cout << moveIsFromBook;

                // show how much time was used to think
                cout.fill('0');
                cout << " (time: ";
                cout << setprecision(2) << timeUsed / 1000.00f << "s)";

                cout.fill(' ');
            }


            // show the board and change side to move
            board.display();
            changeSide();


            // check for the end of the game
            Move tmpMove;
            if (board.isEndOfgame(number, tmpMove))
                dealEnd();
        }


        // if the input was a command, execute it and prompt again.
        else if (itype == INPUT_TYPE_CMD)
        {
            exec(input);
        }


        // if the input is not a command nor a move, display an error and prompt
        // again.
        else
        {
            cerr << "Unknown command or move '" << input << "'." << endl << endl;
        }
    }


    return 0;
}



// prompt()
//
// Show the prompt to the user, waiting for an input.
void prompt(unsigned nmove)
{
    ostringstream line(ostringstream::out);

    if (board.nextMove)
    {
        line << "Black (" << nmove << "): ";
    }
    else
    {
        line << "White (" << nmove << "): ";
    }

    cout << endl << line.str();
}



// getInput()
//
// Retrieve an input line from the user.
string getInput()
{
    // clean the input, if necessary
    if (cin.peek() == '\n')
        cin.ignore(1, '\n');

    char input[MAX_INPUT_SIZE];


    // get a line from the input until we reach the maximum input size.
    cin.getline(input, MAX_INPUT_SIZE);
    string input_str(input);


    // If Ctrl+D is entered, then terminate the program.
    if (cin.eof())
        terminateApp();

    return input_str;
}



// terminateApp()
//
// Terminate the app in a safe way.
void terminateApp()
{
    cout << endl << "Goodbye!" << endl;
    exit(0);
}



// getInputType()
//
// Analyze a given input and find out whether it is a move, a command or an
// invalid input.
InputType getInputType(string input)
{
    // Handle null moves
    if ((input == "pass") || (input == "null"))
        return INPUT_TYPE_MOVE;


    // First check if the input matches any of the supported commands.
    int pos = input.find_first_of(' ', 0);
    string cmd = input.substr(0, pos);
    if (find(listOfCommands.begin(), listOfCommands.end(), cmd) != listOfCommands.end())
        return INPUT_TYPE_CMD;


    // If it is not a command, it must be a move. Moves must have a length
    // between 2 and 6 characters.
    if ((input.length() < 2) || (input.length() > 7))
        return INPUT_TYPE_INVALID;


    // Look for pawn moves => column + row (e.g. e4, d4, f3, b7..)
    else if ((input.length() == 2) && (isFile(input[0])) && (isRank(input[1])))
        return INPUT_TYPE_MOVE;


    // Look for simple piece moves => piece + column|row + column|row (e.g. Nd4)
    // and short castling moves (0-0).
    else if (input.length() == 3)
    {
        if ((input == "0-0") || (input == "O-O"))
            return INPUT_TYPE_MOVE;

        else if (isPiece(input[0]) && isFile(input[1]) && isRank(input[2]))
            return INPUT_TYPE_MOVE;

        else if (isFile(input[0]) && (input[1] == 'x') && isFile(input[2]))
            return INPUT_TYPE_MOVE;

        else if (isPiece(input[0]) && (input[1] == 'x') && isFile(input[2]))
            return INPUT_TYPE_MOVE;

        else
            return INPUT_TYPE_INVALID;
    }


    // Look for 4-character moves, including:
    // - move_orig_dest   => column + row + column + row (e.g. d2d4)
    // - capture_simple   => piece|column + 'x' + column + row (e.g. exd4, Nxd4)
    // - promotion_simple => column + row + '=' + piece
    else if (input.length() == 4)
    {
        // look for promotions
        if (isFile(input[0]) && isRank(input[1]) && (input[2] == '=') &&
                isPiece(input[3])  && (input[3] != 'K') && (input[3] != 'k'))
            return INPUT_TYPE_MOVE;

        // look for algebraic move (xyXY)
        else if ((isFile(input[0])) && (isRank(input[1]))
                && (isFile(input[2]))
                && (isRank(input[3])))
            return INPUT_TYPE_MOVE;

        // look for captures
        else if (isFile(input[0]) && (input[1] == 'x'))
            return INPUT_TYPE_MOVE;

        else if (isPiece(input[0]) && (input[1] == 'x'))
            return INPUT_TYPE_MOVE;

        // look for long moves
        else if (isPiece(input[0]) && (isFile(input[1]) || isRank(input[1]))
                && isFile(input[2]) && isRank(input[3]))
            return INPUT_TYPE_MOVE;

        else
            return INPUT_TYPE_INVALID;

    }


    // Look for 5-character moves, including:
    // - algebraic promotion     => e7e8Q
    // - move_piece_ambiguous    => piece + column|row + column + row
    // - castle_long             => 0-0-0
    // - promotion_capture_short => column + 'x' + column + '=' + piece
    else if (input.length() == 5)
    {
        // look for algebraic promotions
        if (isFile(input[0]) && isRank(input[1]) &&
                isFile(input[2]) && isRank(input[3]) &&
                isPiece(input[4]))
            return INPUT_TYPE_MOVE;

        // look for promotions
        else if (isFile(input[0]) && (input[1] == 'x') && isFile(input[2]) &&
                (input[3] == '=')  && (input[4] != 'K') && (input[4] != 'k'))
            return INPUT_TYPE_MOVE;

        // look for long castling
        else if ((input == "0-0-0") || (input == "O-O-O"))
            return INPUT_TYPE_MOVE;

        // look for long moves
        else if (isPiece(input[0]) && (isFile(input[1]) || isRank(input[1]))
                && (input[2] == 'x')
                && isFile(input[3]) && isRank(input[4]))
            return INPUT_TYPE_MOVE;

        else
            return INPUT_TYPE_INVALID;
    }


    // Look for 6-character moves => promotion after capture in SAN notation =>
    // column + 'x' + column + '8' + '=' + piece.
    else if (input.length() == 6)
    {
        if (isFile(input[0]) && (input[1] == 'x') && isFile(input[2]) &&
                ((input[3] == '8') || (input[3] == '1')) && (input[4] == '=') &&
                (input[5] != 'K') && (input[5] != 'k'))
            return INPUT_TYPE_MOVE;

        else
            return INPUT_TYPE_INVALID;
    }


    // by default, an input is invalid unless proven otherwise.
    return INPUT_TYPE_INVALID;
}



// changeSide()
//
// Change the side playing, including color, type of input (human vs computer),
// clocks, etc.
void changeSide()
{
    if (playMode == HUMAN_CPU)
        (curPlayerType == PLAYER_TYPE_HUMAN) ? curPlayerType = PLAYER_TYPE_COMPUTER : curPlayerType = PLAYER_TYPE_HUMAN;
    else if (playMode == CPU_CPU)
        curPlayerType = PLAYER_TYPE_COMPUTER;

    cursor++;

    numberOfMove = (cursor / 2) + 1;
}



// dealEnd()
//
// Deal with the end of the current match.
void dealEnd()
{
    cout << endl;

    // show result for draw due to insufficient material.
    if (bitCnt(board.whitePieces | board.blackPieces) < 3)
        cout << "1/2-1/2 {Insufficient material}" << endl;


    // deal with 50-fold repetition which ends the game.
    else if (board.fiftyMove > 99)
        cout << "1/2-1/2 {Draw by 50-move repetition}" << endl;


    // show results for mate.
    //else if (check == COLOR_TYPE_BLACK)
    //    cout << "1-0 {White mates}" << endl;

    //else if (check == COLOR_TYPE_WHITE)
    //    cout << "0-1 {Black mates}" << endl;


    // Resign.
    if (gameEnd == END_TYPE_RESIGN)
    {
        if (board.nextMove)
            cout << "1-0 {Black resigns}" << endl;
        else
            cout << "0-1 {White resigns}" << endl;
    }


    // Show result for stale mate.
    //else if (check == COLOR_TYPE_NONE)
    //    cout << "1/2-1/2 {Stale mate}" << endl;


    // don't let the computer control the program after a game
    wPlayer = PLAYER_TYPE_HUMAN;
    bPlayer = PLAYER_TYPE_COMPUTER;
    curPlayerType = wPlayer;
    playMode = HUMAN_CPU;
    cursor = 0;
    numberOfMove = 1;
    gameEnd = END_TYPE_NOEND;

    dataInit();
    board.init();

    // enable book by default
    useBook = true;
}



// displayGame()
//
// Display the list of moves of this game.
void displayGame()
{
    if (board.endOfGame)
    {
        // make a temporary copy of board.gameLine[];
        number = board.endOfGame;
        GameLineRecord *tmp = new GameLineRecord[number];
        memcpy(tmp, board.gameLine, number * sizeof(GameLineRecord));

        // unmake all moves:
        for (i = number-1 ; i >= 0 ; i--) 
        { 
            unmakeMove(tmp[i].move);
            board.endOfSearch = --board.endOfGame;
        }

        // redo all moves:
        j = board.nextMove;
        for (i = 0 ; i < number; i++)
        {
            // move numbering:
            if (!((i+j+2)%2)) cout << (i+2*j+2)/2 << ". ";
            else if (!i) cout << "1. ... ";

            // construct the move string
            toSan(tmp[i].move, sanMove);
            cout << sanMove; 

            // output CRLF, or space:
            if (!((i+j+1)%2)) cout << endl;
            else cout << ", ";

            // make the move:
            makeMove(tmp[i].move);
            board.endOfSearch = ++board.endOfGame;
        }
        cout << endl;

        // delete the temporary copy:
        delete[] tmp;
    } 
    else
    {
        cout << "there are no game moves" << endl;        
    }
}



// getGameSequence()
//
// Get the whole game history into a linear sequence, encoded in a string.
string getGameSequence()
{
    int count = 1;
    string sequence = "";

    if (board.endOfGame)
    {
        // make a temporary copy of board.gameLine[];
        number = board.endOfGame;
        GameLineRecord *tmp = new GameLineRecord[number];
        memcpy(tmp, board.gameLine, number * sizeof(GameLineRecord));

        // unmake all moves
        for (i = number-1 ; i >= 0 ; i--) 
        { 
            unmakeMove(tmp[i].move);
            board.endOfSearch = --board.endOfGame;
        }

        for (i = 0; i < number; i++)
        {
            toSan(tmp[i].move, sanMove);
            string ts(sanMove);

            if (i % 2)
            {
                sequence += " " + ts;
                count++;
            }
            else
            {
                // don't add a blank before the first move
                if (count == 1)
                    sequence += to_string(count) + ". " + ts;
                else
                    sequence += " " + to_string(count) + ". " + ts;
            }

            // make the move:
            makeMove(tmp[i].move);
            board.endOfSearch = ++board.endOfGame;
        }


        // delete the temporary copy
        delete[] tmp;
    }

    return sequence; 
}
