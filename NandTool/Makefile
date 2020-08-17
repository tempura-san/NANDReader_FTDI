CC = /usr/bin/g++
LD = /usr/bin/g++
RM = /bin/rm -f

LIBNAME=libftdi1

CFLAGS = -c -O3 -g -ggdb -Wall -Wextra
CFLAGS += $(shell pkg-config --cflags $(LIBNAME))

# uncomment next line for debugging output
#CFLAGS += -DDEBUG

LDFLAGS = $(shell pkg-config --libs $(LIBNAME))

BIN = NandTool
SRCS = FtdiNand.cpp  NandChip.cpp  NandData.cpp  NandDataLP.cpp  NandDataSP.cpp  NandID.cpp  NandTool.cpp  stdafx.cpp
OBJS := $(SRCS:.cpp=.o)

all: $(BIN)

.cpp.o: %.cpp
	$(CC) $(CFLAGS) $< -o $@

$(BIN): $(OBJS)
	$(LD) -o $(BIN) $(OBJS) $(LDFLAGS)

clean:
	$(RM) core $(OBJS) $(BIN)
