cmake_minimum_required(VERSION 3.24)

project(
    bauman_doomlike
    VERSION 0.1.0
    DESCRIPTION "Учебный pseudo-3D shooter на raycasting для курса по классам и шаблонам"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DOOMLIKE_FETCH_DEPS "Автоматически загружать SDL3 и ENet через FetchContent" ON)
option(DOOMLIKE_ENABLE_WARNINGS "Включить строгие предупреждения компилятора" ON)

include(FetchContent)

if(DOOMLIKE_FETCH_DEPS)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.4.4
        GIT_SHALLOW TRUE
    )

    set(ENET_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(ENET_BUILD_TOOLS OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        enet
        GIT_REPOSITORY https://github.com/lsalzman/enet.git
        GIT_TAG v1.3.18
        GIT_SHALLOW TRUE
    )

    FetchContent_MakeAvailable(SDL3 enet)
else()
    find_package(SDL3 CONFIG REQUIRED)
    find_package(enet CONFIG REQUIRED)
endif()

set(SDL3_TARGET SDL3::SDL3)
if(NOT TARGET ${SDL3_TARGET})
    if(TARGET SDL3::SDL3-static)
        set(SDL3_TARGET SDL3::SDL3-static)
    elseif(TARGET SDL3::SDL3-shared)
        set(SDL3_TARGET SDL3::SDL3-shared)
    else()
        message(FATAL_ERROR "Не удалось определить CMake target для SDL3")
    endif()
endif()

set(ENET_TARGET enet)
if(TARGET enet::enet)
    set(ENET_TARGET enet::enet)
elseif(TARGET ENet::enet)
    set(ENET_TARGET ENet::enet)
elseif(NOT TARGET enet)
    message(FATAL_ERROR "Не удалось определить CMake target для ENet")
endif()

set(DOOMLIKE_SOURCES
    src/main.cpp
    src/application/GameLoop.cpp
    src/application/GameSession.cpp
    src/domain/Map.cpp
    src/domain/World.cpp
    src/domain/CollisionSystem.cpp
    src/domain/ShootingSystem.cpp
    src/domain/DamageSystem.cpp
    src/infrastructure/TextMapLoader.cpp
    src/presentation/Raycaster.cpp
    src/presentation/SdlApp.cpp
    src/network/PacketSerializer.cpp
    src/network/EnetClient.cpp
    src/network/EnetHost.cpp
)

add_executable(${PROJECT_NAME} ${DOOMLIKE_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE src)

# Upstream ENet CMake builds the library but does not publish include directories
# as target usage requirements, so the consumer has to add the include path explicitly.
if(DOOMLIKE_FETCH_DEPS AND DEFINED enet_SOURCE_DIR AND EXISTS "${enet_SOURCE_DIR}/include")
    target_include_directories(${PROJECT_NAME} PRIVATE "${enet_SOURCE_DIR}/include")
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL3_TARGET} ${ENET_TARGET})

if(DOOMLIKE_ENABLE_WARNINGS)
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(
            ${PROJECT_NAME}
            PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wsign-conversion
        )
    endif()
endif()

target_compile_definitions(
    ${PROJECT_NAME}
    PRIVATE
    DOOMLIKE_PROJECT_NAME="${PROJECT_NAME}"
    DOOMLIKE_PROJECT_VERSION="${PROJECT_VERSION}"
    DOOMLIKE_PROJECT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
