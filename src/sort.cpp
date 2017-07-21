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



// @file sort.cpp
//
// XXX
#include <iostream>
#include "definitions.h"
#include "extglobals.h"
#include "functions.h"
#include "board.h"



/*!
 * Re-order the move list so that the best move is selected as the next move to try.
 */
void Board::selectmove(int &ply, int &i, int &depth, bool &followpv)
{
    int j, k;
    unsigned int best;
    Move temp;

    if (followpv && depth > 1)
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
