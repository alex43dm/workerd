#------------------------------------------------------------------------------#
# This makefile was generated by 'cbp2make' tool rev.138                       #
#------------------------------------------------------------------------------#


WORKDIR = `pwd`

CC = gcc
CXX = g++
AR = ar
LD = g++
WINDRES = windres

INC =  -Iinclude -I.
CFLAGS =  -std=c++0x -Wall -fexceptions
RESINC = 
LIBDIR = 
LIB =  -lmongoclient -lfcgi -lsphinxclient -lGeoIP -lrabbitmq -lboost_date_time -lboost_regex -lboost_filesystem -lboost_thread -lboost_system -ltinyxml -lsqlite3 -lpthread
LDFLAGS =  /usr/lib/libamqpcpp.a

INC_DEBUG =  $(INC) -Iinclude -I../libredis/
CFLAGS_DEBUG =  $(CFLAGS) -g -DDEBUG
RESINC_DEBUG =  $(RESINC)
RCFLAGS_DEBUG =  $(RCFLAGS)
LIBDIR_DEBUG =  $(LIBDIR)
LIB_DEBUG = $(LIB) ../libredis/lib/libredis.a -lrt
LDFLAGS_DEBUG =  $(LDFLAGS)
OBJDIR_DEBUG = obj/Debug
DEP_DEBUG = 
OUT_DEBUG = bin/Debug/getmyad

INC_RELEASE =  $(INC) -Iinclude -I../libredis
CFLAGS_RELEASE =  $(CFLAGS) -O2
RESINC_RELEASE =  $(RESINC)
RCFLAGS_RELEASE =  $(RCFLAGS)
LIBDIR_RELEASE =  $(LIBDIR)
LIB_RELEASE = $(LIB) ../libredis/lib/libredis.a -lrt
LDFLAGS_RELEASE =  $(LDFLAGS) -s
OBJDIR_RELEASE = obj/Release
DEP_RELEASE = 
OUT_RELEASE = bin/Release/getmyad

OBJ_DEBUG = $(OBJDIR_DEBUG)/src/HistoryManagerLongTerm.o $(OBJDIR_DEBUG)/src/GeoRerions.o $(OBJDIR_DEBUG)/src/EBranch.o $(OBJDIR_DEBUG)/main.o $(OBJDIR_DEBUG)/src/HistoryManagerOffer.o $(OBJDIR_DEBUG)/utils/base64.o $(OBJDIR_DEBUG)/utils/UrlParser.o $(OBJDIR_DEBUG)/utils/SearchEngines.o $(OBJDIR_DEBUG)/utils/GeoIPTools.o $(OBJDIR_DEBUG)/utils/Cookie.o $(OBJDIR_DEBUG)/src/sphinxRequests.o $(OBJDIR_DEBUG)/src/json.o $(OBJDIR_DEBUG)/src/XXXSearcher.o $(OBJDIR_DEBUG)/src/Server.o $(OBJDIR_DEBUG)/src/RedisClient.o $(OBJDIR_DEBUG)/src/ParentDB.o $(OBJDIR_DEBUG)/src/ParamParse.o $(OBJDIR_DEBUG)/src/HistoryManagerShortTerm.o $(OBJDIR_DEBUG)/src/HistoryManagerRetargeting.o $(OBJDIR_DEBUG)/src/HistoryManagerPageKeyWords.o $(OBJDIR_DEBUG)/KompexSQLiteStatement.o $(OBJDIR_DEBUG)/KompexSQLiteDatabase.o $(OBJDIR_DEBUG)/InformerTemplate.o $(OBJDIR_DEBUG)/Informer.o $(OBJDIR_DEBUG)/HistoryManager.o $(OBJDIR_DEBUG)/Log.o $(OBJDIR_DEBUG)/DataBase.o $(OBJDIR_DEBUG)/DB.o $(OBJDIR_DEBUG)/Core.o $(OBJDIR_DEBUG)/Config.o $(OBJDIR_DEBUG)/CgiService.o $(OBJDIR_DEBUG)/Campaign.o $(OBJDIR_DEBUG)/BaseCore.o $(OBJDIR_DEBUG)/Params.o $(OBJDIR_DEBUG)/Offer.o

OBJ_RELEASE = $(OBJDIR_RELEASE)/src/HistoryManagerLongTerm.o $(OBJDIR_RELEASE)/src/GeoRerions.o $(OBJDIR_RELEASE)/src/EBranch.o $(OBJDIR_RELEASE)/main.o $(OBJDIR_RELEASE)/src/HistoryManagerOffer.o $(OBJDIR_RELEASE)/utils/base64.o $(OBJDIR_RELEASE)/utils/UrlParser.o $(OBJDIR_RELEASE)/utils/SearchEngines.o $(OBJDIR_RELEASE)/utils/GeoIPTools.o $(OBJDIR_RELEASE)/utils/Cookie.o $(OBJDIR_RELEASE)/src/sphinxRequests.o $(OBJDIR_RELEASE)/src/json.o $(OBJDIR_RELEASE)/src/XXXSearcher.o $(OBJDIR_RELEASE)/src/Server.o $(OBJDIR_RELEASE)/src/RedisClient.o $(OBJDIR_RELEASE)/src/ParentDB.o $(OBJDIR_RELEASE)/src/ParamParse.o $(OBJDIR_RELEASE)/src/HistoryManagerShortTerm.o $(OBJDIR_RELEASE)/src/HistoryManagerRetargeting.o $(OBJDIR_RELEASE)/src/HistoryManagerPageKeyWords.o $(OBJDIR_RELEASE)/KompexSQLiteStatement.o $(OBJDIR_RELEASE)/KompexSQLiteDatabase.o $(OBJDIR_RELEASE)/InformerTemplate.o $(OBJDIR_RELEASE)/Informer.o $(OBJDIR_RELEASE)/HistoryManager.o $(OBJDIR_RELEASE)/Log.o $(OBJDIR_RELEASE)/DataBase.o $(OBJDIR_RELEASE)/DB.o $(OBJDIR_RELEASE)/Core.o $(OBJDIR_RELEASE)/Config.o $(OBJDIR_RELEASE)/CgiService.o $(OBJDIR_RELEASE)/Campaign.o $(OBJDIR_RELEASE)/BaseCore.o $(OBJDIR_RELEASE)/Params.o $(OBJDIR_RELEASE)/Offer.o

all: debug release

clean: clean_debug clean_release

before_debug: 
	test -d bin/Debug || mkdir -p bin/Debug
	test -d $(OBJDIR_DEBUG)/src || mkdir -p $(OBJDIR_DEBUG)/src
	test -d $(OBJDIR_DEBUG) || mkdir -p $(OBJDIR_DEBUG)
	test -d $(OBJDIR_DEBUG)/utils || mkdir -p $(OBJDIR_DEBUG)/utils

after_debug: 

debug: before_debug out_debug after_debug

out_debug: before_debug $(OBJ_DEBUG) $(DEP_DEBUG)
	$(LD) $(LIBDIR_DEBUG) -o $(OUT_DEBUG) $(OBJ_DEBUG)  $(LDFLAGS_DEBUG) $(LIB_DEBUG)

$(OBJDIR_DEBUG)/src/HistoryManagerLongTerm.o: src/HistoryManagerLongTerm.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c src/HistoryManagerLongTerm.cpp -o $(OBJDIR_DEBUG)/src/HistoryManagerLongTerm.o

$(OBJDIR_DEBUG)/src/GeoRerions.o: src/GeoRerions.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c src/GeoRerions.cpp -o $(OBJDIR_DEBUG)/src/GeoRerions.o

$(OBJDIR_DEBUG)/src/EBranch.o: src/EBranch.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c src/EBranch.cpp -o $(OBJDIR_DEBUG)/src/EBranch.o

$(OBJDIR_DEBUG)/main.o: main.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c main.cpp -o $(OBJDIR_DEBUG)/main.o

$(OBJDIR_DEBUG)/src/HistoryManagerOffer.o: src/HistoryManagerOffer.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c src/HistoryManagerOffer.cpp -o $(OBJDIR_DEBUG)/src/HistoryManagerOffer.o

$(OBJDIR_DEBUG)/utils/base64.o: utils/base64.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c utils/base64.cpp -o $(OBJDIR_DEBUG)/utils/base64.o

$(OBJDIR_DEBUG)/utils/UrlParser.o: utils/UrlParser.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c utils/UrlParser.cpp -o $(OBJDIR_DEBUG)/utils/UrlParser.o

$(OBJDIR_DEBUG)/utils/SearchEngines.o: utils/SearchEngines.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c utils/SearchEngines.cpp -o $(OBJDIR_DEBUG)/utils/SearchEngines.o

$(OBJDIR_DEBUG)/utils/GeoIPTools.o: utils/GeoIPTools.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c utils/GeoIPTools.cpp -o $(OBJDIR_DEBUG)/utils/GeoIPTools.o

$(OBJDIR_DEBUG)/utils/Cookie.o: utils/Cookie.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c utils/Cookie.cpp -o $(OBJDIR_DEBUG)/utils/Cookie.o

$(OBJDIR_DEBUG)/src/sphinxRequests.o: src/sphinxRequests.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c src/sphinxRequests.cpp -o $(OBJDIR_DEBUG)/src/sphinxRequests.o

$(OBJDIR_DEBUG)/src/json.o: src/json.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c src/json.cpp -o $(OBJDIR_DEBUG)/src/json.o

$(OBJDIR_DEBUG)/src/XXXSearcher.o: src/XXXSearcher.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c src/XXXSearcher.cpp -o $(OBJDIR_DEBUG)/src/XXXSearcher.o

$(OBJDIR_DEBUG)/src/Server.o: src/Server.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c src/Server.cpp -o $(OBJDIR_DEBUG)/src/Server.o

$(OBJDIR_DEBUG)/src/RedisClient.o: src/RedisClient.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c src/RedisClient.cpp -o $(OBJDIR_DEBUG)/src/RedisClient.o

$(OBJDIR_DEBUG)/src/ParentDB.o: src/ParentDB.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c src/ParentDB.cpp -o $(OBJDIR_DEBUG)/src/ParentDB.o

$(OBJDIR_DEBUG)/src/ParamParse.o: src/ParamParse.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c src/ParamParse.cpp -o $(OBJDIR_DEBUG)/src/ParamParse.o

$(OBJDIR_DEBUG)/src/HistoryManagerShortTerm.o: src/HistoryManagerShortTerm.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c src/HistoryManagerShortTerm.cpp -o $(OBJDIR_DEBUG)/src/HistoryManagerShortTerm.o

$(OBJDIR_DEBUG)/src/HistoryManagerRetargeting.o: src/HistoryManagerRetargeting.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c src/HistoryManagerRetargeting.cpp -o $(OBJDIR_DEBUG)/src/HistoryManagerRetargeting.o

$(OBJDIR_DEBUG)/src/HistoryManagerPageKeyWords.o: src/HistoryManagerPageKeyWords.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c src/HistoryManagerPageKeyWords.cpp -o $(OBJDIR_DEBUG)/src/HistoryManagerPageKeyWords.o

$(OBJDIR_DEBUG)/KompexSQLiteStatement.o: KompexSQLiteStatement.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c KompexSQLiteStatement.cpp -o $(OBJDIR_DEBUG)/KompexSQLiteStatement.o

$(OBJDIR_DEBUG)/KompexSQLiteDatabase.o: KompexSQLiteDatabase.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c KompexSQLiteDatabase.cpp -o $(OBJDIR_DEBUG)/KompexSQLiteDatabase.o

$(OBJDIR_DEBUG)/InformerTemplate.o: InformerTemplate.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c InformerTemplate.cpp -o $(OBJDIR_DEBUG)/InformerTemplate.o

$(OBJDIR_DEBUG)/Informer.o: Informer.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c Informer.cpp -o $(OBJDIR_DEBUG)/Informer.o

$(OBJDIR_DEBUG)/HistoryManager.o: HistoryManager.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c HistoryManager.cpp -o $(OBJDIR_DEBUG)/HistoryManager.o

$(OBJDIR_DEBUG)/Log.o: Log.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c Log.cpp -o $(OBJDIR_DEBUG)/Log.o

$(OBJDIR_DEBUG)/DataBase.o: DataBase.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c DataBase.cpp -o $(OBJDIR_DEBUG)/DataBase.o

$(OBJDIR_DEBUG)/DB.o: DB.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c DB.cpp -o $(OBJDIR_DEBUG)/DB.o

$(OBJDIR_DEBUG)/Core.o: Core.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c Core.cpp -o $(OBJDIR_DEBUG)/Core.o

$(OBJDIR_DEBUG)/Config.o: Config.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c Config.cpp -o $(OBJDIR_DEBUG)/Config.o

$(OBJDIR_DEBUG)/CgiService.o: CgiService.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c CgiService.cpp -o $(OBJDIR_DEBUG)/CgiService.o

$(OBJDIR_DEBUG)/Campaign.o: Campaign.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c Campaign.cpp -o $(OBJDIR_DEBUG)/Campaign.o

$(OBJDIR_DEBUG)/BaseCore.o: BaseCore.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c BaseCore.cpp -o $(OBJDIR_DEBUG)/BaseCore.o

$(OBJDIR_DEBUG)/Params.o: Params.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c Params.cpp -o $(OBJDIR_DEBUG)/Params.o

$(OBJDIR_DEBUG)/Offer.o: Offer.cpp
	$(CXX) $(CFLAGS_DEBUG) $(INC_DEBUG) -c Offer.cpp -o $(OBJDIR_DEBUG)/Offer.o

clean_debug: 
	rm -f $(OBJ_DEBUG) $(OUT_DEBUG)
	rm -rf bin/Debug
	rm -rf $(OBJDIR_DEBUG)/src
	rm -rf $(OBJDIR_DEBUG)
	rm -rf $(OBJDIR_DEBUG)/utils

before_release: 
	test -d bin/Release || mkdir -p bin/Release
	test -d $(OBJDIR_RELEASE)/src || mkdir -p $(OBJDIR_RELEASE)/src
	test -d $(OBJDIR_RELEASE) || mkdir -p $(OBJDIR_RELEASE)
	test -d $(OBJDIR_RELEASE)/utils || mkdir -p $(OBJDIR_RELEASE)/utils

after_release: 

release: before_release out_release after_release

out_release: before_release $(OBJ_RELEASE) $(DEP_RELEASE)
	$(LD) $(LIBDIR_RELEASE) -o $(OUT_RELEASE) $(OBJ_RELEASE)  $(LDFLAGS_RELEASE) $(LIB_RELEASE)

$(OBJDIR_RELEASE)/src/HistoryManagerLongTerm.o: src/HistoryManagerLongTerm.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c src/HistoryManagerLongTerm.cpp -o $(OBJDIR_RELEASE)/src/HistoryManagerLongTerm.o

$(OBJDIR_RELEASE)/src/GeoRerions.o: src/GeoRerions.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c src/GeoRerions.cpp -o $(OBJDIR_RELEASE)/src/GeoRerions.o

$(OBJDIR_RELEASE)/src/EBranch.o: src/EBranch.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c src/EBranch.cpp -o $(OBJDIR_RELEASE)/src/EBranch.o

$(OBJDIR_RELEASE)/main.o: main.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c main.cpp -o $(OBJDIR_RELEASE)/main.o

$(OBJDIR_RELEASE)/src/HistoryManagerOffer.o: src/HistoryManagerOffer.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c src/HistoryManagerOffer.cpp -o $(OBJDIR_RELEASE)/src/HistoryManagerOffer.o

$(OBJDIR_RELEASE)/utils/base64.o: utils/base64.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c utils/base64.cpp -o $(OBJDIR_RELEASE)/utils/base64.o

$(OBJDIR_RELEASE)/utils/UrlParser.o: utils/UrlParser.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c utils/UrlParser.cpp -o $(OBJDIR_RELEASE)/utils/UrlParser.o

$(OBJDIR_RELEASE)/utils/SearchEngines.o: utils/SearchEngines.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c utils/SearchEngines.cpp -o $(OBJDIR_RELEASE)/utils/SearchEngines.o

$(OBJDIR_RELEASE)/utils/GeoIPTools.o: utils/GeoIPTools.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c utils/GeoIPTools.cpp -o $(OBJDIR_RELEASE)/utils/GeoIPTools.o

$(OBJDIR_RELEASE)/utils/Cookie.o: utils/Cookie.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c utils/Cookie.cpp -o $(OBJDIR_RELEASE)/utils/Cookie.o

$(OBJDIR_RELEASE)/src/sphinxRequests.o: src/sphinxRequests.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c src/sphinxRequests.cpp -o $(OBJDIR_RELEASE)/src/sphinxRequests.o

$(OBJDIR_RELEASE)/src/json.o: src/json.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c src/json.cpp -o $(OBJDIR_RELEASE)/src/json.o

$(OBJDIR_RELEASE)/src/XXXSearcher.o: src/XXXSearcher.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c src/XXXSearcher.cpp -o $(OBJDIR_RELEASE)/src/XXXSearcher.o

$(OBJDIR_RELEASE)/src/Server.o: src/Server.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c src/Server.cpp -o $(OBJDIR_RELEASE)/src/Server.o

$(OBJDIR_RELEASE)/src/RedisClient.o: src/RedisClient.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c src/RedisClient.cpp -o $(OBJDIR_RELEASE)/src/RedisClient.o

$(OBJDIR_RELEASE)/src/ParentDB.o: src/ParentDB.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c src/ParentDB.cpp -o $(OBJDIR_RELEASE)/src/ParentDB.o

$(OBJDIR_RELEASE)/src/ParamParse.o: src/ParamParse.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c src/ParamParse.cpp -o $(OBJDIR_RELEASE)/src/ParamParse.o

$(OBJDIR_RELEASE)/src/HistoryManagerShortTerm.o: src/HistoryManagerShortTerm.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c src/HistoryManagerShortTerm.cpp -o $(OBJDIR_RELEASE)/src/HistoryManagerShortTerm.o

$(OBJDIR_RELEASE)/src/HistoryManagerRetargeting.o: src/HistoryManagerRetargeting.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c src/HistoryManagerRetargeting.cpp -o $(OBJDIR_RELEASE)/src/HistoryManagerRetargeting.o

$(OBJDIR_RELEASE)/src/HistoryManagerPageKeyWords.o: src/HistoryManagerPageKeyWords.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c src/HistoryManagerPageKeyWords.cpp -o $(OBJDIR_RELEASE)/src/HistoryManagerPageKeyWords.o

$(OBJDIR_RELEASE)/KompexSQLiteStatement.o: KompexSQLiteStatement.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c KompexSQLiteStatement.cpp -o $(OBJDIR_RELEASE)/KompexSQLiteStatement.o

$(OBJDIR_RELEASE)/KompexSQLiteDatabase.o: KompexSQLiteDatabase.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c KompexSQLiteDatabase.cpp -o $(OBJDIR_RELEASE)/KompexSQLiteDatabase.o

$(OBJDIR_RELEASE)/InformerTemplate.o: InformerTemplate.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c InformerTemplate.cpp -o $(OBJDIR_RELEASE)/InformerTemplate.o

$(OBJDIR_RELEASE)/Informer.o: Informer.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c Informer.cpp -o $(OBJDIR_RELEASE)/Informer.o

$(OBJDIR_RELEASE)/HistoryManager.o: HistoryManager.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c HistoryManager.cpp -o $(OBJDIR_RELEASE)/HistoryManager.o

$(OBJDIR_RELEASE)/Log.o: Log.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c Log.cpp -o $(OBJDIR_RELEASE)/Log.o

$(OBJDIR_RELEASE)/DataBase.o: DataBase.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c DataBase.cpp -o $(OBJDIR_RELEASE)/DataBase.o

$(OBJDIR_RELEASE)/DB.o: DB.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c DB.cpp -o $(OBJDIR_RELEASE)/DB.o

$(OBJDIR_RELEASE)/Core.o: Core.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c Core.cpp -o $(OBJDIR_RELEASE)/Core.o

$(OBJDIR_RELEASE)/Config.o: Config.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c Config.cpp -o $(OBJDIR_RELEASE)/Config.o

$(OBJDIR_RELEASE)/CgiService.o: CgiService.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c CgiService.cpp -o $(OBJDIR_RELEASE)/CgiService.o

$(OBJDIR_RELEASE)/Campaign.o: Campaign.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c Campaign.cpp -o $(OBJDIR_RELEASE)/Campaign.o

$(OBJDIR_RELEASE)/BaseCore.o: BaseCore.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c BaseCore.cpp -o $(OBJDIR_RELEASE)/BaseCore.o

$(OBJDIR_RELEASE)/Params.o: Params.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c Params.cpp -o $(OBJDIR_RELEASE)/Params.o

$(OBJDIR_RELEASE)/Offer.o: Offer.cpp
	$(CXX) $(CFLAGS_RELEASE) $(INC_RELEASE) -c Offer.cpp -o $(OBJDIR_RELEASE)/Offer.o

clean_release: 
	rm -f $(OBJ_RELEASE) $(OUT_RELEASE)
	rm -rf bin/Release
	rm -rf $(OBJDIR_RELEASE)/src
	rm -rf $(OBJDIR_RELEASE)
	rm -rf $(OBJDIR_RELEASE)/utils

.PHONY: before_debug after_debug clean_debug before_release after_release clean_release

