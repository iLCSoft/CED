#include "FontSettings.h"

FontDimensions getFontDimensions(int fontSetting) {
    FontDimensions dim;

    switch (fontSetting) {
        case 0:
            dim.height = 10;
            dim.width = 6;
            break;
        case 1:
            dim.height = 12;
            dim.width = 8;
            break;
        case 2:
            dim.height = 20;
            dim.width = 11;
            break;
        default:
            // Default to smallest size if invalid font setting
            dim.height = 10;
            dim.width = 2;
            break;
    }

    return dim;
}
