#include "common_define.h"

// common_define.h 에 있던 전역 유틸 함수 정의부.
// 헤더(거의 모든 TU가 stdafx.h 경유로 포함)에 static으로 두면 TU마다 사본이 생기므로
// 여기로 옮기고 헤더에는 선언만 남긴다. (매크로/enum/상수는 헤더 유지)

int getMilliCount()
{
#ifdef _WIN32
	timeb tb;
	ftime(&tb);
	return (int)(tb.millitm + (tb.time & 0xfffff) * 1000);
#else
	struct timeval tv;
	gettimeofday(&tv, 0);
	return (int)((tv.tv_sec & 0xfffff) * 1000 + tv.tv_usec / 1000);
#endif
}

std::string countryToFlag(const std::string& cc)
{
    if (cc.size() != 2) return cc;
    uint32_t a = 0x1F1E6u + ((unsigned char)toupper((unsigned char)cc[0]) - 'A');
    uint32_t b = 0x1F1E6u + ((unsigned char)toupper((unsigned char)cc[1]) - 'A');
    auto u8 = [](uint32_t cp) -> std::string {
        return { (char)(0xF0 | (cp >> 18)),
                 (char)(0x80 | ((cp >> 12) & 0x3F)),
                 (char)(0x80 | ((cp >> 6)  & 0x3F)),
                 (char)(0x80 | (cp         & 0x3F)) };
    };
    return u8(a) + u8(b);
}

// 디바이스 언어에 맞는 자국어 인삿말 반환 (greetings.plist 참조)
std::string getLocalGreeting()
{
    auto langType = Application::getInstance()->getCurrentLanguage();
    std::string nativeLang;
    switch (langType) {
        case LanguageType::KOREAN:     nativeLang = "KOREAN";     break;
        case LanguageType::JAPANESE:   nativeLang = "JAPANESE";   break;
        case LanguageType::CHINESE:    nativeLang = "CHINESE";    break;
        case LanguageType::FRENCH:     nativeLang = "FRENCH";     break;
        case LanguageType::SPANISH:    nativeLang = "SPANISH";    break;
        case LanguageType::GERMAN:     nativeLang = "GERMAN";     break;
        case LanguageType::ITALIAN:    nativeLang = "ITALIAN";    break;
        case LanguageType::PORTUGUESE: nativeLang = "PORTUGUESE"; break;
        case LanguageType::RUSSIAN:    nativeLang = "RUSSIAN";    break;
        case LanguageType::DUTCH:      nativeLang = "DUTCH";      break;
        case LanguageType::TURKISH:    nativeLang = "TURKISH";    break;
        case LanguageType::NORWEGIAN:  nativeLang = "NORWEGIAN";  break;
        case LanguageType::POLISH:     nativeLang = "POLISH";     break;
        case LanguageType::HUNGARIAN:  nativeLang = "HUNGARIAN";  break;
        case LanguageType::ARABIC:     nativeLang = "ARABIC";     break;
        default:                        nativeLang = "ENGLISH";    break;
    }
    auto data = FileUtils::getInstance()->getValueVectorFromFile("greetings.plist");
    for (auto& v : data) {
        auto& d = v.asValueMap();
        if (d["lang"].asString() == nativeLang)
            return d["text"].asString();
    }
    return "Hello";
}

RecordTime getRecordTime(int time)
{
	RecordTime res;
	res.min = time / (1000 * 60);
	res.sec = (time % (1000 * 60)) / 1000;
	res.ms = ((time % (1000 * 60)) % 1000) / 10;
	return res;
}
