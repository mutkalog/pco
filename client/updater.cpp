#include "updater.h"

void Updater::getManifest()
{
    auto res = client.Get("/manifest?type=rpi4&place=machine&id=72");
    std::cout << res->status << "\n";
    std::cout << res->body << "\n";
}

Updater::Updater()
    : client("http://localhost:8080")
{}
