#ifndef REPORTSERVICE_MOCK_H
#define REPORTSERVICE_MOCK_H

#include <gmock/gmock.h>
#include "core/service/interfaces/ireportservice.h"


class MockReportService : public IReportService
{
public:
    MOCK_METHOD(void, parseReport, (std::shared_ptr<ServerContext> &sc, const json& report), (override));
};

#endif // REPORTSERVICE_MOCK_H
