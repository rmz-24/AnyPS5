#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <string>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

extern "C" {
int APS5_VABI sceSysmoduleLoadModuleInternal(std::uint32_t id);
int APS5_VABI sceSysmoduleUnloadModuleInternal(std::uint32_t id);
int APS5_VABI sceSysmoduleGetModuleHandleInternal(std::uint32_t id, std::int32_t* handle);
int APS5_VABI sceKernelDlsym(std::int32_t handle, const char* symbol, void** addr);
}

namespace {

void Require(bool value) { if (!value) std::abort(); }

constexpr std::uint32_t kFiberModuleId = 0x00000006;
constexpr std::uint32_t kKeyboardModuleId = 0x00000106;
constexpr int kModuleNotLoaded = static_cast<int>(0x80A90003);
constexpr int kInvalidId = static_cast<int>(0x805A1000);
constexpr int kModuleUnloaded = static_cast<int>(0x805A1001);

std::string ModuleFileName(void* address) {
#ifdef _WIN32
    HMODULE module = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, static_cast<LPCWSTR>(address), &module)) return {};
    wchar_t file[MAX_PATH] = {};
    GetModuleFileNameW(module, file, MAX_PATH);
    return std::filesystem::path(file).filename().string();
#else
    Dl_info info{};
    if (!dladdr(address, &info) || info.dli_fname == nullptr) return {};
    return std::filesystem::path(info.dli_fname).filename().string();
#endif
}

}

int main() {
    Require(sceSysmoduleLoadModuleInternal(kFiberModuleId) == 0);
    Require(sceSysmoduleLoadModuleInternal(kFiberModuleId) == 0);
    Require(sceSysmoduleUnloadModuleInternal(kFiberModuleId) == 0);
    Require(sceSysmoduleUnloadModuleInternal(kFiberModuleId) == 0);
    Require(sceSysmoduleUnloadModuleInternal(kFiberModuleId) == kModuleNotLoaded);

    std::int32_t handle = -1;
    Require(sceSysmoduleGetModuleHandleInternal(0, &handle) == kInvalidId);
    Require(sceSysmoduleGetModuleHandleInternal(0x80000000u, &handle) == kInvalidId);
    Require(sceSysmoduleGetModuleHandleInternal(0x7ffffffeu, &handle) == kInvalidId);
    Require(sceSysmoduleGetModuleHandleInternal(kKeyboardModuleId, &handle) == kModuleUnloaded);
    Require(handle == -1);
    Require(sceSysmoduleLoadModuleInternal(kKeyboardModuleId) == 0);
    Require(sceSysmoduleGetModuleHandleInternal(kKeyboardModuleId, nullptr) == 0);
    Require(sceSysmoduleGetModuleHandleInternal(kKeyboardModuleId, &handle) == 0);
    Require(handle >= 0);
    std::int32_t again = -1;
    Require(sceSysmoduleGetModuleHandleInternal(kKeyboardModuleId, &again) == 0);
    Require(again == handle);
    void* init = nullptr;
    Require(sceKernelDlsym(handle, "sceKeyboardInit", &init) == 0);
    Require(init != nullptr && ModuleFileName(init) == "libSceKeyboard.prx");
    void* missing = nullptr;
    Require(sceKernelDlsym(handle, "sceKeyboardNoSuchFunction", &missing) != 0);
    Require(sceSysmoduleUnloadModuleInternal(kKeyboardModuleId) == 0);
    Require(sceSysmoduleGetModuleHandleInternal(kKeyboardModuleId, &again) == kModuleUnloaded);
}
