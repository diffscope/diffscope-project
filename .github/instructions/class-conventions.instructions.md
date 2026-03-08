# C++ Class Conventions

This document outlines the coding conventions for C++ classes in the DiffScope project.

## Class Categories

### 1. External Interface Classes (with Private Implementation)

These are classes that inherit from QObject or its subclasses and serve as public APIs. They use the private implementation idiom (pimpl) to hide implementation details.

#### File Naming Convention
- **Header file**: `classname.h`
- **Implementation file**: `classname.cpp`
- **Private header file**: `classname_p.h`

#### Example Structure

**notificationmessage.h**:
```cpp
#ifndef DIFFSCOPE_COREPLUGIN_NOTIFICATIONMESSAGE_H
#define DIFFSCOPE_COREPLUGIN_NOTIFICATIONMESSAGE_H

#include <QObject>
#include <qqmlintegration.h>

#include <SVSCraftCore/SVSCraftNamespace.h>

#include <coreplugin/coreglobal.h>

namespace Core {

    class NotificationMessagePrivate;

    class CORE_EXPORT NotificationMessage : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(NotificationMessage)
        
        Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
        Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)

    public:
        explicit NotificationMessage(QObject *parent = nullptr);
        ~NotificationMessage() override;

        QString title() const;
        void setTitle(const QString &title);

        QString text() const;
        void setText(const QString &text);

        Q_INVOKABLE void hide();
        Q_INVOKABLE void close();

    Q_SIGNALS:
        void titleChanged(const QString &title);
        void textChanged(const QString &text);
        void hidden();
        void closed();

    private:
        QScopedPointer<NotificationMessagePrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_NOTIFICATIONMESSAGE_H
```

**notificationmessage_p.h**:
```cpp
#ifndef DIFFSCOPE_COREPLUGIN_NOTIFICATIONMESSAGE_P_H
#define DIFFSCOPE_COREPLUGIN_NOTIFICATIONMESSAGE_P_H

#include <coreplugin/notificationmessage.h>

namespace UIShell {
    class BubbleNotificationHandle;
}

namespace Core {

    class NotificationMessagePrivate {
        Q_DECLARE_PUBLIC(NotificationMessage)
    public:
        NotificationMessage *q_ptr;
        UIShell::BubbleNotificationHandle *handle;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_NOTIFICATIONMESSAGE_P_H
```

**notificationmessage.cpp**:
```cpp
#include "notificationmessage.h"
#include "notificationmessage_p.h"

#include <uishell/BubbleNotificationHandle.h>

namespace Core {
    NotificationMessage::NotificationMessage(QObject *parent) 
        : QObject(parent), d_ptr(new NotificationMessagePrivate) {
        Q_D(NotificationMessage);
        d->q_ptr = this;
        d->handle = new UIShell::BubbleNotificationHandle;
        // ... implementation
    }

    NotificationMessage::~NotificationMessage() = default;

    QString NotificationMessage::title() const {
        Q_D(const NotificationMessage);
        return d->handle->title();
    }

    void NotificationMessage::setTitle(const QString &title) {
        Q_D(NotificationMessage);
        d->handle->setTitle(title);
    }
}
```

### 2. Internal Implementation Classes (without Private Implementation)

These classes inherit from QObject or its subclasses but are used internally. They declare private members directly without using pimpl.

#### Example Structure

**projectwindowworkspacemanager.h**:
```cpp
#ifndef DIFFSCOPE_COREPLUGIN_PROJECTWINDOWWORKSPACEMANAGER_H
#define DIFFSCOPE_COREPLUGIN_PROJECTWINDOWWORKSPACEMANAGER_H

#include <QObject>

#include <coreplugin/internal/projectwindowworkspacelayout.h>

namespace Core::Internal {

    class ProjectWindowWorkspaceLayout;

    class ProjectWindowWorkspaceManager : public QObject {
        Q_OBJECT
        Q_PROPERTY(ProjectWindowWorkspaceLayout currentLayout READ currentLayout WRITE setCurrentLayout NOTIFY currentLayoutChanged)

    public:
        explicit ProjectWindowWorkspaceManager(QObject *parent = nullptr);
        ~ProjectWindowWorkspaceManager() override;

        ProjectWindowWorkspaceLayout currentLayout() const;
        void setCurrentLayout(const ProjectWindowWorkspaceLayout &layout);

        void load();
        void save() const;

    Q_SIGNALS:
        void currentLayoutChanged();

    private:
        ProjectWindowWorkspaceLayout m_currentLayout;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_PROJECTWINDOWWORKSPACEMANAGER_H
```

### 3. QML Element Classes

These classes are designed to be used as QML elements and should not be called directly from C++.

**Note**: For QML module classes, property notify signals typically do not include parameters, unlike regular QObject classes. This follows QML property binding conventions where only the signal emission is needed to trigger property change notifications.

#### File Naming Convention
- **Header file**: `classname_p.h` (note the `_p` suffix)
- **Implementation file**: `classname.cpp`
- **Private header file**: `classname_p_p.h` (note the `_p_p` suffix)

#### Example Structure

**ThemedItem_p.h**:
```cpp
#ifndef SVSCRAFT_THEMEDITEM_P_H
#define SVSCRAFT_THEMEDITEM_P_H

#include <QObject>
#include <QtQuickTemplates2/private/qquickicon_p.h>

#include <SVSCraftQuick/Theme.h>

namespace SVS {

    class ThemedItemAttachedType;
    class ThemedItemPrivate;

    class ThemedItem : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(ThemedItem)
        QML_ANONYMOUS

        Q_PROPERTY(SVS::SVSCraft::ControlType controlType READ controlType WRITE setControlType NOTIFY controlTypeChanged)
        Q_PROPERTY(bool foldable READ foldable WRITE setFoldable NOTIFY foldableChanged)

    public:
        ~ThemedItem() override;

        SVSCraft::ControlType controlType() const;
        void setControlType(SVSCraft::ControlType value);

        bool foldable() const;
        void setFoldable(bool value);

    Q_SIGNALS:
        void controlTypeChanged();
        void foldableChanged();

    private:
        QScopedPointer<ThemedItemPrivate> d_ptr;
    };

}

#endif //SVSCRAFT_THEMEDITEM_P_H
```

### 4. QML Attached Type Classes

Some QML elements include Attached Types for providing additional functionality to parent objects.

#### Example Structure

**ThemedItem_p_p.h**:
```cpp
#ifndef SVSCRAFT_THEMEDITEM_P_P_H
#define SVSCRAFT_THEMEDITEM_P_P_H

#include <SVSCraftQuick/private/ThemedItem_p.h>

namespace SVS {

    class ThemedItemAttachedType : public QObject {
        Q_OBJECT
        QML_NAMED_ELEMENT(ThemedItem)
        QML_ATTACHED(ThemedItem)
    public:
        static ThemedItem *qmlAttachedProperties(QObject *object);
    };

    class ThemedItemPrivate {
        Q_DECLARE_PUBLIC(ThemedItem)
    public:
        ThemedItem *q_ptr;
        SVSCraft::ControlType controlType = SVSCraft::CT_Normal;
        bool foldable = false;
        bool folded = false;
        QQuickIcon icon;
    };

}

#endif // SVSCRAFT_THEMEDITEM_P_P_H
```

**Attached Type Implementation**:
```cpp
ThemedItem *ThemedItemAttachedType::qmlAttachedProperties(QObject *object) {
    // Type checking for parent object
    if (auto *quickItem = qobject_cast<QQuickItem *>(object)) {
        return new ThemedItem(object);
    }
    
    // Handle other parent types as needed
    qmlWarning(object) << "ThemedItem can only be attached to QQuickItem or its subclasses";
    return nullptr;
}
```

## Code Formatting Standards

### Include Guard Format
Use `#ifndef` style guards with the pattern:
```cpp
#ifndef [PROJECT]_[MODULE]_[CLASSNAME]_[SUFFIX]_H
#define [PROJECT]_[MODULE]_[CLASSNAME]_[SUFFIX]_H
// ... content
#endif //[PROJECT]_[MODULE]_[CLASSNAME]_[SUFFIX]_H
```

Examples:
- `DIFFSCOPE_COREPLUGIN_NOTIFICATIONMESSAGE_H`
- `SVSCRAFT_THEMEDITEM_P_H`
- `SVSCRAFT_THEMEDITEM_P_P_H`

### Forward Declarations
Prefer forward declarations in headers when possible:
```cpp
namespace Core {
    class NotificationMessagePrivate;
}

namespace UIShell {
    class BubbleNotificationHandle;
}
```

### Include Order and Format

#### Header Files (.h)
All includes in header files must use full path format:
```cpp
#include <QObject>                                    // Qt headers
#include <qqmlintegration.h>                        // Qt headers

#include <SVSCraftCore/SVSCraftNamespace.h>         // Dependency libraries
#include <SVSCraftQuick/Theme.h>

#include <coreplugin/coreglobal.h>                   // Current project headers
#include <coreplugin/internal/projectwindowworkspacelayout.h>
```

#### Implementation Files (.cpp)
Local headers can use relative paths, others use full paths:
```cpp
#include "notificationmessage.h"                     // Own header (relative)
#include "notificationmessage_p.h"                  // Own private header (relative)

#include <algorithm>                                 // Standard library
#include <memory>

#include <QSettings>                                 // Qt headers
#include <QQmlEngine>

#include <uishell/BubbleNotificationHandle.h>       // Project headers (full path)
```

### Include Path Conventions

For most external libraries implemented in this project, the include path format is:
- **Public headers**: `<LIBRARY_INCLUDE_PREFIX>/<HEADER_NAME>.h`
- **Private headers** (with `_p` suffix): `<LIBRARY_INCLUDE_PREFIX>/private/<HEADER_NAME>.h`

For plugins, the include path format is:
- **Public headers**: `<PLUGIN_INCLUDE_PREFIX>/<HEADER_NAME>.h`
- **Internal headers** (located in internal directory within source): `<PLUGIN_INCLUDE_PREFIX>/internal/<HEADER_NAME>.h`
- **Private headers**: `<PLUGIN_INCLUDE_PREFIX>/private/<HEADER_NAME>.h`

Regardless of the actual file path in the source directory, the include path only contains the above components and follows this fixed format.

When generating code, refer to other implemented files to understand what the include prefix should be.

Examples:
```cpp
// External library headers
#include <SVSCraftCore/SVSCraftNamespace.h>           // Public header
#include <SVSCraftQuick/private/ThemedItem_p.h>       // Private header

// Plugin headers  
#include <coreplugin/coreglobal.h>                    // Public header
#include <coreplugin/internal/projectwindowworkspacelayout.h>  // Internal header
#include <coreplugin/private/notificationmessage_p.h> // Private header
```

### Include Priority Order
1. Standard C++ library headers
2. Qt framework headers  
3. Third-party dependency headers
4. Current project headers (dependency priority within project)

### Qt Macro Usage
Always use full Qt macro names, never abbreviated forms:
```cpp
// Correct
Q_OBJECT
Q_SIGNALS:
Q_SLOTS:
Q_PROPERTY(...)
Q_INVOKABLE

// Incorrect
signals:
slots:
```

### Member Naming
- Use descriptive names, avoid abbreviations except for widely accepted ones (HTTP, URL, etc.)
- Private members in pimpl classes use plain names
- Member variables in non-pimpl classes use `m_` prefix:
```cpp
class ProjectWindowWorkspaceManager : public QObject {
private:
    ProjectWindowWorkspaceLayout m_currentLayout;
};
```

### Spacing and Indentation
- Use 4 spaces for indentation
- Single blank line between different sections (properties, public methods, signals, private members)
- No blank lines between consecutive similar declarations
- Blank line before and after namespace blocks

### Property Declarations
Group related properties together:
```cpp
Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
Q_PROPERTY(bool closable READ closable WRITE setClosable NOTIFY closableChanged)
```

### Signal and Slot Declarations
Use `Q_SIGNALS:` and group related signals:
```cpp
Q_SIGNALS:
    void titleChanged(const QString &title);
    void textChanged(const QString &text);
    
    void hidden();
    void closed();
```

## Singleton Classes

Singleton classes in DiffScope follow specific patterns based on their initialization requirements and access control needs.

### 1. Friend-Controlled Singletons

These singletons have private constructors and grant access to specific classes through friend declarations. Used when only certain classes should be able to create the singleton instance.

#### Example Structure

**coreinterface.h**:
```cpp
#ifndef DIFFSCOPE_COREPLUGIN_ICORE_H
#define DIFFSCOPE_COREPLUGIN_ICORE_H

#include <CoreApi/coreinterfacebase.h>
#include <coreplugin/coreglobal.h>

namespace Core {

    namespace Internal {
        class CorePlugin;
    }

    class CoreInterfacePrivate;

    class CORE_EXPORT CoreInterface : public CoreInterfaceBase {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON
        Q_DECLARE_PRIVATE(CoreInterface)
    public:
        static CoreInterface *instance();
        static inline CoreInterface *create(QQmlEngine *, QJSEngine *) { return instance(); }

        // Public interface methods
        static QAK::ActionRegistry *actionRegistry();

    private:
        explicit CoreInterface(QObject *parent = nullptr);
        ~CoreInterface();

        CoreInterface(CoreInterfacePrivate &d, QObject *parent = nullptr);

        friend class Internal::CorePlugin;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_ICORE_H
```

**coreinterface.cpp**:
```cpp
#include "coreinterface.h"

namespace Core {

    CoreInterface *CoreInterface::instance() {
        return static_cast<CoreInterface *>(CoreInterfaceBase::instance());
    }

    CoreInterface::CoreInterface(QObject *parent) : CoreInterface(*new CoreInterfacePrivate(), parent) {
    }

    CoreInterface::CoreInterface(CoreInterfacePrivate &d, QObject *parent) : CoreInterfaceBase(d, parent) {
        d.q_ptr = this;
        d.init();
    }

}
```

### 2. Explicitly Created Singletons

These singletons are created during application initialization. The constructor uses `Q_ASSERT` to prevent multiple instances.

#### Example Structure

**runtimeinterface.cpp**:
```cpp
namespace Core {

    static RuntimeInterface *m_instance = nullptr;

    RuntimeInterface::RuntimeInterface(QObject *parent)
        : ObjectPool(*new RuntimeInterfacePrivate(), parent) {
        Q_ASSERT(!m_instance);
        m_instance = this;
    }

    RuntimeInterface::~RuntimeInterface() {
        m_instance = nullptr;
    }

    RuntimeInterface *RuntimeInterface::instance() {
        return m_instance;
    }

}
```

### 3. QML Singletons

QML singletons use `QML_SINGLETON` and implement a static `create` function for QML access.

#### Example Structure

**GlobalHelper_p.h**:
```cpp
#ifndef SVSCRAFT_GLOBALHELPER_P_H
#define SVSCRAFT_GLOBALHELPER_P_H

#include <QObject>
#include <qqmlintegration.h>

namespace SVS {

    class GlobalHelper : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON
    public:
        explicit GlobalHelper(QObject *parent = nullptr);
        ~GlobalHelper() override;

        static inline GlobalHelper *create(QQmlEngine *, QJSEngine *) {
            return new GlobalHelper;
        }

        Q_INVOKABLE static QPoint cursorPos();

    private:
        static AlertHandler m_alertHandler;
    };

}

#endif // SVSCRAFT_GLOBALHELPER_P_H
```

### 4. Window Singletons

Window classes use a simple static instance pattern with explicit assignment in constructor.

#### Example Structure

**homewindowinterface.cpp**:
```cpp
namespace Core {

    static HomeWindowInterface *m_instance = nullptr;

    HomeWindowInterface::HomeWindowInterface(QObject *parent) : HomeWindowInterface(*new HomeWindowInterfacePrivate, parent) {
        m_instance = this;
    }

    HomeWindowInterface::~HomeWindowInterface() {
        m_instance = nullptr;
    }

    HomeWindowInterface *HomeWindowInterface::instance() {
        return m_instance;
    }

}
```

### Singleton Pattern Guidelines

1. **Initialization-Dependent Singletons**: If the singleton's creation is tied to specific initialization flows, it should be explicitly created with `Q_ASSERT(!m_instance)` in the constructor.

2. **Access-Controlled Singletons**: If only specific classes should create the singleton, use private constructors with friend declarations.

3. **QML Integration**: For singletons exposed to QML, declare `QML_SINGLETON` and implement a static `create(QQmlEngine *, QJSEngine *)` function.

4. **Instance Tracking**: Always use static instance variables and reset them in destructors:
   ```cpp
   Constructor() {
       Q_ASSERT(!m_instance);  // For explicitly created singletons
       m_instance = this;
   }
   
   ~Destructor() {
       m_instance = nullptr;
   }
   ```

5. **Thread Safety**: DiffScope singletons are designed for single-threaded use in the main thread. No additional thread synchronization is required.

This convention ensures consistency across the codebase and maintains clear separation between public interfaces, internal implementations, and QML-specific classes.