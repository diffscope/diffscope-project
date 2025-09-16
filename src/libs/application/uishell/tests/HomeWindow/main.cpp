#include <QQmlApplicationEngine>
#include <QGuiApplication>
#include <QSurfaceFormat>
#include <QQuickStyle>
#include <QStandardItemModel>

#include <uishell/USDef.h>

using namespace UIShell;

int main(int argc, char *argv[]) {
    QGuiApplication a(argc, argv);

    auto sf = QSurfaceFormat::defaultFormat();
    sf.setSamples(8);
    QSurfaceFormat::setDefaultFormat(sf);

    QQuickStyle::setStyle("SVSCraft.UIComponents");
    QQuickStyle::setFallbackStyle("Basic");

    auto recentFilesModel = new QStandardItemModel(&a);
    {
        auto item = new QStandardItem;
        item->setData("File 1", USDef::RF_NameRole);
        item->setData("/path/to/file1.dspx", USDef::RF_PathRole);
        item->setData("11:45 Today", USDef::RF_LastModifiedTextRole);
        item->setData("qrc:/qt/qml/DiffScope/UIShell/Test/HomeWindow/test_banner.png", USDef::RF_ThumbnailRole);
        item->setData("qrc:/qt/qml/DiffScope/UIShell/Test/HomeWindow/test_banner.png", USDef::RF_IconRole);
        recentFilesModel->appendRow(item);
    }
    {
        auto item = new QStandardItem;
        item->setData("File Without Thumbnail", USDef::RF_NameRole);
        item->setData("/path/to/file1.dspx", USDef::RF_PathRole);
        item->setData("11:45 Today", USDef::RF_LastModifiedTextRole);
        item->setData("", USDef::RF_ThumbnailRole);
        item->setData("qrc:/qt/qml/DiffScope/UIShell/Test/HomeWindow/test_banner.png", USDef::RF_IconRole);
        recentFilesModel->appendRow(item);
    }
    {
        auto item = new QStandardItem;
        item->setData("Very Long Very Long Very Long Very Long", USDef::RF_NameRole);
        item->setData("/path/to/file1.dspx", USDef::RF_PathRole);
        item->setData("Very Long Very Long Very Long Very Long", USDef::RF_LastModifiedTextRole);
        item->setData("qrc:/qt/qml/DiffScope/UIShell/Test/HomeWindow/test_banner.png", USDef::RF_ThumbnailRole);
        item->setData("qrc:/qt/qml/DiffScope/UIShell/Test/HomeWindow/test_banner.png", USDef::RF_IconRole);
        recentFilesModel->appendRow(item);
    }
    for (int i = 2; i <= 8; i++) {
        auto item = new QStandardItem;
        item->setData(QString("File %1").arg(i), USDef::RF_NameRole);
        item->setData("/path/to/file1.dspx", USDef::RF_PathRole);
        item->setData("11:45 Today", USDef::RF_LastModifiedTextRole);
        item->setData("qrc:/qt/qml/DiffScope/UIShell/Test/HomeWindow/test_banner.png", USDef::RF_ThumbnailRole);
        item->setData("qrc:/qt/qml/DiffScope/UIShell/Test/HomeWindow/test_banner.png", USDef::RF_IconRole);
        recentFilesModel->appendRow(item);
    }

    auto recoveryFilesModel = new QStandardItemModel(&a);
    {
        auto item = new QStandardItem;
        item->setData("Recovery File", USDef::RF_NameRole);
        item->setData("/path/to/file1.dspx", USDef::RF_PathRole);
        item->setData("11:45 Today", USDef::RF_LastModifiedTextRole);
        item->setData("qrc:/qt/qml/DiffScope/UIShell/Test/HomeWindow/test_banner.png", USDef::RF_IconRole);
        recoveryFilesModel->appendRow(item);
    }
    {
        auto item = new QStandardItem;
        item->setData("Unsaved", USDef::RF_NameRole);
        item->setData("", USDef::RF_PathRole);
        item->setData("11:45 Today", USDef::RF_LastModifiedTextRole);
        item->setData("qrc:/qt/qml/DiffScope/UIShell/Test/HomeWindow/test_banner.png", USDef::RF_IconRole);
        recoveryFilesModel->appendRow(item);
    }


    QQmlApplicationEngine engine;
    engine.setInitialProperties({
        {"recentFilesModel", QVariant::fromValue(recentFilesModel)},
        {"recoveryFilesModel", QVariant::fromValue(recoveryFilesModel)},
    });
    engine.load(":/qt/qml/DiffScope/UIShell/Test/HomeWindow/main.qml");

    return a.exec();
}
