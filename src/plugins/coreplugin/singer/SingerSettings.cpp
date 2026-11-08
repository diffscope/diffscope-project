// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "SingerSettings.h"

#include <QByteArray>
#include <QSettings>

#include <CoreApi/runtimeinterface.h>

#include <coreplugin/SourcesPickerModel.h>

namespace Core::SingerSettings {

    DspxProject::SourcesSelection defaultSinger() {
        auto *settings = RuntimeInterface::settings();
        settings->beginGroup(QStringLiteral("org.diffscope.core.document"));
        const auto data = settings->value(QStringLiteral("defaultSinger"), QByteArray{}).toByteArray();
        settings->endGroup();
        SourcesPickerModel sources;
        if (data.isEmpty() || !sources.deserialize(data))
            return {};
        return sources.sourcesSelection();
    }

    bool setDefaultSinger(const DspxProject::SourcesSelection &sources) {
        SourcesPickerModel model;
        model.setSourcesSelection(sources);
        const auto data = model.serialize();
        if (data.isEmpty())
            return false;
        auto *settings = RuntimeInterface::settings();
        settings->beginGroup(QStringLiteral("org.diffscope.core.document"));
        settings->setValue(QStringLiteral("defaultSinger"), data);
        settings->endGroup();
        return true;
    }

    bool applyDefaultSinger(dspx::SingingClip *clip) {
        return defaultSinger().applyTo(clip);
    }

}
