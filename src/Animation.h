// File: Animation.h
#pragma once
#include "Renderable.h"
#include "ANIMATION_MODE.h"
#include "Frame.h"
#include "SpriteManager.h"
#include <string>
#include <vector>
class Animation : public Renderable
{
    ANIMATION_MODE m_eMode;
    bool m_bForward;
    bool m_bPlaying;
	unsigned int _frameIndex;
    float m_fSpeed;
    float m_fTimer;
    std::string m_szName;
	Factory<Frame> m_Frames;

protected:
	void _AdvanceFrame(void);
	void _LastFrame(void);
    void _Reset(void);

public:
    Animation(void);
    Animation(const char* szName);

    ANIMATION_MODE GetMode(void) const { return m_eMode; }
    bool IsForward(void) const { return m_bForward; }
    bool IsPlaying(void) const { return m_bPlaying; }
    float GetSpeed(void) const { return m_fSpeed; }
    const char* GetName(void) const { return m_szName.c_str(); }
	unsigned int GetCurrentFrameIndex(void) const { return _frameIndex; }
	unsigned int GetFrameCount(void) { return m_Frames.Size(); }
    Frame* GetCurrentFrame(void) { return m_Frames[_frameIndex]; }
	Frame* operator[](unsigned int i) { return m_Frames[i]; }

    void SetIsForward(bool bForward) { m_bForward = bForward; }
    void SetMode(ANIMATION_MODE eMode) { m_eMode = eMode; }
    void SetSpeed(float fSpeed) { m_fSpeed = fSpeed; }
    void SetName(const char* szName) { m_szName = szName; }
	void setPosition(float x,float y) { setPosition(vector2(x,y)); }
    void setPosition(vector2 position);

    void Mirror(bool bHorizontal, bool bVertical);

	Frame* AddFrame(Sprite* sprite);
	Frame* AddFrame(Sprite* sprite, float duration);
	void AddFrame(Frame* frame) { m_Frames.Store(frame); }
    void RemoveFrame(Frame* frame);

    void Play(void) { this->_Reset(); m_bPlaying = true; }
    void Pause(void) { m_bPlaying = false; }
    void Resume(void) { m_bPlaying = true; }
    void Stop(void) { _Reset(); m_bPlaying = false; }

    bool update(float time); // Should we always force the user to use animation managers and not be able to call this?
	bool operator==(const Animation& a)const;

	static Animation* loadAnimationsFrom(const char* szFilename);
};
// Author: Stanley Taveras