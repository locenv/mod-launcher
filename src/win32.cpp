#include "launcher.hpp"

#include <locenv/api.hpp>

#include <memory>
#include <sstream>
#include <stdexcept>
#include <system_error>

#include <windows.h>

static HANDLE open_nul()
{
	// Enable inherit handles.
	SECURITY_ATTRIBUTES sa;

	ZeroMemory(&sa, sizeof(sa));

	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;

	// Open NUL.
	auto h = CreateFileW(
		L"NUL",
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		&sa,
		OPEN_EXISTING,
		0,
		nullptr);

	if (h == INVALID_HANDLE_VALUE) {
		throw std::system_error(GetLastError(), std::system_category());
	}

	return h;
}

static std::unique_ptr<wchar_t[]> from_utf8(const std::string &utf8)
{
    // Get buffer size.
    auto bytes = static_cast<int>(utf8.length() + 1);
    auto required = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), bytes, nullptr, 0);

    if (!required) {
        auto code = GetLastError();
        std::stringstream message;

        message << "Cannot get a length for a buffer to decode " << utf8 << " (" << code << ")";

        throw std::runtime_error(message.str());
    }

    // Decode.
    auto buffer = std::make_unique<wchar_t[]>(required);

    if (!MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), bytes, buffer.get(), required)) {
        auto code = GetLastError();
        std::stringstream message;

        message << "Cannot decode " << utf8 << " (" << code << ")";

        throw std::runtime_error(message.str());
    }

    return buffer;
}

process launcher::launch()
{
	auto bin = from_utf8(binary);

	// Setup child creation.
	STARTUPINFOW si;

	ZeroMemory(&si, sizeof(si));

	si.cb = sizeof(si);
	si.dwFlags = STARTF_FORCEOFFFEEDBACK | STARTF_USESTDHANDLES;

	// Redirect stdin to NUL.
	si.hStdInput = open_nul();

	// Redirect stdout to NUL.
	try {
		si.hStdOutput = open_nul();
	} catch (...) {
		CloseHandle(si.hStdInput);
		throw;
	}

	// Redirect stderr to NUL.
	try {
		si.hStdError = open_nul();
	} catch (...) {
		CloseHandle(si.hStdOutput);
		CloseHandle(si.hStdInput);
		throw;
	}

	// Create child process.
	BOOL res;
	PROCESS_INFORMATION pi;

	res = CreateProcessW(
		bin.get(),
		nullptr,
		nullptr,
		nullptr,
		TRUE,
		CREATE_UNICODE_ENVIRONMENT,
		nullptr,
		working_directory.c_str(),
		&si,
		&pi);

	CloseHandle(si.hStdError);
	CloseHandle(si.hStdOutput);
	CloseHandle(si.hStdInput);

	if (!res) {
		throw std::system_error(GetLastError(), std::system_category());
	}

	return process(pi);
}

process::process(PROCESS_INFORMATION pi) :
	pi(pi)
{
}

process::process(process &&o) :
	pi(o.pi)
{
	ZeroMemory(&o.pi, sizeof(o.pi));
}

process::~process()
{
	if (pi.hProcess) {
		TerminateProcess(pi.hProcess, 1);
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}
}

int process::wait(locenv::lua l)
{
	if (!pi.hProcess) {
		return locenv::api->aux_error(l, "process cannot be waited multiple times");
	}

	if (WaitForSingleObject(pi.hProcess, INFINITE) != WAIT_OBJECT_0) {
		return locenv::api->aux_error(l, "waiting error with code %U", GetLastError());
	}

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	ZeroMemory(&pi, sizeof(pi));

	return 0;
}
