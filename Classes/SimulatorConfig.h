#pragma once
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)

#include "cocos2d.h"
#include <fstream>
#include <string>

struct DeviceProfile {
    const char* key;
    const char* label;
    float width;   // landscape width  (points)
    float height;  // landscape height (points)
};

// All supported iPhone / iPad resolutions in landscape (points = logical pixels)
// (*) = estimated ??iPhone 17 released after knowledge cutoff, verify against official specs
static const DeviceProfile kDevices[] = {
    // ?? Legacy ??????????????????????????????????????????????????????????????
    { "iphone4",         "iPhone 4/4S",               480,  320 },
    { "iphone_se1",      "iPhone SE1 / 5 / 5S",       568,  320 },
    { "iphone_6789",     "iPhone 6/7/8/SE2/SE3",      667,  375 },
    { "iphone_plus",     "iPhone 6+/7+/8+",            736,  414 },
    { "iphone_x",        "iPhone X/XS/11 Pro",         812,  375 },
    { "iphone_xr",       "iPhone XR/11",               896,  414 },
    // ?? iPhone 12 / 13 ??????????????????????????????????????????????????????
    { "iphone_mini",     "iPhone 12/13 mini",          780,  360 },
    { "iphone_12",       "iPhone 12/13",               844,  390 },
    { "iphone_12pro",    "iPhone 12/13 Pro",           844,  390 },
    { "iphone_12promax", "iPhone 12/13 Pro Max",       926,  428 },
    // ?? iPhone 14 ???????????????????????????????????????????????????????????
    { "iphone_14",       "iPhone 14",                  844,  390 },
    { "iphone_14plus",   "iPhone 14 Plus",             932,  430 },
    { "iphone_14pro",    "iPhone 14/15 Pro",           852,  393 },
    { "iphone_14promax", "iPhone 14/15 Pro Max",       932,  430 },
    // ?? iPhone 15 ???????????????????????????????????????????????????????????
    { "iphone_15",       "iPhone 15/16",               852,  393 },
    { "iphone_15plus",   "iPhone 15/16 Plus",          932,  430 },
    { "iphone_15pro",    "iPhone 15 Pro",              852,  393 },
    { "iphone_15promax", "iPhone 15 Pro Max",          932,  430 },
    // ?? iPhone 16 ???????????????????????????????????????????????????????????
    { "iphone_16pro",    "iPhone 16 Pro",              874,  402 },
    { "iphone_16promax", "iPhone 16 Pro Max",          956,  440 },
    // ?? iPhone 17 (*) ???????????????????????????????????????????????????????
    { "iphone_17",       "iPhone 17 (*)",              852,  393 },
    { "iphone_17air",    "iPhone 17 Air (*)",          932,  430 },
    { "iphone_17pro",    "iPhone 17 Pro (*)",          874,  402 },
    { "iphone_17promax", "iPhone 17 Pro Max (*)",      956,  440 },
    // ?? iPad ????????????????????????????????????????????????????????????????
    { "ipad_mini5",      "iPad mini 5th",             1024,  768 },
    { "ipad_mini6",      "iPad mini 6th",             1133,  744 },
    { "ipad_10",         "iPad 10th gen",             1180,  820 },
    { "ipad_air",        "iPad Air / Pro 11\"",       1194,  834 },
    { "ipad_pro13",      "iPad Pro 12.9\"",           1366, 1024 },
};

static const int kDeviceCount = (int)(sizeof(kDevices) / sizeof(kDevices[0]));
static const int kDefaultDeviceIndex = 7; // iphone_14

static int findDeviceIndex(const std::string& key)
{
    for (int i = 0; i < kDeviceCount; ++i)
        if (key == kDevices[i].key) return i;
    return kDefaultDeviceIndex;
}

// Reads simulator_device.cfg from working directory.
// File format: one device key per line, lines starting with # are comments.
static cocos2d::Size getSimulatorWindowSize()
{
    std::ifstream f("simulator_device.cfg");
    if (f.is_open()) {
        std::string line;
        while (std::getline(f, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' '))
                line.pop_back();
            if (line.empty() || line[0] == '#') continue;
            int idx = findDeviceIndex(line);
            const DeviceProfile& d = kDevices[idx];
            cocos2d::log("Simulator device: %s (%gx%g)", d.label, d.width, d.height);
            return cocos2d::Size(d.width, d.height);
        }
    }
    const DeviceProfile& d = kDevices[kDefaultDeviceIndex];
    cocos2d::log("Simulator device: %s (%gx%g) [default]", d.label, d.width, d.height);
    return cocos2d::Size(d.width, d.height);
}

#endif // desktop only
