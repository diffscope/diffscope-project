#include "ClipboardConverter.h"
#include "ClipboardConverter_p.h"

#include <QMimeData>

namespace ImportExportManager {

    ClipboardConverter::ClipboardConverter(QObject *parent) : QObject(parent), d_ptr(new ClipboardConverterPrivate) {
        Q_D(ClipboardConverter);
        d->q_ptr = this;
    }

    ClipboardConverter::~ClipboardConverter() = default;

    QString ClipboardConverter::name() const {
        Q_D(const ClipboardConverter);
        return d->name;
    }

    QString ClipboardConverter::description() const {
        Q_D(const ClipboardConverter);
        return d->description;
    }

    QStringList ClipboardConverter::mimeTypes() const {
        Q_D(const ClipboardConverter);
        return d->mimeTypes;
    }

    ClipboardConverter::Modes ClipboardConverter::modes() const {
        Q_D(const ClipboardConverter);
        return d->modes;
    }

    bool ClipboardConverter::paste(const QMimeData *mimeData, QDspx::Model &model) {
        Q_UNUSED(mimeData)
        Q_UNUSED(model)
        return false;
    }

    bool ClipboardConverter::copy(QMimeData *mimeData, const QDspx::Model &model) {
        Q_UNUSED(mimeData)
        Q_UNUSED(model)
        return false;
    }

    void ClipboardConverter::setName(const QString &name) {
        Q_D(ClipboardConverter);
        d->name = name;
    }

    void ClipboardConverter::setDescription(const QString &description) {
        Q_D(ClipboardConverter);
        d->description = description;
    }

    void ClipboardConverter::setMimeTypes(const QStringList &mimeTypes) {
        Q_D(ClipboardConverter);
        d->mimeTypes = mimeTypes;
    }

    void ClipboardConverter::setModes(Modes modes) {
        Q_D(ClipboardConverter);
        d->modes = modes;
    }

}
