// File: Animation.h
#pragma once
#include "Renderable.h"
#include "Frame.h"
#include "SpriteManager.h"
#include <string>
#include <vector>
class Animation : public Renderable
{
public:
   enum Mode
   {
      eOnce = 1,
      eOscillate,
      eLoop
   };
private:

   Mode m_eMode;

   unsigned int _frameIndex;
   bool m_bForward;
   bool m_bPlaying;
   float m_fSpeed;
   float m_fTimer;
   std::string m_szName;
   Factory<Frame> m_Frames;

protected:
   void _advanceFrame(void);
   void _lastFrame(void);
   void _reset(void);

public:
   Animation(void);
   Animation(const char* szName);

   Mode getMode(void) const { return m_eMode; }
   bool isForward(void) const { return m_bForward; }
   bool isPlaying(void) const { return m_bPlaying; }
   float getSpeed(void) const { return m_fSpeed; }
   const char* getName(void) const { return m_szName.c_str(); }
   size_t getCurrentFrameIndex(void) const { return _frameIndex; }
   size_t getFrameCount(void) { return m_Frames.Size(); }
   Frame* getCurrentFrame(void) { return m_Frames[_frameIndex]; }
   Frame* operator[](unsigned int i) { return m_Frames[i]; }

   void setIsForward(bool bForward) { m_bForward = bForward; }
   void setMode(Mode eMode) { m_eMode = eMode; }
   void setSpeed(float fSpeed) { m_fSpeed = fSpeed; }
   void setName(const char* szName) { m_szName = szName; }
   void setPosition(float x, float y) { setPosition(vector2(x, y)); }
   void setPosition(vector2 position);

   Frame* createFrame(Sprite* sprite, float duration = 0.0f);

   void addFrame(Frame* frame) { m_Frames.Store(frame); }
   void removeFrame(Frame* frame);

   // If you've set individual frame durations, this will overwrite them all
   void setFrameRate(int targetFrameRate) {
      float frameTime = (1.0f / targetFrameRate) * m_Frames.Size();
      for (unsigned int i = 0; i < m_Frames.Size(); i++) {
         m_Frames[i]->SetDuration(frameTime);
      }
   }

   void play(void) { this->_reset(); m_bPlaying = true; }
   void pause(void) { m_bPlaying = false; }
   void resume(void) { m_bPlaying = true; }
   void stop(void) { _reset(); m_bPlaying = false; }

   void mirror(bool horizontal, bool vertical);

   void center(void) {
      for (unsigned int i = 0; i < m_Frames.Size(); i++) {
         m_Frames[i]->GetSprite()->center();
      }
   }

   bool update(float time); // Should we always force the user to use animation managers and not be able to call this?
   bool operator==(const Animation& a) const;
};
// Author: Stanley Taveras