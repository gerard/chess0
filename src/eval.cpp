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



// @file eval.cpp
//
// This file contains the necessary functions to evaluate the current board
// position (i.e., static evaluation) taking into account both material and
// position.
#include <iostream>
#include "definitions.h"
#include "functions.h"
#include "extglobals.h"
#include "board.h"



// Board::eval()
//
// This is Chess0's evaluation function, including both material and positional
// evaluation.
int Board::eval()
{
    int score, square;
    int whitepawns, whiteknights, whitebishops, whiterooks, whitequeens;
    int blackpawns, blackknights, blackbishops, blackrooks, blackqueens;
    int whitekingsquare, blackkingsquare;
    int whitetotalmat, blacktotalmat;
    int whitetotal, blacktotal;
    bool endgame;
    Bitboard temp, whitepassedpawns, blackpassedpawns;


    // Material
    score = board.Material;


    // Remember where the kings are
    whitekingsquare = firstOne(board.whiteKing);
    blackkingsquare = firstOne(board.blackKing);


    // Piece counts, note that we could have done this incrementally in (un)makeMove
    // because it's basically the same thing as keeping board.Material up to date..
    whitepawns = bitCnt(board.whitePawns);
    whiteknights = bitCnt(board.whiteKnights);
    whitebishops = bitCnt(board.whiteBishops);
    whiterooks = bitCnt(board.whiteRooks);
    whitequeens = bitCnt(board.whiteQueens);
    whitetotalmat = 3 * whiteknights + 3 * whitebishops + 5 * whiterooks + 10 * whitequeens;
    whitetotal = whitepawns + whiteknights + whitebishops + whiterooks + whitequeens;
    blackpawns = bitCnt(board.blackPawns);
    blackknights = bitCnt(board.blackKnights);
    blackbishops = bitCnt(board.blackBishops);
    blackrooks = bitCnt(board.blackRooks);
    blackqueens = bitCnt(board.blackQueens);
    blacktotalmat = 3 * blackknights + 3 * blackbishops + 5 * blackrooks + 10 * blackqueens;
    blacktotal = blackpawns + blackknights + blackbishops + blackrooks + blackqueens;


    // Check if we are in the endgame
    // Anything less than a queen (=10) + rook (=5) is considered endgame
    // (pawns excluded in this count)
    endgame = (whitetotalmat < 15 || blacktotalmat < 15);


    // Evaluate for draws due to insufficient material:
    if (!whitepawns && !blackpawns)
    {
        // king versus king:
        if ((whitetotalmat == 0) && (blacktotalmat == 0)) 
        {
            if (board.nextMove) return -DRAWSCORE;
            else return DRAWSCORE;
        }

        // king and knight versus king:
        if (((whitetotalmat == 3) && (whiteknights == 1) && (blacktotalmat == 0)) ||
                ((blacktotalmat == 3) && (blackknights == 1) && (whitetotalmat == 0))) 
        {
            if (board.nextMove) return -DRAWSCORE;
            else return DRAWSCORE;
        }

        // 2 kings with one or more bishops, and all bishops on the same colour:
        if ((whitebishops + blackbishops) > 0)
        {
            if ((whiteknights == 0) && (whiterooks == 0) && (whitequeens == 0) &&
                    (blackknights == 0) && (blackrooks == 0) && (blackqueens == 0))
            {
                if (!((board.whiteBishops | board.blackBishops) & WHITE_SQUARES) ||
                        !((board.whiteBishops | board.blackBishops) & BLACK_SQUARES))
                {
                    if (board.nextMove) return -DRAWSCORE;
                    else return DRAWSCORE;
                }
            }
        }
    }


    // Evaluate MATERIAL

    // Have the winning side prefer to exchange pieces
    // Every exchange with unequal material adds 3 centipawns to the score
    // Loosing a piece (from balanced material) becomes more
    // severe in the endgame
    if (whitetotalmat + whitepawns > blacktotalmat + blackpawns)
        score += 45 + 3 * whitetotal - 6 * blacktotal;

    else if (whitetotalmat + whitepawns < blacktotalmat + blackpawns)
        score += -45 - 3 * blacktotal + 6 * whitetotal;


    // Evaluate WHITE PIECES

    // Evaluate white pawns
    // - position on the board
    // - distance from opponent king
    // - distance from own king
    // - passed, doubled, isolated or backward pawns
    whitepassedpawns = 0;
    temp = board.whitePawns;
    while (temp)
    {
        square = firstOne(temp);
        score += PAWNPOS_W[square];
        score += PAWN_OPPONENT_DISTANCE[DISTANCE[square][blackkingsquare]];

        if (endgame)
            score += PAWN_OWN_DISTANCE[DISTANCE[square][whitekingsquare]];

        if (!(PASSED_WHITE[square] & board.blackPawns))
        {
            score += BONUS_PASSED_PAWN;

            // remember its location, we need it later when evaluating the white rooks:
            whitepassedpawns ^= BITSET[square];
        }

        if ((board.whitePawns ^ BITSET[square]) & FILEMASK[square])
            score -= PENALTY_DOUBLED_PAWN;

        if (!(ISOLATED_WHITE[square] & board.whitePawns))
        {
            score -= PENALTY_ISOLATED_PAWN;
        }


        // If it is not isolated, then it might be backward. Two conditions must be true:
        //  1) if the next square is controlled by an enemy pawn - we use the PAWN_ATTACKS Bitboards to check this
        //  2) if there are no pawns left that could defend this pawn
        else
        {
            if ((WHITE_PAWN_ATTACKS[square + 8] & board.blackPawns))
                if (!(BACKWARD_WHITE[square] & board.whitePawns))
                    score -= PENALTY_BACKWARD_PAWN;
        }

        temp ^= BITSET[square];
    }


    // Evaluate white knights
    // - position on the board
    // - distance from opponent king
    temp = board.whiteKnights;
    while (temp)
    {
        square = firstOne(temp);
        score += KNIGHTPOS_W[square];
        score += KNIGHT_DISTANCE[DISTANCE[square][blackkingsquare]];
        temp ^= BITSET[square];
    }


    // Evaluate white bishops
    // - having the pair
    // - position on the board
    // - distance from opponent king
    if (board.whiteBishops)
        if ((board.whiteBishops & WHITE_SQUARES) && (board.whiteBishops & BLACK_SQUARES))
            score += BONUS_BISHOP_PAIR;

    temp = board.whiteBishops;
    while (temp)
    {
        square = firstOne(temp);
        score += BISHOPPOS_W[square];
        score += BISHOP_DISTANCE[DISTANCE[square][blackkingsquare]];
        temp ^= BITSET[square];
    }


    // Evaluate white rooks
    // - position on the board
    // - distance from opponent king
    // - on the same file as a passed pawn
    temp = board.whiteRooks;
    while (temp)
    {
        square = firstOne(temp);
        score += ROOKPOS_W[square];
        if (!board.castleWhite)
            score += ROOK_DISTANCE[DISTANCE[square][blackkingsquare]];

        if (FILEMASK[square] & whitepassedpawns)
            if ((unsigned int) square < lastOne(FILEMASK[square] & whitepassedpawns))
                score += BONUS_ROOK_BEHIND_PASSED_PAWN;

        temp ^= BITSET[square];
    }


    // Evaluate white queens
    // - position on the board
    // - distance from opponent king
    temp = board.whiteQueens;
    while (temp)
    {
        square = firstOne(temp);
        score += QUEENPOS_W[square];
        score += QUEEN_DISTANCE[DISTANCE[square][blackkingsquare]];
        temp ^= BITSET[square];
    }


    // Evaluate the white king
    // - position on the board
    // - proximity to the pawns
    // - pawn shield (not in the endgame)
    if (endgame)
    {
        score += KINGPOS_ENDGAME_W[whitekingsquare];
    }
    else
    {
        score += KINGPOS_W[whitekingsquare];

        // add pawn shield bonus if we're not in the endgame:
        // strong pawn shield bonus if the pawns are near the king:
        score += BONUS_PAWN_SHIELD_STRONG * bitCnt(KINGSHIELD_STRONG_W[whitekingsquare] & board.whitePawns);

        // weaker pawn shield bonus if the pawns are not so near the king:
        score += BONUS_PAWN_SHIELD_WEAK * bitCnt(KINGSHIELD_WEAK_W[whitekingsquare] & board.whitePawns);
    }


    // Evaluate BLACK PIECES

    // Evaluate black pawns
    // - position on the board
    // - distance from opponent king
    // - distance from own king
    // - passed, doubled, isolated or backward pawns
    blackpassedpawns = 0;
    temp = board.blackPawns;
    while (temp)
    {
        square = firstOne(temp);
        score -= PAWNPOS_B[square];
        score -= PAWN_OPPONENT_DISTANCE[DISTANCE[square][whitekingsquare]];

        if (endgame)
            score -= PAWN_OWN_DISTANCE[DISTANCE[square][blackkingsquare]];

        if (!(PASSED_BLACK[square] & board.whitePawns))
        {
            score -= BONUS_PASSED_PAWN;

            // remember its location, we need it later when evaluating the black rooks:
            blackpassedpawns ^= BITSET[square];
        }

        if ((board.blackPawns ^ BITSET[square]) & FILEMASK[square])
            score += PENALTY_DOUBLED_PAWN;

        if (!(ISOLATED_BLACK[square] & board.blackPawns))
        {
            score += PENALTY_ISOLATED_PAWN;
        }


        // If it is not isolated, then it might be backward. Two conditions must be true:
        //  1) if the next square is controlled by an enemy pawn - we use the PAWN_ATTACKS Bitboards to check this
        //  2) if there are no pawns left that could defend this pawn
        else
        {
            if ((BLACK_PAWN_ATTACKS[square - 8] & board.whitePawns))
                if (!(BACKWARD_BLACK[square] & board.blackPawns))
                    score += PENALTY_BACKWARD_PAWN;
        }

        temp ^= BITSET[square];

    }


    // Evaluate black knights
    // - position on the board
    // - distance from opponent king
    temp = board.blackKnights;
    while (temp)
    {
        square = firstOne(temp);
        score -= KNIGHTPOS_B[square];
        score -= KNIGHT_DISTANCE[DISTANCE[square][whitekingsquare]];
        temp ^= BITSET[square];
    }


    // Evaluate black bishops
    // - having the pair
    // - position on the board
    // - distance from opponent king
    if (board.blackBishops)
        if ((board.blackBishops & WHITE_SQUARES) && (board.blackBishops & BLACK_SQUARES))
            score -= BONUS_BISHOP_PAIR;

    temp = board.blackBishops;
    while (temp)
    {
        square = firstOne(temp);
        score -= BISHOPPOS_B[square];
        score -= BISHOP_DISTANCE[DISTANCE[square][whitekingsquare]];
        temp ^= BITSET[square];
    }


    // Evaluate black rooks
    // - position on the board
    // - distance from opponent king
    // - on the same file as a passed pawn
    temp = board.blackRooks;
    while (temp)
    {
        square = firstOne(temp);
        score -= ROOKPOS_B[square];
        if (!board.castleBlack)
            score -= ROOK_DISTANCE[DISTANCE[square][whitekingsquare]];

        if (FILEMASK[square] & blackpassedpawns)
            if ((unsigned int) square > firstOne(FILEMASK[square] & blackpassedpawns))
                score -= BONUS_ROOK_BEHIND_PASSED_PAWN;

        temp ^= BITSET[square];
    }


    // Evaluate black queens
    // - position on the board
    // - distance from opponent king
    temp = board.blackQueens;
    while (temp)
    {
        square = firstOne(temp);
        score -= QUEENPOS_B[square];
        score -= QUEEN_DISTANCE[DISTANCE[square][whitekingsquare]];
        temp ^= BITSET[square];
    }


    // Evaluate the black king
    // - position on the board
    // - proximity to the pawns
    // - pawn shield (not in the endgame)
    if (endgame)
    {
        score -= KINGPOS_ENDGAME_B[blackkingsquare];
    }
    else
    {
        score -= KINGPOS_B[blackkingsquare];

        // add pawn shield bonus if we're not in the endgame:
        // strong pawn shield bonus if the pawns are near the king:
        score -= BONUS_PAWN_SHIELD_STRONG * bitCnt(KINGSHIELD_STRONG_B[blackkingsquare] & board.blackPawns);

        // weaker pawn shield bonus if the pawns are not so near the king:
        score -= BONUS_PAWN_SHIELD_WEAK * bitCnt(KINGSHIELD_WEAK_B[blackkingsquare] & board.blackPawns);
    }

    // Bonus for tempo
    int tempoBonus = 0;
    if (endgame)
        tempoBonus = BONUS_TEMPO_ENDGAME;
    else
        tempoBonus = BONUS_TEMPO_MIDGAME;
    if (board.nextMove)
        score -= tempoBonus;
    else
        score += tempoBonus;


    // Return the score
    if (board.nextMove)
        return -score;
    else
        return score;
}
