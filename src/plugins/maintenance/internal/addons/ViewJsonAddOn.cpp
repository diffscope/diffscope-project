#include "ViewJsonAddOn.h"

#include <optional>

#include <QDesktopServices>
#include <QDir>
#include <QLoggingCategory>
#include <QQmlComponent>
#include <QSaveFile>
#include <QUrl>
#include <QUuid>
#include <QVariant>
#include <QJsonDocument>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftQuick/MessageBox.h>

#include <opendspx/model.h>
#include <opendspxserializer/serializer.h>

#include <dspxmodel/Model.h>

#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/DspxDocument.h>

namespace Maintenance {

    Q_STATIC_LOGGING_CATEGORY(lcViewJsonAddOn, "diffscope.maintenance.viewjsonaddon")

    ViewJsonAddOn::ViewJsonAddOn(QObject *parent) : Core::WindowInterfaceAddOn(parent) {
    }

    ViewJsonAddOn::~ViewJsonAddOn() = default;

    void ViewJsonAddOn::initialize() {
        generateSessionId();

        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.Maintenance", "ViewJsonAddOnActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto object = component.createWithInitialProperties({{"addOn", QVariant::fromValue(this)}});
        object->setParent(this);
        QMetaObject::invokeMethod(object, "registerToContext", windowInterface->actionContext());
    }

    void ViewJsonAddOn::extensionsInitialized() {
    }

    bool ViewJsonAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

    void ViewJsonAddOn::generateSessionId() {
        auto uuidBytes = QUuid::createUuid().toRfc4122();
        auto base64 = uuidBytes.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
        m_sessionId = QString::fromLatin1(base64);
    }

    QByteArray ViewJsonAddOn::serializeJson() const {
        auto model = windowHandle()->cast<Core::ProjectWindowInterface>()->projectDocumentContext()->document()->model()->toQDspx();
        QDspx::SerializationErrorList errors;
        auto data = QDspx::Serializer::serialize(model, errors, QDspx::Serializer::CheckError);
        data = QJsonDocument::fromJson(data).toJson(QJsonDocument::Indented);
        return data;
    }

    void ViewJsonAddOn::generateJsonFileAndOpen() {
        const auto data = serializeJson();

        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        auto attemptWrite = [&](const QString &sessionId) -> std::optional<QString> {
            auto path = QDir(QDir::tempPath()).filePath(sessionId + QStringLiteral(".json"));
            QSaveFile file(path);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
                qCWarning(lcViewJsonAddOn) << "Failed to open the file for writing" << path << file.errorString();
                return std::nullopt;
            }
            if (file.write(data) == -1) {
                qCWarning(lcViewJsonAddOn) << "Failed to write the file" << path << file.errorString();
                return std::nullopt;
            }
            if (!file.commit()) {
                qCWarning(lcViewJsonAddOn) << "Failed to commit the file" << path << file.errorString();
                return std::nullopt;
            }
            return path;
        };

        auto path = attemptWrite(m_sessionId);
        if (!path) {
            generateSessionId();
            path = attemptWrite(m_sessionId);
            if (!path) {
                auto errorPath = QDir(QDir::tempPath()).filePath(m_sessionId + QStringLiteral(".json"));
                SVS::MessageBox::critical(
                    Core::RuntimeInterface::qmlEngine(),
                    windowInterface ? windowInterface->window() : nullptr,
                    tr("Failed to create JSON file"),
                    tr("Could not create temporary JSON file:\n\n%1").arg(QDir::toNativeSeparators(errorPath))
                );
                return;
            }
        }

        QDesktopServices::openUrl(QUrl::fromLocalFile(*path));
    }

}
