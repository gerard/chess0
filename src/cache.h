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



// @file cache.h
//
// XXX
#ifndef _CACHE_H_
#define _CACHE_H_


#include <unordered_map>



struct ttEntry
{
    int score = 0;
    int depth = -1;
};



class Cache
{
    private:
        std::unordered_map<uint64_t, ttEntry> cacheData;

    public:
        ttEntry  find(uint64_t, int);
        void     add(uint64_t, ttEntry);
        void     remove(uint64_t);
        void     clear();
        uint64_t size();
        uint64_t positions();
};



#endif // _CACHE_H_
