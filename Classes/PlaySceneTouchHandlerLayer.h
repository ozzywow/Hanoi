#pragma once

#include "cocos2d.h"
using namespace cocos2d;

class PlayScene ;
class Discus ;

class PlaySceneTouchHandlerLayer : public Layer 
{
public:
	static PlaySceneTouchHandlerLayer* createAppleTouchedHandleLayer(PlayScene* p)
	{
		PlaySceneTouchHandlerLayer* pRes = PlaySceneTouchHandlerLayer::create();
		pRes->isTouchEnabled = true;
		pRes->m_pPlayScene = p;
		pRes->m_pTouchingDiscus = NULL;

		pRes->OnEnter();

		return pRes;
	}
	CREATE_FUNC(PlaySceneTouchHandlerLayer);

	virtual bool init()
	{
		m_pPlayScene = NULL;
		m_pTouchingDiscus = NULL;		
		return true;
	}

	EventListenerTouchOneByOne* _touchListener;
	PlayScene*		m_pPlayScene ;
	Discus*			m_pTouchingDiscus ;
	bool			isTouchEnabled;

		
	void Reset();
	int GetTouchedPoleID(Point convertedLocation);

	void OnEnter();
	virtual bool onTouchBegan(Touch* touch, Event* unused_event);
	virtual void onTouchMoved(Touch* touch, Event* unused_event);
	virtual void onTouchCancelled(Touch* touch, Event* unused_event);
	virtual void onTouchEnded(Touch* touch, Event *unused_event);
};

