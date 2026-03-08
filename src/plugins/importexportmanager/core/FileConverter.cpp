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

    QStringList FileConverter::fileDialogFilters() const {
        Q_D(const FileConverter);
        return d->fileDialogFilters;
    }

    FileConverter::Mode FileConverter::mode() const {
        Q_D(const FileConverter);
        return d->mode;
    }

    FileConverter::HeuristicPriority FileConverter::heuristicPriority() const {
        Q_D(const FileConverter);
        return d->heuristicPriority;
    }

    QStringList FileConverter::heuristicFilters() const {
        Q_D(const FileConverter);
        return d->heuristicFilters;
    }

    bool FileConverter::runPreExecCheck() {
        return true;
    }

    bool FileConverter::execImport(const QString &path, QDspx::Model &model, QWindow *window) {
        return false;
    }

    bool FileConverter::execExport(const QString &path, const QDspx::Model &model, QWindow *window) {
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

    void FileConverter::setFileDialogFilters(const QStringList &filters) {
        Q_D(FileConverter);
        d->fileDialogFilters = filters;
    }

    void FileConverter::setMode(Mode mode) {
        Q_D(FileConverter);
        d->mode = mode;
    }

    void FileConverter::setHeuristicPriority(HeuristicPriority priority) {
        Q_D(FileConverter);
        d->heuristicPriority = priority;
    }

    void FileConverter::setHeuristicFilters(const QStringList &filters) {
        Q_D(FileConverter);
        d->heuristicFilters = filters;
    }

}
