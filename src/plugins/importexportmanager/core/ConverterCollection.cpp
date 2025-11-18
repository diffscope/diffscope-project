#include "ConverterCollection.h"

#include <importexportmanager/FileConverter.h>
#include <importexportmanager/ClipboardConverter.h>

namespace ImportExportManager {

    ConverterCollection *m_instance = nullptr;

    ConverterCollection::ConverterCollection(QObject *parent) : Core::ObjectPool(parent) {
        Q_ASSERT(!m_instance);
        m_instance = this;
    }

    ConverterCollection::~ConverterCollection() {
        m_instance = nullptr;
    }

    ConverterCollection * ConverterCollection::instance() {
        return m_instance;
    }

    QList<FileConverter *> ConverterCollection::fileConverters() const {
        return getObjects<FileConverter>();
    }

    QList<ClipboardConverter *> ConverterCollection::clipboardConverters() const {
        return getObjects<ClipboardConverter>();
    }

}
