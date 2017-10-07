
#pragma once

#include <functional>
#include <string>

class Command
{
	float m_fTimeStamp;
	size_t m_dwLength;
	char* m_szCommand;

public:
	Command(void):m_fTimeStamp(0.0f),m_dwLength(0),m_szCommand(NULL) {}
	Command(const char* szCommand):m_fTimeStamp(0.0f),m_dwLength(0),m_szCommand(_strdup(szCommand)) {}
	Command(const Command& c);
	~Command(void) { if(m_szCommand) free(m_szCommand); }

	float GetTimeStamp(void) const { return m_fTimeStamp; }
	char* GetCommandString(void) { return m_szCommand; }

	void SetTimeStamp(float fTime) { m_fTimeStamp = fTime; }

	size_t Length(void);

	struct less : public std::binary_function<Command*, Command*, bool>
	{
		inline bool operator()(Command* lhs, Command* rhs) const { return (*lhs < *rhs); }
	};

	Command& operator=(const Command& rhs);
	bool operator<(const Command& rhs) const { return (strlen(m_szCommand) < strlen(rhs.m_szCommand)); }
	bool operator==(const Command& rhs) const { return !(strcmp(m_szCommand, rhs.m_szCommand)); }
};