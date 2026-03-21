#include "fontmanager.h"

#include <algorithm>
#include <cassert>
#include <cstdio>

#include <celutil/logger.h>

using namespace std::string_view_literals;

using celestia::util::GetLogger;

namespace celestia::ttf
{

namespace
{

constexpr char32_t Substitute = U'?';

constexpr std::size_t FontManagerClearCacheAt = 8;
constexpr std::size_t FaceClearCacheAt = 8;

constexpr std::array InitialRanges
{
    U"\u0020\u007e"sv, // ASCII
    U"\u00b2\u00b3"sv, // superscript 2, 3
    U"\u00b9"sv, // superscript 1
    U"\u03b1\u03c9"sv, // lowercase Greek
    U"\u2070"sv, // superscript 0
    U"\u2074\u2079"sv, // superscript 4-9
};

unsigned long
readStream(FT_Stream stream, unsigned long offset, unsigned char* buffer, unsigned long count)
{
    auto file = static_cast<std::FILE*>(stream->descriptor.pointer);
    auto result = std::fseek(file, static_cast<long>(offset), SEEK_SET);
    if (!count)
        return static_cast<unsigned long>(result);

    if (result)
        return 0;

    return static_cast<unsigned long>(std::fread(buffer, 1, count, file));
}

void
closeStream(FT_Stream stream)
{
    auto file = static_cast<std::FILE*>(stream->descriptor.pointer);
    std::fclose(file);
    delete stream;
}

} // end unnamed namespace

SizedFace::SizedFace(NoCreate,
                     const std::shared_ptr<Face>& face,
                     UniqueSize&& size,
                     int maxAscender,
                     int maxDescender) :
    m_face(face),
    m_size(std::move(size)),
    m_maxAscender(maxAscender),
    m_maxDescender(maxDescender)
{
    // precondition : face is active
    for (const auto& range : InitialRanges)
    {
        char32_t lower;
        char32_t upper;
        switch (range.size())
        {
        case 1:
            lower = upper = range[0];
            break;

        case 2:
            lower = range[0];
            upper = range[1];
            break;

        default:
            assert(0);
            continue;
        }

        for (char32_t ch = lower; ch <= upper; ++ch)
        {
            loadGlyphInfo(*findGlyph(ch).first, ch);
        }
    }
}

SizedFace::~SizedFace() = default;

const GlyphInfo&
SizedFace::getGlyph(char32_t ch)
{
    auto [glyphInfo, inserted] = findGlyph(ch);
    if (!inserted)
        return *glyphInfo;

    if (FT_Activate_Size(m_size.get()) != 0)
    {
        GetLogger()->error("Could not activate font size\n");
        return *glyphInfo;
    }

    loadGlyphInfo(*glyphInfo, ch);
    return *glyphInfo;
}

int
SizedFace::width(std::u16string_view text)
{
    FT_Activate_Size(m_size.get());
    int width = 0;
    while (!text.empty())
    {
        char32_t ch;
        auto codeUnit = text.front();
        if (codeUnit < u'\ud800' || codeUnit >= u'\ue000')
        {
            ch = static_cast<char32_t>(codeUnit);
            text = text.substr(1);
        }
        else if (codeUnit >= u'\udc00' || text.size() < 2)
        {
            ch = Substitute;
            text = text.substr(1);
        }
        else if (auto codeUnit2 = text[1]; codeUnit2 < u'\udc00' || codeUnit2 >= u'\ue000')
        {
            ch = Substitute;
            text = text.substr(2);
        }
        else
        {
            text = text.substr(2);
        }

        auto [glyph, inserted] = findGlyph(ch);
        if (inserted)
            loadGlyphInfo(*glyph, ch);

        width += glyph->advanceX;
    }

    return width;
}

std::pair<GlyphInfo*, bool>
SizedFace::findGlyph(char32_t ch)
{
    if (ch >= U'\U00010000')
    {
        auto [it, inserted] = m_astral.try_emplace(ch);
        return { &it->second, inserted };
    }

    bool inserted = false;
    auto blockNumber = ch >> 8;
    std::uint8_t glyphBlockIdx;
    if (m_blocks.enabled[blockNumber])
    {
        glyphBlockIdx = m_blocks.indices[blockNumber];
    }
    else
    {
        inserted = true;
        glyphBlockIdx = static_cast<std::uint8_t>(m_glyphBlocks.size());
        m_blocks.indices[blockNumber] = glyphBlockIdx;
        m_glyphBlocks.emplace_back();
    }

    auto blockEntryNumber = ch & 0xffu;
    auto& idx = m_glyphBlocks[glyphBlockIdx].glyphIndices[blockEntryNumber];
    if (idx < 0)
    {
        inserted = true;
        idx = static_cast<std::int32_t>(m_glyphs.size());
    }

    return { &m_glyphs[idx], inserted };
}

void
SizedFace::loadGlyphInfo(GlyphInfo& glyphInfo, char32_t ch)
{
    if (FT_Load_Char(m_size->face, ch, FT_LOAD_DEFAULT) != 0)
        return;

    FT_GlyphSlot glyph = m_size->face->glyph;
    glyphInfo.glyphIndex = glyph->glyph_index;
    glyphInfo.advanceX = static_cast<int>(glyph->advance.x >> 6);
    glyphInfo.advanceY = static_cast<int>(glyph->advance.y >> 6);
    glyphInfo.bitmapLeft = glyph->bitmap_left;
    glyphInfo.bitmapTop = glyph->bitmap_top;
    glyphInfo.bitmapWidth = glyph->bitmap.width;
    glyphInfo.bitmapHeight = glyph->bitmap.rows;
}

Face::SizeEntry::SizeEntry(int _size, int _dpi) :
    size(_size), dpi(_dpi)
{
}

Face::Face(NoCreate, const std::shared_ptr<FontManager>& manager, UniqueFace&& face) :
    m_manager(manager),
    m_face(std::move(face))
{
}

Face::~Face() = default;

std::shared_ptr<SizedFace>
Face::getSized(int size, int dpi)
{
    // assume we don't have many font sizes, so do a linear scan
    auto it = std::find_if(m_entries.begin(), m_entries.end(),
                           [size, dpi](const SizeEntry& se) { return se.size == size && se.dpi == dpi; });
    if (it == m_entries.end())
    {
        if (m_entries.size() >= FaceClearCacheAt)
        {
            it = std::remove_if(m_entries.begin(), m_entries.end(),
                                [](const SizeEntry& se) { return se.sizedFace.expired(); });
            m_entries.erase(it, m_entries.end());
        }

        m_entries.emplace_back(size, dpi);
        it = m_entries.end() - 1;
    }
    else if (auto locked = it->sizedFace.lock(); locked)
    {
        return locked;
    }

    UniqueSize ftSize;
    if (FT_Size sizePtr; FT_New_Size(m_face.get(), &sizePtr) == 0)
    {
        ftSize = UniqueSize{sizePtr};
    }
    else
    {
        GetLogger()->error("Could not create font size object\n");
        return nullptr;
    }

    if (FT_Activate_Size(ftSize.get()) != 0)
    {
        GetLogger()->error("Could not activate size object\n");
        return nullptr;
    }

    if (FT_Set_Char_Size(m_face.get(), 0, size << 6, dpi, dpi) != 0)
    {
        GetLogger()->error("Could not set font size\n");
        return nullptr;
    }

    auto maxAscender = static_cast<int>(m_face->size->metrics.ascender >> 6);
    auto maxDescender = static_cast<int>(-m_face->size->metrics.descender >> 6);
    auto result = std::make_shared<SizedFace>(SizedFace::NoCreate{}, shared_from_this());

    it->sizedFace = result;
    return result;
}

FontManager::FaceEntry::FaceEntry(const std::filesystem::path& _path, FT_Long _index) :
    path(_path),
    index(_index)
{
}

std::shared_ptr<FontManager>
FontManager::create()
{
    auto fontManager = std::make_shared<FontManager>(NoCreate{});
    if (FT_Library lib; FT_Init_FreeType(&lib) == 0)
    {
        fontManager->m_library = UniqueLibrary{lib};
    }
    else
    {
        GetLogger()->error("Failed to initialize FreeType library\n");
        return nullptr;
    }

    return fontManager;
}

std::shared_ptr<Face>
FontManager::getFace(const std::filesystem::path& path, FT_Long index)
{
    // assume we don't have many fonts, so just do a linear scan
    auto it = std::find_if(m_entries.begin(), m_entries.end(),
                           [&path, index](const FaceEntry& fe) { return fe.index == index && fe.path == path; });
    if (it == m_entries.end())
    {
        if (m_entries.size() >= FontManagerClearCacheAt)
        {
            it = std::remove_if(m_entries.begin(), m_entries.end(),
                                [](const FaceEntry& fe) { return fe.face.expired(); });
            m_entries.erase(it, m_entries.end());
        }

        m_entries.emplace_back(path, index);
        it = m_entries.end() - 1;
    }
    else if (auto locked = it->face.lock(); locked)
    {
        return locked;
    }

#ifdef _WIN32
    std::FILE* file = _wfopen(path.c_str(), L"rb");
#else
    std::FILE* file = std::fopen(path.c_str(), "rb");
#endif

    if (!file)
    {
        GetLogger()->error("Could not open font file {}\n", path);
        return nullptr;
    }

    auto stream = std::make_unique<FT_StreamRec>();
    stream->size = 0x7ffffffful; // default size hint
    if (std::fseek(file, 0, SEEK_END) == 0)
    {
        stream->size = static_cast<unsigned long>(std::ftell(file));
        if (std::fseek(file, 0, SEEK_SET) != 0)
        {
            GetLogger()->error("Could not seek in font file {}\n", path);
            std::fclose(file);
            return nullptr;
        }
    }

    stream->descriptor.pointer = file;
    stream->read = &readStream;
    stream->close = &closeStream;

    FT_Open_Args args;
    args.flags = FT_OPEN_STREAM;
    args.stream = stream.release();
    args.driver = nullptr;
    args.num_params = 0;
    args.params = 0;

    UniqueFace face;
    if (FT_Face facePtr; FT_Open_Face(m_library.get(), &args, index, &facePtr) == 0)
    {
        face = UniqueFace{facePtr};
    }
    else
    {
        GetLogger()->error("Could not open face {}\n", path);
        return nullptr;
    }

    if (!FT_IS_SCALABLE(face.get()))
    {
        GetLogger()->error("Face {} is not scalable\n", path);
        return nullptr;
    }

    auto result = std::make_shared<Face>(Face::NoCreate{}, shared_from_this(), std::move(face));
    it->face = result;
    return result;
}

} // end namespace celestia::ttf
