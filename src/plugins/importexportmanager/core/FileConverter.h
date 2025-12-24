#ifndef DIFFSCOPE_IMPORT_EXPORT_MANAGER_FILECONVERTER_H
#define DIFFSCOPE_IMPORT_EXPORT_MANAGER_FILECONVERTER_H

#include <QObject>

#include <importexportmanager/importexportmanagerglobal.h>

class QWindow;

namespace QDspx {
    struct Model;
}

namespace ImportExportManager {

    class FileConverterPrivate;

    class IMPORT_EXPORT_MANAGER_EXPORT FileConverter : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(FileConverter)
        Q_PROPERTY(QString name READ name CONSTANT)
        Q_PROPERTY(QString description READ description CONSTANT)
        Q_PROPERTY(QStringList fileDialogFilters READ fileDialogFilters CONSTANT)
        Q_PROPERTY(Mode mode READ mode CONSTANT)
    public:
        ~FileConverter() override;

        QString name() const;
        QString description() const;
        QStringList fileDialogFilters() const;

        enum Mode {
            Import,
            Export,
        };
        Q_ENUM(Mode)

        Mode mode() const;

        enum HeuristicPriority {
            Normal,
            Low,
        };
        Q_ENUM(HeuristicPriority)

        HeuristicPriority heuristicPriority() const;

        QStringList heuristicFilters() const;

        virtual bool runPreExecCheck();
        virtual bool execImport(const QString &path, QDspx::Model &model, QWindow *window);
        virtual bool execExport(const QString &path, const QDspx::Model &model, QWindow *window);

    protected:
        explicit FileConverter(QObject *parent = nullptr);
        void setName(const QString &name);
        void setDescription(const QString &description);
        void setFileDialogFilters(const QStringList &filters);
        void setMode(Mode mode);
        void setHeuristicPriority(HeuristicPriority priority);
        void setHeuristicFilters(const QStringList &filters);

    private:
        QScopedPointer<FileConverterPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_IMPORT_EXPORT_MANAGER_FILECONVERTER_H
