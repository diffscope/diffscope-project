#ifndef DIFFSCOPE_UISHELL_MIDITRACKSELECTORDIALOG_H
#define DIFFSCOPE_UISHELL_MIDITRACKSELECTORDIALOG_H

#include <QByteArray>
#include <QDialog>
#include <QScopedPointer>
#include <QList>
#include <QString>

#include <uishell/UIShellGlobal.h>

namespace UIShell {

    class MIDITrackSelectorDialogPrivate;
    class MIDITrackSelectorStringConverter;

    class UISHELL_EXPORT MIDITrackSelectorDialog : public QDialog {
        Q_OBJECT
        Q_DECLARE_PRIVATE(MIDITrackSelectorDialog)

        Q_PROPERTY(QString codec READ codec WRITE setCodec NOTIFY codecChanged)
        Q_PROPERTY(QList<int> selectedIndexes READ selectedIndexes NOTIFY selectedIndexesChanged)

    public:
        struct TrackInfo {
            QByteArray name;
            QString rangeText;
            int noteCount = 0;
            QList<QByteArray> lyrics;
            bool disabled = false;
            bool selectedByDefault = false;
        };

        explicit MIDITrackSelectorDialog(QWidget *parent = nullptr, MIDITrackSelectorStringConverter *converter = nullptr);
        ~MIDITrackSelectorDialog() override;

        QList<TrackInfo> trackInfoList() const;
        void setTrackInfoList(const QList<TrackInfo> &trackInfoList);

        QString codec() const;
        void setCodec(const QString &codec);

        QList<int> selectedIndexes() const;

        Q_INVOKABLE void detectCodec();

    Q_SIGNALS:
        void codecChanged(const QString &codec);
        void selectedIndexesChanged();

    private:
        QScopedPointer<MIDITrackSelectorDialogPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_UISHELL_MIDITRACKSELECTORDIALOG_H
