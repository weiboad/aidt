INCLUDE_DIRECTORIES(./)
INCLUDE_DIRECTORIES(./thridpart)
LINK_DIRECTORIES(/usr/local/lib)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -W -static-libstdc++")  

configure_file (
	"${PROJECT_SOURCE_DIR}/src/Version.hpp.in"	
	"${PROJECT_SOURCE_DIR}/src/Version.hpp"	
)

configure_file (
	"${PROJECT_SOURCE_DIR}/rpm/aidt.spec.in"	
	"${PROJECT_SOURCE_DIR}/rpm/aidt.spec"	
)

configure_file (
	"${PROJECT_SOURCE_DIR}/rpm/build_rpm.in"	
	"${PROJECT_SOURCE_DIR}/rpm/build_rpm"	
	@ONLY
)

SET(AIDT_SRC aidt.cpp
	BootStrap.cpp	
	App.cpp	
	AdServer.cpp
	Http.cpp
	Http/HttpInterface.cpp
	Http/Server.cpp
	Http/Index.cpp
	Http/Config.cpp
	Http/Topic.cpp
	Http/Message.cpp
	McProcessor.cpp
	HeadProcessor.cpp
	Aims.cpp
	Aims/Kafka/ProducerIn.cpp
	Timer.cpp
    App/Config.cpp
    App/ConfigPaser.cpp
    App/Message.cpp
    App/Watcher.cpp
    App/Reader.cpp
    thridpart/inotify/inotifytools.c
    thridpart/inotify/redblack.c
)
ADD_EXECUTABLE(aidt ${AIDT_SRC})
TARGET_LINK_LIBRARIES(aidt libadbase.a 
	libadbase_kafka.a
	librdkafka++.a 
	librdkafka.a 
	libevent.a
    libyaml-cpp.a
	pthread
	rt
	z
	)
INSTALL(TARGETS aidt RUNTIME DESTINATION adinf/aidt/bin)
