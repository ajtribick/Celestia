#include "winmoviecapture.h"

#include <array>
#include <cstdint>
#include <string_view>

#include <fmt/format.h>
#include <fmt/xchar.h>

#include <celcompat/filesystem.h>
#include <celestia/celestiacore.h>
#include <celestia/ffmpegcapture.h>
#include <celutil/gettext.h>

#include "commdlg.h"

#include "res/resource.h"
#include "tcharconv.h"
#include "tstring.h"

using namespace std::string_view_literals;

namespace celestia::win32
{

namespace
{

struct MovieSize { int width; int height; };

constexpr std::array<MovieSize, 8> MovieSizes
{
    MovieSize{ 160, 120 },
    MovieSize{ 320, 240 },
    MovieSize{ 640, 480 },
    MovieSize{ 720, 480 },
    MovieSize{ 720, 576 },
    MovieSize{ 1024, 768 },
    MovieSize{ 1280, 720 },
    MovieSize{ 1920, 1080 },
};

constexpr std::array<float, 5> MovieFramerates{ 15.0f, 24.0f, 25.0f, 29.97f, 30.0f };

struct MovieCodec
{
    AVCodecID   codecId;
    const char *codecDesc;
};

constexpr std::array<MovieCodec, 2> MovieCodecs
{
    MovieCodec{ AV_CODEC_ID_FFVHUFF, N_("Lossless")      },
    MovieCodec{ AV_CODEC_ID_H264,    N_("Lossy (H.264)") },
};

int movieSize = 1;
int movieFramerate = 1;
int movieCodec = 1;
std::int64_t movieBitrate = 400000;

bool
BeginMovieCapture(CelestiaCore* appCore,
                  const fs::path& filename,
                  int width, int height,
                  float framerate,
                  AVCodecID codec,
                  std::int64_t bitrate)
{
    auto* movieCapture = new FFMPEGCapture(appCore->getRenderer());
    movieCapture->setVideoCodec(codec);
    movieCapture->setBitRate(bitrate);
    if (codec == AV_CODEC_ID_H264)
        movieCapture->setEncoderOptions(appCore->getConfig()->x264EncoderOptions);
    else
        movieCapture->setEncoderOptions(appCore->getConfig()->ffvhEncoderOptions);

    bool success = movieCapture->start(filename, width, height, framerate);
    if (success)
        appCore->initMovieCapture(movieCapture);
    else
        delete movieCapture;

    return success;
}

BOOL
InitDialog(HWND hDlg)
{
    HWND hwnd = GetDlgItem(hDlg, IDC_COMBO_MOVIE_SIZE);
    fmt::basic_memory_buffer<wchar_t, 128> buf;
#ifndef _UNICODE
    fmt::basic_memory_buffer<char, 128> acpBuf;
#endif
    for (const auto& movieSize : MovieSizes)
    {
        buf.clear();
        fmt::format_to(std::back_inserter(buf), L"{} x {}", movieSize.width, movieSize.height);
        buf.push_back(L'\0');
#ifdef _UNICODE
        SendMessage(hwnd, CB_INSERTSTRING, -1, reinterpret_cast<LPARAM>(buf.data()));
#else
        WideToCurrentCP(std::wstring_view(buf.data(), buf.size()), acpBuf);
        SendMessage(hwnd, CB_INSERTSTRING, -1, reinterpret_cast<LPARAM>(acpBuf.data()));
#endif
    }
    SendMessage(hwnd, CB_SETCURSEL, movieSize, 0);

    hwnd = GetDlgItem(hDlg, IDC_COMBO_MOVIE_FRAMERATE);
    for (float frameRate : MovieFramerates)
    {
        buf.clear();
        fmt::format_to(std::back_inserter(buf), L"{:.2f}", frameRate);
        buf.push_back(L'\0');
#ifdef _UNICODE
        SendMessage(hwnd, CB_INSERTSTRING, -1, reinterpret_cast<LPARAM>(buf.data()));
#else
        WideToCurrentCP(std::wstring_view(buf.data(), buf.size()), acpBuf);
        SendMessage(hwnd, CB_INSERTSTRING, -1, reinterpret_cast<LPARAM>(acpBuf.data()));
#endif
    }
    SendMessage(hwnd, CB_SETCURSEL, movieFramerate, 0);

    hwnd = GetDlgItem(hDlg, IDC_COMBO_MOVIE_CODEC);
    for (const auto& movieCodec : MovieCodecs)
    {
#ifdef _UNICODE
        UTF8ToTChar(_(movieCodec.codecDesc), buf);
        SendMessage(hwnd, CB_INSERTSTRING, -1, reinterpret_cast<LPARAM>(buf.data()));
#else
        UTF8ToTChar(_(movieCodec.codecDesc), acpBuf);
        SendMessage(hwnd, CB_INSERTSTRING, -1, reinterpret_cast<LPARAM>(acpBuf.data()));
#endif
    }
    SendMessage(hwnd, CB_SETCURSEL, movieCodec, 0);

    hwnd = GetDlgItem(hDlg, IDC_EDIT_MOVIE_BITRATE);
    SetWindowText(hwnd, TEXT("400000"));

    return TRUE;
}

UINT CALLBACK
ChooseMovieParamsProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        return InitDialog(hDlg);

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_COMBO_MOVIE_SIZE)
        {
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                HWND hwnd = reinterpret_cast<HWND>(lParam);
                int item = SendMessage(hwnd, CB_GETCURSEL, 0, 0);
                if (item != CB_ERR)
                    movieSize = item;
            }
            return TRUE;
        }
        else if (LOWORD(wParam) == IDC_COMBO_MOVIE_FRAMERATE)
        {
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                HWND hwnd = reinterpret_cast<HWND>(lParam);
                int item = SendMessage(hwnd, CB_GETCURSEL, 0, 0);
                if (item != CB_ERR)
                    movieFramerate = item;
            }
            return TRUE;
        }
        else if (LOWORD(wParam) == IDC_COMBO_MOVIE_CODEC)
        {
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                HWND hwnd = reinterpret_cast<HWND>(lParam);
                int item = SendMessage(hwnd, CB_GETCURSEL, 0, 0);
                if (item != CB_ERR)
                    movieCodec = item;
            }
            return TRUE;
        }
        else if (LOWORD(wParam) == IDC_EDIT_MOVIE_BITRATE)
        {
            if (HIWORD(wParam) == EN_CHANGE)
            {
                std::array<TCHAR, 24> buf;
                int len = GetDlgItemText(hDlg, IDC_EDIT_MOVIE_BITRATE, buf.data(), buf.size());
                if (auto ec = from_tchars(buf.data(), buf.data() + len, movieBitrate).ec; ec != std::errc{})
                    movieBitrate = INT64_C(400000);
            }
            return TRUE;
        }
    }

    return FALSE;
}

} // end unnamed namespace

void HandleCaptureMovie(HINSTANCE appInstance, HWND hWnd, CelestiaCore* appCore)
{
    // TODO: The menu item should be disable so that the user doesn't even
    // have the opportunity to record two movies simultaneously; the only
    // thing missing to make this happen is notification when recording
    // is complete.
    if (appCore->isCaptureActive())
    {
        auto message = UTF8ToTString(_("Stop current movie capture before starting another one."));
        auto error = UTF8ToTString(_("Error"));
        MessageBox(hWnd, message.c_str(), error.c_str(), MB_OK | MB_ICONERROR);
        return;
    }

    // Display File SaveAs dialog to allow user to specify name and location of
    // of captured movie
    OPENFILENAME Ofn;
    std::array<TCHAR, _MAX_PATH+1> szFile;
    szFile.fill(TEXT('\0'));
    // std::array<TCHAR, _MAX_PATH+1> szFileTitle;
    // szFileTitle.fill(TEXT('\0'));

    // Initialize OPENFILENAME
    ZeroMemory(&Ofn, sizeof(OPENFILENAME));
    Ofn.lStructSize = sizeof(OPENFILENAME);
    Ofn.hwndOwner = hWnd;
    Ofn.lpstrFilter = TEXT("Matroska (*.mkv)\0*.mkv\0");
    Ofn.lpstrFile = szFile.data();
    Ofn.nMaxFile = static_cast<DWORD>(szFile.size());
    // Ofn.lpstrFileTitle = szFileTitle.data();
    // Ofn.nMaxFileTitle = static_cast<DWORD>(szFileTitle.size());
    Ofn.lpstrFileTitle = nullptr;
    Ofn.nMaxFileTitle = 0;
    Ofn.lpstrInitialDir = nullptr;

    // Comment this out if you just want the standard "Save As" caption.
    tstring title = UTF8ToTString(_("Save As - Specify Output File for Capture Movie"));
    Ofn.lpstrTitle = title.c_str();

    // OFN_HIDEREADONLY - Do not display read-only video files
    // OFN_OVERWRITEPROMPT - If user selected a file, prompt for overwrite confirmation.
    Ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT  | OFN_EXPLORER | OFN_ENABLETEMPLATE | OFN_ENABLEHOOK | OFN_NOCHANGEDIR;

    Ofn.hInstance = appInstance;
    Ofn.lpTemplateName = MAKEINTRESOURCE(IDD_MOVIE_PARAMS_CHOOSER);
    Ofn.lpfnHook = (LPOFNHOOKPROC)ChooseMovieParamsProc;

    // Display the Save dialog box.
    if (!GetSaveFileName(&Ofn))
        return;

    // If you got here, a path and file has been specified.
    // Ofn.lpstrFile contains full path to specified file
    // Ofn.lpstrFileTitle contains just the filename with extension

    fs::path filename(szFile.data());

    constexpr std::array<std::string_view, 1> defaultExtensions
    {
        ".mkv"sv,
    };

    auto extension = filename.extension().native();
    if ((extension.empty() || extension == L"."sv) &&
        Ofn.nFilterIndex > 0 && Ofn.nFilterIndex <= defaultExtensions.size())
    {
        // If no extension was specified or extension was just a period,
        // use the selection of filter to determine which type of file
        // should be created.
        filename.replace_extension(defaultExtensions[Ofn.nFilterIndex - 1]);
    }

    bool success = false;
    if (DetermineFileType(filename) == ContentType::MKV)
    {
        success = BeginMovieCapture(appCore,
                                    std::move(filename),
                                    MovieSizes[movieSize].width,
                                    MovieSizes[movieSize].height,
                                    MovieFramerates[movieFramerate],
                                    MovieCodecs[movieCodec].codecId,
                                    movieBitrate);
    }
    else
    {
        // Invalid file extension specified.
        auto message = UTF8ToTString(_("Unknown file extension specified for movie capture."));
        auto title = UTF8ToTString(_("Error"));
        MessageBox(hWnd, message.c_str(), title.c_str(), MB_OK | MB_ICONERROR);
        return;
    }

    if (!success)
    {
        auto message = UTF8ToTString(_("Could not capture movie."));
        auto title = UTF8ToTString(_("Error"));
        MessageBox(hWnd, message.c_str(), title.c_str(), MB_OK | MB_ICONERROR);
    }
}

} // end namespace celestia::win32
