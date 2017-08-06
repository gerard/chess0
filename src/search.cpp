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



// @file search.cpp
//
// This file contains the main search methods for finding the best next move.
//
// It is based on a simple alpha-beta pruning mechanism runnning on top of a
// miniMAX, and it includes multiple optimizations, such as:
// - Principal Variation search (PVS)
// - null move heuristics
// - history & killer heuristics
// - late move reductions (LMR)
// - quiescence search
#if defined(_WIN32) || defined(_WIN64)
#include <conio.h>
#else
#include <unistd.h>
#endif



#include <stdio.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <iomanip>
#include <cinttypes>
#include <unordered_map>
#include "definitions.h" 
#include "extglobals.h" 
#include "functions.h" 
#include "board.h" 
#include "timer.h" 
#include "app.h"
#include "cache.h"



using namespace std;



unsigned short nextDepth = 0;
unsigned short nextPly = 0;
ttEntry tt;
float cacheHit;
unsigned moveNo = 0;



// Board::think()
//
// This is the entry point for search, it is intended to drive iterative deepening 
// The search stops if (whatever comes first): 
// - there is no legal move (checkmate or stalemate)
// - there is only one legal move (in this case we don't need to search)
// - time is up 
// - the search is interrupted by the user, or by winboard
// - the search depth is reached
Move Board::think()
{
    int score, legalmoves, currentdepth;
    Move singlemove;
    cacheHit = 0;


    //  if the game has ended, don't search
    if (isEndOfgame(legalmoves, singlemove))
        return NOMOVE;


    // if only one legal move possible, don't search
    if (legalmoves == 1) 
    {
        cout << endl; 

        if (XB_MODE && XB_POST) 
        {
            cout << "0 0 0 0 "; 
            displayMove(singlemove);
            cout << endl;
        }
        return singlemove;
    }


    //  prepare for normal search
    if (XB_MODE)
        timeControl();


    lastPVLength = 0;
    memset(lastPV, 0 , sizeof(lastPV));
    memset(whiteHeuristics, 0, sizeof(whiteHeuristics));
    memset(blackHeuristics, 0, sizeof(blackHeuristics));
    inodes = 0;
    countdown = UPDATEINTERVAL;
    timedout = false;


    // display console header
    displaySearchStats(1, 0, 0);  
    timer.init();
    msStart = timer.getms();


    //  iterative deepening:
    for (currentdepth = 1; currentdepth <= searchDepth; currentdepth++)
    {
        // clear the buffers
        memset(moveBufLen, 0, sizeof(moveBufLen));
        memset(moveBuffer, 0, sizeof(moveBuffer));
        memset(triangularLength, 0, sizeof(triangularLength));
        memset(triangularArray, 0, sizeof(triangularArray));
        followpv = true;
        allownull = true;


        // enter actual search
        score = alphabetapvs(0, currentdepth, -LARGE_NUMBER, LARGE_NUMBER);


        // now check if time is up and decide whether to start a new iteration
        if (timedout) 
        {
            cout << endl;
            return (lastPV[0]);
        }
        else
        {
            if (!XB_NO_TIME_LIMIT)
            {
                msStop = timer.getms();
                if ((msStop - msStart) > (STOPFRAC * maxTime)) 
                {
                    if (!XB_MODE) cout << "    ok" << endl;
                    return (lastPV[0]);
                }
            }
        }


        // display search analysis
        if (currentdepth > 1)
            displaySearchStats(2, currentdepth, score);


        // stop searching if the current depth leads to a forced mate:
        if ((score > (CHECKMATESCORE-currentdepth)) || (score < -(CHECKMATESCORE-currentdepth))) 
        {
            rememberPV();
            currentdepth = searchDepth + 1;
        }
    }

    return lastPV[0];
}



// Board::alphabetapvs()
//
// Main alphabeta algorithm coupled with Principal Variation search. This is the
// recurrent loop that is called millions of times in every search.
int Board::alphabetapvs(int ply, int depth, int alpha, int beta)
{
    int i, j, movesfound, pvmovesfound, val, qval;
    bool cached = false;


    // if at leaf node, return qval
    if (depth <= 0) 
    {
        followpv = false;
        qval = qsearch(ply, alpha, beta);
        return qval;
    }


    // repetition check:
    if (repetitionCount() >= 3)
        return DRAWSCORE;


    // prepare structure to store the principal variation (PV)
    triangularLength[ply] = ply;


    // now try a null move to get an early beta cut-off:
    if (!followpv && allownull)
    {
        if ((nextMove && (board.totalBlackPieces > NULLMOVE_LIMIT)) || (!nextMove && (board.totalWhitePieces > NULLMOVE_LIMIT)))
        {
            if (!isOwnKingAttacked())
            {
                allownull = false;
                inodes++;

                if (--countdown <=0)
                    readClockAndInput();

                nextMove = !nextMove;
                hashkey ^= KEY.side; 
                val = -alphabetapvs(ply, depth - NULLMOVE_REDUCTION, -beta, -beta+1);
                nextMove = !nextMove;
                hashkey ^= KEY.side;

                if (timedout)
                    return 0;

                allownull = true;

                if (val >= beta)
                    return val;
            }
        }
    }
    allownull = true;

    movesfound = 0;
    pvmovesfound = 0;
    moveNo = 0;
    moveBufLen[ply+1] = movegen(moveBufLen[ply]);
    for (i = moveBufLen[ply]; i < moveBufLen[ply+1]; i++)
    {
        // pick the next best move from a sorted list
        selectmove(ply, i, depth, followpv); 

        // make the move and evaluate the board
        makeMove(moveBuffer[i]);

        if (!isOtherKingAttacked()) 
        {
            inodes++;
            moveNo++;

            if (--countdown <=0)
                readClockAndInput();

            movesfound++;


            if (!ply && (depth > 1))
                displaySearchStats(3, ply, i); 


            // 1) Look up the cache
            if (useCache)
            {
                tt = cache.find(board.hashkey, ply);
                if (tt.depth < ply)
                {
                    cached = false;
                }
                else
                {
                    cacheHit++;
                    val = tt.score;
                    rememberPV();
                    cached = true;
                }
            }


            if (!cached)
            {
                // 2) LMR
                //
                // Configure late-move reductions (LMR): assuming that the moves in the
                // list are ordered from potential best to potential worst, analyzing 
                // the first moves is more critical than the last ones. Therefore, 
                // using LMR we analyze the first 2 moves in full-depth, but cut down
                // the analysis depth for the rest of moves.
                nextPly   = ply + 1;
                nextDepth = depth - 1;
                if (LMR && (ply > LMR_PLY_START) && (depth > LMR_SEARCH_DEPTH) && !((moveBuffer[i]).isCapture()) && !((moveBuffer[i]).isPromo())
                        && !(isOwnKingAttacked()) && !(isOtherKingAttacked()) && (moveNo > LMR_MOVE_START))
                {
                    nextPly   = ply + 2;
                    nextDepth = depth - 2;
                }


                // 3) Alphabeta with Principal Variation Search (PVS)
                if (pvmovesfound)
                {
                    val = -alphabetapvs(nextPly, nextDepth, -alpha-1, -alpha); 

                    // in case of failure, proceed with normal alphabeta
                    if ((val > alpha) && (val < beta))
                        val = -alphabetapvs(ply+1, nextDepth, -beta, -alpha);               
                }
                else
                {
                    val = -alphabetapvs(ply+1, nextDepth, -beta, -alpha);     
                }


                // 4) Store in cache (replacement scheme --> always replace)
                if (useCache && (ply > 4))
                {
                    if ((val > -CHECKMATESCORE) && (val < CHECKMATESCORE))
                    {
                        tt.score = val;
                        tt.depth = ply;
                        cache.add(board.hashkey, tt);
                    }
                }
            }


            // 5) unmake the move
            unmakeMove(moveBuffer[i]);


            // if time is up, then return
            if (timedout)
                return 0;


            if (val >= beta)
            {
                // update the history heuristic
                if (nextMove) 
                    blackHeuristics[moveBuffer[i].getFrom()][moveBuffer[i].getTosq()] += depth*depth;
                else 
                    whiteHeuristics[moveBuffer[i].getFrom()][moveBuffer[i].getTosq()] += depth*depth;

                return beta;
            }


            // both sides want to maximize from *their* perspective
            if (val > alpha)
            {
                // update bounds
                alpha = val;
                pvmovesfound++;


                // save this move
                triangularArray[ply][ply] = moveBuffer[i];


                // append the latest best PV from deeper pleis
                for (j = ply + 1; j < triangularLength[ply+1]; j++) 
                    triangularArray[ply][j] = triangularArray[ply+1][j];
                triangularLength[ply] = triangularLength[ply+1];


                // show intermediate search results
                if (!ply && (depth > 1))
                    displaySearchStats(2, depth, val);
            } 
        }
        else unmakeMove(moveBuffer[i]);
    }


    // update the history heuristic
    if (pvmovesfound)
    {
        if (nextMove) 
            blackHeuristics[triangularArray[ply][ply].getFrom()][triangularArray[ply][ply].getTosq()] += depth*depth;
        else
            whiteHeuristics[triangularArray[ply][ply].getFrom()][triangularArray[ply][ply].getTosq()] += depth*depth;
    }


    //  50-move rule:
    if (fiftyMove > 99)
        return DRAWSCORE;


    //  Checkmate/stalemate detection:
    if (!movesfound)
    {
        if (isOwnKingAttacked())
            return (-CHECKMATESCORE+ply-1);

        else
            return (STALEMATESCORE);
    }


    return alpha;
}



// displaySearchStats()
//
// Display analysis statistics about the calculation of a move.
void Board::displaySearchStats(int mode, int depth, int score)
{
    char sanMove[12], timestring[8];
    float dt = (timer.getms() - msStart) / 1000.0;
    int netScore = score;
    int z = 1;


    // '+' is advantage for White, '-' is advantage for Black
    if (movingSide == BLACK_MOVE)
    {
        netScore *= -1;
        z = -1;
    }


    // display various search statistics
    //
    // mode = 1 : display header
    // mode = 2 : display full stats, including score and latest PV
    // mode = 3 : display current root move that is being searched
    //            depth = ply, score = loop counter in the search move list 
    switch (mode)
    {
        case 1: 
            if (!XB_MODE) cout << "  Ply  Score    Nodes     Time\t Speed kN/s  PV" << endl;
            break;

        case 2:
            if (XB_MODE && XB_POST)
            {
                printf("%5d %6d %8f %9llu ", depth, netScore, dt/10, inodes);
                rememberPV();
                displayPV();
            }
            else
            {
                // depth
                cout << setw(5) << depth;

                // score
                cout << showpos << setw(7) << setprecision(2) << float(netScore/100.0);
                cout << noshowpos;

                // display the amount of nodes searched
                float cacheHitRatio = cacheHit / inodes;
                cout.fill(' ');
                if (cacheHitRatio > CACHE_HIT_LEVEL)
                {
                    cout << "   Cached";
                }
                else
                {
                    cout << " ";
                    if (inodes > 1000000)
                        cout << setw(7) << setprecision(1) << float(inodes/1000000.0) << "M";
                    else if (inodes > 1000)
                        cout << setw(7) << setprecision(1) << float(inodes/1000.0) << "K";
                    else
                        cout << setw(8) << setprecision(0) << inodes;
                }
                cout.fill(' ');


                // search time
                cout << setw(8) << fixed << setprecision(2) << dt << "s ";


                // search speed
                float knps = (inodes / (dt * 1000)) / 1.0;
                if (dt > 0)
                    cout << fixed << setprecision(1) << setw(7) << knps << " kN/s  ";
                else
                    cout << "           -  ";


                // store this PV:
                rememberPV();

                // display the PV
                displayPV();
            }
            break;

        case 3: // Note that the numbers refer to pseudo-legal moves:
            if (!TO_CONSOLE) break;
            if (XB_MODE)
            {

            }
            else
            {
                mstostring(dt, timestring);
                printf("             (%2d/%2d) %8s       ", score+1, moveBufLen[depth+1]-moveBufLen[depth], timestring);
                unmakeMove(moveBuffer[score]);
                toSan(moveBuffer[score], sanMove);
                cout << sanMove;
                makeMove(moveBuffer[score]);
                printf("...    \r");
                cout.flush();
            }
            break;

        default: break;
    }
}



// isEndOfgame()
//
// Checks if the current position is end-of-game due to:
// checkmate, stalemate, 50-move rule, or insufficient material.
bool Board::isEndOfgame(int &legalmoves, Move &singlemove)
{
    int whiteknights, whitebishops, whiterooks, whitequeens, whitetotalmat;
    int blackknights, blackbishops, blackrooks, blackqueens, blacktotalmat;
    int i;

    // are we checkmating the other side?
    if (isOtherKingAttacked()) 
    {
        if (nextMove) cout << "1-0 {Black mates}" << endl;
        else cout << "1-0 {White mates}" << endl;
        return true;
    }

    // how many legal moves do we have?
    legalmoves = 0;
    moveBufLen[0] = 0;
    moveBufLen[1] = movegen(moveBufLen[0]);
    for (auto i = moveBufLen[0]; i < moveBufLen[1]; i++)
    {
        makeMove(moveBuffer[i]);
        if (!isOtherKingAttacked())
        { 
            legalmoves++;
            singlemove = moveBuffer[i];
        }
        unmakeMove(moveBuffer[i]);
    }

    // checkmate or stalemate?
    if (!legalmoves) 
    {
        if (isOwnKingAttacked()) 
        {
            if (nextMove) cout << "1-0 {White mates}" << endl;
            else cout << "1-0 {Black mates}" << endl;
        }
        else cout << "1/2-1/2 {stalemate}" << endl;
        return true;
    }

    // draw due to insufficient material:
    if (!whitePawns && !blackPawns)
    {
        whiteknights = bitCnt(whiteKnights);
        whitebishops = bitCnt(whiteBishops);
        whiterooks = bitCnt(whiteRooks);
        whitequeens = bitCnt(whiteQueens);
        whitetotalmat = 3 * whiteknights + 3 * whitebishops + 5 * whiterooks + 10 * whitequeens;
        blackknights = bitCnt(blackKnights);
        blackbishops = bitCnt(blackBishops);
        blackrooks = bitCnt(blackRooks);
        blackqueens = bitCnt(blackQueens);
        blacktotalmat = 3 * blackknights + 3 * blackbishops + 5 * blackrooks + 10 * blackqueens;

        // king versus king:
        if ((whitetotalmat == 0) && (blacktotalmat == 0)) 
        {
            cout << "1/2-1/2 {material}" << endl;
            return true;
        }

        // king and knight versus king:
        if (((whitetotalmat == 3) && (whiteknights == 1) && (blacktotalmat == 0)) ||
                ((blacktotalmat == 3) && (blackknights == 1) && (whitetotalmat == 0))) 
        {
            cout << "1/2-1/2 {material}" << endl;
            return true;
        }

        // 2 kings with one or more bishops, all bishops on the same colour:
        if ((whitebishops + blackbishops) > 0)
        {
            if ((whiteknights == 0) && (whiterooks == 0) && (whitequeens == 0) &&
                    (blackknights == 0) && (blackrooks == 0) && (blackqueens == 0))
            {
                if (!((whiteBishops | blackBishops) & WHITE_SQUARES) ||
                        !((whiteBishops | blackBishops) & BLACK_SQUARES))
                {
                    cout << "1/2-1/2 {material}" << endl;
                    return true;
                }
            }
        }
    }

    // draw due to repetition:
    if (repetitionCount() >= 3) 
    {
        cout << "1/2-1/2 {repetition}" << endl;
        return true;

    }

    // draw due to 50 move rule:
    if (fiftyMove > 99) 
    {
        cout << "1/2-1/2 {50-move rule}" << endl;
        return true;
    }

    return false;
}



// repetitionCount()
//
// RepetitionCount is used to detect threefold repetitions of the current position.
int Board::repetitionCount()
{
    int i, ilast, rep;
    rep = 1;                                              // current position has occurred once, namely now!
    ilast = board.endOfSearch - board.fiftyMove;          // we don't need to go back all the way
    for (i = board.endOfSearch - 2; i >= ilast; i -= 2)   // we can skip every other position
    {
        if (gameLine[i].key == board.hashkey) rep++;
    }
    return rep;
}



// rememberPV()
//
// Remember the last PV, and also the 5 previous ones because
// they usually contain good moves to try.
void Board::rememberPV()
{
    int i;
    lastPVLength = board.triangularLength[0];
    for (i = 0; i < board.triangularLength[0]; i++)
    {
        lastPV[i] = board.triangularArray[0][i];
    }
}



// mstostring()
//
// convert milliseconds to a time string (hh:mm:ss, mm:ss, s, ms)
void mstostring(uint64_t dt, char *timestring)
{
    uint64_t hh, mm, ss;

    if (dt > 3600000) 
    {      
        hh = dt/3600000;
        mm = (dt - 3600000*hh)/60000;
        ss = (dt - 3600000*hh - 60000*mm)/1000;
        sprintf(timestring, "%02" PRIu64 ":%01" PRIu64 ":%02" PRIu64 "", hh, mm, ss);
    }
    else if (dt > 60000) 
    {      
        mm = dt/60000;
        ss = (dt - 60000*mm)/1000;
        sprintf(timestring, "%02" PRIu64 ":%02" PRIu64 "", mm, ss);
    }
    else if (dt > 10000)        sprintf(timestring, "%6.1f%s", float(dt/1000.0), "s");
    else if (dt > 1000)         sprintf(timestring, "%6.2f%s", float(dt/1000.0), "s");
    else if (dt > 0)            sprintf(timestring, "%5" PRIu64 "ms", dt);
    else                        sprintf(timestring, "0ms");

    return;

}



//  This routine is used to calculate maxTime, the maximum time for this move 
//  in millisceonds. Based on:
//  XB_CTIM = computer's time, milliseconds
//  XB_OTIM = opponents' time, millseconds
//  XB_INC = time increment, milliseconds
void timeControl()
{
    int xb_ctim, movesLeft;

    //  First build in a safety buffer of 2000 milliseconds:
    xb_ctim = XB_CTIM - 2000;
    if (xb_ctim < 1) xb_ctim = 1;

    //  Estimate the number of moves per side that are left. Assume 80 half moves 
    //  per game with a minimum of 10 half moves left to play, no matter how many moves are played:
    movesLeft = 80 - board.endOfSearch;
    if (movesLeft < 20) movesLeft = 20;

    //  Use up part of the thinking time advantage that we may have:
    if ((XB_OTIM + XB_INC) < xb_ctim)
        board.maxTime = (xb_ctim / movesLeft) + XB_INC + (int)(0.80*(xb_ctim - XB_OTIM - XB_INC)); 
    else
        board.maxTime = (xb_ctim / movesLeft);


    //  If an XB_INC is defined, then there is no reason to run out of time:
    if ((XB_INC) && (xb_ctim < XB_INC)) board.maxTime = xb_ctim;

    //  Final checks, all moves should be > something:
    if (board.maxTime > xb_ctim) board.maxTime = (int)(0.8 * xb_ctim);
    if (board.maxTime < 1) board.maxTime = 1;

    return;
}



// qsearch()
//
// Perform a search indefinitely until all the following conditions happen:
// a) no more possible captures
// b) no more checks possible
// c) no more pawn promotions
int Board::qsearch(int ply, int alpha, int beta)
{
    int i, j, val;

    if (timedout)
        return 0;

    triangularLength[ply] = ply;


    // in-check extension (search one more ply when in check)
    if (isOwnKingAttacked())
        return alphabetapvs(ply, 1, alpha, beta);

    val = board.eval();
    if (val >= beta) return val;
    if (val > alpha) alpha = val;


    // generate captures & promotions:
    // captgen returns a sorted move list
    moveBufLen[ply+1] = captgen(moveBufLen[ply]);
    for (i = moveBufLen[ply]; i < moveBufLen[ply+1]; i++)
    {
        makeMove(moveBuffer[i]);
        {
            if (!isOtherKingAttacked()) 
            {
                inodes++;

                if (--countdown <=0)
                    readClockAndInput();

                val = -qsearch(ply+1, -beta, -alpha);
                unmakeMove(moveBuffer[i]);

                if (val >= beta)
                    return val;

                if (val > alpha)
                {
                    alpha = val;
                    triangularArray[ply][ply] = moveBuffer[i];
                    for (j = ply + 1; j < triangularLength[ply+1]; j++) 
                    {
                        triangularArray[ply][j] = triangularArray[ply+1][j];
                    }
                    triangularLength[ply] = triangularLength[ply+1];
                }
            }
            else unmakeMove(moveBuffer[i]);
        }
    }

    return alpha;
}



// Board::selectmove()
//
// Re-order the move list so that the best move is selected as the next move to try.
void Board::selectmove(int &ply, int &i, int &depth, bool &isFollowPV)
{
    int j, k;
    unsigned int best;
    Move temp;

    if (isFollowPV && depth > 1)
    {
        for (j = i; j < moveBufLen[ply+1]; j++)
        {
            if (moveBuffer[j].moveInt == lastPV[ply].moveInt)
            {
                temp.moveInt = moveBuffer[j].moveInt;
                moveBuffer[j].moveInt = moveBuffer[i].moveInt;
                moveBuffer[i].moveInt = temp.moveInt;
                return;
            }
        }
    }


    if (nextMove) 
    {
        best = blackHeuristics[moveBuffer[i].getFrom()][moveBuffer[i].getTosq()];
        j = i;
        for (k = i + 1; k < moveBufLen[ply+1]; k++)
        {
            if (blackHeuristics[moveBuffer[k].getFrom()][moveBuffer[k].getTosq()] > best)
            {
                best = blackHeuristics[moveBuffer[k].getFrom()][moveBuffer[k].getTosq()];
                j = k;                  
            }
        }
        if (j > i)
        {
            temp.moveInt = moveBuffer[j].moveInt;
            moveBuffer[j].moveInt = moveBuffer[i].moveInt;
            moveBuffer[i].moveInt = temp.moveInt;
        }
    }
    else
    {
        best = whiteHeuristics[moveBuffer[i].getFrom()][moveBuffer[i].getTosq()];
        j = i;
        for (k = i + 1; k < moveBufLen[ply+1]; k++)
        {
            if (whiteHeuristics[moveBuffer[k].getFrom()][moveBuffer[k].getTosq()] > best)
            {
                best = whiteHeuristics[moveBuffer[k].getFrom()][moveBuffer[k].getTosq()];
                j = k;                  
            }
        }
        if (j > i)
        {
            temp.moveInt = moveBuffer[j].moveInt;
            moveBuffer[j].moveInt = moveBuffer[i].moveInt;
            moveBuffer[i].moveInt = temp.moveInt;
        }
    }
}
