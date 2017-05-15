/* 
    This file is part of Chess0, a computer chess program based on Winglet chess
    by Stef Luijten.
    
    Copyright (C) 2017 Claudio M. Camacho
                                                                           
    Chess0x is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Chess0x is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar. If not, see <http://www.gnu.org/licenses/>.
*/



// @file app.h
//
// This file contains all the variables and method definitions that can
// be used across the whole application.
#ifndef APP_H
#define APP_H

#include <vector>
#include "definitions.h"

using namespace std;



/*!
 * Application types and definitions.
 */
#define PLAYER_TYPE_HUMAN    1
#define PLAYER_TYPE_COMPUTER 2

#define HUMAN_CPU   1
#define HUMAN_HUMAN 2
#define CPU_CPU     3


// different types of inputs
enum InputType
{
    INPUT_TYPE_MOVE,
    INPUT_TYPE_CMD,
    INPUT_TYPE_INVALID,
};



/*!
 * Application methods and variables.
 */
extern int             appMode;
extern vector<string>  listOfCommands;
extern string          lastErrorMessage;
extern vector<string>  history;

int startApp(int);
/*void prompt(ColorType, unsigned);
std::string getInput();
void terminateApp();
void changeSide();
uint64_t perft(unsigned, ColorType);
void dealEnd();
InputType getInputType(string);
void addToHistory(Move);
void removeFromHistory(int);
void displayHistory();
string getHistorySequence();
*/



/*!
 * Game status variables.
 */
extern int playMode;
extern int wPlayer;
extern int bPlayer;
extern int curPlayerType;
//extern ColorType sideToMove;
extern unsigned numberOfMove;
extern unsigned cursor;
//extern EndType gameEnd;
//extern ColorType gameMate;
extern bool useBook;



/*!
 * Built-in helper functions.
 */
inline bool isFile(char c) { return ((c >= 'a' ) && (c <= 'h')); }
inline bool isRank(char r) { return ((r >= '1' ) && (r <= '8')); }



#endif // APP_H
