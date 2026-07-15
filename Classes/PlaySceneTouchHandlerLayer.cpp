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

	if (m_pPlayScene->m_isReplaying)                // 재생 중 탭 = 건너뛰기(최종 상태로 점프)
	{
		m_pPlayScene->skipReplay();
		return true;
	}
	if (m_pPlayScene->m_isSpectate)                 // 관전: 재생 시작 전 카운트다운 방지
		return true;


	if (NONE == m_pPlayScene->GetPlayState())
	{
		if (m_pPlayScene->m_countOfDiscus > MAX_LIMIT_LEVEL_FOR_LITE)
		{
			if (false == UserDataManager::Instance()->GetCart())
			{
				SoundFactory::Instance()->play("efs_cancel_select");
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
		int shownTime = m_pPlayScene->m_popupShownTime;
		if (shownTime == 0 || (getMilliCount() - shownTime) < 2000)
			return true;
		SoundFactory::Instance()->play("efs_click");
		// 다음 레벨로 넘어가지 않고 현재 레벨에서 다시 시작 (힌트 "TAP TO PLAY AGAIN"과 일치).
		// 새 씬으로 교체해 타이머/RPM/리플레이 버퍼/팝업을 전부 초기화.
		PlayScene* sameScene = PlayScene::createScene(m_pPlayScene->m_countOfDiscus);
		Director::getInstance()->replaceScene(TransitionFade::create(0.2f, sameScene));
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
			if( touchedPoleID != -1 )
			{
				bool isAbleToMove = m_pPlayScene->IsAbleToMoveDiscus(m_pTouchingDiscus, touchedPoleID) ;
				if (isAbleToMove)
				{
					m_pPlayScene->AttachDiscusToPole(m_pTouchingDiscus, touchedPoleID) ;
					m_pPlayScene->SelectPole(touchedPoleID, isAbleToMove);
					SoundFactory::Instance()->play("efs_move_disc_ok");
				}
				else
				{
					m_pPlayScene->recordReplayEvent(EV_MOVE_FAIL, touchedPoleID);   // 실패(탭 경로)
					SoundFactory::Instance()->play("efs_cancel_select") ;
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
				m_pPlayScene->SelectPole(touchedPoleID, true);
				m_pPlayScene->recordReplayEvent(EV_SELECT, touchedPoleID);   // 폴 선택
				SoundFactory::Instance()->play("efs_select_disc");
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
	if (m_pPlayScene->m_isReplaying) return;   // 재생 중 입력 차단
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
	if (m_pPlayScene->m_isReplaying) return;   // 재생 중 입력 차단
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
				SoundFactory::Instance()->play("efs_move_disc_ok");
			}
			else
			{
				m_pPlayScene->recordReplayEvent(EV_MOVE_FAIL, touchedPoleID);   // 실패(드래그 경로)
				m_pPlayScene->SelectPole(-1, false);
				SoundFactory::Instance()->play("efs_cancel_select");
			}

		}
	}

	m_pTouchingDiscus = NULL;


	return;
	
}

