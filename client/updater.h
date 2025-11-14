#ifndef UPDATER_H
#define UPDATER_H

#include <httplib.h>
#include "data.h"
#include <nlohmann/json.hpp>
using json = nlohmann::ordered_json;

class Updater
{
public:
    httplib::Client client;
    void getManifest();
    Updater();

private:
    ArtifactInfo m_artifact;

    void fillArtifact(const nlohmann::json &data);
};

#endif // UPDATER_H
