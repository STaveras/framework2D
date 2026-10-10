// File: Widgets.cpp
// Author: Stanley Taveras
// Purpose: Implementation of the reusable screen-space UI widgets (Menu).

#include "Widgets.h"
#include "Debug.h"
#include "Engine2D.h"

Menu::Menu(IRenderer* renderer, const std::string& fontPath, const vector2& top,
           float scale, float itemSpacing)
    : _renderer(renderer)
    , _renderList(NULL)
    , _top(top)
    , _scale(scale)
    , _itemSpacing(itemSpacing)
{
    if (renderer) {
        _renderList = renderer->createRenderList(true); // screenSpace
    }
    if (!_renderList) {
        DEBUG_MSG("Menu: no screen-space render list available\n");
    }

    // The font is stored and loaded per item in addItem(), so a bad font
    // path degrades gracefully (item fails) instead of breaking the menu.
    _fontPath = fontPath;
}

Menu::~Menu()
{
    _destroyItems();
    if (_renderList) {
        if (_renderer) {
            _renderer->destroyRenderList(_renderList);
        }
        _renderList = NULL;
    }
}

int Menu::addItem(const std::string& label)
{
    if (!_renderList) {
        return -1;
    }

    Font* text = new Font();
    if (!text->loadFromJSON(_fontPath)) {
        DEBUG_MSG(("Menu: failed to load font from: " + _fontPath + "\n").c_str());
        SAFE_DELETE(text);
        return -1;
    }

    Item item;
    item.text = text;
    item.label = label;
    item.position = vector2(0.0f, 0.0f);
    item.width = 0.0f;
    item.height = 0.0f;

    text->setText(label);
    text->setTint(_selection >= 0 ? _unselectedTint : _selectedTint);
    text->setScale(_scale, _scale);
    text->setVisibility(true);
    text->setPosition(item.position);

    _items.push_back(item);
    _renderList->push_back(text);

    _layout();

    // First item becomes the initial selection.
    if (_selection < 0) {
        _selection = 0;
    }

    return static_cast<int>(_items.size()) - 1;
}

int Menu::getItemCount(void) const
{
    return static_cast<int>(_items.size());
}

int Menu::getItemIndex(const std::string& label) const
{
    for (size_t i = 0; i < _items.size(); ++i) {
        if (_items[i].label == label) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int Menu::getSelection(void) const
{
    return _selection;
}

void Menu::setSelection(int index)
{
    if (index < 0 || index >= static_cast<int>(_items.size())) {
        return;
    }
    _selection = index;
    applyTint(_selectedTint, _unselectedTint);
}

void Menu::selectNext(void)
{
    if (_items.empty()) {
        return;
    }
    setSelection((_selection + 1) % static_cast<int>(_items.size()));
}

void Menu::selectPrevious(void)
{
    if (_items.empty()) {
        return;
    }
    const int count = static_cast<int>(_items.size());
    setSelection((_selection - 1 + count) % count);
}

int Menu::hitTest(const vector2& position) const
{
    for (int i = 0; i < static_cast<int>(_items.size()); ++i) {
        const Item& item = _items[i];
        if (position.x >= item.position.x &&
            position.x <= item.position.x + item.width &&
            position.y >= item.position.y &&
            position.y <= item.position.y + item.height) {
            return i;
        }
    }
    return -1;
}

const Menu::Item& Menu::getItem(int index) const
{
    return _items[static_cast<size_t>(index)];
}

void Menu::applyTint(uint32_t selected, uint32_t unselected)
{
    _selectedTint = selected;
    _unselectedTint = unselected;
    for (size_t i = 0; i < _items.size(); ++i) {
        _items[i].text->setTint(i == _selection ? selected : unselected);
    }
}

void Menu::_layout(void)
{
    // Group the items together: stack them vertically from _top, centered
    // on _top.x, with _itemSpacing between the bottom of one item and the
    // top of the next.  New items always land directly below the previous
    // one, so they never overlap or spill off screen on their own.
    float y = _top.y;
    for (size_t i = 0; i < _items.size(); ++i) {
        Item& item = _items[i];
        item.width = item.text->getTextWidth() * _scale;
        item.height = item.text->getHeight() * _scale;
        item.position = vector2(_top.x - item.width / 2.0f, y);
        item.text->setPosition(item.position);
        y += item.height + _itemSpacing;
    }
}

void Menu::_destroyItems(void)
{
    for (size_t i = 0; i < _items.size(); ++i) {
        if (_items[i].text && _renderList) {
            _renderList->remove(_items[i].text);
        }
        SAFE_DELETE(_items[i].text);
    }
    _items.clear();
    _selection = -1;
}

std::unique_ptr<Menu> Widgets::createMenu(IRenderer* renderer, const std::string& fontPath,
                                          const vector2& top, float scale, float itemSpacing)
{
    return std::make_unique<Menu>(renderer, fontPath, top, scale, itemSpacing);
}
