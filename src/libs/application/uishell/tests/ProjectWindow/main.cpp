// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include <QQmlApplicationEngine>
#include <QApplication>
#include <QSurfaceFormat>
#include <QQuickStyle>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    auto sf = QSurfaceFormat::defaultFormat();
    sf.setSamples(8);
    QSurfaceFormat::setDefaultFormat(sf);

    QQuickStyle::setStyle("SVSCraft.UIComponents");
    QQuickStyle::setFallbackStyle("Basic");

    QQmlApplicationEngine engine;
    engine.load(":/qt/qml/DiffScope/UIShell/Test/ProjectWindow/main.qml");

    return a.exec();
}
