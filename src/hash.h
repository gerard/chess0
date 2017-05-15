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



// @file hash.h
//
// This file contains the definition of Hash table for caching different
// positions during move searches.
#ifndef _HASH_H_
#define _HASH_H_



#include "definitions.h"



// random  64-bit keys to give every position an 'almost' unique signature:
struct HashKeys
{
	// total size = 1093 * 8 = 8744 bytes (minimum required is 6312):
	U64 keys[64][16];  // position, piece (only 12 out of 16 piece are values used)
	U64 side;          // side to move (black)
	U64 ep[64];        // ep targets (only 16 used)
	U64 wk;            // white king-side castling right
	U64 wq;            // white queen-side castling right
	U64 bk;            // black king-side castling right
	U64 bq;            // black queen-side castling right

	void init();       // initialize the random data
	U64 rand64();      // 64-bit random number generator
};

#endif // _HASH_H_
