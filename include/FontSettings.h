#ifndef FONT_SETTINGS_H
#define FONT_SETTINGS_H

struct FontDimensions {
    int height;
    int width;

    constexpr FontDimensions(int h, int w) : height(h), width(w) {}
};

inline constexpr FontDimensions getFontDimensions(int fontSetting) {
    switch (fontSetting) {
        case 0: return {10, 6};
        case 1: return {12, 8};
        case 2: return {20, 11};
        default: return {10, 2};
    }
}

#endif
