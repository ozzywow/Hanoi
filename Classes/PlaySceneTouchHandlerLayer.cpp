#include "stdafx.h"

#include "PlaySceneTouchHandlerLayer.h"
#include "PlayScene.h"
#include "Discus.h"
#include "SoundFactory.h"
#include "UserDataManager.h"


void PlaySceneTouchHandlerLayer::Reset()
{
	m_pTouchingDiscus = NULL ;	
}


void PlaySceneTouchHandlerLayer::OnEnter()
{
	_touchListener = EventListenerTouchOneByOne::create();
	_touchListener->setSwallowTouches(true);

	_touchListener->onTouchBegan = CC_CALLBACK_2(PlaySceneTouchHandlerLayer::onTouchBegan, this);
	_touchListener->onTouchMoved = CC_CALLBACK_2(PlaySceneTouchHandlerLayer::onTouchMoved, this);
	_touchListener->onTouchCancelled = CC_CALLBACK_2(PlaySceneTouchHandlerLayer::onTouchCancelled, this);
	_touchListener->onTouchEnded = CC_CALLBACK_2(PlaySceneTouchHandlerLayer::onTouchEnded, this);

	EventDispatcher* dispatcher = Director::getInstance()->getEventDispatcher();
	dispatcher->addEventListenerWithSceneGraphPriority(_touchListener, this);

}


int PlaySceneTouchHandlerLayer::GetTouchedPoleID(Point convertedLocation)
{
	
	int touchedPoleID = -1 ;
	if(convertedLocation.x > 30/2 &&
	   convertedLocation.x < 330/2 )
	{
		touchedPoleID = 0 ;
	}
	else if(convertedLocation.x > 330/2 &&
			convertedLocation.x < 630/2 )
	{
		touchedPoleID = 1 ;
	}
	else if(convertedLocation.x > 630/2 &&
			convertedLocation.x < 930/2 )
	{
		touchedPoleID = 2 ;
	}

	return touchedPoleID ;
}


///////////////
// 처음 손가락이 화면에 닿는 순간 호출됩니다.
bool PlaySceneTouchHandlerLayer::onTouchBegan(Touch* touch, Event* unused_event)
{	
	Point location = touch->getLocation();
	
	
	if (NONE == m_pPlayScene->GetPlayState()) 
	{
		if (m_pPlayScene->m_countOfDiscus > MAX_LIMIT_LEVEL_FOR_LITE)
		{
			if (false == UserDataManager::Instance()->GetCart())
			{
				SoundFactory::Instance()->play("Cancel");
				return true;
			}
		}
		

		
		if (location.x > 50 )
		{
			m_pPlayScene->CountDown();			
		}
		
		return true;
	}

	if (COMPLATE == m_pPlayScene->GetPlayState())
	{
		m_pPlayScene->InitGame();
		m_pPlayScene->ResetGame();
		m_pPlayScene->DrawDiscus();
		m_pPlayScene->DrawInfoText();
		return true;
	}
			
    
	if( PLAY != m_pPlayScene->GetPlayState())
	{
		return  true;
	}
	
	
	
	Discus* pTouchedDiscus = NULL ;
	int touchedPoleID = GetTouchedPoleID(location) ;
	if( touchedPoleID != -1 )
	{	
		// 이미 원반이 선택되어있는 상태
		if( m_pTouchingDiscus != NULL && true == m_pTouchingDiscus->IsTouching()) 
		{
			int touchedPoleID = GetTouchedPoleID(location) ;
			if( touchedPoleID != -1 )
			{					
				bool isAbleToMove = m_pPlayScene->IsAbleToMoveDiscus(m_pTouchingDiscus, touchedPoleID) ;
				if (isAbleToMove) 
				{
					m_pPlayScene->AttachDiscusToPole(m_pTouchingDiscus, touchedPoleID) ;
					m_pPlayScene->SelectPole(touchedPoleID, isAbleToMove);
					SoundFactory::Instance()->play("move_ok") ;
				}
				else
				{
					SoundFactory::Instance()->play("Cancel") ;
				}
			}
			m_pPlayScene->SelectPole(-1, false);
			m_pTouchingDiscus->SetTouchingState(false);
			
			m_pTouchingDiscus = NULL;
			return  true;
		}
		else 
		{
			pTouchedDiscus = m_pPlayScene->GetTopDiscus(touchedPoleID);
			if( pTouchedDiscus )
			{
				pTouchedDiscus->SetTouchingState(true);
				int touchedPoleID = GetTouchedPoleID(location) ;
				if( touchedPoleID != -1 )
				{	
					m_pPlayScene->SelectPole(touchedPoleID, true);
				}
			}
		}

		
	}
	
	
	m_pTouchingDiscus = pTouchedDiscus ;	
    return  true;
	
	
}


// 손가락을 화면에서 떼지않고 이리저리 움직일 때 계속해서 호출됩니다.  얼마나 자주 호출되느냐는 전적으로

// 이벤트를 핸들링하는 애플리케이션의 Run Loop에 달려있습니다.

void PlaySceneTouchHandlerLayer::onTouchMoved(Touch* touch, Event* unused_event)
{	
	if (PLAY != m_pPlayScene->GetPlayState())
	{
		return ;
	}
	
	if( m_pTouchingDiscus == NULL ) return  ;
	
	Point location = touch->getLocation();


	// 손 위치가 있는 폴에 임시로 어테치함..	
	int touchedPoleID = GetTouchedPoleID(location);
	if( touchedPoleID != -1 )
	{	
		bool isAbleToMove = m_pPlayScene->IsAbleToMoveDiscus(m_pTouchingDiscus, touchedPoleID);
		m_pPlayScene->SelectPole(touchedPoleID, isAbleToMove);
	}	
	return ;
}


// 손가락을 화면에서 떼는 순간 호출되는 함수입니다.
void PlaySceneTouchHandlerLayer::onTouchCancelled(Touch* touch, Event* unused_event)
{	
	m_pTouchingDiscus = NULL ;
	return;	
}


void PlaySceneTouchHandlerLayer::onTouchEnded(Touch* touch, Event *unused_event)
{
	if (PLAY != m_pPlayScene->GetPlayState())
	{
		return;
	}


	if (m_pTouchingDiscus == NULL) return;



	Point location = touch->getLocation();


	int touchedPoleID = GetTouchedPoleID(location);
	if (touchedPoleID != -1)
	{
		if (m_pTouchingDiscus->GetCurrPoleID() == touchedPoleID)
		{
			// 여기로 빠지면 그냥 클릭이라는 거임..
			return;
		}
		else
		{

			bool isAbleToMove = m_pPlayScene->IsAbleToMoveDiscus(m_pTouchingDiscus, touchedPoleID);
			if (isAbleToMove)
			{
				m_pPlayScene->AttachDiscusToPole(m_pTouchingDiscus, touchedPoleID);
				m_pPlayScene->SelectPole(touchedPoleID, isAbleToMove);
				SoundFactory::Instance()->play("move_ok");
			}
			else
			{
				m_pPlayScene->SelectPole(-1, false);
				SoundFactory::Instance()->play("Cancel");
			}

		}
	}

	m_pTouchingDiscus = NULL;


	return;
	
}

