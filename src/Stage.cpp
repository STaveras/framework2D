// File: Stage.cpp
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#include "Stage.h"
#include "Renderer.h"

void Stage::setVisibility(bool bVisible)
{
	if (bVisible)
	{
		std::list<Layer>::iterator itr = m_lsBackgroundLayers.begin();

		for(; itr != m_lsBackgroundLayers.end(); itr++)
			if (_renderList != NULL)
				_renderList->push_back(itr->m_pImage);

		_bVisible = true;
	}
	else
	{
		std::list<Layer>::iterator itr = m_lsBackgroundLayers.begin();

		// TODO : Add stage layer remove
		//for(; itr != m_lsBackgroundLayers.end(); itr++)
		//	Renderer::RemoveFromRenderList(itr->m_pImage);

		_bVisible = false;
	}
}

void Stage::AddLayer(Image* layer, vector2 position)
{ 
	layer->SetPosition(position); m_lsBackgroundLayers.push_back(Layer(layer, position));

	//if (_bVisible)
	//	for(; itr != m_lsBackgroundLayers.end(); itr++)
	//		if (_renderList != NULL)
	//			_renderList->push_back(itr->m_pImage);
}

void Stage::PopLayer(void)
{ 
	//if (_bVisible)
	//	Renderer::RemoveFromRenderList(m_lsBackgroundLayers.back().m_pImage);

	m_lsBackgroundLayers.pop_back();
}

void Stage::Move(vector2 amount)
{
	//std::list<StageLayer>::iterator itr = m_lsBackgroundLayers.begin();

	//for(; itr != m_lsBackgroundLayers.end(); itr++)
	//{
	//	itr->m_pImage->SetPosition(itr->m_pImage->GetPosition() + amount);
	//	amount = amount / 2;
	//}
}

bool Stage::LoadFromFile(const char* filepath)
{
	return false;
}

void Stage::Update(float fTime)
{
	// TODO : This will be for adding special background items
}

void Stage::Reset()
{
	std::list<Layer>::iterator itr = m_lsBackgroundLayers.begin();

	for(; itr != m_lsBackgroundLayers.end(); itr++)
		itr->m_pImage->SetPosition(itr->m_Position);
}