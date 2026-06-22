#include "stdafx.h"
#include "SoundFactory.h"

SoundFactory::SoundFactory(float initVol)
{
	m_soundIdMap.clear();
	setMasterVolume(initVol);
}

SoundFactory::~SoundFactory()
{	
}

void SoundFactory::play(char* soundFile, bool bOn, bool bBGM, bool bLoop)
{	
	std::string filePath = StringUtils::format("Sound/%s.wav", soundFile);
	if (bBGM)
	{
		m_strBGMfile = soundFile;

		if (false == CocosDenshion::SimpleAudioEngine::getInstance()->isBackgroundMusicPlaying())
		{
			CocosDenshion::SimpleAudioEngine::getInstance()->playBackgroundMusic(filePath.c_str(), bLoop);
		}		

		if(bOn)
		{
			CocosDenshion::SimpleAudioEngine::getInstance()->resumeBackgroundMusic();			
		}
		else
		{
			CocosDenshion::SimpleAudioEngine::getInstance()->pauseBackgroundMusic();
		}
	}
	else
	{
		int soundId = CocosDenshion::SimpleAudioEngine::getInstance()->playEffect(filePath.c_str());
		m_soundIdMap[std::string(soundFile)] = soundId;
	}		
}

void SoundFactory::setMasterVolume(float vol)
{
	m_mastVolume = vol;
	CocosDenshion::SimpleAudioEngine::getInstance()->setEffectsVolume(vol);
	CocosDenshion::SimpleAudioEngine::getInstance()->setBackgroundMusicVolume(vol);
}

void SoundFactory::stop(char* soundFile)
{
	if (m_strBGMfile == soundFile)
	{
		CocosDenshion::SimpleAudioEngine::getInstance()->stopBackgroundMusic(true);
	}
	else
	{
		int soundId = -1;
		auto itr = m_soundIdMap.find(std::string(soundFile));
		if (itr != m_soundIdMap.end())
		{
			soundId = itr->second;
		}
		CocosDenshion::SimpleAudioEngine::getInstance()->stopEffect(soundId);
	}
	
}