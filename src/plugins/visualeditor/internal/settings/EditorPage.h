#ifndef DIFFSCOPE_VISUALEDITOR_EDITORPAGE_H
#define DIFFSCOPE_VISUALEDITOR_EDITORPAGE_H

#include <CoreApi/isettingpage.h>

namespace VisualEditor::Internal {

    class EditorPage : public Core::ISettingPage {
        Q_OBJECT
        Q_PROPERTY(QStringList scrollModifierTexts READ scrollModifierTexts CONSTANT)
        Q_PROPERTY(QString shiftText READ shiftText CONSTANT)
    public:
        explicit EditorPage(QObject *parent = nullptr);
        ~EditorPage() override;

        QString sortKeyword() const override;
        bool matches(const QString &word) override;
        QObject *widget() override;
        void beginSetting() override;
        bool accept() override;
        void endSetting() override;

        static QStringList scrollModifierTexts();
        static QString shiftText();

    private:
        bool widgetMatches(const QString &word);
        mutable QObject *m_widget{};
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_EDITORPAGE_H