// File: Stage.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010
// Notes: Stages are to provide layered, full-image backgrounds to provide a setting for game objects
#if !defined(_STAGE_H)
#define _STAGE_H
#include "Sprite.h"
#include "GameObject.h"
#include "Types.h"
#include "Plane.h"
#include "IRenderer.h"
#include <list>
#include <vector>
// Forward declarations
class Music;
class Stage
{
public:
	 struct Layer
	 {
		  Image* m_pImage; // Use Texture instead?
		  vector2 m_Position;

		  Layer(Image* pImage, vector2 position) :m_pImage(pImage), m_Position(position) {}
	 };

	 bool _bVisible;
	 Plane m_pGroundPlane;
	 IRenderer::RenderList* _renderList;
	 std::vector<Music*> m_lsRoundMusic;
	 std::list<Layer> m_lsBackgroundLayers;

	 // std::list<ObjectOperator*> _ruleSet;

public:
	 Stage(void) :_bVisible(false) {}

	 bool isVisible(void) const { return _bVisible; }
	 void setVisibility(bool bVisible);

	 Music* GetRoundMusic(unsigned int uiIndex) { return m_lsRoundMusic[uiIndex]; }
	 void AddRoundMusic(Music* pMusic) { m_lsRoundMusic.push_back(pMusic); }

	 void AddLayer(Image* layer, vector2 position);
	 void PopLayer(void);
	 void Move(vector2 amount);
	 bool LoadFromFile(const char* filepath);
	 void update(float fTime);
	 void Reset(void);
};

#endif  //_STAGE_H
