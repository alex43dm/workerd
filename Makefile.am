ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = workerd

sysconfdir = ${prefix}/etc/worker

SUBDIRS = libs/libredis

if DEBUG
AM_CFLAGS = -g -DDEBUG -Wall
AM_CXXFLAGS = -g -DDEBUG -Wall
else
AM_CFLAGS = -Ofast
AM_CXXFLAGS = -Ofast
endif

workerd_CPPFLAGS=-std=c++11 -Iinclude -Ilibs/libredis
workerd_SOURCES=src/base64.cpp src/Core.cpp src/GeoRerions.cpp src/HistoryManagerShortTerm.cpp src/Log.cpp src/RedisClient.cpp src/BaseCore.cpp src/DataBase.cpp src/HistoryManager.cpp src/Informer.cpp src/main.cpp src/Server.cpp src/Campaign.cpp src/DB.cpp src/HistoryManagerLongTerm.cpp src/Offer.cpp src/sphinxRequests.cpp src/CgiService.cpp src/EBranch.cpp src/HistoryManagerOffer.cpp src/json.cpp src/ParamParse.cpp src/UrlParser.cpp src/Config.cpp src/KompexSQLiteDatabase.cpp src/Params.cpp src/XXXSearcher.cpp src/Cookie.cpp src/GeoIPTools.cpp src/HistoryManagerRetargeting.cpp src/KompexSQLiteStatement.cpp src/ParentDB.cpp
workerd_LDADD = \
	$(BOOST_DATE_TIME_LIBS) \
	$(BOOST_FILESYSTEM_LIBS) \
	$(BOOST_THREAD_LIBS) \
	$(BOOST_SYSTEM_LIBS) \
	$(BOOST_REGEX_LIBS) \
	libs/libredis/lib/libredis.a

dist_doc_DATA=README

sysconf_DATA= \
db_dump/requests/02.sql \
db_dump/requests/01.sql \
db_dump/requests/04.sql \
db_dump/requests/03.sql \
db_dump/UA_RU \
db_dump/tables/11_GeoCountries.sql \
db_dump/tables/00_Campaign.sql \
db_dump/tables/17_Retargeting.sql \
db_dump/tables/12_GeoRerions.sql \
db_dump/tables/20_GeoLiteCity.sql \
db_dump/tables/06_Accounts.sql \
db_dump/tables/02_Offer.sql \
db_dump/tables/07_Campaign2Accounts.sql \
db_dump/tables/15_CampaignNow.sql \
db_dump/tables/16_Informer.sql \
db_dump/tables/18_Informer2OfferRating.sql \
db_dump/tables/10_CronCampaign.sql \
db_dump/tables/05_Campaign2Informer.sql \
db_dump/tables/14_regionTargeting.sql \
db_dump/tables/08_Domains.sql \
db_dump/tables/19_Campaign2Categories.sql \
db_dump/tables/04_Categories2Domain.sql \
db_dump/tables/13_geoTargeting.sql \
db_dump/tables/03_Categories.sql \
db_dump/tables/09_Campaign2Domains.sql \
db_dump/tables/04_Categories2Informer.sql \
db_dump/view/00_Campaign2GeoRerions.sql \
db_dump/view/03_Campaign2Acnts.sql \
db_dump/view/02_Campaign2Doms.sql \
db_dump/view/04_Campaign2Infs.sql \
db_dump/post/01.sql \
db_dump/schema/CampaignNow.sql \
template/swfobject.js \
template/banner.html \
template/teaser.html

install: install-binPROGRAMS
	$(MKDIR_P) $(sysconfdir)/template
	$(MKDIR_P) $(sysconfdir)/db_dump
	$(MKDIR_P) $(sysconfdir)/db_dump/schema
	$(MKDIR_P) $(sysconfdir)/db_dump/post
	$(MKDIR_P) $(sysconfdir)/db_dump/view
	$(MKDIR_P) $(sysconfdir)/db_dump/tables
	$(MKDIR_P) $(sysconfdir)/db_dump/requests
	$(SED) 's/\/home\/alex\/Projects\/worker/\/usr/' < ./config.xml > $(sysconfdir)/config.xml
	@list='$(sysconf_DATA)'; \
	for p in $$list; do \
	$(INSTALL_DATA) $$p "$(DESTDIR)$(sysconfdir)/$$p" || exit $$?; \
	done
