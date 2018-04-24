#pragma once

#include <stdio.h>
#include <assert.h>

template <typename T>
class Singleton
{
public:

	Singleton()
	{
		assert(Singleton::_instance == 0);
		Singleton::_instance = static_cast<T*>(this);
	}

	~Singleton()
	{
		if (Singleton::_instance != 0)
		{
			delete Singleton::_instance;
		}
		Singleton::_instance = 0;
	}

	static T* Instance() 
	{
		if (Singleton::_instance == 0)
		{
			Singleton::_instance = CreateInstance();
		}
		return Singleton::_instance;
	}



private:
	static T* _instance;
	static T* CreateInstance()
	{
		return new T();
	}
};

template<typename T>
T* Singleton<T>::_instance = 0;
