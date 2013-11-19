#------------------------------------------------------------------------------#
# This makefile was generated by 'cbp2make' tool rev.138                       #
#------------------------------------------------------------------------------#


WORKDIR = `pwd`

CC = gcc
CXX = g++
AR = ar
LD = g++
WINDRES = windres

INC =  -Iinclude
CFLAGS =  -Wall -fexceptions
RESINC = 
LIBDIR = 
LIB =  -ltinyxml -lfastdb
LDFLAGS = 

INC_DEBUG =  $(INC)
CFLAGS_DEBUG =  $(CFLAGS) -std=c++11 -g -DDEBUG
RESINC_DEBUG =  $(RESINC)
RCFLAGS_DEBUG =  $(RCFLAGS)
LIBDIR_DEBUG =  $(LIBDIR)
LIB_DEBUG = $(LIB)
LDFLAGS_DEBUG =  $(LDFLAGS)
OBJDIR_DEBUG = obj/Debug
DEP_DEBUG = 
OUT_DEBUG = bin/Debug/workerd

INC_RELEASE =  $(INC)
CFLAGS_RELEASE =  $(CFLAGS) -O2 -std=c++11
RESINC_RELEASE =  $(RESINC)
RCFLAGS_RELEASE =  $(RCFLAGS)
LIBDIR_RELEASE =  $(LIBDIR)
LIB_RELEASE = $(LIB)
LDFLAGS_RELEASE =  $(LDFLAGS) -s
OBJDIR_RELEASE = obj/Release
DEP_RELEASE = 
OUT_RELEASE = bin/Release/workerd

OBJ_DEBUG = $(OBJDIR_DEBUG)/main.o $(OBJDIR_DEBUG)/src/Config.o $(OBJDIR_DEBUG)/src/Log.o $(OBJDIR_DEBUG)/src/MainDb.o $(OBJDIR_DEBUG)/src/Message.o $(OBJDIR_DEBUG)/src/Queue.o $(OBJDIR_DEBUG)/src/UnixSocketServer.o $(OBJDIR_DEBUG)/src/Worker.o

OBJ_RELEASE = $(OBJDIR_RELEASE)/main.o $(OBJDIR_RELEASE)/src/Config.o $(OBJDIR_RELEASE)/src/Log.o $(OBJDIR_RELEASE)/src/MainDb.o $(OBJDIR_RELEASE)/src/Message.o $(OBJDIR_RELEASE)/src/Queue.o $(OBJDIR_RELEASE)/src/UnixSocketServer.o $(OBJDIR_RELEASE)/src/Worker.o

all: debug release

clean: clean_debug clean_release

before_debug: 
	test -d bin/Debug || mkdir -p bin/Debug
	test -d $(OBJDIR_DEBUG) || mkdir -p $(OBJDIR_DEBUG)
	test -d $(OBJDIR_DEBUG)/src || mkdir -p $(OBJDIR_DEBUG)/src

after_debug: 

debug: before_debug out_debug after_debug

out_debug: before_debug $(OBJ_DEBUG) $(DEP_DEBUG)
	$(LD) $(LIBDIR_DEBUG) -o $(OUT_DEBUG) $(OBJ_DEBUG)  $(LDFLAGS_DEBUG) $(LIB_DEBUG)

$(OBJDIR_DEBUG)/main.o: main.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c main.cpp -o $(OBJDIR_DEBUG)/main.o

$(OBJDIR_DEBUG)/src/Config.o: src/Config.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c src/Config.cpp -o $(OBJDIR_DEBUG)/src/Config.o

$(OBJDIR_DEBUG)/src/Log.o: src/Log.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c src/Log.cpp -o $(OBJDIR_DEBUG)/src/Log.o

$(OBJDIR_DEBUG)/src/MainDb.o: src/MainDb.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c src/MainDb.cpp -o $(OBJDIR_DEBUG)/src/MainDb.o

$(OBJDIR_DEBUG)/src/Message.o: src/Message.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c src/Message.cpp -o $(OBJDIR_DEBUG)/src/Message.o

$(OBJDIR_DEBUG)/src/Queue.o: src/Queue.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c src/Queue.cpp -o $(OBJDIR_DEBUG)/src/Queue.o

$(OBJDIR_DEBUG)/src/UnixSocketServer.o: src/UnixSocketServer.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c src/UnixSocketServer.cpp -o $(OBJDIR_DEBUG)/src/UnixSocketServer.o

$(OBJDIR_DEBUG)/src/Worker.o: src/Worker.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c src/Worker.cpp -o $(OBJDIR_DEBUG)/src/Worker.o

clean_debug: 
	rm -f $(OBJ_DEBUG) $(OUT_DEBUG)
	rm -rf bin/Debug
	rm -rf $(OBJDIR_DEBUG)
	rm -rf $(OBJDIR_DEBUG)/src

before_release: 
	test -d bin/Release || mkdir -p bin/Release
	test -d $(OBJDIR_RELEASE) || mkdir -p $(OBJDIR_RELEASE)
	test -d $(OBJDIR_RELEASE)/src || mkdir -p $(OBJDIR_RELEASE)/src

after_release: 

release: before_release out_release after_release

out_release: before_release $(OBJ_RELEASE) $(DEP_RELEASE)
	$(LD) $(LIBDIR_RELEASE) -o $(OUT_RELEASE) $(OBJ_RELEASE)  $(LDFLAGS_RELEASE) $(LIB_RELEASE)

$(OBJDIR_RELEASE)/main.o: main.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c main.cpp -o $(OBJDIR_RELEASE)/main.o

$(OBJDIR_RELEASE)/src/Config.o: src/Config.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c src/Config.cpp -o $(OBJDIR_RELEASE)/src/Config.o

$(OBJDIR_RELEASE)/src/Log.o: src/Log.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c src/Log.cpp -o $(OBJDIR_RELEASE)/src/Log.o

$(OBJDIR_RELEASE)/src/MainDb.o: src/MainDb.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c src/MainDb.cpp -o $(OBJDIR_RELEASE)/src/MainDb.o

$(OBJDIR_RELEASE)/src/Message.o: src/Message.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c src/Message.cpp -o $(OBJDIR_RELEASE)/src/Message.o

$(OBJDIR_RELEASE)/src/Queue.o: src/Queue.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c src/Queue.cpp -o $(OBJDIR_RELEASE)/src/Queue.o

$(OBJDIR_RELEASE)/src/UnixSocketServer.o: src/UnixSocketServer.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c src/UnixSocketServer.cpp -o $(OBJDIR_RELEASE)/src/UnixSocketServer.o

$(OBJDIR_RELEASE)/src/Worker.o: src/Worker.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c src/Worker.cpp -o $(OBJDIR_RELEASE)/src/Worker.o

clean_release: 
	rm -f $(OBJ_RELEASE) $(OUT_RELEASE)
	rm -rf bin/Release
	rm -rf $(OBJDIR_RELEASE)
	rm -rf $(OBJDIR_RELEASE)/src

.PHONY: before_debug after_debug clean_debug before_release after_release clean_release
