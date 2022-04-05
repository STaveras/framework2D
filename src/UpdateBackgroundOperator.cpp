
#include "UpdateBackgroundOperator.h"

void UpdateBackgroundOperator::_updatePos(vector2& lastPos, short axis, short flip)
{
   lastPos.x += (axis == 0 ? flip * _background->getWidth() : 0);
   lastPos.y += (axis == 1 ? flip * _background->getHeight() : 0);
}

void UpdateBackgroundOperator::_updateCache(void) {

    if (_background) {

        short flip = 1;
        short axis = 0;
        short count = 0;

        vector2 lastPos = _background->getPosition();

        // We need a better way to make a spiral
        // TODO: Needs to be a generalized math function to make it any size, to support zooming (just cus)
        for (int i = 0; i < 8; i++) {

            if (_firstRun) {

                _cache.push_back(new Image(*_background));

                if (_renderList)
                    _renderList->push_front(_cache.back());

                _cache.back()->center();
            }

            // Ugh I hate the below so bad but my brain isn't working as good as it used to... D:
            if (i < 2)
            {
                _updatePos(lastPos, axis++, flip);

                if (axis > 1)
                {
                    axis = 0;
                    flip = -flip;
                }
            }
            else {

                if (i == 5)
                    flip = -flip;

                _updatePos(lastPos, axis, flip);

                if (i % 2 == 1)
                    axis = !axis;
            }

            _cache.back()->setPosition(lastPos);
        }

        _firstRun = false;
    }
}

void UpdateBackgroundOperator::_clearCache(void) 
{
	 while (!_cache.empty()) {
		  _renderList->remove(_cache.back());
		  delete _cache.back();
		  _cache.pop_back();
	 }
	 _firstRun = true;
}