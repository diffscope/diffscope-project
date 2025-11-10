#ifndef DIFFSCOPE_VISUALEDITOR_EDITORPREFERENCE_H
#define DIFFSCOPE_VISUALEDITOR_EDITORPREFERENCE_H

#include <qqmlintegration.h>

#include <QObject>

class QQmlEngine;
class QJSEngine;

namespace VisualEditor::Internal {

    class VisualEditorPlugin;

    class EditorPreferencePrivate;

    class EditorPreference : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON
        Q_DECLARE_PRIVATE(EditorPreference)
        Q_PROPERTY(EditorPreference::ScrollModifier alternateAxisModifier READ alternateAxisModifier WRITE setAlternateAxisModifier NOTIFY alternateAxisModifierChanged)
        Q_PROPERTY(EditorPreference::ScrollModifier zoomModifier READ zoomModifier WRITE setZoomModifier NOTIFY zoomModifierChanged)
        Q_PROPERTY(EditorPreference::ScrollModifier pageModifier READ pageModifier WRITE setPageModifier NOTIFY pageModifierChanged)
        Q_PROPERTY(bool usePageModifierAsAlternateAxisZoom READ usePageModifierAsAlternateAxisZoom WRITE setUsePageModifierAsAlternateAxisZoom NOTIFY usePageModifierAsAlternateAxisZoomChanged)
        Q_PROPERTY(bool middleButtonAutoScroll READ middleButtonAutoScroll WRITE setMiddleButtonAutoScroll NOTIFY middleButtonAutoScrollChanged)

    public:
        ~EditorPreference() override;

        static EditorPreference *instance();

        static inline EditorPreference *create(QQmlEngine *, QJSEngine *) {
            return instance();
        }

        void load();
        void save() const;

        enum ScrollModifier {
            SM_Control,
            SM_Alt,
            SM_Shift,
        };
        Q_ENUM(ScrollModifier)

        static ScrollModifier alternateAxisModifier();
        static void setAlternateAxisModifier(ScrollModifier alternateAxisModifier);

        static ScrollModifier zoomModifier();
        static void setZoomModifier(ScrollModifier zoomModifier);

        static ScrollModifier pageModifier();
        static void setPageModifier(ScrollModifier pageModifier);

        static bool usePageModifierAsAlternateAxisZoom();
        static void setUsePageModifierAsAlternateAxisZoom(bool usePageModifierAsAlternateAxisZoom);

        static bool middleButtonAutoScroll();
        static void setMiddleButtonAutoScroll(bool middleButtonAutoScroll);

    Q_SIGNALS:
        void alternateAxisModifierChanged();
        void zoomModifierChanged();
        void pageModifierChanged();
        void usePageModifierAsAlternateAxisZoomChanged();
        void middleButtonAutoScrollChanged();

    private:
        friend class VisualEditorPlugin;
        explicit EditorPreference(QObject *parent = nullptr);
        QScopedPointer<EditorPreferencePrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_EDITORPREFERENCE_H