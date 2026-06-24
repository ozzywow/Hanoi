#pragma once

#include "cocos2d.h"
using namespace cocos2d;

#include "Singleton.h"
#include <string>
#include <map>
#include <ctime>

#define MAX_LEVEL_FOR_LITE_VERTION  4

static constexpr double kRecordExpirySec = 12.0 * 3600.0;  // 12h

class UserDataManager : public Singleton<UserDataManager>
{
public:
	std::string				m_userName ="" ;
	int						m_level =0;

	std::map<int,int>		m_arrRecord;
	double					m_recordTimestamp = 0.0;  // 첫 기록 시각 (unix epoch); 0 = 기록 없음
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
		if (level < 3 || level > MAX_PLAY_LEVEL) { return 0; }
		return m_arrRecord[level];
	}
	void SetBestRecord(int level, int time)
	{
		if (level < 3 || level > MAX_PLAY_LEVEL) { return; }
		m_arrRecord[level] = time;
		if (m_recordTimestamp == 0.0)
			m_recordTimestamp = static_cast<double>(::time(nullptr));
	}
	void ResetRecords()
	{
		for (auto& kv : m_arrRecord) kv.second = 0;
		m_recordTimestamp = 0.0;
		SaveUserData();
	}

	bool GetSoundOpt() { return m_soundOption;  }
	void SetSoundOpt(bool opt) { m_soundOption = opt; }


	void SetCart(bool b) { m_cart = b; }
	bool GetCart() { return m_cart; }

	bool IsProgressing() { return m_isProgress; }

	// In-memory only: score pending submission after first-play name entry
	int  m_pendingSubmitLevel = 0;
	int  m_pendingSubmitTime  = 0;
	bool m_justRegistered     = false;
	bool m_justGotNewRecord   = false;  // PlayFab 전파 지연 보상용 — 신기록 갱신 후 2초 재갱신 트리거

	void SetPendingSubmit(int level, int time) { m_pendingSubmitLevel = level; m_pendingSubmitTime = time; }
	bool HasPendingSubmit() const              { return m_pendingSubmitTime > 0; }
	void ClearPendingSubmit()                  { m_pendingSubmitLevel = 0; m_pendingSubmitTime = 0; }

};
