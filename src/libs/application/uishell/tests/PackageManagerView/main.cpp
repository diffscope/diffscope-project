#include <QQmlApplicationEngine>
#include <QGuiApplication>
#include <QStandardItemModel>
#include <QSurfaceFormat>
#include <QQuickStyle>
#include <QVariant>

#include <SVSCraftFluentSystemIcons/FluentSystemIconsImageProvider.h>

#include <uishell/USDef.h>

#include <utility>

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
        packageItem->setChild(UIShell::USDef::PI_Inferences, createSubtree("Inferences"));
        packageItem->setChild(UIShell::USDef::PI_Singers, createSubtree("Singers"));
    }

    QStandardItem *addSinger(QStandardItem *singers, QStandardItem *singer) {
        singer->setChild(UIShell::USDef::PI_SingerImports, createSubtree("Imports"));
        singer->setChild(UIShell::USDef::PI_SingerDemoAudioList, createSubtree("Demo Audio List"));
        singers->appendRow(singer);
        return singer;
    }

    void populatePackageModel(QStandardItemModel *model) {

        constexpr auto resourcePrefix = "qrc:/qt/qml/DiffScope/UIShell/Test/PackageManagerView/";

        auto basePackage = createItem({
            {UIShell::USDef::PR_IdRole, "org.diffscope.base-inference"},
            {UIShell::USDef::PR_VersionRole, "1.0.0"},
            {UIShell::USDef::PR_PathRole, "C:/packages/base-inference"},
            {UIShell::USDef::PR_InstallationTimeRole, "2026-05-01T10:00:00+08:00"},
            {UIShell::USDef::PR_NameRole, "Base Inference Pack"},
            {UIShell::USDef::PR_DescriptionRole, "Shared inference engines for sample singers."},
            {UIShell::USDef::PR_VendorRole, "DiffScope Samples"},
            {UIShell::USDef::PR_UrlRole, "https://example.com/packages/base-inference"},
        });
        addPackageSubtrees(basePackage);
        basePackage->child(UIShell::USDef::PI_Inferences)->appendRow(createItem({
            {UIShell::USDef::PR_IdRole, "inf.base.vocoder"},
            {UIShell::USDef::PR_ClassNameRole, "BaseVocoderInference"},
            {UIShell::USDef::PR_NameRole, "Base Vocoder"},
            {UIShell::USDef::PR_PathRole, "C:/packages/base-inference/inferences/base-vocoder.json"},
        }));
        basePackage->child(UIShell::USDef::PI_Inferences)->appendRow(createItem({
            {UIShell::USDef::PR_IdRole, "inf.base.diffusion"},
            {UIShell::USDef::PR_ClassNameRole, "BaseDiffusionInference"},
            {UIShell::USDef::PR_PathRole, "C:/packages/base-inference/inferences/base-diffusion.json"},
        }));

        auto runtimePackage = createItem({
            {UIShell::USDef::PR_IdRole, "org.diffscope.runtime-inference"},
            {UIShell::USDef::PR_VersionRole, "1.1.0"},
            {UIShell::USDef::PR_PathRole, "C:/packages/runtime-inference"},
            {UIShell::USDef::PR_InstallationTimeRole, "2026-05-01T14:15:00+08:00"},
            {UIShell::USDef::PR_NameRole, "Runtime Inference Pack"},
            {UIShell::USDef::PR_DescriptionRole, "Runtime inference engines used by singer packages."},
            {UIShell::USDef::PR_VendorRole, "DiffScope Samples"},
            {UIShell::USDef::PR_ReadmePathRole, "C:/packages/runtime-inference/README.md"},
            {UIShell::USDef::PR_LicensePathRole, "C:/packages/runtime-inference/LICENSE.txt"},
        });
        addPackageSubtrees(runtimePackage);
        runtimePackage->child(UIShell::USDef::PI_Inferences)->appendRow(createItem({
            {UIShell::USDef::PR_IdRole, "inf.runtime.realtime"},
            {UIShell::USDef::PR_ClassNameRole, "RuntimeRealtimeInference"},
            {UIShell::USDef::PR_NameRole, "Runtime Realtime"},
            {UIShell::USDef::PR_PathRole, "C:/packages/runtime-inference/inferences/runtime-realtime.json"},
        }));
        runtimePackage->child(UIShell::USDef::PI_Inferences)->appendRow(createItem({
            {UIShell::USDef::PR_IdRole, "inf.runtime.batch"},
            {UIShell::USDef::PR_ClassNameRole, "RuntimeBatchInference"},
            {UIShell::USDef::PR_PathRole, "C:/packages/runtime-inference/inferences/runtime-batch.json"},
        }));

        auto advancedPackage = createItem({
            {UIShell::USDef::PR_IdRole, "org.diffscope.advanced-inference"},
            {UIShell::USDef::PR_VersionRole, "1.2.0"},
            {UIShell::USDef::PR_PathRole, "C:/packages/advanced-inference"},
            {UIShell::USDef::PR_InstallationTimeRole, "2026-05-02T11:30:00+08:00"},
            {UIShell::USDef::PR_NameRole, "Advanced Inference Pack"},
            {UIShell::USDef::PR_DescriptionRole, "Additional inference backend built on the base pack."},
            {UIShell::USDef::PR_VendorRole, "DiffScope Samples"},
            {UIShell::USDef::PR_ReadmePathRole, "C:/packages/advanced-inference/README.md"},
            {UIShell::USDef::PR_LicensePathRole, "C:/packages/advanced-inference/LICENSE.txt"},
            {UIShell::USDef::PR_UrlRole, "https://example.com/packages/advanced-inference"},
        });
        addPackageSubtrees(advancedPackage);
        advancedPackage->child(UIShell::USDef::PI_Dependencies)->appendRow(createItem({
            {UIShell::USDef::PR_IdRole, "org.diffscope.base-inference"},
            {UIShell::USDef::PR_VersionRole, "1.0.0"},
            {UIShell::USDef::PR_NameRole, "Base Inference Pack"},
        }));
        advancedPackage->child(UIShell::USDef::PI_Dependencies)->appendRow(createItem({
            {UIShell::USDef::PR_IdRole, "org.diffscope.runtime-inference"},
            {UIShell::USDef::PR_VersionRole, "1.1.0"},
            {UIShell::USDef::PR_NameRole, "Runtime Inference Pack"},
        }));
        advancedPackage->child(UIShell::USDef::PI_Inferences)->appendRow(createItem({
            {UIShell::USDef::PR_IdRole, "inf.advanced.voice"},
            {UIShell::USDef::PR_ClassNameRole, "AdvancedVoiceInference"},
            {UIShell::USDef::PR_NameRole, "Advanced Voice"},
            {UIShell::USDef::PR_PathRole, "C:/packages/advanced-inference/inferences/advanced-voice.json"},
        }));
        advancedPackage->child(UIShell::USDef::PI_Inferences)->appendRow(createItem({
            {UIShell::USDef::PR_IdRole, "inf.advanced.expression"},
            {UIShell::USDef::PR_ClassNameRole, "AdvancedExpressionInference"},
            {UIShell::USDef::PR_PathRole, "C:/packages/advanced-inference/inferences/advanced-expression.json"},
        }));

        auto alphaPackage = createItem({
            {UIShell::USDef::PR_IdRole, "org.diffscope.singer-alpha"},
            {UIShell::USDef::PR_VersionRole, "2.0.0"},
            {UIShell::USDef::PR_PathRole, "C:/packages/singer-alpha"},
            {UIShell::USDef::PR_InstallationTimeRole, "2026-05-03T09:20:00+08:00"},
            {UIShell::USDef::PR_NameRole, "Singer Alpha Pack"},
            {UIShell::USDef::PR_DescriptionRole, "Sample singer package using the base vocoder."},
            {UIShell::USDef::PR_VendorRole, "DiffScope Samples"},
            {UIShell::USDef::PR_ReadmePathRole, "C:/packages/singer-alpha/README.md"},
            {UIShell::USDef::PR_LicensePathRole, "C:/packages/singer-alpha/LICENSE.txt"},
            {UIShell::USDef::PR_UrlRole, "https://example.com/packages/singer-alpha"},
        });
        addPackageSubtrees(alphaPackage);
        alphaPackage->child(UIShell::USDef::PI_Dependencies)->appendRow(createItem({
            {UIShell::USDef::PR_IdRole, "org.diffscope.base-inference"},
            {UIShell::USDef::PR_VersionRole, "1.0.0"},
            {UIShell::USDef::PR_NameRole, "Base Inference Pack"},
        }));
        alphaPackage->child(UIShell::USDef::PI_Dependencies)->appendRow(createItem({
            {UIShell::USDef::PR_IdRole, "org.diffscope.runtime-inference"},
            {UIShell::USDef::PR_VersionRole, "1.1.0"},
            {UIShell::USDef::PR_NameRole, "Runtime Inference Pack"},
        }));
        auto alphaSinger = addSinger(alphaPackage->child(UIShell::USDef::PI_Singers), createItem({
            {UIShell::USDef::PR_IdRole, "singer.alpha"},
            {UIShell::USDef::PR_ClassNameRole, "AlphaSinger"},
            {UIShell::USDef::PR_NameRole, "Alpha"},
            {UIShell::USDef::PR_PathRole, "C:/packages/singer-alpha/singers/alpha.json"},
            {UIShell::USDef::PR_AvatarPathRole, QString::fromLatin1(resourcePrefix) + "avatar_alpha.png"},
            {UIShell::USDef::PR_BackgroundPathRole, QString::fromLatin1(resourcePrefix) + "background_alpha.png"},
        }));
        alphaSinger->child(UIShell::USDef::PI_SingerImports)->appendRow(createItem({
            {UIShell::USDef::PR_IdRole, "org.diffscope.base-inference"},
            {UIShell::USDef::PR_VersionRole, "1.0.0"},
            {UIShell::USDef::PR_NameRole, "Base Vocoder"},
            {UIShell::USDef::PR_ImportInferenceIdRole, "inf.base.vocoder"},
        }));
        alphaSinger->child(UIShell::USDef::PI_SingerImports)->appendRow(createItem({
            {UIShell::USDef::PR_IdRole, "org.diffscope.runtime-inference"},
            {UIShell::USDef::PR_VersionRole, "1.1.0"},
            {UIShell::USDef::PR_NameRole, "Runtime Realtime"},
            {UIShell::USDef::PR_ImportInferenceIdRole, "inf.runtime.realtime"},
        }));
        alphaSinger->child(UIShell::USDef::PI_SingerDemoAudioList)->appendRow(createItem({
            {UIShell::USDef::PR_NameRole, "Alpha Demo"},
            {UIShell::USDef::PR_PathRole, "C:/packages/singer-alpha/demo/alpha.wav"},
        }));
        alphaSinger->child(UIShell::USDef::PI_SingerDemoAudioList)->appendRow(createItem({
            {UIShell::USDef::PR_NameRole, "Alpha Soft Demo"},
            {UIShell::USDef::PR_PathRole, "C:/packages/singer-alpha/demo/alpha-soft.wav"},
        }));
        auto alphaEchoSinger = addSinger(alphaPackage->child(UIShell::USDef::PI_Singers), createItem({
            {UIShell::USDef::PR_IdRole, "singer.alpha-echo"},
            {UIShell::USDef::PR_ClassNameRole, "AlphaEchoSinger"},
            {UIShell::USDef::PR_NameRole, "Alpha Echo"},
            {UIShell::USDef::PR_PathRole, "C:/packages/singer-alpha/singers/alpha-echo.json"},
            {UIShell::USDef::PR_AvatarPathRole, QString::fromLatin1(resourcePrefix) + "avatar_alpha_echo.png"},
            {UIShell::USDef::PR_BackgroundPathRole, QString::fromLatin1(resourcePrefix) + "background_alpha_echo.png"},
        }));
        alphaEchoSinger->child(UIShell::USDef::PI_SingerImports)->appendRow(createItem({
            {UIShell::USDef::PR_IdRole, "org.diffscope.base-inference"},
            {UIShell::USDef::PR_VersionRole, "1.0.0"},
            {UIShell::USDef::PR_NameRole, "Alpha Echo Diffusion"},
            {UIShell::USDef::PR_ImportInferenceIdRole, "inf.base.diffusion"},
        }));
        alphaEchoSinger->child(UIShell::USDef::PI_SingerImports)->appendRow(createItem({
            {UIShell::USDef::PR_IdRole, "org.diffscope.runtime-inference"},
            {UIShell::USDef::PR_VersionRole, "1.1.0"},
            {UIShell::USDef::PR_NameRole, "Alpha Echo Batch"},
            {UIShell::USDef::PR_ImportInferenceIdRole, "inf.runtime.batch"},
        }));
        alphaEchoSinger->child(UIShell::USDef::PI_SingerDemoAudioList)->appendRow(createItem({
            {UIShell::USDef::PR_NameRole, "Alpha Echo Demo"},
            {UIShell::USDef::PR_PathRole, "C:/packages/singer-alpha/demo/alpha-echo.wav"},
        }));
        alphaEchoSinger->child(UIShell::USDef::PI_SingerDemoAudioList)->appendRow(createItem({
            {UIShell::USDef::PR_NameRole, "Alpha Echo Bright Demo"},
            {UIShell::USDef::PR_PathRole, "C:/packages/singer-alpha/demo/alpha-echo-bright.wav"},
        }));

        auto betaPackage = createItem({
            {UIShell::USDef::PR_IdRole, "org.diffscope.singer-beta"},
            {UIShell::USDef::PR_VersionRole, "2.1.0"},
            {UIShell::USDef::PR_PathRole, "C:/packages/singer-beta"},
            {UIShell::USDef::PR_InstallationTimeRole, "2026-05-04T08:45:00+08:00"},
            {UIShell::USDef::PR_NameRole, "Singer Beta Pack"},
            {UIShell::USDef::PR_DescriptionRole, "Sample singer package using base and advanced inference packs."},
            {UIShell::USDef::PR_VendorRole, "DiffScope Samples"},
            {UIShell::USDef::PR_ReadmePathRole, "C:/packages/singer-beta/README.md"},
            {UIShell::USDef::PR_LicensePathRole, "C:/packages/singer-beta/LICENSE.txt"},
            {UIShell::USDef::PR_UrlRole, "https://example.com/packages/singer-beta"},
        });
        addPackageSubtrees(betaPackage);
        betaPackage->child(UIShell::USDef::PI_Dependencies)->appendRow(createItem({
            {UIShell::USDef::PR_IdRole, "org.diffscope.base-inference"},
            {UIShell::USDef::PR_VersionRole, "1.0.0"},
            {UIShell::USDef::PR_NameRole, "Base Inference Pack"},
        }));
        betaPackage->child(UIShell::USDef::PI_Dependencies)->appendRow(createItem({
            {UIShell::USDef::PR_IdRole, "org.diffscope.advanced-inference"},
            {UIShell::USDef::PR_VersionRole, "1.2.0"},
            {UIShell::USDef::PR_NameRole, "Advanced Inference Pack"},
        }));
        auto betaSinger = addSinger(betaPackage->child(UIShell::USDef::PI_Singers), createItem({
            {UIShell::USDef::PR_IdRole, "singer.beta"},
            {UIShell::USDef::PR_ClassNameRole, "BetaSinger"},
            {UIShell::USDef::PR_NameRole, "Beta"},
            {UIShell::USDef::PR_PathRole, "C:/packages/singer-beta/singers/beta.json"},
            // {UIShell::USDef::PR_AvatarPathRole, QString::fromLatin1(resourcePrefix) + "avatar_beta.png"},
            // {UIShell::USDef::PR_BackgroundPathRole, QString::fromLatin1(resourcePrefix) + "background_beta.png"},
        }));
        betaSinger->child(UIShell::USDef::PI_SingerImports)->appendRow(createItem({
            {UIShell::USDef::PR_IdRole, "org.diffscope.advanced-inference"},
            {UIShell::USDef::PR_VersionRole, "1.2.0"},
            {UIShell::USDef::PR_NameRole, "Advanced Voice"},
            {UIShell::USDef::PR_ImportInferenceIdRole, "inf.advanced.voice"},
        }));
        betaSinger->child(UIShell::USDef::PI_SingerImports)->appendRow(createItem({
            {UIShell::USDef::PR_IdRole, "org.diffscope.base-inference"},
            {UIShell::USDef::PR_VersionRole, "1.0.0"},
            {UIShell::USDef::PR_NameRole, "Beta Diffusion"},
            {UIShell::USDef::PR_ImportInferenceIdRole, "inf.base.diffusion"},
        }));
        betaSinger->child(UIShell::USDef::PI_SingerDemoAudioList)->appendRow(createItem({
            {UIShell::USDef::PR_NameRole, "Beta Demo"},
            {UIShell::USDef::PR_PathRole, "C:/packages/singer-beta/demo/beta.wav"},
        }));
        betaSinger->child(UIShell::USDef::PI_SingerDemoAudioList)->appendRow(createItem({
            {UIShell::USDef::PR_NameRole, "Beta Power Demo"},
            {UIShell::USDef::PR_PathRole, "C:/packages/singer-beta/demo/beta-power.wav"},
        }));
        auto betaHarmonicSinger = addSinger(betaPackage->child(UIShell::USDef::PI_Singers), createItem({
            {UIShell::USDef::PR_IdRole, "singer.beta-harmonic"},
            {UIShell::USDef::PR_ClassNameRole, "BetaHarmonicSinger"},
            {UIShell::USDef::PR_NameRole, "Beta Harmonic"},
            {UIShell::USDef::PR_PathRole, "C:/packages/singer-beta/singers/beta-harmonic.json"},
            {UIShell::USDef::PR_AvatarPathRole, QString::fromLatin1(resourcePrefix) + "avatar_beta_harmonic.png"},
            {UIShell::USDef::PR_BackgroundPathRole, QString::fromLatin1(resourcePrefix) + "background_beta_harmonic.png"},
        }));
        betaHarmonicSinger->child(UIShell::USDef::PI_SingerImports)->appendRow(createItem({
            {UIShell::USDef::PR_IdRole, "org.diffscope.advanced-inference"},
            {UIShell::USDef::PR_VersionRole, "1.2.0"},
            {UIShell::USDef::PR_NameRole, "Beta Harmonic Expression"},
            {UIShell::USDef::PR_ImportInferenceIdRole, "inf.advanced.expression"},
        }));
        betaHarmonicSinger->child(UIShell::USDef::PI_SingerImports)->appendRow(createItem({
            {UIShell::USDef::PR_IdRole, "org.diffscope.base-inference"},
            {UIShell::USDef::PR_VersionRole, "1.0.0"},
            {UIShell::USDef::PR_NameRole, "Base Vocoder"},
            {UIShell::USDef::PR_ImportInferenceIdRole, "inf.base.vocoder"},
        }));
        betaHarmonicSinger->child(UIShell::USDef::PI_SingerDemoAudioList)->appendRow(createItem({
            {UIShell::USDef::PR_NameRole, "Beta Harmonic Demo"},
            {UIShell::USDef::PR_PathRole, "C:/packages/singer-beta/demo/beta-harmonic.wav"},
        }));
        betaHarmonicSinger->child(UIShell::USDef::PI_SingerDemoAudioList)->appendRow(createItem({
            {UIShell::USDef::PR_NameRole, "Beta Harmonic Soft Demo"},
            {UIShell::USDef::PR_PathRole, "C:/packages/singer-beta/demo/beta-harmonic-soft.wav"},
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
