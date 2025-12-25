#include "ConverterCollection.h"

#include <importexportmanager/FileConverter.h>

namespace ImportExportManager {

    ConverterCollection *m_instance = nullptr;

    static QList<FileConverter *> s_fileConverters;

    ConverterCollection::ConverterCollection(QObject *parent) : QObject(parent) {
        Q_ASSERT(!m_instance);
        m_instance = this;
    }

    ConverterCollection::~ConverterCollection() {
        m_instance = nullptr;
    }

    ConverterCollection * ConverterCollection::instance() {
        return m_instance;
    }

    QList<FileConverter *> ConverterCollection::fileConverters() {
        return s_fileConverters;
    }

    void ConverterCollection::addFileConverter(FileConverter *converter) {
        s_fileConverters.append(converter);
    }

}

#include "moc_ConverterCollection.cpp"