#pragma once

#include "cocos2d.h"
using namespace cocos2d;

class Discus : public Sprite 
{
public:	
	static Discus* createScene(int discusID)
	{
		Discus* pRes = Discus::create();
		pRes->initWithDiscusID(discusID);
		return pRes;
	}
	CREATE_FUNC(Discus);

	int			m_discusID ;
	int			m_currPoleID ;
	bool		m_isTouching ;


	Discus() : m_discusID(0), m_currPoleID(0), m_isTouching(false)
	{
	};	
	virtual bool init();


	Discus*		initWithDiscusID(int discusID);
	int			GetDiscusID();
	int			GetCurrPoleID();
	void		SetCurrPoleID(int poleID);
	bool		IsTouching();
	void		SetTouchingState(bool bIsTouching);
};
