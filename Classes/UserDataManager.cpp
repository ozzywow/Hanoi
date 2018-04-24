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

	for (int level = 3; level < MAX_PLAY_LEVEL; ++level)
	{
		std::string key = StringUtils::format("level_%d", level);
		int record = UserDefault::getInstance()->getIntegerForKey(key.c_str(), 0);
		m_arrRecord[level] = record;
	}
}


void UserDataManager::SaveUserData()
{
	UserDefault::getInstance()->setStringForKey("userName", m_userName);
	UserDefault::getInstance()->setIntegerForKey("level", m_level);
	UserDefault::getInstance()->setBoolForKey("soundOption", m_soundOption);
	UserDefault::getInstance()->setBoolForKey("cart", m_cart);

	for (int level = 3; level < MAX_PLAY_LEVEL; ++level)
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

