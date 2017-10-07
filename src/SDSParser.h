
#pragma once

#include "SDSNode.h"

class SDSParser
{
public:
	enum SDS_ERROR { SDS_ERR_NONE, SDS_ERR_SYNTAX, SDS_ERR_INVALIDFILE};

private:
	SDSNode* m_pRoot;
	SDSNode* m_pScope;

	// Helper functions
	SDSNode* _NodeExists(const char* szName, SDSNodeData::SDSNodeType eType, SDSNode* pNode = NULL, SDSMetaNode** pMetaNode = NULL);

public:
	SDSParser(void);
	~SDSParser(void);

	SDS_ERROR open(const char* szFilename);
	void close(void);

	bool setScope(const char* szScope);
	bool setProperty(const char* szName, const char* szValue);

	const char* getScope(void) const;
	size_t getNumSubScopes(void) const;
	size_t getNumPropertiesInScope(void) const;
	SDSNodeData* getProperty(const char* szName);
};