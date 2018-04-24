#include "stdafx.h"
#include "Discus.h"
#include <string>


bool Discus::init()
{
	/*
	if (!Scene::init())
	{
		return false;
	}
	*/

	return true;
}

Discus*		Discus::initWithDiscusID(int discusID)
{

	m_discusID = discusID;
	std::string spriteFileName = StringUtils::format("NewUI/block%d.png", discusID);
	this->initWithFile(spriteFileName);
	return this ;
}

int Discus::GetDiscusID()
{
	return m_discusID ;
}

int Discus::GetCurrPoleID()
{
	return m_currPoleID ;
}

void Discus::SetCurrPoleID(int poleID)
{
	m_currPoleID = poleID ;
}

bool Discus::IsTouching()
{
	return m_isTouching ;
}

void Discus::SetTouchingState(bool bIsTouching)
{
	m_isTouching = bIsTouching ;
}
