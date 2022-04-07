
#pragma once

template<class T>
class ISingleton
{
	ISingleton(const ISingleton& i);
	ISingleton& operator=(const ISingleton& i);

public:
	static T* getInstance(void) { static T instance; return &instance; }

protected:
#if __cplusplus > 199711L
	ISingleton(void) = default;
	virtual ~ISingleton(void) = default;
#else
	ISingleton(void){}
	virtual ~ISingleton(void) = 0{}
#endif
};