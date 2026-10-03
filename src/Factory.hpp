// File: Factory.cpp
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 1/16/2023

template<typename Type>
Type* Factory<Type>::at(unsigned int index)
{
	typename std::list<Type *>::iterator itr = this->begin();
	for (unsigned int i = 0; itr != this->end(); itr++, i++) {
		if (i == index)
			return (*itr);
	}
	return NULL;
}

template<typename Type>
const Type* Factory<Type>::at(unsigned int index) const
{
	typename std::list<Type*>::const_iterator itr = this->begin();
	for (unsigned int i = 0; itr != this->end(); itr++, i++) {
		if (i == index)
			return (*itr);
	}
	return NULL;
}

template<typename Type>
void Factory<Type>::_release(Type* item)
{
	if (_borrowed.erase(item) == 0) {
		delete item;
	}
}

template<typename Type>
Type* Factory<Type>::create()
{
	Type* item = new Type();
	_borrowed.erase(item);
	this->push_back(item);
	return item;
}

template<typename Type>
Type* Factory<Type>::create(const Type& rhs)
{
	Type* item = new Type(rhs);
	_borrowed.erase(item);
	this->push_back(item);
	return item;
}

template<typename Type>
void Factory<Type>::destroy(Type* item)
{
	typename std::list<Type*>::iterator itr = this->begin();

	for (;itr != this->end(); itr++)
	{
		if ((*itr) == item)
		{
			_release(*itr);
			std::list<Type*>::erase(itr);
			break;
		}
	}
}

template<typename Type>
void Factory<Type>::clear()
{
	typename std::list<Type*>::iterator itr = this->begin();

	for(;itr != this->end(); itr++) {
		_release(*itr);
	}

	std::list<Type*>::clear();
	_borrowed.clear();
}

template<typename Type>
void Factory<Type>::store(Type* item, bool owned)
{
	if (owned) {
		_borrowed.erase(item);
	}
	else {
		_borrowed.insert(item);
	}
	this->push_back(item);
}

template<typename Type>
void Factory<Type>::erase(unsigned int index)
{
	typename std::list<Type*>::iterator itr = this->begin();

	for (unsigned int i = 0; itr != this->end(); itr++, i++)
	{
		if (i == index) {
			this->erase(itr);
			return;
		}
	}
}

template<typename Type>
void Factory<Type>::erase(factory_iterator itr)
{
	_release(*itr);
	std::list<Type*>::erase(itr);
}

template<typename Type>
void Factory<Type>::erase(const_factory_iterator itr) 
{
	_release(*itr);
	std::list<Type*>::erase(itr);
}

template<typename Type>
Type* Factory<Type>::find(const Type& itemDesc)
{
	Type* pReturn = NULL;
	typename std::list<Type*>::iterator itr = this->begin();

	for (;itr != this->end(); itr++) {
		if (itemDesc == *(*itr)) {
			pReturn = (*itr);
		}
	}
	return pReturn;
}

template<typename Type>
template<class Derived>
Derived* Factory<Type>::createDerived(void)
{
	Derived* item = new Derived();
	_borrowed.erase((Type*)item);
	this->push_back((Type*)item); // Would using dynamic cast here be safer...?
	return item;
}

template<typename Type>
template<class Derived>
Derived* Factory<Type>::createDerived(const Derived& rhs)
{
	Derived* item = new Derived(rhs);
	_borrowed.erase((Type*)item);
	this->push_back((Type*)item);
	return item;
}

template<class Type>
bool Factory<Type>::operator==(Factory<Type>& f)
{
   return (this == (const_cast<const Factory<Type>>(f))); // Why is this here again...?
}

template<typename Type>
bool Factory<Type>::operator==(const Factory<Type>& f) const
{
	if (this->size() == f.size())
	{
		typename std::list<Type*>::const_iterator i = this->begin();
		typename std::list<Type*>::const_iterator j = f.begin();

		for (; i != this->end(); i++, j++)
			if ((*i) != (*j))
				return false;

	}
	return true;
}
