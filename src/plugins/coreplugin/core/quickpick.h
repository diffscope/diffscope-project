#ifndef DIFFSCOPE_COREPLUGIN_QUICKPICK_H
#define DIFFSCOPE_COREPLUGIN_QUICKPICK_H

#include <QObject>
#include <qqmlintegration.h>

#include <coreplugin/coreglobal.h>

class QAbstractItemModel;

namespace Core {

    class QuickPickPrivate;
    class WindowInterface;

    class CORE_EXPORT QuickPick : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(QuickPick)

        Q_PROPERTY(QAbstractItemModel *model READ model WRITE setModel NOTIFY modelChanged)
        Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged)
        Q_PROPERTY(QString placeholderText READ placeholderText WRITE setPlaceholderText NOTIFY placeholderTextChanged)
        Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
        Q_PROPERTY(WindowInterface *windowHandle READ windowHandle WRITE setWindowHandle NOTIFY windowHandleChanged)
        Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)

    public:
        explicit QuickPick(QObject *parent = nullptr);
        ~QuickPick() override;

        QAbstractItemModel *model() const;
        void setModel(QAbstractItemModel *model);

        QString filterText() const;
        void setFilterText(const QString &filterText);

        QString placeholderText() const;
        void setPlaceholderText(const QString &placeholderText);

        int currentIndex() const;
        void setCurrentIndex(int currentIndex);

        WindowInterface *windowHandle() const;
        void setWindowHandle(WindowInterface *windowHandle);

        bool visible() const;
        void setVisible(bool visible);

    public Q_SLOTS:
        void show();
        int exec();
        void accept();
        void done(int result);
        void reject();

    Q_SIGNALS:
        void modelChanged(QAbstractItemModel *model);
        void filterTextChanged(const QString &filterText);
        void placeholderTextChanged(const QString &placeholderText);
        void currentIndexChanged(int currentIndex);
        void windowHandleChanged(Core::WindowInterface *windowHandle);
        void visibleChanged(bool visible);

        void accepted();
        void rejected();
        void finished(int result);

    private Q_SLOTS:
        void updateFromCommandPalette();
        void handleWindowHandleDestroyed();

    private:
        QScopedPointer<QuickPickPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_QUICKPICK_H
