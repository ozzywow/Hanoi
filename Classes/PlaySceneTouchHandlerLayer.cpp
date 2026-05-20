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
// 泥섏쓬 ?먭??쎌씠 ?붾㈃???용뒗 ?쒓컙 ?몄텧?⑸땲??
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
		// ?대? ?먮컲???좏깮?섏뼱?덈뒗 ?곹깭
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


// ?먭??쎌쓣 ?붾㈃?먯꽌 ?쇱??딄퀬 ?대━?由??吏곸씪 ??怨꾩냽?댁꽌 ?몄텧?⑸땲??  ?쇰쭏???먯＜ ?몄텧?섎뒓?먮뒗 ?꾩쟻?쇰줈

// ?대깽?몃? ?몃뱾留곹븯???좏뵆由ъ??댁뀡??Run Loop???щ젮?덉뒿?덈떎.

void PlaySceneTouchHandlerLayer::onTouchMoved(Touch* touch, Event* unused_event)
{	
	if (PLAY != m_pPlayScene->GetPlayState())
	{
		return ;
	}
	
	if( m_pTouchingDiscus == NULL ) return  ;
	
	Point location = touch->getLocation();


	// ???꾩튂媛 ?덈뒗 ?댁뿉 ?꾩떆濡??댄뀒移섑븿..	
	int touchedPoleID = GetTouchedPoleID(location);
	if( touchedPoleID != -1 )
	{	
		bool isAbleToMove = m_pPlayScene->IsAbleToMoveDiscus(m_pTouchingDiscus, touchedPoleID);
		m_pPlayScene->SelectPole(touchedPoleID, isAbleToMove);
	}	
	return ;
}


// ?먭??쎌쓣 ?붾㈃?먯꽌 ?쇰뒗 ?쒓컙 ?몄텧?섎뒗 ?⑥닔?낅땲??
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
			// ?ш린濡?鍮좎?硫?洹몃깷 ?대┃?대씪??嫄곗엫..
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

