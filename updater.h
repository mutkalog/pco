#ifndef UPDATER_H
#define UPDATER_H

#include <httplib.h>

class Updater
{
public:
    httplib::Client client;
    void getManifest();
    Updater();
};

#endif // UPDATER_H
