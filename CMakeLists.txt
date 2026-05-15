cmake_minimum_required(VERSION 3.10)

set(APP_NAME HanoiOlympic)
project(${APP_NAME})

if(XCODE)
    if(NOT DEFINED CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET)
        set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET 9.0)
    endif()
endif()

# ------------------------------------------------------------
# Cocos2d-x engine  (./cocos2d/ must contain cocos2d-x 4.0)
# One-time setup:
#   git clone --depth 1 --branch cocos2d-x-4.0 \
#     https://github.com/cocos2d/cocos2d-x.git cocos2d
#   cd cocos2d && python download-deps.py
# ------------------------------------------------------------
if(NOT DEFINED BUILD_ENGINE_DONE)
    set(COCOS2DX_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cocos2d)
    set(CMAKE_MODULE_PATH ${COCOS2DX_ROOT_PATH}/cmake/Modules/)

    include(CocosBuildSet)
    add_subdirectory(${COCOS2DX_ROOT_PATH}/cocos ${ENGINE_BINARY_PATH}/cocos/core)
endif()

# ------------------------------------------------------------
# Resources
# ------------------------------------------------------------
set(GAME_RES_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/Resources")
# Apple only: mark resources for bundling into .app
if(APPLE)
    cocos_mark_multi_resources(common_res_files RES_TO "Resources"
                               FOLDERS ${GAME_RES_FOLDER})
endif()

# ------------------------------------------------------------
# Cross-platform headers
# ------------------------------------------------------------
set(GAME_HEADER
    Classes/AppDelegate.h
    Classes/Discus.h
    Classes/HelloWorldScene.h
    Classes/MainScene.h
    Classes/PlayScene.h
    Classes/PlaySceneTouchHandlerLayer.h
    Classes/SoundFactory.h
    Classes/UserDataManager.h
    Classes/common_define.h
    Classes/GameConfig.h
    Classes/Singleton.h
    Classes/stdafx.h
    Classes/MKStoreManager_cpp.h
    Classes/MKStoreManager_mm.h
    Classes/MKStoreManagerDelegate.h
    Classes/SimulatorConfig.h
)

# ------------------------------------------------------------
# Cross-platform C++ source files
# ------------------------------------------------------------
set(GAME_SOURCE
    Classes/AppDelegate.cpp
    Classes/Discus.cpp
    Classes/HelloWorldScene.cpp
    Classes/MainScene.cpp
    Classes/PlayScene.cpp
    Classes/PlaySceneTouchHandlerLayer.cpp
    Classes/SoundFactory.cpp
    Classes/UserDataManager.cpp
    Classes/stdafx.cpp
    Classes/MKStoreManager_cpp.cpp
    Classes/MKStoreManagerDelegate.cpp
)

# ------------------------------------------------------------
# Platform-specific entry points
# .m / .mm files are iOS-only — excluded on Win/Mac desktop
# ------------------------------------------------------------
if(ANDROID)
    set(APP_NAME MyGame)
    list(APPEND GAME_SOURCE proj.android/app/jni/hellocpp/main.cpp)

elseif(LINUX)
    list(APPEND GAME_SOURCE proj.linux/main.cpp)

elseif(WINDOWS)
    list(APPEND GAME_HEADER
        proj.win32/main.h
    )
    list(APPEND GAME_SOURCE
        proj.win32/main.cpp
    )

elseif(APPLE)
    if(IOS)
        list(APPEND GAME_HEADER
            Classes/HanoiAppDelegate.h
            Classes/RootViewController.h
            Classes/MKStoreObserver.h
            Classes/MKDelegateCPP.h
        )
        set(APP_UI_RES
            proj.ios_mac/ios/LaunchScreen.storyboard
        )
        list(APPEND GAME_SOURCE
            proj.ios_mac/ios/main.m
            Classes/HanoiAppDelegate.m
            Classes/RootViewController.m
            Classes/MKDelegateCPP.mm
            Classes/MKStoreManager.mm
            Classes/MKStoreManager_mm.mm
            Classes/MKStoreObserver.mm
            ${APP_UI_RES}
        )
    elseif(MACOSX)
        list(APPEND GAME_SOURCE
            proj.ios_mac/mac/main.cpp
        )
    endif()
    list(APPEND GAME_SOURCE ${common_res_files})
endif()

# ------------------------------------------------------------
# Build target
# ------------------------------------------------------------
set(all_code_files ${GAME_HEADER} ${GAME_SOURCE})

if(NOT ANDROID)
    add_executable(${APP_NAME} ${all_code_files})
else()
    add_library(${APP_NAME} SHARED ${all_code_files})
    add_subdirectory(${COCOS2DX_ROOT_PATH}/cocos/platform/android
                     ${ENGINE_BINARY_PATH}/cocos/platform)
    target_link_libraries(${APP_NAME}
        -Wl,--whole-archive cpp_android_spec -Wl,--no-whole-archive)
endif()

target_link_libraries(${APP_NAME} cocos2d)

target_include_directories(${APP_NAME}
    PRIVATE Classes
    PRIVATE ${COCOS2DX_ROOT_PATH}/cocos/audio/include/
    PRIVATE ${COCOS2DX_ROOT_PATH}/cocos/editor-support/
)

# setup_cocos_app_config must come AFTER add_executable + target_link_libraries
setup_cocos_app_config(${APP_NAME})

if(APPLE)
    set_target_properties(${APP_NAME} PROPERTIES RESOURCE "${APP_UI_RES}")
    if(MACOSX)
        set_xcode_property(${APP_NAME} INFOPLIST_FILE
            "${CMAKE_CURRENT_SOURCE_DIR}/proj.ios_mac/mac/Info.plist")
    elseif(IOS)
        set_xcode_property(${APP_NAME} INFOPLIST_FILE
            "${CMAKE_CURRENT_SOURCE_DIR}/proj.ios_mac/ios/Info.plist")
        set_xcode_property(${APP_NAME} ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon")
    endif()
elseif(WINDOWS)
    cocos_copy_target_dll(${APP_NAME})
    # Run from project root so Resources/ is found without copying
    set_target_properties(${APP_NAME} PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
endif()

if(MACOSX)
    # Run from project root so Resources/ is found without copying
    set_target_properties(${APP_NAME} PROPERTIES
        XCODE_GENERATE_SCHEME YES
        XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
endif()

if(LINUX)
    cocos_get_resource_path(APP_RES_DIR ${APP_NAME})
    cocos_copy_target_res(${APP_NAME} LINK_TO ${APP_RES_DIR}
                          FOLDERS ${GAME_RES_FOLDER})
endif()
