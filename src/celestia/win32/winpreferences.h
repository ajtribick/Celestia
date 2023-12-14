#pragma once

#include <cstdint>
#include <string>

#include <celengine/body.h>
#include <celengine/render.h>
#include <celengine/starcolors.h>

#include <windows.h>

class CelestiaCore;

namespace celestia::win32
{

struct AppPreferences
{
    // Specify some default values in case registry keys are not found. Ideally, these
    // defaults should never be used--they should be overridden by settings in
    // celestia.cfg.
    int winWidth{ 800 };
    int winHeight{ 600 };
    int winX{ CW_USEDEFAULT };
    int winY{ CW_USEDEFAULT };
    std::uint64_t renderFlags{ Renderer::DefaultRenderFlags };
    int labelMode{ 0 };
    std::uint64_t locationFilter{ 0 };
    int orbitMask{ Body::Planet | Body::Moon };
    float visualMagnitude{ 8.0f };
    float ambientLight{ 0.1f }; // Low
    float galaxyLightGain{ 0.0f };
    int showLocalTime{ 0 };
    int dateFormat{ 0 };
    int hudDetail{ 2 };
    int fullScreenMode{ -1 };
    int starsColor{ static_cast<int>(ColorTableType::Blackbody_D65) };
    std::uint32_t lastVersion{ 0 };
    std::string altSurfaceName;
    std::uint32_t textureResolution{ 1 };
    Renderer::StarStyle starStyle{ Renderer::PointStars };
    bool ignoreOldFavorites{ false };
};

bool LoadPreferencesFromRegistry(AppPreferences& prefs);
bool SavePreferencesToRegistry(AppPreferences& prefs);

}
