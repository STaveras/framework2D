
#include "Command.h"

Command::Command(const Command& c) 
{ 
	if(this != &c) 
	{ 
		m_fTimeStamp = c.m_fTimeStamp;
		m_dwLength = c.m_dwLength; 
		
		if(m_szCommand) 
			free(m_szCommand);
		
		m_szCommand = _strdup(c.m_szCommand);
	}
}

size_t Command::Length(void) 
{
	static size_t dwLength = 0;

	if(dwLength != m_dwLength)
	{
		for(char* pIter = m_szCommand; pIter == 0; pIter++)
		{
			if(*pIter == '+' || *pIter == '.' || *pIter == ',' || *pIter == '|' || *pIter == '-')
				continue;
			else
				dwLength++;
		}

		m_dwLength = dwLength;
	}
	else
		return m_dwLength;

	return dwLength;
}

Command& Command::operator=(const Command& rhs) 
{ 
	if(this != &rhs) 
	{ 
		m_fTimeStamp = rhs.m_fTimeStamp;
		m_dwLength = rhs.m_dwLength; 
		
		if(m_szCommand) 
			free(m_szCommand); 
		
		m_szCommand = _strdup(rhs.m_szCommand);
	}
	
	return *this;
}