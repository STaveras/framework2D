#include "IInput.h"

IInput::~IInput(void) {
    if (m_pKeyboard) { 
        delete m_pKeyboard; 
    }
}