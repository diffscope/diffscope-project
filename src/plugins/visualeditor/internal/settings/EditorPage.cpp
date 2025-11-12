#include "EditorPage.h"

#include <QApplication>
#include <QLoggingCategory>
#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>

#include <visualeditor/internal/EditorPreference.h>

namespace VisualEditor::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcEditorPage, "diffscope.visualeditor.editorpage")

    EditorPage::EditorPage(QObject *parent) : Core::ISettingPage("org.diffscope.visualeditor.Editor", parent) {
        setTitle(tr("Editor"));
        setDescription(tr("Configure editor appearance and interaction behaviors"));
    }

    EditorPage::~EditorPage() {
        delete m_widget;
    }

    QString EditorPage::sortKeyword() const {
        return QStringLiteral("Editor");
    }

    bool EditorPage::matches(const QString &word) {
        return Core::ISettingPage::matches(word) || widgetMatches(word);
    }

    QObject *EditorPage::widget() {
        if (m_widget)
            return m_widget;
        qCDebug(lcEditorPage) << "Creating widget";
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.VisualEditor", "EditorPage");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        m_widget = component.createWithInitialProperties({{"pageHandle", QVariant::fromValue(this)}});
        m_widget->setParent(this);
        return m_widget;
    }

    void EditorPage::beginSetting() {
        qCInfo(lcEditorPage) << "Beginning setting";
        widget();
        m_widget->setProperty("alternateAxisModifier", EditorPreference::instance()->property("alternateAxisModifier"));
        qCDebug(lcEditorPage) << m_widget->property("alternateAxisModifier");
        m_widget->setProperty("zoomModifier", EditorPreference::instance()->property("zoomModifier"));
        qCDebug(lcEditorPage) << m_widget->property("zoomModifier");
        m_widget->setProperty("pageModifier", EditorPreference::instance()->property("pageModifier"));
        qCDebug(lcEditorPage) << m_widget->property("pageModifier");
        m_widget->setProperty("usePageModifierAsAlternateAxisZoom", EditorPreference::instance()->property("usePageModifierAsAlternateAxisZoom"));
        qCDebug(lcEditorPage) << m_widget->property("usePageModifierAsAlternateAxisZoom");
        m_widget->setProperty("middleButtonAutoScroll", EditorPreference::instance()->property("middleButtonAutoScroll"));
        qCDebug(lcEditorPage) << m_widget->property("middleButtonAutoScroll");
        m_widget->setProperty("autoDurationPositionAlignment", EditorPreference::instance()->property("autoDurationPositionAlignment"));
        qCDebug(lcEditorPage) << m_widget->property("autoDurationPositionAlignment");
        m_widget->setProperty("enableTemporarySnapOff", EditorPreference::instance()->property("enableTemporarySnapOff"));
        qCDebug(lcEditorPage) << m_widget->property("enableTemporarySnapOff");
        m_widget->setProperty("started", true);
        Core::ISettingPage::beginSetting();
    }

    bool EditorPage::accept() {
        qCInfo(lcEditorPage) << "Accepting";
        qCDebug(lcEditorPage) << "alternateAxisModifier" << m_widget->property("alternateAxisModifier");
        EditorPreference::instance()->setProperty("alternateAxisModifier", m_widget->property("alternateAxisModifier"));
        qCDebug(lcEditorPage) << "zoomModifier" << m_widget->property("zoomModifier");
        EditorPreference::instance()->setProperty("zoomModifier", m_widget->property("zoomModifier"));
        qCDebug(lcEditorPage) << "pageModifier" << m_widget->property("pageModifier");
        EditorPreference::instance()->setProperty("pageModifier", m_widget->property("pageModifier"));
        qCDebug(lcEditorPage) << "usePageModifierAsAlternateAxisZoom" << m_widget->property("usePageModifierAsAlternateAxisZoom");
        EditorPreference::instance()->setProperty("usePageModifierAsAlternateAxisZoom", m_widget->property("usePageModifierAsAlternateAxisZoom"));
        qCDebug(lcEditorPage) << "middleButtonAutoScroll" << m_widget->property("middleButtonAutoScroll");
        EditorPreference::instance()->setProperty("middleButtonAutoScroll", m_widget->property("middleButtonAutoScroll"));
        qCDebug(lcEditorPage) << "autoDurationPositionAlignment" << m_widget->property("autoDurationPositionAlignment");
        EditorPreference::instance()->setProperty("autoDurationPositionAlignment", m_widget->property("autoDurationPositionAlignment"));
        qCDebug(lcEditorPage) << "enableTemporarySnapOff" << m_widget->property("enableTemporarySnapOff");
        EditorPreference::instance()->setProperty("enableTemporarySnapOff", m_widget->property("enableTemporarySnapOff"));
        EditorPreference::instance()->save();
        return Core::ISettingPage::accept();
    }

    void EditorPage::endSetting() {
        qCInfo(lcEditorPage) << "Ending setting";
        m_widget->setProperty("started", false);
        Core::ISettingPage::endSetting();
    }

    QStringList EditorPage::scrollModifierTexts() {
        return {
            QKeySequence(Qt::ControlModifier).toString(QKeySequence::NativeText) + tr("Scroll"),
            QKeySequence(Qt::AltModifier).toString(QKeySequence::NativeText) + tr("Scroll"),
            QKeySequence(Qt::ShiftModifier).toString(QKeySequence::NativeText) + tr("Scroll"),
        };
    }

    QString EditorPage::shiftText() {
        return QKeySequence(Qt::Key_Shift).toString(QKeySequence::NativeText);
    }

    bool EditorPage::widgetMatches(const QString &word) {
        widget();
        auto matcher = m_widget->property("matcher").value<QObject *>();
        bool ret = false;
        QMetaObject::invokeMethod(matcher, "matches", qReturnArg(ret), word);
        return ret;
    }

}