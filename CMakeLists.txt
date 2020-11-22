project(pvr.jellyfin)

cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(Kodi REQUIRED)
find_package(JsonCpp REQUIRED)

include_directories(${JSONCPP_INCLUDE_DIRS}
                    ${KODI_INCLUDE_DIR}/..)

set(DEPLIBS ${JSONCPP_LIBRARIES})

set(JELLYFIN_SOURCES src/client.cpp
                     src/base64.cpp
                     src/rest.cpp
                     )

set(JELLYFIN_HEADERS src/client.h
                     src/base64.h
                     src/rest.h
                     )

build_addon(pvr.jellyfin JELLYFIN DEPLIBS)

include(CPack)
