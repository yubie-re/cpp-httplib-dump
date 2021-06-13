#include <httplib.h>

int main()
{
    system("PAUSE");
    httplib::Client cli("http://ptsv2.com");
    auto res = cli.Post("/t/g6v3g-1623543346/post", "postbody", "application/x-www-form-urlencoded");
    printf("%s", res->body.c_str());
    system("PAUSE");
    auto res2 = cli.Post("/t/g6v3g-1623543346/post", httplib::Params({{"test", "test"}}));
    printf("%s", res2->body.c_str());
    system("PAUSE");
    auto res3 = cli.Get("/t/g6v3g-1623543346/post", (httplib::ContentReceiver)([&](const char* data, size_t length) -> bool {
        printf("%s", data);
        return true;
    }));
    
    system("PAUSE");
    return 0;
}