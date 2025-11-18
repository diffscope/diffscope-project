import QtQml

QtObject {

    readonly property var importFormatComboBoxModel: [...ConverterCollection.fileConverters]
        .filter(converter => converter.modes & FileConverter.Import)
        .map(converter => ({text: converter.name, data: converter}))

    readonly property var exportFormatComboBoxModel: [...ConverterCollection.fileConverters]
        .filter(converter => converter.modes & FileConverter.Export)
        .map(converter => ({text: converter.name, data: converter}))

}