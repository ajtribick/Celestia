#pragma once

#include <array>
#include <bitset>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_SIZES_H

#include <celutil/blockarray.h>
#include <celutil/uniquedel.h>

namespace celestia::ttf
{

using UniqueLibrary = util::UniquePtrDel<std::pointer_traits<FT_Library>::element_type, &FT_Done_FreeType>;
using UniqueFace = util::UniquePtrDel<std::pointer_traits<FT_Face>::element_type, &FT_Done_Face>;
using UniqueSize = util::UniquePtrDel<std::pointer_traits<FT_Size>::element_type, &FT_Done_Size>;

struct GlyphInfo
{
    FT_UInt glyphIndex{ 0 };
    int advanceX{ 0 };
    int advanceY{ 0 };
    FT_Int bitmapLeft{ 0 };
    FT_Int bitmapTop{ 0 };
    unsigned int bitmapWidth{ 0 };
    unsigned int bitmapHeight{ 0 };
};

class FontManager;
class Face;

class SizedFace
{
private:
    struct NoCreate { explicit NoCreate() = default; };

public:
    SizedFace(NoCreate, const std::shared_ptr<Face>&, UniqueSize&&, int, int);
    ~SizedFace();

    int maxAscender() const noexcept { return m_maxAscender; }
    int maxDescender() const noexcept { return m_maxDescender; }
    int height() const { return m_maxAscender + m_maxDescender; }
    int width(std::u16string_view text);

    const GlyphInfo& getGlyph(char32_t);

private:
    struct BlockIndices
    {
        std::array<std::uint8_t, 256> indices;
        std::bitset<256> enabled;
    };

    struct GlyphBlock
    {
        GlyphBlock() { glyphIndices.fill(-1); }
        std::array<std::int32_t, 256> glyphIndices;
    };

    std::pair<GlyphInfo*, bool> findGlyph(char32_t);
    void loadGlyphInfo(GlyphInfo&, char32_t);

    std::shared_ptr<Face> m_face;
    UniqueSize m_size;
    int m_maxAscender;
    int m_maxDescender;

    std::vector<GlyphInfo> m_glyphs;
    BlockIndices m_blocks;
    std::vector<GlyphBlock> m_glyphBlocks;
    std::unordered_map<char32_t, GlyphInfo> m_astral;

    friend class Face;
};

class Face : public std::enable_shared_from_this<Face>
{
private:
    struct NoCreate { explicit NoCreate() = default; };

public:
    Face(NoCreate, const std::shared_ptr<FontManager>&, UniqueFace&&);
    ~Face();

    std::shared_ptr<SizedFace> getSized(int size, int dpi);

private:
    struct SizeEntry
    {
        explicit SizeEntry(int, int);

        int size;
        int dpi;
        std::weak_ptr<SizedFace> sizedFace;
    };

    std::shared_ptr<FontManager> m_manager;
    UniqueFace m_face;
    std::vector<SizeEntry> m_entries;

    friend class FontManager;
};

class FontManager : public std::enable_shared_from_this<FontManager>
{
private:
    struct NoCreate { explicit NoCreate() = default; };

public:
    explicit FontManager(NoCreate) {}

    static std::shared_ptr<FontManager> create();

    std::shared_ptr<Face> getFace(const std::filesystem::path& path, FT_Long index);

private:
    struct FaceEntry
    {
        FaceEntry(const std::filesystem::path&, FT_Long);

        std::filesystem::path path;
        FT_Long index;
        std::weak_ptr<Face> face;
    };

    UniqueLibrary m_library;

    std::vector<FaceEntry> m_entries;
};

}
