#include <QCoreApplication>
#include <QDebug>
#include <QJsonObject>

#include <opendspx/qdspxmodel.h>

#include <dspxmodel/Model.h>
#include <dspxmodel/BasicModelStrategy.h>
#include <dspxmodel/Timeline.h>
#include <dspxmodel/LabelSequence.h>
#include <dspxmodel/Label.h>
#include <dspxmodel/Workspace.h>
#include <dspxmodel/WorkspaceInfo.h>

using namespace dspx;

int main(int argc, char **argv) {
    QCoreApplication a(argc, argv);

    QDspx::Model qDspxModel;
    auto result = qDspxModel.load(":/tst_dspxmodel_dspx/test.dspx");
    qDebug() << result.type << result.code;


    BasicModelStrategy basicModelStrategy;
    Model model(&basicModelStrategy);

    model.fromQDspx(qDspxModel);
    qDebug() << model.toQDspx().saveData();

    qDebug() << model.timeline()->labels()->firstItem()->text();

    auto workspaceInfo = model.createWorkspaceInfo();
    workspaceInfo->setJsonObject({{"test", "test c"}});
    model.workspace()->insertItem("c", workspaceInfo);

    qDebug() << model.toQDspx().saveData();

    return 0;
}