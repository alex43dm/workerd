project( workerd )
cmake_minimum_required( VERSION 2.6 )

set ( CMAKE_BUILD_TYPE Release )
add_definitions ( -std=c++11 -Wall )

find_package(Boost
 COMPONENTS system filesystem thread 
	program_options date_time regex REQUIRED)
#pkg_check_modules(SQLITE3	sqlite3 	REQUIRED)

include_directories (
	${CMAKE_SOURCE_DIR} 
	${CMAKE_CURRENT_BINARY_DIR}
	include
	../libredis
	)

set ( EXECUTABLE_OUTPUT_PATH bin )
set ( workerd_HDRS
	base64.h
	BaseCore.h
	benchmark.h
	Branch.h.h
	Campaign.h
	CgiService.h
	Comparators.h
	Config.h
	Cookie.h
	Core.h
	DataBase.h
	DB.h
	EBranch.h
	GeoIPTools.h
	GeoRerions.h
	HistoryManager.h
	Informer.h
	InformerTemplate.h
	json.h
	KompexSQLiteDatabase.h
	KompexSQLiteException.h
	KompexSQLitePrerequisites.h
	KompexSQLiteStatement.h
	Log.h
	Offer.h
	Params.h
	pch.h
	RedisClient.h
	Server.h
	UrlParser.h
	XXXSearcher.h
	ParamParse.h
	ParentDB.h
	)

set ( workerd_SRCS
	utils/base64.cpp
	utils/Cookie.cpp
	utils/GeoIPTools.cpp
	utils/UrlParser.cpp
	src/EBranch.cpp
	src/json.cpp
	src/Server.cpp
	src/GeoRerions.cpp
	src/RedisClient.cpp
	src/sphinxRequests.cpp
	src/XXXSearcher.cpp
	src/ParamParse.cpp
	src/ParentDB.cpp
	src/HistoryManagerLongTerm.cpp
	src/HistoryManagerOffer.cpp
	src/HistoryManagerRetargeting.cpp
	src/HistoryManagerShortTerm.cpp
	BaseCore.cpp
	Campaign.cpp
	CgiService.cpp
	Config.cpp
	Core.cpp
	DataBase.cpp
	DB.cpp
	HistoryManager.cpp
	Informer.cpp
	InformerTemplate.cpp
	KompexSQLiteDatabase.cpp
	KompexSQLiteStatement.cpp
	Log.cpp
	main.cpp
	Offer.cpp
	Params.cpp
	)
	
IF(CMAKE_COMPILER_IS_GNUCXX)
    ADD_DEFINITIONS(-std=c++0x)
ENDIF()

add_executable ( workerd 
	${workerd_SRCS} 
)

target_link_libraries ( workerd
	/usr/local/lib/libamqpcpp.a
	/home/alex/Projects/libredis/lib/libredis.a
	${Boost_FILESYSTEM_LIBRARY}
	${Boost_SYSTEM_LIBRARY}
	${Boost_PROGRAM_OPTIONS_LIBRARY}
	${Boost_DATE_TIME_LIBRARY}
	${Boost_THREAD_LIBRARY}
    	${Boost_REGEX_LIBRARY}
	mongoclient
	tinyxml
	rabbitmq
	fcgi 
	GeoIP
    	sphinxclient
	sqlite3
	pthread
	icui18n
	icuuc
	icudata
	rt
    	${EXTRA_LIBS}
)
