#include "stdafx.h"

#include "UserDataManager.h"


UserDataManager::UserDataManager()
{
	m_arrRecord.clear();

	LoadUserData();
}
UserDataManager::~UserDataManager()
{

}




void UserDataManager::LoadUserData()
{
	m_userName = UserDefault::getInstance()->getStringForKey("userName", "");
	m_level = UserDefault::getInstance()->getIntegerForKey("level", 3);
	m_soundOption = UserDefault::getInstance()->getBoolForKey("soundOption", true);
	m_cart = UserDefault::getInstance()->getBoolForKey("cart", false);

	m_recordTimestamp = UserDefault::getInstance()->getDoubleForKey("recordTimestamp", 0.0);

	for (int level = 3; level <= MAX_PLAY_LEVEL; ++level)
	{
		std::string key = StringUtils::format("level_%d", level);
		int record = UserDefault::getInstance()->getIntegerForKey(key.c_str(), 0);
		m_arrRecord[level] = record;
	}

	// 12h 경과 시 기록 초기화
	if (m_recordTimestamp > 0.0)
	{
		double now = static_cast<double>(::time(nullptr));
		if (now - m_recordTimestamp >= kRecordExpirySec)
		{
			for (auto& kv : m_arrRecord) kv.second = 0;
			m_recordTimestamp = 0.0;
		}
	}
}


void UserDataManager::SaveUserData()
{
	UserDefault::getInstance()->setStringForKey("userName", m_userName);
	UserDefault::getInstance()->setIntegerForKey("level", m_level);
	UserDefault::getInstance()->setBoolForKey("soundOption", m_soundOption);
	UserDefault::getInstance()->setBoolForKey("cart", m_cart);
	UserDefault::getInstance()->setDoubleForKey("recordTimestamp", m_recordTimestamp);

	for (int level = 3; level <= MAX_PLAY_LEVEL; ++level)
	{
		int record = m_arrRecord[level];
		std::string key = StringUtils::format("level_%d", level);
		UserDefault::getInstance()->setIntegerForKey(key.c_str(), record);
	}
}

void UserDataManager::SetUserName(std::string& name)
{
	m_userName = name;
}

std::string& UserDataManager::GetUserName() 
{
	return m_userName;
}

