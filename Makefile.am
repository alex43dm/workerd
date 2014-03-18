ACLOCAL_AMFLAGS = -I m4

SRC_ALL=src/base64.cpp src/Core.cpp src/GeoRerions.cpp src/HistoryManagerShortTerm.cpp src/Log.cpp src/RedisClient.cpp src/BaseCore.cpp src/DataBase.cpp src/HistoryManager.cpp src/Informer.cpp src/main.cpp src/Server.cpp src/Campaign.cpp src/DB.cpp src/HistoryManagerLongTerm.cpp src/Offer.cpp src/sphinxRequests.cpp src/CgiService.cpp src/EBranch.cpp src/HistoryManagerOffer.cpp src/json.cpp src/ParamParse.cpp src/UrlParser.cpp src/Config.cpp src/KompexSQLiteDatabase.cpp src/Params.cpp src/XXXSearcher.cpp src/Cookie.cpp src/GeoIPTools.cpp src/HistoryManagerRetargeting.cpp src/KompexSQLiteStatement.cpp src/ParentDB.cpp
SRC_DUMMY=src/base64.cpp src/Core.cpp src/GeoRerions.cpp src/Log.cpp src/BaseCore.cpp src/DataBase.cpp src/Informer.cpp src/main.cpp src/Server.cpp src/Campaign.cpp src/DB.cpp src/Offer.cpp src/CgiService.cpp src/EBranch.cpp src/json.cpp src/ParamParse.cpp src/UrlParser.cpp src/Config.cpp src/KompexSQLiteDatabase.cpp src/Params.cpp src/Cookie.cpp src/GeoIPTools.cpp src/KompexSQLiteStatement.cpp src/ParentDB.cpp

CMN_CPPFLAGS=-std=c++11 -Iinclude
CMN_LDADD = $(BOOST_DATE_TIME_LIBS) $(BOOST_FILESYSTEM_LIBS) $(BOOST_THREAD_LIBS) $(BOOST_SYSTEM_LIBS) $(BOOST_REGEX_LIBS)

if DUMMY

if DEBUG
bin_PROGRAMS = workerd-dummy-dev
workerd_dummy_dev_SOURCES=$(SRC_DUMMY)
workerd_dummy_dev_CPPFLAGS=$(CMN_CPPFLAGS)
workerd_dummy_dev_LDADD = $(CMN_LDADD)
else
bin_PROGRAMS = workerd-dummy
workerd_dummy_SOURCES=$(SRC_DUMMY)
workerd_dummy_CPPFLAGS=$(CMN_CPPFLAGS)
workerd_dummy_LDADD = $(CMN_LDADD)
endif

DEBDEP=bash (>= 2.05a-11), awk, procps (>= 1:2.0.7-8), sed (>= 3.02-8), grep (>= 2.4.2-3), coreutils (>= 5.0-5), libtinyxml2.6.2 (>=2.6.2), libsqlite3-0 (>=3.7.13), libfcgi0ldbl (>=2.4.0), geoip-database (>=20130213-1), rabbitmq-server (>=3.2.3), libboost-system1.49.0 (>=1.49.0), libboost-regex1.49.0 (>=1.49.0), libboost-thread1.49.0 (>=1.49.0), libboost-filesystem1.49.0 (>=1.49.0), libboost-date-time1.49.0 (>=1.49.0)
else #DUMMY

if DEBUG
bin_PROGRAMS = workerd-dev
workerd_dev_SOURCES=$(SRC_ALL)
workerd_dev_CPPFLAGS=$(CMN_CPPFLAGS) -Ilibs/libredis
workerd_dev_LDADD=$(CMN_LDADD) libs/libredis/lib/libredis.a
else
bin_PROGRAMS = workerd
workerd_SOURCES=$(SRC_ALL)
workerd_CPPFLAGS=$(CMN_CPPFLAGS) -Ilibs/libredis
workerd_LDADD=$(CMN_LDADD) libs/libredis/lib/libredis.a
endif

SUBDIRS = libs/libredis

DEBDEP=bash (>= 2.05a-11), awk, procps (>= 1:2.0.7-8), sed (>= 3.02-8), grep (>= 2.4.2-3), coreutils (>= 5.0-5), libtinyxml2.6.2 (>=2.6.2), libsqlite3-0 (>=3.7.13), libfcgi0ldbl (>=2.4.0), sphinxsearch (>=2.1.5), geoip-database (>=20130213-1), rabbitmq-server (>=3.2.3), libboost-system1.49.0 (>=1.49.0), libboost-regex1.49.0 (>=1.49.0), libboost-thread1.49.0 (>=1.49.0), libboost-filesystem1.49.0 (>=1.49.0), libboost-date-time1.49.0 (>=1.49.0)
endif #DUMMY

if DEBUG
AM_CFLAGS = -g -DDEBUG -Wall
AM_CXXFLAGS = -g -DDEBUG -Wall
else
AM_CFLAGS = -Ofast
AM_CXXFLAGS = -Ofast
endif

debian_BUILD = $(bin_PROGRAMS)-$(VERSION)

dist_doc_DATA=README

sysconf_DATA= \
db_dump/requests/02.sql \
db_dump/requests/01.sql \
db_dump/requests/05.sql \
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
template/error.html \
template/teaser.html

install_f =\
echo "install to $1"; \
$(MKDIR_P) $(1)/etc/worker/template; \
$(MKDIR_P) $(1)/etc/worker/db_dump; \
$(MKDIR_P) $(1)/etc/worker/db_dump/schema; \
$(MKDIR_P) $(1)/etc/worker/db_dump/post; \
$(MKDIR_P) $(1)/etc/worker/db_dump/view; \
$(MKDIR_P) $(1)/etc/worker/db_dump/tables; \
$(MKDIR_P) $(1)/etc/worker/db_dump/requests; \
$(MKDIR_P) $(1)/etc/init.d; \
$(MKDIR_P) $(1)/usr/sbin; \
$(SED) 's/\/home\/alex\/Projects\/worker/\/$(1)\/etc\/worker/' < ./config.xml > $(1)/etc/worker/config-$(bin_PROGRAMS).xml; \
$(INSTALL) $(bin_PROGRAMS) $(1)/usr/sbin; \
$(SED) 's/\/etc\/workerd\/config.xml/\/$(1)/etc/worker/config-$(bin_PROGRAMS).xml/;s/\/usr\/sbin\/workerd/\/$(1)/usr/sbin/$(bin_PROGRAMS)/' < ./install/workerd > $(1)/etc/init.d/$(bin_PROGRAMS); \
@list='$(sysconf_DATA)'; \
for p in $$list; do \
$(INSTALL_DATA) $$p "$(1)/etc/worker/$$p" || exit $$?; \
done

install: install-binPROGRAMS
	@DOLLAR_SIGN@(call install_f,$(prefix))

deb: $(bin_PROGRAMS)
	rm -rf $(debian_BUILD)
	$(MKDIR_P) $(debian_BUILD)/DEBIAN
	echo "Package: worker">  $(debian_BUILD)/DEBIAN/control
	echo "Version: $(VERSION)" >> $(debian_BUILD)/DEBIAN/control
	echo "Section: base" >> $(debian_BUILD)/DEBIAN/control
	echo "Priority: optional" >> $(debian_BUILD)/DEBIAN/control
	echo "Architecture: all" >> $(debian_BUILD)/DEBIAN/control
	echo "Depends: $(DEBDEP)" >> $(debian_BUILD)/DEBIAN/control
	echo "Maintainer: Alex D <wvdial@gmail.com>" >> $(debian_BUILD)/DEBIAN/control
	echo "Description:  multithread demon, to return html(inframe) with offers. Runs on background and listen unix socket(config.xml) ready to http requests from nginx." >> $(debian_BUILD)/DEBIAN/control
	@DOLLAR_SIGN@(call install_f,$(debian_BUILD))
	for p in `find $(debian_BUILD) -type f -print | grep -v DEBIAN`; do \
	md5sum $$p | $(SED) 's/$(debian_BUILD)\///'>>$(debian_BUILD)/DEBIAN/md5sum; \
	done
	dpkg-deb --build $(debian_BUILD)
	rm -rf $(debian_BUILD)

