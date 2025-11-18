#ifndef DIFFSCOPE_IMPORT_EXPORT_MANAGER_CLIPBOARDCONVERTER_H
#define DIFFSCOPE_IMPORT_EXPORT_MANAGER_CLIPBOARDCONVERTER_H

#include <QObject>

class QMimeData;

namespace QDspx {
    struct Model;
}

namespace ImportExportManager {

    class ClipboardConverterPrivate;

    class ClipboardConverter : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(ClipboardConverter)
    public:
        explicit ClipboardConverter(QObject *parent = nullptr);
        ~ClipboardConverter() override;

        QString name() const;
        QString description() const;
        QStringList mimeTypes() const;

        enum Mode {
            Paste = 0x1,
            Copy = 0x2,
        };
        Q_ENUM(Mode)
        Q_DECLARE_FLAGS(Modes, Mode)

        Modes modes() const;

        virtual bool paste(const QMimeData *mimeData, QDspx::Model &model);
        virtual bool copy(QMimeData *mimeData, const QDspx::Model &model);

    protected:
        void setName(const QString &name);
        void setDescription(const QString &description);
        void setMimeTypes(const QStringList &mimeTypes);
        void setModes(Modes modes);

    private:
        QScopedPointer<ClipboardConverterPrivate> d_ptr;
    };

    Q_DECLARE_OPERATORS_FOR_FLAGS(ClipboardConverter::Modes)

}

#endif //DIFFSCOPE_IMPORT_EXPORT_MANAGER_CLIPBOARDCONVERTER_H
