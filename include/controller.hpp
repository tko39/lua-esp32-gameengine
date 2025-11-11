#pragma once
#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

enum KeyCode
{
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_SELECT,
    KEY_BACK,
    KEY_NONE
};

using KeyPressedCallback = bool (*)(KeyCode key);

#endif // CONTROLLER_HPP