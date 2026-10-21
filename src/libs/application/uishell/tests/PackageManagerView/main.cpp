// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include <initializer_list>
#include <utility>

#include <QDateTime>
#include <QGuiApplication>
#include <QLocale>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QStandardItemModel>
#include <QSurfaceFormat>
#include <QVariant>

#include <SVSCraftFluentSystemIcons/FluentSystemIconsImageProvider.h>

#include <uishell/USDef.h>

namespace {

    QStandardItem *createItem(std::initializer_list<std::pair<int, QVariant>> roles) {
        auto item = new QStandardItem();
        for (const auto &[role, value] : roles) {
            item->setData(value, role);
        }
        return item;
    }

    QStandardItem *createSubtree(const QString &name) {
        return createItem({
            {UIShell::USDef::PR_NameRole, name},
        });
    }

    void addPackageSubtrees(QStandardItem *packageItem) {
        packageItem->setChild(UIShell::USDef::PI_Dependencies, createSubtree("Dependencies"));
        packageItem->setChild(UIShell::USDef::PI_OtherContributions, createSubtree("Other Contributions"));
        packageItem->setChild(UIShell::USDef::PI_Singers, createSubtree("Singers"));
    }

    void addSinger(QStandardItem *singers, QStandardItem *singer) {
        singers->appendRow(singer);
    }

    void populatePackageModel(QStandardItemModel *model) {

        constexpr auto resourcePrefix = "qrc:/qt/qml/DiffScope/UIShell/Test/PackageManagerView/";

        auto basePackage = createItem({
            {UIShell::USDef::PR_IdRole, "diffscope/base-inference"},
            {UIShell::USDef::PR_VersionRole, "1.0.0.0"},
            {UIShell::USDef::PR_PathRole, "C:/packages/base-inference"},
            {UIShell::USDef::PR_InstallationTimeRole, QDateTime::fromString(QStringLiteral("2026-05-01T10:00:00+08:00"), Qt::ISODate).toSecsSinceEpoch()},
            {UIShell::USDef::PR_NameRole, "Base Inference Pack"},
            {UIShell::USDef::PR_DescriptionRole, "Shared inference engines for sample singers."},
            {UIShell::USDef::PR_VendorRole, "DiffScope Samples"},
            {UIShell::USDef::PR_UrlRole, "https://example.com/packages/base-inference"},
        });
        addPackageSubtrees(basePackage);
        basePackage->child(UIShell::USDef::PI_OtherContributions)->appendRow(createItem({
            {UIShell::USDef::PR_IdRole, "base-vocoder"},
            {UIShell::USDef::PR_CategoryRole, "inference"},
            {UIShell::USDef::PR_PathRole, "C:/packages/base-inference/inferences/base-vocoder.json"},
        }));
        basePackage->child(UIShell::USDef::PI_OtherContributions)->appendRow(createItem({
            {UIShell::USDef::PR_IdRole, "base-diffusion"},
            {UIShell::USDef::PR_CategoryRole, "inference"},
            {UIShell::USDef::PR_PathRole, "C:/packages/base-inference/inferences/base-diffusion.json"},
        }));

        basePackage->child(UIShell::USDef::PI_OtherContributions)->appendRow(createItem({
            {UIShell::USDef::PR_IdRole, "base-vocoder"},
            {UIShell::USDef::PR_CategoryRole, "resources.dictionary"},
            {UIShell::USDef::PR_PathRole, ""},
        }));
        basePackage->child(UIShell::USDef::PI_OtherContributions)->appendRow(createItem({
            {UIShell::USDef::PR_IdRole, "alpha"},
            {UIShell::USDef::PR_CategoryRole, "singer.extra"},
            {UIShell::USDef::PR_PathRole, ""},
        }));

        auto runtimePackage = createItem({
            {UIShell::USDef::PR_IdRole, "diffscope/runtime-inference"},
            {UIShell::USDef::PR_VersionRole, "1.1.0.0"},
            {UIShell::USDef::PR_PathRole, "C:/packages/runtime-inference"},
            {UIShell::USDef::PR_InstallationTimeRole, QDateTime::fromString(QStringLiteral("2026-05-01T14:15:00+08:00"), Qt::ISODate).toSecsSinceEpoch()},
            {UIShell::USDef::PR_NameRole, "Runtime Inference Pack"},
            {UIShell::USDef::PR_DescriptionRole, "Runtime inference engines used by singer packages."},
            {UIShell::USDef::PR_VendorRole, "DiffScope Samples"},
            {UIShell::USDef::PR_ReadmePathRole, "C:/packages/runtime-inference/README.md"},
            {UIShell::USDef::PR_LicensePathRole, "C:/packages/runtime-inference/LICENSE.txt"},
        });
        addPackageSubtrees(runtimePackage);
        runtimePackage->child(UIShell::USDef::PI_OtherContributions)->appendRow(createItem({
            {UIShell::USDef::PR_IdRole, "runtime-realtime"},
            {UIShell::USDef::PR_CategoryRole, "inference"},
            {UIShell::USDef::PR_PathRole, "C:/packages/runtime-inference/inferences/runtime-realtime.json"},
        }));
        runtimePackage->child(UIShell::USDef::PI_OtherContributions)->appendRow(createItem({
            {UIShell::USDef::PR_IdRole, "runtime-batch"},
            {UIShell::USDef::PR_CategoryRole, "inference"},
            {UIShell::USDef::PR_PathRole, "C:/packages/runtime-inference/inferences/runtime-batch.json"},
        }));

        auto advancedPackage = createItem({
            {UIShell::USDef::PR_IdRole, "diffscope/advanced-inference"},
            {UIShell::USDef::PR_VersionRole, "1.2.0.0"},
            {UIShell::USDef::PR_PathRole, "C:/packages/advanced-inference"},
            {UIShell::USDef::PR_InstallationTimeRole, QDateTime::fromString(QStringLiteral("2026-05-02T11:30:00+08:00"), Qt::ISODate).toSecsSinceEpoch()},
            {UIShell::USDef::PR_NameRole, "Advanced Inference Pack"},
            {UIShell::USDef::PR_DescriptionRole, "Additional inference backend built on the base pack."},
            {UIShell::USDef::PR_VendorRole, "DiffScope Samples"},
            {UIShell::USDef::PR_ReadmePathRole, "C:/packages/advanced-inference/README.md"},
            {UIShell::USDef::PR_LicensePathRole, "C:/packages/advanced-inference/LICENSE.txt"},
            {UIShell::USDef::PR_UrlRole, "https://example.com/packages/advanced-inference"},
        });
        addPackageSubtrees(advancedPackage);
        advancedPackage->child(UIShell::USDef::PI_Dependencies)->appendRow(createItem({
            {UIShell::USDef::PR_IdRole, "diffscope/base-inference"},
            {UIShell::USDef::PR_VersionRole, "1.0.0.0"},
        }));
        advancedPackage->child(UIShell::USDef::PI_Dependencies)->appendRow(createItem({
            {UIShell::USDef::PR_IdRole, "diffscope/runtime-inference"},
            {UIShell::USDef::PR_VersionRole, "1.1.0.0"},
        }));
        advancedPackage->child(UIShell::USDef::PI_OtherContributions)->appendRow(createItem({
            {UIShell::USDef::PR_IdRole, "advanced-voice"},
            {UIShell::USDef::PR_CategoryRole, "inference"},
            {UIShell::USDef::PR_PathRole, "C:/packages/advanced-inference/inferences/advanced-voice.json"},
        }));
        advancedPackage->child(UIShell::USDef::PI_OtherContributions)->appendRow(createItem({
            {UIShell::USDef::PR_IdRole, "advanced-expression"},
            {UIShell::USDef::PR_CategoryRole, "inference"},
            {UIShell::USDef::PR_PathRole, "C:/packages/advanced-inference/inferences/advanced-expression.json"},
        }));

        auto alphaPackage = createItem({
            {UIShell::USDef::PR_IdRole, "diffscope/singer-alpha"},
            {UIShell::USDef::PR_VersionRole, "2.0.0.0"},
            {UIShell::USDef::PR_PathRole, "C:/packages/singer-alpha"},
            {UIShell::USDef::PR_InstallationTimeRole, QDateTime::fromString(QStringLiteral("2026-05-03T09:20:00+08:00"), Qt::ISODate).toSecsSinceEpoch()},
            {UIShell::USDef::PR_NameRole, "Singer Alpha Pack"},
            {UIShell::USDef::PR_DescriptionRole, "Sample singer package with extracted metadata."},
            {UIShell::USDef::PR_VendorRole, "DiffScope Samples"},
            {UIShell::USDef::PR_ReadmePathRole, "C:/packages/singer-alpha/README.md"},
            {UIShell::USDef::PR_LicensePathRole, "C:/packages/singer-alpha/LICENSE.txt"},
            {UIShell::USDef::PR_UrlRole, "https://example.com/packages/singer-alpha"},
        });
        addPackageSubtrees(alphaPackage);
        alphaPackage->child(UIShell::USDef::PI_Dependencies)->appendRow(createItem({
            {UIShell::USDef::PR_IdRole, "diffscope/base-inference"},
            {UIShell::USDef::PR_VersionRole, "1.0.0.0"},
        }));
        alphaPackage->child(UIShell::USDef::PI_Dependencies)->appendRow(createItem({
            {UIShell::USDef::PR_IdRole, "diffscope/runtime-inference"},
            {UIShell::USDef::PR_VersionRole, "1.1.0.0"},
        }));
        addSinger(alphaPackage->child(UIShell::USDef::PI_Singers), createItem({
            {UIShell::USDef::PR_IdRole, "alpha"},
            {UIShell::USDef::PR_InterfaceRole, "diffsinger"},
            {UIShell::USDef::PR_LevelRole, QLocale().toString(1)},
            {UIShell::USDef::PR_VariantRole, "default"},
            {UIShell::USDef::PR_DemoAudioPathRole, "C:/packages/demo.wav"},
            {UIShell::USDef::PR_NameRole, "Alpha"},
            {UIShell::USDef::PR_PathRole, "C:/packages/singer-alpha/singers/alpha.json"},
            {UIShell::USDef::PR_AvatarPathRole, QString::fromLatin1(resourcePrefix) + "avatar_alpha.png"},
            {UIShell::USDef::PR_BackgroundPathRole, QString::fromLatin1(resourcePrefix) + "background_alpha.png"},
        }));
        addSinger(alphaPackage->child(UIShell::USDef::PI_Singers), createItem({
            {UIShell::USDef::PR_IdRole, "alpha-echo"},
            {UIShell::USDef::PR_InterfaceRole, "diffsinger"},
            {UIShell::USDef::PR_LevelRole, QLocale().toString(1)},
            {UIShell::USDef::PR_VariantRole, "default"},
            {UIShell::USDef::PR_DemoAudioPathRole, "C:/packages/demo.wav"},
            {UIShell::USDef::PR_NameRole, "Alpha Echo"},
            {UIShell::USDef::PR_PathRole, "C:/packages/singer-alpha/singers/alpha-echo.json"},
            {UIShell::USDef::PR_AvatarPathRole, QString::fromLatin1(resourcePrefix) + "avatar_alpha_echo.png"},
            {UIShell::USDef::PR_BackgroundPathRole, QString::fromLatin1(resourcePrefix) + "background_alpha_echo.png"},
        }));
        addSinger(alphaPackage->child(UIShell::USDef::PI_Singers), createItem({
            {UIShell::USDef::PR_IdRole, "unavailable-metadata"},
            {UIShell::USDef::PR_PathRole, "C:/packages/singer-alpha/singers/unavailable-metadata.json"},
            {UIShell::USDef::PR_MetadataErrorRole, "Required singer metadata fields are unavailable: interface, level, variant."},
        }));

        auto betaPackage = createItem({
            {UIShell::USDef::PR_IdRole, "diffscope/singer-beta"},
            {UIShell::USDef::PR_VersionRole, "2.1.0.0"},
            {UIShell::USDef::PR_PathRole, "C:/packages/singer-beta"},
            {UIShell::USDef::PR_InstallationTimeRole, QDateTime::fromString(QStringLiteral("2026-05-04T08:45:00+08:00"), Qt::ISODate).toSecsSinceEpoch()},
            {UIShell::USDef::PR_NameRole, "Singer Beta Pack"},
            {UIShell::USDef::PR_DescriptionRole, "Sample singer package using base and advanced inference packs."},
            {UIShell::USDef::PR_VendorRole, "DiffScope Samples"},
            {UIShell::USDef::PR_ReadmePathRole, "C:/packages/singer-beta/README.md"},
            {UIShell::USDef::PR_LicensePathRole, "C:/packages/singer-beta/LICENSE.txt"},
            {UIShell::USDef::PR_UrlRole, "https://example.com/packages/singer-beta"},
        });
        addPackageSubtrees(betaPackage);
        betaPackage->child(UIShell::USDef::PI_Dependencies)->appendRow(createItem({
            {UIShell::USDef::PR_IdRole, "diffscope/base-inference"},
            {UIShell::USDef::PR_VersionRole, "1.0.0.0"},
        }));
        betaPackage->child(UIShell::USDef::PI_Dependencies)->appendRow(createItem({
            {UIShell::USDef::PR_IdRole, "diffscope/advanced-inference"},
            {UIShell::USDef::PR_VersionRole, "1.2.0.0"},
        }));
        addSinger(betaPackage->child(UIShell::USDef::PI_Singers), createItem({
            {UIShell::USDef::PR_IdRole, "beta"},
            {UIShell::USDef::PR_InterfaceRole, "diffsinger"},
            {UIShell::USDef::PR_LevelRole, QLocale().toString(1)},
            {UIShell::USDef::PR_VariantRole, "default"},
            {UIShell::USDef::PR_DemoAudioPathRole, "C:/packages/demo.wav"},
            {UIShell::USDef::PR_NameRole, "Beta"},
            {UIShell::USDef::PR_PathRole, "C:/packages/singer-beta/singers/beta.json"},
        }));
        addSinger(betaPackage->child(UIShell::USDef::PI_Singers), createItem({
            {UIShell::USDef::PR_IdRole, "beta-harmonic"},
            {UIShell::USDef::PR_InterfaceRole, "diffsinger"},
            {UIShell::USDef::PR_LevelRole, QLocale().toString(1)},
            {UIShell::USDef::PR_VariantRole, "default"},
            {UIShell::USDef::PR_DemoAudioPathRole, "C:/packages/demo.wav"},
            {UIShell::USDef::PR_NameRole, "Beta Harmonic"},
            {UIShell::USDef::PR_PathRole, "C:/packages/singer-beta/singers/beta-harmonic.json"},
            {UIShell::USDef::PR_AvatarPathRole, QString::fromLatin1(resourcePrefix) + "avatar_beta_harmonic.png"},
            {UIShell::USDef::PR_BackgroundPathRole, QString::fromLatin1(resourcePrefix) + "background_beta_harmonic.png"},
        }));

        model->appendRow(basePackage);
        model->appendRow(runtimePackage);
        model->appendRow(advancedPackage);
        model->appendRow(alphaPackage);
        model->appendRow(betaPackage);
    }

}

int main(int argc, char *argv[]) {
    QGuiApplication a(argc, argv);

    auto sf = QSurfaceFormat::defaultFormat();
    sf.setSamples(8);
    QSurfaceFormat::setDefaultFormat(sf);

    QQuickStyle::setStyle("SVSCraft.UIComponents");
    QQuickStyle::setFallbackStyle("Basic");

    QStandardItemModel packageModel(&a);
    populatePackageModel(&packageModel);

    QQmlApplicationEngine engine;
    SVS::FluentSystemIconsImageProvider::addToEngine(&engine);
    engine.setInitialProperties({
        {"model", QVariant::fromValue(&packageModel)}
    });
    engine.load(":/qt/qml/DiffScope/UIShell/Test/PackageManagerView/main.qml");

    return a.exec();
}
