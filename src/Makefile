# This file is part of Chess0, a computer chess program based on Winglet Chess
# by Stef Luijten.
#    
# Copyright (C) 2017 Claudio M. Camacho
#
# Chess0 is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Chess0 is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.


### Establish the operating system name
UNAME = $(shell uname)

### Executable name
APP = chess0

### Installation dir definitions
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

### Object files
OBJS = app.o bit.o board.o cache.o cmd.o data.o displaymove.o hash.o make.o move.o movgen.o peek.o perft.o main.o search.o fen.o eval.o book.o see.o setup.o timer.o

### Compilation flags
CXXFLAGS += -g -O3 -Wall -Wcast-qual -fno-exceptions -fno-rtti -std=c++11 -m64 -flto -msse
DEPENDFLAGS += -std=c++11

### Build targets
all: $(APP) .depend

install:
	-mkdir -p -m 755 $(BINDIR)
	-cp $(APP) $(BINDIR)
	-strip $(BINDIR)/$(APP)

clean:
	$(RM) $(APP) $(APP).exe *.o .depend *~ 

$(APP): $(OBJS)
	$(CXX) -o $@ $(OBJS)

.depend:
	-@$(CXX) $(DEPENDFLAGS) -MM $(OBJS:.o=.cpp) > $@ 2> /dev/null

-include .depend
