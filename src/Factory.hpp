// File: Factory.cpp
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 3/29/2010

template<typename Type>
Type* Factory<Type>::At(unsigned int index)
{
	std::list<Type*>::iterator itr = m_lsItems.begin();
	for(unsigned int i = 0; itr != m_lsItems.end(); itr++, i++)
	{
		if(i == index)
			return (*itr);
	}
	throw "Not in range or not found.";
	return NULL;
}

template<typename Type>
Type* Factory<Type>::Create()
{
	Type* item = new Type();
	m_lsItems.push_back(item);

	return item;
}

template<typename Type>
Type* Factory<Type>::Create(const Type& rhs)
{
	Type* item = new Type(rhs);
	m_lsItems.push_back(item);

	return item;
}

template<typename Type>
void Factory<Type>::Destroy(Type* item)
{
	std::list<Type*>::iterator itr = m_lsItems.begin();

	for(;itr != m_lsItems.end(); itr++)
	{
		if((*itr) == item)
		{
			delete (*itr);
			m_lsItems.erase(itr);
			break;
		}
	}
}

template<typename Type>
void Factory<Type>::Clear()
{
	std::list<Type*>::iterator itr = m_lsItems.begin();

	for(;itr != m_lsItems.end(); itr++)
	{
		delete (*itr);
	}

	m_lsItems.clear();
}

template<typename Type>
void Factory<Type>::Store(Type* item)
{	
	m_lsItems.push_back(item);
}

template<typename Type>
void Factory<Type>::Erase(unsigned int index)
{
	std::list<Type*>::iterator itr = m_lsItems.begin();

	for(unsigned int i = 0; itr != m_lsItems.end(); itr++, i++)
	{
		if(i == index)
		{
			m_lsItems.erase(itr);
		}
	}
}

template<typename Type>
Type* Factory<Type>::Find(const Type& itemDesc)
{
	Type* pReturn = NULL;
	std::list<Type*>::iterator itr = m_lsItems.begin();

	for(;itr != m_lsItems.end(); itr++)
	{
		if(itemDesc == *(*itr))
		{
			pReturn = (*itr);
		}
	}

	return pReturn;
}

template<typename Type>
template<class Derived>
Derived* Factory<Type>::CreateDerived(void)
{
	Derived* item = new Derived();
	m_lsItems.push_back((Type*)item); // Would using dynamic cast here be safer...?

	return item;
}

template<typename Type>
template<class Derived>
Derived* Factory<Type>::CreateDerived(const Derived& rhs)
{
	Derived* item = new Derived(rhs);
	m_lsItems.push_back((Type*)item);

	return item;
}

template<typename Type>
bool Factory<Type>::operator==(const Factory<Type>& f) const
{
	if (this->Size() == f.Size())
	{
		std::list<Type*>::const_iterator i = m_lsItems.begin();
		std::list<Type*>::const_iterator j = f.m_lsItems.begin();

		for (; i != m_lsItems.end(); i++, j++)
			if ((*i) != (*j))
				return false;
	}

	return true;
}