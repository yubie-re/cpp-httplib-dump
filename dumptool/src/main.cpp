#include "inc.hpp"
#include "scan.hpp"
#include <string>

bool g_hooked_write = false;
void *g_clientimpl_constructor = nullptr;
void *g_process_request = nullptr;
void *g_read_content_with_length = nullptr;

void hook_func(void **orig, void *address, void *hook)
{
    auto res = MH_CreateHook(address, hook, orig);
    if (res != MH_OK)
    {
        printf("CreateHook failed: %s\n", MH_StatusToString(res));
    }
    res = MH_EnableHook(address);
    if (res != MH_OK)
    {
        printf("EnableHook failed: %s\n", MH_StatusToString(res));
    }
}

void unhook_func(void *address)
{
    auto res = MH_DisableHook(address);
    if (res != MH_OK)
    {
        printf("DisableHook failed: %s\n", MH_StatusToString(res));
    }
}

char(__fastcall *o_read_content_with_length)(httplib::Stream *strm, size_t len, std::function<bool(size_t, size_t)> *progress, std::function<bool(char const *, size_t, size_t, size_t)> *out);

bool g_custom_reciever = false;

char __fastcall read_content_with_length(httplib::Stream *strm, size_t len, std::function<bool(size_t, size_t)> *progress, std::function<bool(char const *, size_t, size_t, size_t)> *out)
{
    /*if(g_custom_reciever) // to modify responses with custom recievers
    { 
        auto new_resp = "Cock\n";
        (*out)(new_resp, strlen(new_resp), 0, strlen(new_resp));
        return true;
    }*/
    return o_read_content_with_length(strm, len, progress, out);
}

void(__fastcall *o_clientimpl_constructor)(httplib::ClientImpl *this_, const std::string *host, int port, const std::string *client_cert_path, const std::string *client_key_path);

void __fastcall clientimpl_constructor(httplib::ClientImpl *this_, const std::string *host, int port, const std::string *client_cert_path, const std::string *client_key_path)
{
    printf("Client constructed: %s:%d (%p)\n", (*host).c_str(), port, (void*)this_);
    return o_clientimpl_constructor(this_, host, port, client_cert_path, client_key_path);
}

char(__fastcall *o_process_request)(httplib::ClientImpl *this_, void *strm, httplib::Request *req, httplib::Response *res, bool close_connection, void *error);

char __fastcall process_request(httplib::ClientImpl *this_, void *strm, httplib::Request *req, httplib::Response *res, bool close_connection, void *error)
{
    if (!req->body.empty())
        printf("Request body: %s\n", req->body.c_str());
    if (!req->params.empty())
    {
        for (auto &[key, val] : req->params)
        {
            printf("Parameter: %s->%s\n", key.c_str(), val.c_str());
        }
    }
    if (!req->headers.empty())
    {
        for (auto &[key, val] : req->headers)
        {
            printf("Header: %s->%s\n", key.c_str(), val.c_str());
        }
    }
    if (req->content_receiver)
    {
       printf("Content Reciever Active\n", req->content_receiver);
       g_custom_reciever = true;
    }
    printf("Processing request: %s (%s) %p\n", req->path.c_str(), req->method.c_str(), res);
    auto result = o_process_request(this_, strm, req, res, close_connection, error);
    printf("Response %d: %s\n", res->status, res->body.c_str());
    g_custom_reciever = false;
    /*res->body = "new response\n"; // to modify the response
    res->content_length_ = res->body.size();*/
    return result;
}

void hook()
{
    MH_Initialize();
    // only x64 for now lol x86 doesn't like me using their stuff
    g_clientimpl_constructor = scanner::rip(scanner::scan("E8 ? ? ? ? 90 48 8D 05 ? ? ? ? 48 89 03 48 8D 8B ? ? ? ? BA", "clientimpl::clientimpl", GetModuleHandleA(nullptr)), 1);
    g_process_request = scanner::rip(scanner::scan("E8 ? ? ? ? 0F B6 F0 40 84 F6 0F 84 ? ? ? ? 41 8B 45 20 8B D8 3D ? ? ? ? 7E 3C", "process_request", GetModuleHandleA(nullptr)), 1);
    g_read_content_with_length = scanner::rip(scanner::scan("E8 ? ? ? ? ? ? ? ? ? ? ? B9 90 01 00 00 B8 9D 01 00 00", "read_content_with_length", GetModuleHandleA(nullptr)), 1);
    hook_func((void **)&o_clientimpl_constructor, g_clientimpl_constructor, clientimpl_constructor);
    hook_func((void **)&o_read_content_with_length, g_read_content_with_length, read_content_with_length);
    hook_func((void **)&o_process_request, g_process_request, process_request);
}

void unhook()
{
    if (g_clientimpl_constructor)
        unhook_func(g_clientimpl_constructor);
    if (g_read_content_with_length)
        unhook_func(g_read_content_with_length);
    if (g_process_request)
        unhook_func(g_process_request);
}

DWORD WINAPI main_thread(PVOID module)
{
    hook();
    while (!GetAsyncKeyState(VK_DELETE))
    {
        std::this_thread::yield();
    }
    unhook();
    return 1;
}

// Entrypoint
BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        CreateThread(nullptr, 0, &main_thread, (void *)module, 0, nullptr);
    }
    return TRUE;
}