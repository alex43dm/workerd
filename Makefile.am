ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = workerd

SUBDIRS = libs/libredis libs/amqpcpp

workerd_CFLAGS=-O2
workerd_CPPFLAGS=-Wall -std=c++11 -Iinclude -Ilibs/libredis
workerd_SOURCES=src/base64.cpp src/Core.cpp src/GeoRerions.cpp src/HistoryManagerShortTerm.cpp src/Log.cpp src/RedisClient.cpp src/BaseCore.cpp src/DataBase.cpp src/HistoryManager.cpp src/Informer.cpp src/main.cpp src/Server.cpp src/Campaign.cpp src/DB.cpp src/HistoryManagerLongTerm.cpp src/InformerTemplate.cpp src/Offer.cpp src/sphinxRequests.cpp src/CgiService.cpp src/EBranch.cpp src/HistoryManagerOffer.cpp src/json.cpp src/ParamParse.cpp src/UrlParser.cpp src/Config.cpp src/KompexSQLiteDatabase.cpp src/Params.cpp src/XXXSearcher.cpp src/Cookie.cpp src/GeoIPTools.cpp src/HistoryManagerRetargeting.cpp src/KompexSQLiteStatement.cpp src/ParentDB.cpp
workerd_LDADD = \
	$(BOOST_DATE_TIME_LIBS) \
	$(BOOST_FILESYSTEM_LIBS) \
	$(BOOST_THREAD_LIBS) \
	$(BOOST_SYSTEM_LIBS) \
	$(BOOST_REGEX_LIBS) \
	libs/libredis/lib/libredis.a \
	libs/amqpcpp/libamqpcpp.a

dist_doc_DATA=README
