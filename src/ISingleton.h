
#pragma once

template<class T>
class ISingleton
{
	ISingleton(const ISingleton& i);
	ISingleton& operator=(const ISingleton& i);

public:
	static T* getInstance(void) { static T instance; return &instance; }
	//virtual void shutdown(void) = 0;

protected:
	ISingleton(void){}
	virtual ~ISingleton(void) = 0{}
};