#ifndef REPORTSERVICE_H
#define REPORTSERVICE_H

#include "database.h"
#include "servercontext.h"
#include <nlohmann/json.hpp>



using json = nlohmann::ordered_json;

class ReportService
{
public:
    ReportService() : conn_(Database::instance().getConnection()) {}
    void parseReport(ServerContext *sc, const json& report);

private:
    std::unique_ptr<pqxx::connection> conn_;
};

#endif // REPORTSERVICE_H
