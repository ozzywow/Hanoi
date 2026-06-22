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

void SoundFactory::fadeOutBGM(float duration)
{
	auto engine   = CocosDenshion::SimpleAudioEngine::getInstance();
	float startVol = engine->getBackgroundMusicVolume();
	const int STEPS = 20;
	float interval  = duration / STEPS;
	float volStep   = startVol / STEPS;
	float mastVol   = m_mastVolume;

	auto curVol   = std::make_shared<float>(startVol);
	auto remaining = std::make_shared<int>(STEPS);

	auto scheduler = cocos2d::Director::getInstance()->getScheduler();
	scheduler->unschedule("bgm_fadeout", (void*)this);
	scheduler->schedule(
		[=](float) mutable {
			*curVol = std::max(0.0f, *curVol - volStep);
			engine->setBackgroundMusicVolume(*curVol);
			(*remaining)--;
			if (*remaining <= 0) {
				engine->stopBackgroundMusic(true);
				engine->setBackgroundMusicVolume(mastVol);
			}
		},
		(void*)this, interval, (unsigned int)(STEPS - 1), 0.0f, false, "bgm_fadeout"
	);
}

void SoundFactory::preloadAll()
{
	const char* effects[] = {
		"efs_click", "efs_turn_playScene", "efs_start_countdown",
		"efs_count_sec", "efs_go", "efs_move_disc_ok",
		"efs_cancel_select", "efs_select_disc", "efs_clean", "efs_new_record"
	};
	for (auto name : effects)
	{
		std::string path = StringUtils::format("Sound/%s.wav", name);
		CocosDenshion::SimpleAudioEngine::getInstance()->preloadEffect(path.c_str());
	}
	CocosDenshion::SimpleAudioEngine::getInstance()->preloadBackgroundMusic("Sound/bgm_intro.wav");
	CocosDenshion::SimpleAudioEngine::getInstance()->preloadBackgroundMusic("Sound/bgm_play.wav");
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