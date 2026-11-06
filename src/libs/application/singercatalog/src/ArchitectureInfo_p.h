// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SINGERCATALOG_ARCHITECTUREINFO_P_H
#define DIFFSCOPE_SINGERCATALOG_ARCHITECTUREINFO_P_H

#include <singercatalog/ArchitectureInfo.h>

#include <QSharedData>

namespace SingerCatalog {

    class ArchitectureInfoData : public QSharedData {
    public:
        QString name;
        ArchitectureInfo::ParameterMap parameters;
        QJsonValue defaultExtra;
        QQmlComponent *controlPanelComponent{};
    };

}

#endif // DIFFSCOPE_SINGERCATALOG_ARCHITECTUREINFO_P_H
