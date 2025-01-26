#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <filesystem>
#include <ctime>
#include <windows.h>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

namespace fs = std::filesystem;

void SaveBitmapToFile(HBITMAP hBitmap, const std::wstring& filePath) {
    Gdiplus::Bitmap bmp(hBitmap, nullptr);
    CLSID pngClsid;
    Gdiplus::GetEncoderClsid(L"image/png", &pngClsid);
    bmp.Save(filePath.c_str(), &pngClsid, nullptr);
}

CLSID GetEncoderClsid(const wchar_t* format) {
    UINT num = 0;
    UINT size = 0;
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) return CLSID{};

    auto pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
    Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

    CLSID clsid;
    for (UINT i = 0; i < num; i++) {
        if (wcscmp(pImageCodecInfo[i].MimeType, format) == 0) {
            clsid = pImageCodecInfo[i].Clsid;
            free(pImageCodecInfo);
            return clsid;
        }
    }
    free(pImageCodecInfo);
    return CLSID{};
}

void CaptureScreen(const std::wstring& folderPath, const std::wstring& fileName) {
    HDC hScreen = GetDC(nullptr);
    HDC hDcMem = CreateCompatibleDC(hScreen);
    int width = 1280;
    int height = 720;

    HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, width, height);
    SelectObject(hDcMem, hBitmap);
    BitBlt(hDcMem, 0, 0, width, height, hScreen, 0, 0, SRCCOPY);

    std::wstring filePath = folderPath + L"\\" + fileName;
    SaveBitmapToFile(hBitmap, filePath);

    DeleteObject(hBitmap);
    DeleteDC(hDcMem);
    ReleaseDC(nullptr, hScreen);
}

std::wstring GetTimestamp() {
    auto now = std::time(nullptr);
    std::tm localTime;
    localtime_s(&localTime, &now);

    wchar_t buffer[32];
    wcsftime(buffer, sizeof(buffer), L"%Y-%m-%d_%H-%M-%S.png", &localTime);
    return std::wstring(buffer);
}

void ScreenCaptureTask(const std::wstring& folderPath) {
    while (true) {
        std::wstring fileName = GetTimestamp();
        CaptureScreen(folderPath, fileName);
        std::this_thread::sleep_for(std::chrono::minutes(3));
    }
}

int main() {
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    std::wstring folderPath = L"C:\\CapturedScreenshots";
    if (!fs::exists(folderPath)) {
        fs::create_directory(folderPath);
    }

    std::thread captureThread(ScreenCaptureTask, folderPath);

    // Prevents the console application from closing
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    captureThread.join();

    Gdiplus::GdiplusShutdown(gdiplusToken);
    return 0;
}
