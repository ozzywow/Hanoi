#pragma once

#include "cocostudio/SimpleAudioEngine.h"
#include "Singleton.h"
#include <string>


class SoundFactory : public Singleton<SoundFactory>
{
public:

	float m_mastVolume;
	std::string m_strBGMfile="";

	std::map<std::string, int> m_soundIdMap;

	SoundFactory(float initVol = 1);
	~SoundFactory();

	void play(char* soundFile, bool bPuase = false, bool bBGM = false, bool bLoop = true);
	void stop(char* soundFile);
	void fadeOutBGM(float duration = 1.0f);
	void switchBGM(const char* soundFile, bool loop = true);
	void crossfadeToTrack(const char* soundFile, float fadeDuration = 0.3f);
	bool isBGMPlaying() const;
	void setMasterVolume(float vol);
	void preloadAll();


};
