#ifndef DIFFSCOPE_IMPORT_EXPORT_MANAGER_FILECONVERTER_H
#define DIFFSCOPE_IMPORT_EXPORT_MANAGER_FILECONVERTER_H

#include <QObject>

namespace QDspx {
    struct Model;
}

namespace ImportExportManager {

    class FileConverterPrivate;

    class FileConverter : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(FileConverter)
    public:
        explicit FileConverter(QObject *parent = nullptr);
        ~FileConverter() override;

        QString name() const;
        QString description() const;
        QStringList filters() const;

        enum Mode {
            Import = 0x1,
            Export = 0x2,
        };
        Q_ENUM(Mode)
        Q_DECLARE_FLAGS(Modes, Mode)

        Modes modes() const;

        virtual bool execImport(const QString &filename, QDspx::Model &model);
        virtual bool execExport(const QString &filename, const QDspx::Model &model);

    protected:
        void setName(const QString &name);
        void setDescription(const QString &description);
        void setFilters(const QStringList &filters);
        void setModes(Modes modes);

    private:
        QScopedPointer<FileConverterPrivate> d_ptr;
    };

    Q_DECLARE_OPERATORS_FOR_FLAGS(FileConverter::Modes)

}

#endif //DIFFSCOPE_IMPORT_EXPORT_MANAGER_FILECONVERTER_H
