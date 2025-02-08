#include <winsock2.h> // Include winsock2.h before windows.h
#include <windows.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <filesystem>
#include <ctime>
#include <gdiplus.h>
#include "httplib.h" // Include the httplib header

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "ws2_32.lib") // Link the Winsock library

using namespace std;

#define ID_START   1
#define ID_EXIT    2
#define ID_GO_SYSTEMTRAY  3
#define ID_REMOVE_PNGS 4
#define WM_TRAYICON (WM_USER + 1)

bool STOP_CAPTURE = false;

// Global variables to control the capture thread.
std::atomic<bool> g_Running(false);
std::thread captureThread;
NOTIFYICONDATAA nid;

std::string GetExecutablePath() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    std::string::size_type pos = std::string(buffer).find_last_of("\\/");
    return std::string(buffer).substr(0, pos);
}
std::string folderPath = GetExecutablePath(); // Change folderPath to current directory

// Global variable for the HTTP server
std::unique_ptr<httplib::Server> httpServer;

CLSID GetEncoderClsid(const char* format) {
    UINT num = 0;
    UINT size = 0;
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) return CLSID{};

    auto pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
    Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

    CLSID clsid;
    for (UINT i = 0; i < num; i++) {
        if (wcscmp(pImageCodecInfo[i].MimeType, std::wstring(format, format + strlen(format)).c_str()) == 0) {
            clsid = pImageCodecInfo[i].Clsid;
            free(pImageCodecInfo);
            return clsid;
        }
    }
    free(pImageCodecInfo);
    return CLSID{};
}

void SaveBitmapToFile(HBITMAP hBitmap, const std::string& filePath) {
    Gdiplus::Bitmap bmp(hBitmap, nullptr);
    CLSID pngClsid;
    pngClsid = GetEncoderClsid("image/png");
    std::wstring wFilePath(filePath.begin(), filePath.end());
    bmp.Save(wFilePath.c_str(), &pngClsid, nullptr);
}

void CaptureScreen(const std::string& folderPath, const std::string& fileName) {
    HDC hScreen = GetDC(nullptr);
    HDC hDcMem = CreateCompatibleDC(hScreen);
    int width = 1280;
    int height = 720;

    HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, width, height);
    SelectObject(hDcMem, hBitmap);
    BitBlt(hDcMem, 0, 0, width, height, hScreen, 0, 0, SRCCOPY);

    std::string filePath = folderPath + "\\" + fileName;
    SaveBitmapToFile(hBitmap, filePath);

    DeleteObject(hBitmap);
    DeleteDC(hDcMem);
    ReleaseDC(nullptr, hScreen);
}

std::string GetTimestamp() {
    auto now = std::time(nullptr);
    std::tm localTime;
    localtime_s(&localTime, &now);

    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H-%M-%S.png", &localTime);
    return std::string(buffer);
}

void StartHttpServer() {
    httpServer = std::make_unique<httplib::Server>();

    httpServer->Get("/", [](const httplib::Request&, httplib::Response& res) {
        std::filesystem::path folder(folderPath);
        std::string html = "<html><body><h1>Directory Listing</h1><ul>";
        try {
            for (const auto& entry : std::filesystem::directory_iterator(folder)) {
                std::string filename = entry.path().filename().string();
                html += "<li><a href=\"/" + filename + "\">" + filename + "</a></li>";
            }
        } catch (const std::exception& ex) {
            html += "<li>Error: ";
            html += ex.what();
            html += "</li>";
        }
        html += "</ul></body></html>";
        res.set_content(html, "text/html");
    });

    httpServer->Get(R"(/(.*))", [](const httplib::Request& req, httplib::Response& res) {
        std::string file_path = folderPath + "\\" + req.matches[1].str();
        if (std::filesystem::exists(file_path)) {
            std::ifstream ifs(file_path, std::ios::binary);
            ifs.seekg(0, std::ios::end);
            size_t length = ifs.tellg();
            ifs.seekg(0, std::ios::beg);
            std::string content(length, '\0');
            ifs.read(&content[0], length);
            res.set_content(content, "application/octet-stream");
            res.set_header("Content-Disposition", "attachment; filename=\"" + req.matches[1].str() + "\"");
        } else {
            res.status = 404;
            res.set_content("File not found", "text/plain");
        }
    });

    std::thread serverThread([]() {
        httpServer->listen("0.0.0.0", 8080);
    });
    serverThread.detach();
}

void StopHttpServer() {
    if (httpServer) {
        httpServer->stop();
    }
}

void ScreenCaptureTask(const std::string& folderPath) {
    int INTERVAL = 200; // Capture screen every 200 seconds
    int time = INTERVAL;
    STOP_CAPTURE = false;
    StartHttpServer(); // Start the HTTP server
    while (!STOP_CAPTURE && g_Running.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        time++;
        if (time / INTERVAL) {
            std::string fileName = GetTimestamp();
            CaptureScreen(folderPath, fileName);
            time = 0;
        }
    }
    StopHttpServer(); // Stop the HTTP server
}

void AddTrayIcon(HWND hwnd) {
    nid.cbSize = sizeof(NOTIFYICONDATAA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    strcpy_s(nid.szTip, "SC");
    Shell_NotifyIcon(NIM_ADD, &nid);
}

void RemoveTrayIcon() {
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

std::string GetLocalIP() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    char hostname[256];
    gethostname(hostname, sizeof(hostname));

    addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* result = nullptr;
    getaddrinfo(hostname, nullptr, &hints, &result);

    char ipStr[INET_ADDRSTRLEN];
    sockaddr_in* sockaddr_ipv4 = (sockaddr_in*)result->ai_addr;
    InetNtopA(AF_INET, &sockaddr_ipv4->sin_addr, ipStr, INET_ADDRSTRLEN);

    freeaddrinfo(result);
    WSACleanup();
    return std::string(ipStr);
}

void RemovePngFiles(const std::string& folderPath) {
    for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
        if (entry.path().extension() == ".png") {
            std::filesystem::remove(entry.path());
        }
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hwndLabel;
    static std::string localIP = GetLocalIP();
    switch (msg) {
    case WM_CREATE:
        CreateWindowA("BUTTON", "start", WS_CHILD | WS_VISIBLE,
                     20, 20, 100, 30, hwnd, (HMENU)ID_START, nullptr, nullptr);
        CreateWindowA("BUTTON", "exit", WS_CHILD | WS_VISIBLE,
                     20, 60, 100, 30, hwnd, (HMENU)ID_EXIT, nullptr, nullptr);
        CreateWindowA("BUTTON", "iconfy", WS_CHILD | WS_VISIBLE,
                     20, 100, 100, 30, hwnd, (HMENU)ID_GO_SYSTEMTRAY, nullptr, nullptr);
        CreateWindowA("BUTTON", "Remove PNGs", WS_CHILD | WS_VISIBLE,
                     20, 140, 100, 30, hwnd, (HMENU)ID_REMOVE_PNGS, nullptr, nullptr);
        hwndLabel = CreateWindowA("STATIC", localIP.c_str(), WS_CHILD | WS_VISIBLE,
                                  20, 180, 200, 30, hwnd, nullptr, nullptr, nullptr);
        AddTrayIcon(hwnd);
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_START:
            if (!g_Running.load()) {
                g_Running.store(true);
                // Start the screen capture task in a separate thread.
                captureThread = std::thread(ScreenCaptureTask, folderPath);
            }
            break;
        case ID_EXIT:
            STOP_CAPTURE = true;
            g_Running.store(false);
            if (captureThread.joinable()) {
                captureThread.join();
            }
            RemovePngFiles(folderPath); // Remove PNG files
            PostQuitMessage(0);
            break;
        case ID_GO_SYSTEMTRAY:
            // Minimize the window to the system tray.
            ShowWindow(hwnd, SW_HIDE);
            break;
        case ID_REMOVE_PNGS:
            RemovePngFiles(folderPath); // Remove PNG files
            break;
        }
        break;
    case WM_TRAYICON:
        if (lParam == WM_LBUTTONUP) {
            ShowWindow(hwnd, SW_SHOW);
            SetForegroundWindow(hwnd);
        }
        break;
    case WM_DESTROY:
        // Signal the capture thread to stop and join it.
        STOP_CAPTURE = true;
        g_Running.store(false);
        if (captureThread.joinable()) {
            captureThread.join();
        }
        RemovePngFiles(folderPath); // Remove PNG files
        RemoveTrayIcon();
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    // Register window class.
    WNDCLASSA wc = {};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = "CaptureWindowClass";
    RegisterClassA(&wc);

    // Create the main window.
    HWND hwnd = CreateWindowA(wc.lpszClassName, "SC",
                             WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             300, 300,
                             nullptr, nullptr, hInstance, nullptr);
    if (!hwnd) return -1;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Message loop.
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    Gdiplus::GdiplusShutdown(gdiplusToken);
    return 0;
}