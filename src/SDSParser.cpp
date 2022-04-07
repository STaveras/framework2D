
#include "SDSParser.h"
#include "StrUtil.h"

#include <fstream>
#include <list>
#include <queue>
#include <string>

SDSParser::SDSParser(void):
m_pRoot(NULL),
m_pScope(NULL)
{

}

SDSParser::~SDSParser(void)
{
	if(m_pRoot)
		delete m_pRoot;
}

SDSNode* SDSParser::_NodeExists(const char* szName, SDSNodeData::SDSNodeType eType, SDSNode* pNode, SDSMetaNode** pMetaNode)
{
	if(!pNode)
		pNode = m_pRoot;

	std::queue<SDSNode*> q;
	q.push(pNode);

	std::queue<SDSMetaNode*> mq;

	if(pMetaNode)
		mq.push(*pMetaNode);

	while(!q.empty())
	{
		pNode = q.front();
		q.pop();

		if(pNode->getData().GetType() == eType && !strcmp(pNode->getData().GetName(), szName))
		{
			if(!mq.empty())
				*pMetaNode = mq.front();

			return pNode;
		}

		if(pNode->getData().GetType() == SDSNodeData::SDS_SCOPE)
		{
			if(!mq.empty())
			{
				for(size_t j = 0; j < mq.front()->getChildCount(); j++)
					mq.push(mq.front()->getChild(j));

				mq.pop();
			}
		}

		for(size_t i = 0; i < pNode->getChildCount(); i++)
		{
			q.push(pNode->getChild(i));
		}
	}

	return NULL;
}

SDSParser::SDS_ERROR SDSParser::open(const char* szFilename)
{
	std::ifstream ifl(szFilename);

	if(ifl.good())
	{
		SDS_ERROR error = SDS_ERR_NONE;
		SDSNodeData dataBuffer;

		SDSMetaNode* pMetaTree = new SDSMetaNode(), *pMetaCurr = pMetaTree;		
		SDSNode* pCurrent = pMetaCurr->getData().pSDSNode = m_pRoot = new SDSNode();

		m_pScope = NULL;

		bool ignore = true;
		char readByte;
		std::string buffer;

		while(ifl.good() && !error)
		{
			ifl.read(&readByte, 1);

			if(readByte == '{')
			{
				SDSNode* pResult = NULL;
				
				if(buffer != "")
					pResult = _NodeExists(buffer.c_str(), SDSNodeData::SDS_SCOPE, pCurrent, &pMetaCurr);
				else
				{
					const int max_size = std::numeric_limits<int>::digits10 + 1 /*sign*/ + 1 /*0-terminator*/;
					char scratch[max_size] = {0};
					#if _WIN32
					_itoa_s(pMetaCurr->getData().uiUnnamedScopes, scratch, 256, 10);
					#else
					// scratch = std::to_string(pMetaCurr->getData().uiUnnamedScopes).c_str();
					sprintf(scratch, "%zu", pMetaCurr->getData().uiUnnamedScopes);
					#endif
					pMetaCurr->getData().uiUnnamedScopes++;

					buffer = scratch;
				}

				if(!pResult)
				{
					pMetaCurr = new SDSMetaNode(pMetaCurr);

					pCurrent = pMetaCurr->getData().pSDSNode = new SDSNode(SDSNodeData(), pCurrent);
					pCurrent->getData().SetType(SDSNodeData::SDS_SCOPE);
					pCurrent->getData().SetName(buffer.c_str());
				}
				else
					pCurrent = pResult;
				
				buffer.clear();
			}
			else if(readByte == '}')
			{
				pMetaCurr = pMetaCurr->getParent();
				pCurrent = pCurrent->getParent();
			}
			else if(readByte == '=')
			{
				// If the buffer is empty, then this is syntactically incorrect, and we will not compensate by treating it as an "unnamed" property
				if(buffer == "")
				{
					error = SDS_ERR_SYNTAX;
					continue;
				}

				SDSNode* pResult = _NodeExists(buffer.c_str(), SDSNodeData::SDS_PROPERTY, pCurrent);

				if(!pResult)
				{
					pCurrent = new SDSNode(pCurrent->getData(), pCurrent);
					pCurrent->getData().SetType(SDSNodeData::SDS_PROPERTY);
					pCurrent->getData().SetName(buffer.c_str());
				}
				else
					pCurrent = pResult;

				buffer.clear();
			}
			else if(readByte == '\"' || readByte == '\'')
			{
				ignore = !ignore;
			}
			else if(readByte == ';')
			{
				while(ifl.good())
				{
					ifl.seekg(1, std::ios_base::cur);

					if(ifl.peek() == '\n')
						break;
				}
			}
			else
			{
				if(ignore && !strcspn(&readByte, " ,\t\b\n\0"))
				{
					//if()
					//{
					//	char scratch[256];
					//	_itoa_s(pMetaCurr->getData().uiUnnamedProperties, scratch, 256, 10);
					//	pMetaCurr->getData().uiUnnamedProperties++;

					//	pCurrent = new SDSNode(pCurrent->getData(), pCurrent);
					//	pCurrent->getData().SetType(SDSNodeData::SDS_PROPERTY);
					//	pCurrent->getData().SetName(scratch);
					//}

					if(buffer != "" && pCurrent->getData().GetType() == SDSNodeData::SDS_PROPERTY)
					{
						pCurrent->getData().SetValue(buffer.c_str());
						pCurrent = pCurrent->getParent();
						
						buffer.clear();
					}

					continue;
				}
				
				buffer += readByte;
			}
		}

		delete pMetaTree;

		// Verify the tree or something
		ifl.close();
		ifl.clear();

		return error;
	}

	ifl.close();
	ifl.clear();

	return SDS_ERR_INVALIDFILE;
}

void SDSParser::close(void)
{
	if(m_pRoot)
	{
		delete m_pRoot;
		m_pRoot = m_pScope = NULL;
	}
}

bool SDSParser::setScope(const char* szScope)
{
	std::list<std::string> lsScopes;
	strdiv(lsScopes, szScope, ".:");

	std::list<std::string>::iterator itr = lsScopes.begin();

	SDSNode* pResult = m_pRoot;

	for(; itr != lsScopes.end(); itr++)
	{
		pResult = _NodeExists(itr->c_str(), SDSNodeData::SDS_SCOPE, pResult);
	}

	if(pResult)
	{
		m_pScope = pResult;
		return true;
	}

	return false;
}

bool SDSParser::setProperty(const char* szName, const char* szValue)
{
	SDSNode* pNode = _NodeExists(szName, SDSNodeData::SDS_PROPERTY, m_pScope);

	if(pNode)
	{
		pNode->getData().SetValue(szValue);
		return true;
	}

	return false;
}

const char* SDSParser::getScope(void) const
{
	if(m_pScope)
		return m_pScope->getData().GetName();

	return "";
}

size_t SDSParser::getNumSubScopes(void) const
{
	size_t uiProps = 0;

	for(size_t i = 0; i < m_pScope->getChildCount(); i++)
	{
		if(m_pScope->getChild(i)->getData().GetType() == SDSNodeData::SDS_SCOPE)
			uiProps++;
	}

	return uiProps;
}

size_t SDSParser::getNumPropertiesInScope(void) const
{
	size_t uiProps = 0;

	for(size_t i = 0; i < m_pScope->getChildCount(); i++)
	{
		if(m_pScope->getChild(i)->getData().GetType() == SDSNodeData::SDS_PROPERTY)
			uiProps++;
	}

	return uiProps;
}

SDSNodeData* SDSParser::getProperty(const char* szName)
{
	SDSNode* pResult = _NodeExists(szName, SDSNodeData::SDS_PROPERTY, m_pScope);

	if(pResult)
		return &pResult->getData();

	return NULL;
}