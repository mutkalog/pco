#include "http/server.h"

#include "http/uploadcontroller.h"
#include "http/registrationcontroller.h"
#include "http/manifestcontrller.h"
#include "http/downloadcontroller.h"
#include "http/reportcontroller.h"

#include "rolloutsupervisor.h"
#include "servercontext.h"
#include "updatesupervisor.h"

#include <vector>

int main()
{
    ServerContext context;
    context.data                       = std::make_shared<Data>();
    context.data->updateTimeoutSeconds = std::chrono::seconds(5);
    sleep(1);

    UpdateSupervisor  us(&context, Database::instance().getConnection());
    RolloutSupervisor rs(&context, Database::instance().getConnection());

    std::vector<std::unique_ptr<Controller>> ctrls;
    ctrls.push_back(std::make_unique<UploadController>(&context));
    ctrls.push_back(std::make_unique<ManifestController>(&context));
    ctrls.push_back(std::make_unique<DownloadController>(&context));
    ctrls.push_back(std::make_unique<ReportController>(&context));
    ctrls.push_back(std::make_unique<RegistrationController>(&context));

    Server::instance().setControllers(std::move(ctrls));
    Server::instance().setupRoutes();
    Server::instance().listen("0.0.0.0", 39024);
}
