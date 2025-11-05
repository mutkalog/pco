#include "updater.h"

void Updater::getManifest()
{
    auto res = client.Get("/albums/3/photos");
    std::cout << res->status << "\n";
    // std::cout << res->body << "\n";

}

Updater::Updater()
    : client("http://jsonplaceholder.typicode.com")
{}
