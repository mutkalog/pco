#ifndef REPORTSERVICE_H
#define REPORTSERVICE_H

#include <nlohmann/json.hpp>


using json = nlohmann::ordered_json;

class ReportService
{
public:
    ReportService() = default;
    void parseReport(const json& report);
};

#endif // REPORTSERVICE_H
