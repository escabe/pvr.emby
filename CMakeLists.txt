project(pvr.emby)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(Kodi REQUIRED)
find_package(kodiplatform REQUIRED)
find_package(p8-platform REQUIRED)
find_package(JsonCpp REQUIRED)

include_directories(${kodiplatform_INCLUDE_DIRS}
                    ${p8-platform_INCLUDE_DIRS}
                    ${JSONCPP_INCLUDE_DIRS}
                    ${KODI_INCLUDE_DIR})

set(DEPLIBS ${p8-platform_LIBRARIES}
            ${JSONCPP_LIBRARIES})

set(EMBY_SOURCES src/client.cpp
                 src/md5.cpp
                 src/EmbyData.cpp
                 src/rest.cpp)

set(EMBY_HEADERS src/client.h
                 src/md5.h
                 src/EmbyData.h
                 src/rest.h)

build_addon(pvr.emby EMBY DEPLIBS)

include(CPack)
