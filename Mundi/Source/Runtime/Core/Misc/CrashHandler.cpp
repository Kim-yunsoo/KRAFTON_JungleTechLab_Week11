#include "pch.h"
#include "CrashHandler.h"
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <atomic>

namespace
{
    std::atomic_bool g_Initialized{ false };
    std::atomic_bool g_WritingDump{ false };
}

void FCrashHandler::Initialize()
{
    // 전역 예외 핸들러 등록 (1회만)
    bool expected = false;
    if (!g_Initialized.compare_exchange_strong(expected, true))
        return;
    // Saved/Crashes 폴더를 미리 생성해 둔다 (프로젝트 루트 기준)
    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
    std::filesystem::path projectRoot = exeDir.parent_path().parent_path();
    if (projectRoot.empty())
        projectRoot = exeDir;
    std::filesystem::path dumpDir = projectRoot / L"Saved" / L"Crashes";
    std::error_code ec;
    std::filesystem::create_directories(dumpDir, ec);

    SetUnhandledExceptionFilter(FCrashHandler::OnUnhandledException);
}

LONG WINAPI FCrashHandler::OnUnhandledException(EXCEPTION_POINTERS* ExceptionInfo)
{
    WriteMiniDump(ExceptionInfo);
    return EXCEPTION_EXECUTE_HANDLER; // 프로세스 종료
}

void FCrashHandler::WriteMiniDump(EXCEPTION_POINTERS* ExceptionInfo)
{
    // 재진입 방지
    bool expected = false;
    if (!g_WritingDump.compare_exchange_strong(expected, true))
        return;

    // 프로젝트 루트 기준 Saved/Crashes 경로에 덤프 생성
    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
    // exeDir: .../Binaries/<Config>
    std::filesystem::path projectRoot = exeDir.parent_path().parent_path();
    if (projectRoot.empty())
        projectRoot = exeDir; // 안전 장치

    auto now = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    tm tms{};
#if defined(_MSC_VER)
    localtime_s(&tms, &tt);
#else
    tms = *std::localtime(&tt);
#endif

    std::wstringstream wss;
    wss << L"Crash_" << GetCurrentProcessId() << L"_"
        << std::put_time(&tms, L"%Y%m%d_%H%M%S") << L".dmp";
    std::filesystem::path dumpDir = projectRoot / L"Saved" / L"Crashes";
    std::error_code ec;
    std::filesystem::create_directories(dumpDir, ec); // best-effort
    std::filesystem::path dumpPath = dumpDir / wss.str();

    HANDLE hFile = CreateFileW(
        dumpPath.c_str(),
        GENERIC_WRITE,
        FILE_SHARE_WRITE,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        OutputDebugStringW(L"[CrashHandler] Failed to create dump file\n");
        g_WritingDump.store(false);
        return;
    }

    MINIDUMP_EXCEPTION_INFORMATION dumpInfo{};
    dumpInfo.ThreadId = GetCurrentThreadId();
    dumpInfo.ExceptionPointers = ExceptionInfo;
    dumpInfo.ClientPointers = TRUE;

    BOOL ok = MiniDumpWriteDump(
        GetCurrentProcess(),
        GetCurrentProcessId(),
        hFile,
        MiniDumpWithFullMemory,
        &dumpInfo,
        NULL,
        NULL);

    CloseHandle(hFile);

    if (ok)
    {
        std::wstringstream msg;
        msg << L"[CrashHandler] MiniDump written: " << dumpPath.c_str() << L"\n";
        OutputDebugStringW(msg.str().c_str());
    }
    else
    {
        OutputDebugStringW(L"[CrashHandler] MiniDumpWriteDump failed\n");
    }

    g_WritingDump.store(false);
}
