#pragma once

#include <raylib.h>
#include <algorithm>
#include <cmath>

namespace Nawia::UI {

    inline float fract(float v) { return v - std::floor(v); }
    inline float hash01(float s) { return fract(std::sin(s * 127.1f) * 43758.5453f); }
    
    inline Color withAlpha(Color c, float a) { 
        return { c.r, c.g, c.b, static_cast<unsigned char>(std::clamp(a, 0.0f, 1.0f) * 255.0f) }; 
    }
    
    inline Color LerpColor(Color c1, Color c2, float t) {
        return {
            static_cast<unsigned char>(c1.r + (c2.r - c1.r) * t),
            static_cast<unsigned char>(c1.g + (c2.g - c1.g) * t),
            static_cast<unsigned char>(c1.b + (c2.b - c1.b) * t),
            static_cast<unsigned char>(c1.a + (c2.a - c1.a) * t)
        };
    }

} // namespace Nawia::UI
