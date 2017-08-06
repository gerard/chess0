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



// @file main.cpp
//
// This file contains the main routine to get the application started and
// accepting input from the user.
#include <signal.h>
#include <unistd.h>
#include <iostream>
#include <thread>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <time.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <assert.h>
#include <math.h>
#include <sys/timeb.h>


#include "definitions.h"
#include "functions.h"
#include "app.h"



using namespace std;



/*!
 * Main function that is executed when Chess0 is launched.
 */
int main(int argc, char *argv[])
{
    // don't quit when xboard sends SIGINT */
    if(!isatty(STDIN_FILENO))
        signal(SIGINT, SIG_IGN);



    /*!
     * Check if the program is called in XBoard/Winboard mode (protocol
     * version 1). In version 2 of the protocol, the "xboard" parameter is
     * entered once the program is up and running. However, in order to support
     * version 1 of the XBoard protocol, we need to read the calling arguments
     * of the program and detect whether 'xboard' is one of them.
     */
    if ((argc >= 2) && (strcmp(argv[1], "xboard") == 0))
    {
        return startApp(APP_MODE_XBOARD);
    }
    else
    {
        return startApp(APP_MODE_CLI);
    }


    return 0;
}
