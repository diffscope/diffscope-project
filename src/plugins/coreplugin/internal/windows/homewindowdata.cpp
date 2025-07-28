#include "homewindowdata.h"

#include <QQmlComponent>

#include <coreplugin/icore.h>

namespace Core::Internal {
    HomeWindowData::HomeWindowData(QObject *parent) : QObject(parent) {
        m_actionContext = new QAK::QuickActionContext(this);
        m_actionContext->setMenuComponent(new QQmlComponent(ICore::qmlEngine(), "SVSCraft.UIComponents", "Menu", this));
        m_actionContext->setSeparatorComponent(new QQmlComponent(ICore::qmlEngine(), "SVSCraft.UIComponents", "MenuSeparator", this));
        m_actionContext->setStretchComponent(new QQmlComponent(ICore::qmlEngine(), "SVSCraft.UIComponents", "MenuSeparator", this));
    }
    HomeWindowData::~HomeWindowData() = default;
    QAK::QuickActionContext *HomeWindowData::actionContext() const {
        return m_actionContext;
    }
}
