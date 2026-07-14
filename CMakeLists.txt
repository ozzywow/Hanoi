cmake_minimum_required(VERSION 3.10)

set(APP_NAME HanoiOlympic)
project(${APP_NAME})

if(MSVC)
    add_compile_options(/utf-8)
endif()

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
    # Exclude Info.plist — handled by INFOPLIST_FILE, not bundle resources
    list(FILTER common_res_files EXCLUDE REGEX "Info\\.plist$")
endif()

# ------------------------------------------------------------
# Cross-platform headers
# ------------------------------------------------------------
set(GAME_HEADER
    Classes/AppDelegate.h
    Classes/Discus.h
    Classes/DrawUtils.h
    Classes/PixelFont.h
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
    Classes/LeaderboardManager.h
    Classes/AndroidBilling.h
    Classes/ReviewManager.h
)

# ------------------------------------------------------------
# Cross-platform C++ source files
# ------------------------------------------------------------
set(GAME_SOURCE
    Classes/AppDelegate.cpp
    Classes/Discus.cpp
    Classes/DrawUtils.cpp
    Classes/PixelFont.cpp
    Classes/MainScene.cpp
    Classes/PlayScene.cpp
    Classes/PlaySceneTouchHandlerLayer.cpp
    Classes/SoundFactory.cpp
    Classes/UserDataManager.cpp
    Classes/stdafx.cpp
    Classes/MKStoreManager_cpp.cpp
    Classes/MKStoreManagerDelegate.cpp
    Classes/LeaderboardManager.cpp
    Classes/ReviewManager.cpp
)

# ------------------------------------------------------------
# Platform-specific entry points
# .m / .mm files are iOS-only — excluded on Win/Mac desktop
# ------------------------------------------------------------
if(ANDROID)
    list(APPEND GAME_SOURCE
        proj.android/app/jni/hellocpp/main.cpp
        Classes/AndroidBilling.cpp
    )

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
            proj.ios_mac/ios/Images.xcassets
        )
        list(APPEND GAME_SOURCE
            proj.ios_mac/ios/main.m
            Classes/HanoiAppDelegate.mm
            Classes/RootViewController.mm
            Classes/MKDelegateCPP.mm
            Classes/MKStoreManager.mm
            Classes/MKStoreManager_mm.mm
            Classes/MKStoreObserver.mm
            Classes/ReviewBridge_ios.mm
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
        set_xcode_property(${APP_NAME} DEVELOPMENT_TEAM "GFX32EJL53")
        set_xcode_property(${APP_NAME} CODE_SIGN_STYLE "Automatic")
        # Automatic signing handles certificate selection (Development/Distribution)
        # Do NOT set CODE_SIGN_IDENTITY manually — conflicts with Automatic signing
        set_xcode_property(${APP_NAME} PRODUCT_BUNDLE_IDENTIFIER "com.ozzywow.TowerOfHanoiOlympic")
        set_xcode_property(${APP_NAME} IPHONEOS_DEPLOYMENT_TARGET "15.6")
        set_xcode_property(${APP_NAME} TARGETED_DEVICE_FAMILY "1,2")
        set_xcode_property(${APP_NAME} SUPPORTS_MACCATALYST "NO")
        set_xcode_property(${APP_NAME} SUPPORTS_MAC_DESIGNED_FOR_IPHONE_IPAD "NO")
        set_xcode_property(${APP_NAME} INFOPLIST_KEY_UILaunchStoryboardName "LaunchScreen")
        set_xcode_property(${APP_NAME} INFOPLIST_KEY_CFBundleDisplayName "Tower of Hanoi")
        set_xcode_property(${APP_NAME} INFOPLIST_KEY_LSApplicationCategoryType "public.app-category.puzzle-games")
        # Ensure proper iOS App Archive (not Generic Xcode Archive)
        set_xcode_property(${APP_NAME} SKIP_INSTALL "NO")
        set_xcode_property(${APP_NAME} INSTALL_PATH "$(LOCAL_APPS_DIR)")
        # dSYM for crash symbolication
        set_property(TARGET ${APP_NAME} PROPERTY
            XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT[variant=Release] "dwarf-with-dsym")
        find_library(STOREKIT_LIBRARY StoreKit)
        find_library(GAMEKIT_LIBRARY GameKit)
        target_link_libraries(${APP_NAME} ${STOREKIT_LIBRARY} ${GAMEKIT_LIBRARY})
    endif()
elseif(WINDOWS)
    cocos_copy_target_dll(${APP_NAME})
    set_target_properties(${APP_NAME} PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
    # Create a junction from each build config output dir to project Resources
    add_custom_command(TARGET ${APP_NAME} POST_BUILD
        COMMAND powershell -NoProfile -Command
            "if (-not (Test-Path '$<TARGET_FILE_DIR:${APP_NAME}>/Resources')) { New-Item -ItemType Junction -Path '$<TARGET_FILE_DIR:${APP_NAME}>/Resources' -Target '${CMAKE_CURRENT_SOURCE_DIR}/Resources' | Out-Null }"
        COMMENT "Linking Resources -> output dir"
    )
endif()

if(MACOSX)
    # Run from project root so Resources/ is found without copying
    set_target_properties(${APP_NAME} PROPERTIES
        XCODE_GENERATE_SCHEME YES
        XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
elseif(IOS)
    # Generate scheme so Run action uses Debug config (not Archive)
    set_target_properties(${APP_NAME} PROPERTIES
        XCODE_GENERATE_SCHEME YES
    )
endif()

if(LINUX)
    cocos_get_resource_path(APP_RES_DIR ${APP_NAME})
    cocos_copy_target_res(${APP_NAME} LINK_TO ${APP_RES_DIR}
                          FOLDERS ${GAME_RES_FOLDER})
endif()
