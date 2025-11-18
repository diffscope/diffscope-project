#include "FileConverter.h"
#include "FileConverter_p.h"

namespace ImportExportManager {

    FileConverter::FileConverter(QObject *parent) : QObject(parent), d_ptr(new FileConverterPrivate) {
        Q_D(FileConverter);
        d->q_ptr = this;
    }

    FileConverter::~FileConverter() = default;

    QString FileConverter::name() const {
        Q_D(const FileConverter);
        return d->name;
    }

    QString FileConverter::description() const {
        Q_D(const FileConverter);
        return d->description;
    }

    QStringList FileConverter::filters() const {
        Q_D(const FileConverter);
        return d->filters;
    }

    FileConverter::Modes FileConverter::modes() const {
        Q_D(const FileConverter);
        return d->modes;
    }

    bool FileConverter::execImport(const QString &filename, QDspx::Model &model) {
        Q_UNUSED(filename)
        Q_UNUSED(model)
        return false;
    }

    bool FileConverter::execExport(const QString &filename, const QDspx::Model &model) {
        Q_UNUSED(filename)
        Q_UNUSED(model)
        return false;
    }

    void FileConverter::setName(const QString &name) {
        Q_D(FileConverter);
        d->name = name;
    }

    void FileConverter::setDescription(const QString &description) {
        Q_D(FileConverter);
        d->description = description;
    }

    void FileConverter::setFilters(const QStringList &filters) {
        Q_D(FileConverter);
        d->filters = filters;
    }

    void FileConverter::setModes(Modes modes) {
        Q_D(FileConverter);
        d->modes = modes;
    }

}
