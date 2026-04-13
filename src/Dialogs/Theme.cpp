#include "pch.h"
#include "Theme.h"

Theme* Theme::s_instance = nullptr;

Theme* Theme::Get() {
    if (!s_instance) {
        s_instance = new Theme();
    }
    return s_instance;
}
