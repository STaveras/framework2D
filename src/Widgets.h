// File: Widgets.h
// Author: Stanley Taveras
// Purpose: Standard, reusable screen-space UI widgets.  Currently provides
// Menu, a container that displays a vertically stacked, selectable set of
// text items (RESUME/QUIT style menus).

#pragma once

#include "Font.h"
#include "GameState.h"
#include "maths/Vector2.h"

#include <memory>
#include <string>
#include <vector>

// A menu is a container of selectable text items stacked vertically, drawn in
// its own screen-space render list.  Items are grouped together: each one
// sits directly below the previous one with a fixed gap, so adding items
// never overlaps.  Selection can be driven by keyboard (selectNext/Previous)
// or by mouse (hitTest + setSelection), and the selected/unselected tint is
// applied automatically.
class Menu
{
public:
    struct Item
    {
        Font* text;
        std::string label;
        vector2 position; // top-left corner in render coordinates
        float width;
        float height;
    };

    // Create a menu whose items stack from `top` (horizontal center x and
    // the top edge y) down the screen.  Items are laid out centered on
    // `top.x` with `scale` and a gap of `itemSpacing` between them.
    Menu(IRenderer* renderer, const std::string& fontPath, const vector2& top,
         float scale = 1.0f, float itemSpacing = 8.0f);
    ~Menu();

    int addItem(const std::string& label); // returns new index, or -1 on failure
    int getItemCount(void) const;
    int getItemIndex(const std::string& label) const;

    int getSelection(void) const;
    void setSelection(int index);
    void selectNext(void);
    void selectPrevious(void);

    // Index of the item under `position`, or -1 if none.
    int hitTest(const vector2& position) const;

    const Item& getItem(int index) const;

    // Apply the selected/unselected tints to all items.  Call once per
    // frame after navigation input so highlights track the selection.
    void applyTint(uint32_t selected, uint32_t unselected);

private:
    void _layout(void);
    void _destroyItems(void);

    IRenderer* _renderer;
    IRenderer::RenderList* _renderList;
    std::string _fontPath;
    vector2 _top;
    float _scale;
    float _itemSpacing;
    std::vector<Item> _items;
    int _selection = -1;
    uint32_t _selectedTint = 0xFFFFFFFF;
    uint32_t _unselectedTint = 0xFF888888;
};

// Factory entry point for creating UI widgets.  Menus (and future widgets)
// are created through here so call sites do not depend on the concrete
// widget classes.
class Widgets
{
public:
    static std::unique_ptr<Menu> createMenu(IRenderer* renderer, const std::string& fontPath,
                           const vector2& top, float scale = 1.0f,
                           float itemSpacing = 8.0f);
};
