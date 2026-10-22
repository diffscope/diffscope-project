// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_MIXEDSINGERPRESETS_H
#define DIFFSCOPE_COREPLUGIN_MIXEDSINGERPRESETS_H

#include <QModelIndex>
#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <QVariantList>

#include <coreplugin/coreglobal.h>

namespace Core {

    class SourcesPickerModel;
    class MixedSingerPresetsPrivate;

    class CORE_EXPORT MixedSingerPresets : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(MixedSingerPresets)
        Q_PROPERTY(QVariantList presets READ presets NOTIFY presetsChanged)

    public:
        explicit MixedSingerPresets(QObject *parent = nullptr);
        ~MixedSingerPresets() override;

        QVariantList presets() const;
        Q_INVOKABLE bool savePreset(const QString &name, SourcesPickerModel *sources, const QModelIndex &index);
        Q_INVOKABLE bool renamePreset(const QString &id, const QString &name);
        Q_INVOKABLE bool removePreset(const QString &id);

    Q_SIGNALS:
        void presetsChanged();

    private:
        QScopedPointer<MixedSingerPresetsPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_MIXEDSINGERPRESETS_H
