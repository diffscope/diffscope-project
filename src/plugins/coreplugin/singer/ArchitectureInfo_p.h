#ifndef DIFFSCOPE_COREPLUGIN_ARCHITECTUREINFO_P_H
#define DIFFSCOPE_COREPLUGIN_ARCHITECTUREINFO_P_H

#include <coreplugin/ArchitectureInfo.h>

#include <QSharedData>

namespace Core {

    class ArchitectureInfoData : public QSharedData {
    public:
        QString name;
        ArchitectureInfo::ParameterMap parameters;
        QJsonValue defaultExtra;
        QQmlComponent *controlPanelComponent{};
    };

}

#endif // DIFFSCOPE_COREPLUGIN_ARCHITECTUREINFO_P_H
