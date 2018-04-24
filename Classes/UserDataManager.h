#pragma once

#include "cocos2d.h"
using namespace cocos2d;

#include "Singleton.h"
#include <string>
#include <map>

#define MAX_LEVEL_FOR_LITE_VERTION  4

class UserDataManager : public Singleton<UserDataManager>
{	
public:
	std::string				m_userName ="" ;	
	int						m_level =0;	
	
	std::map<int,int>		m_arrRecord;
	bool					m_soundOption = true;
	bool					m_cart = false;
	
	bool					m_isProgress = false;

	UserDataManager();
	~UserDataManager();


	void LoadUserData();
	void SaveUserData();

	void SetUserName(std::string& name);
	std::string& GetUserName();
	
	void SetLevel(int level) { m_level= level; }
	int GetLevel() { return m_level;  }

	int GetBestRecord(int level)
	{
		if (level < 3 || level >= MAX_PLAY_LEVEL) { return 0; }
		return m_arrRecord[level];
	}
	void SetBestRecord(int level, int time)
	{
		if (level < 3 || level >= MAX_PLAY_LEVEL) { return ; }
		m_arrRecord[level] = time;
	}

	bool GetSoundOpt() { return m_soundOption;  }
	void SetSoundOpt(bool opt) { m_soundOption = opt; }


	void SetCart(bool b) { m_cart = b; }
	bool GetCart() { return m_cart; }

	bool IsProgressing() { return m_isProgress; }

};
