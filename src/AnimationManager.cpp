// File: AnimationManager.cpp
// Author: Stanley Taveras
// Created: 3/1/2010
// Modified: 3/1/2010

#include "AnimationManager.h"
#include "SDSParser.h"
#include "StrUtil.h"

Animation* AnimationManager::GetAnimation(const char* szName)
{
   // This is kinda dumb.
   for (unsigned int i = 0; i < this->Size(); i++) {
      if (this->At(i)->getName() == szName)
         return this->At(i);
   }
   return NULL;
}

Animation* AnimationManager::CreateAnimation(const char* szName, std::vector<Sprite*>* vSprites, int nTargetFPS)
{
   Animation* animation = this->GetAnimation(szName);

   if (!animation) {
      animation = this->Create();
      animation->setName(szName);
   }

	if (vSprites && !vSprites->empty()) {

		std::vector<Sprite*>::const_iterator itr = vSprites->begin();

		for (; itr != vSprites->end(); itr++)
			animation->createFrame((*itr), (float)(nTargetFPS / vSprites->size()));
	}

   //_renderList->push_back(animation);

   return animation;
}

void AnimationManager::DestroyAnimation(Animation* pAnimation)
{
   //for (IRenderer::RenderList::iterator i = _renderList->begin(); i != _renderList->end(); i++)
   //   if ((*i) == pAnimation)
   //      _renderList->erase(i);

   this->Destroy(pAnimation);
}

void AnimationManager::DestroyAnimation(const char* szName)
{
   for (Factory<Animation>::const_factory_iterator itr = this->Begin(); itr != this->End(); itr++)
      if (!strcmp((*itr)->getName(), szName))
         return DestroyAnimation(*itr);
}

void AnimationManager::update(float fTime)
{
   Factory<Animation>::const_factory_iterator itr = this->Begin();

   for (; itr != this->End(); itr++) {
      (*itr)->update(fTime);
   }
}

//Animation* AnimationManager::LoadAnimationFromFile(const char* szFilename)
//{
//	Animation* pAnimation = NULL;
//
//	SDSParser parser;
//
//	if(!parser.open(szFilename))
//	{
//		if(parser.setScope("Animation"))
//		{
//			SDSNodeData* pData = parser.getProperty("Name");
//
//			if(pData)
//			{				
//				pAnimation = _AnimationExists(pData->GetValue()); 
//
//				if(!pAnimation)
//				{
//					pAnimation = m_Animations.Create();
//					pAnimation->setName(pData->GetValue());
//				}
//
//				pData = parser.getProperty("Mode");
//
//				if(pData)
//				{
//					if(streqls(pData->GetValue(), "Loop"))
//						pAnimation->setMode(ANIMATION_MODE_LOOP);
//					else if(streqls(pData->GetValue(), "Once") || pData->GetValueAsInt() == 1)
//						pAnimation->setMode(ANIMATION_MODE_ONCE);
//					else if(streqls(pData->GetValue(), "Oscillate") || pData->GetValueAsInt() >= 2)
//						pAnimation->setMode(ANIMATION_MODE_OSCILLATE);
//				}
//
//				pData = parser.getProperty("Forward");
//
//				if(pData)
//				{
//					if(streqls(pData->GetValue(), "false"))
//						pAnimation->setIsForward(false);
//					else
//						pAnimation->setIsForward(true);
//				}
//
//				pData = parser.getProperty("Speed");
//
//				if(pData)
//					pAnimation->setSpeed((float)pData->GetValueAsDouble());
//
//				if(parser.setScope("Animation:Frames"))
//				{
//					char buffer[256];
//
//					int frames = (int)parser.getNumSubScopes();
//
//					for(int i = 0; i < frames; i++)
//					{
//						sprintf_s(buffer, 256, "Animation:Frames:%d", i);
//
//						if(parser.setScope(buffer))
//						{
//							Frame newFrame;
//
//							pData = parser.getProperty("Duration");
//
//							if(pData)
//								newFrame.SetDuration((float)pData->GetValueAsDouble());
//
//							pData = parser.getProperty("Image");
//
//							if(pData)
//								newFrame.SetSprite(_pSpriteManager->LoadSpriteFromFile(pData->GetValue()));
//
//							sprintf_s(buffer, 256, "Animation:Frames:%d:Anchor", i);
//
//							if(parser.setScope(buffer))
//							{
//								vector2 anchor;
//
//								pData = parser.getProperty("X");
//
//								if(pData)
//									anchor.x = (float)pData->GetValueAsDouble();
//
//								pData = parser.getProperty("Y");
//
//								if(pData)
//									anchor.y = (float)pData->GetValueAsDouble();
//							}
//
//							sprintf_s(buffer, 256, "Animation:Frames:%d:Triggers", i); // Ok. We need a "setSubScope" function in the SDS parser
//
//							if(parser.setScope(buffer))
//							{
//								int triggers = (int)parser.getNumPropertiesInScope();
//
//								for(int t = 0; t < triggers; t++)
//								{
//									_itoa_s(t, buffer, 256, 10);
//									pData = parser.getProperty(buffer);
//
//									if(pData)
//									{
//										Trigger newTrigger;
//										newTrigger.LoadFromFile(pData->GetValue());
//
//										newFrame.AddTrigger(newTrigger);
//									}
//								}
//							} // Trigger
//
//							pAnimation->addFrame(newFrame);
//						} // Frame
//					} // for
//				} // Frames
//			}
//		} // ::Animation
//
//		parser.close();
//	}
//
//	return pAnimation;
//}