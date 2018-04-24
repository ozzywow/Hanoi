#pragma once

#include <SimpleAudioEngine.h>
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

	void play(char* soundFile, bool bPuase = false, bool bBGM = false);
	void stop(char* soundFile);


};
