#include "PackageListProxyModel_p.h"

#include <uishell/USDef.h>

namespace UIShell {

    PackageListProxyModel::PackageListProxyModel(QObject *parent)
        : QIdentityProxyModel(parent) {
    }

    PackageListProxyModel::~PackageListProxyModel() = default;

    QHash<int, QByteArray> PackageListProxyModel::roleNames() const {
        static const QHash<int, QByteArray> m{
            {USDef::PR_IdRole, "id"},
            {USDef::PR_VersionRole, "version"},
            {USDef::PR_PathRole, "path"},
            {USDef::PR_InstallationTimeRole, "installationTime"},
            {USDef::PR_NameRole, "name"},
            {USDef::PR_DescriptionRole, "description"},
            {USDef::PR_VendorRole, "vendor"},
            {USDef::PR_ReadmePathRole, "readmePath"},
            {USDef::PR_LicensePathRole, "licensePath"},
            {USDef::PR_UrlRole, "url"},
            {USDef::PR_ClassNameRole, "className"},
            {USDef::PR_AvatarPathRole, "avatarPath"},
            {USDef::PR_BackgroundPathRole, "backgroundPath"},
            {USDef::PR_ImportInferenceIdRole, "importInferenceId"},
        };
        return m;
    }

    QModelIndex PackageListProxyModel::packageModelIndex(int index) const {
        return this->index(index, 0);
    }

    QModelIndex PackageListProxyModel::entryIndex(const QModelIndex &index) const {
        const auto singerRootIndex = this->singerRootIndexForIndex(index);
        if (!singerRootIndex.isValid() || rowCount(singerRootIndex) == 0) {
            const auto inferenceRootIndex = this->inferenceRootIndexForIndex(index);
            if (!inferenceRootIndex.isValid() || rowCount(inferenceRootIndex) == 0) {
                return singerRootIndex;
            }
            return inferenceRootIndex;
        }
        return this->index(0, 0, singerRootIndex);
    }

    QModelIndex PackageListProxyModel::singerModelIndexForIndex(const QModelIndex &index) const {
        if (isSingerIndex(index)) {
            return index;
        }

        const auto singerRootIndex = this->singerRootIndexForIndex(index);
        if (!singerRootIndex.isValid() || rowCount(singerRootIndex) == 0) {
            return singerRootIndex;
        }

        return this->index(0, 0, singerRootIndex);
    }

    QModelIndex PackageListProxyModel::packageModelIndexForIndex(const QModelIndex &index) const {
        if (!index.isValid() || index.model() != this) {
            return {};
        }

        auto packageIndex = index;
        while (packageIndex.parent().isValid()) {
            packageIndex = packageIndex.parent();
        }

        return packageIndex;
    }

    QModelIndex PackageListProxyModel::dependencyRootIndexForIndex(const QModelIndex &index) const {
        const auto packageIndex = packageModelIndexForIndex(index);
        return packageIndex.isValid() ? this->index(USDef::PI_Dependencies, 0, packageIndex) : QModelIndex();
    }

    QModelIndex PackageListProxyModel::singerRootIndexForIndex(const QModelIndex &index) const {
        const auto packageIndex = packageModelIndexForIndex(index);
        return packageIndex.isValid() ? this->index(USDef::PI_Singers, 0, packageIndex) : QModelIndex();
    }

    QModelIndex PackageListProxyModel::inferenceRootIndexForIndex(const QModelIndex &index) const {
        const auto packageIndex = packageModelIndexForIndex(index);
        return packageIndex.isValid() ? this->index(USDef::PI_Inferences, 0, packageIndex) : QModelIndex();
    }

    QModelIndex PackageListProxyModel::importRootIndexForSingerIndex(const QModelIndex &index) const {
        return isSingerIndex(index) ? this->index(USDef::PI_SingerImports, 0, index) : QModelIndex();
    }

    QModelIndex PackageListProxyModel::demoAudioRootIndexForSingerIndex(const QModelIndex &index) const {
        return isSingerIndex(index) ? this->index(USDef::PI_SingerDemoAudioList, 0, index) : QModelIndex();
    }

    bool PackageListProxyModel::isSingerOrSingerRootIndex(const QModelIndex &index) const {
        if (!index.isValid() || index.model() != this) {
            return false;
        }

        if (isSingerIndex(index)) {
            return true;
        }

        return index.row() == USDef::PI_Singers &&
               index.parent().isValid() &&
               !index.parent().parent().isValid();
    }

    bool PackageListProxyModel::isInferenceOrInferenceRootIndex(const QModelIndex &index) const {
        if (!index.isValid() || index.model() != this) {
            return false;
        }

        if (isInferenceIndex(index)) {
            return true;
        }

        return index.row() == USDef::PI_Inferences &&
               index.parent().isValid() &&
               !index.parent().parent().isValid();
    }

    bool PackageListProxyModel::isPackageIndex(const QModelIndex &index) const {
        return index.isValid() &&
               index.model() == this &&
               !index.parent().isValid();
    }

    bool PackageListProxyModel::isDependencyRootIndex(const QModelIndex &index) const {
        return index.isValid() &&
               index.model() == this &&
               index.row() == USDef::PI_Dependencies &&
               index.parent().isValid() &&
               !index.parent().parent().isValid();
    }

    int PackageListProxyModel::findPackageIndex(const QString &id, const QString &version) const {
        for (int i = 0; i < rowCount(); ++i) {
            const auto packageIndex = index(i, 0);
            if (data(packageIndex, USDef::PR_IdRole).toString() == id &&
                data(packageIndex, USDef::PR_VersionRole).toString() == version) {
                return i;
            }
        }
        return -1;
    }

    QModelIndex PackageListProxyModel::findInferenceIndex(const QString &id, const QString &version, const QString &inferenceId) const {
        const int packageRow = findPackageIndex(id, version);
        if (packageRow < 0) {
            return {};
        }

        const auto packageIndex = index(packageRow, 0);
        const auto inferencesIndex = index(USDef::PI_Inferences, 0, packageIndex);
        if (!inferencesIndex.isValid()) {
            return {};
        }

        for (int i = 0; i < rowCount(inferencesIndex); ++i) {
            const auto inferenceIndex = index(i, 0, inferencesIndex);
            if (data(inferenceIndex, USDef::PR_IdRole).toString() == inferenceId) {
                return inferenceIndex;
            }
        }

        return {};
    }

    int PackageListProxyModel::packageIndexForIndex(const QModelIndex &index) const {
        if (!index.isValid() || index.model() != this) {
            return -1;
        }

        auto packageIndex = index;
        while (packageIndex.parent().isValid()) {
            packageIndex = packageIndex.parent();
        }

        return packageIndex.row();
    }

    bool PackageListProxyModel::isSingerIndex(const QModelIndex &index) const {
        if (!index.isValid() || index.model() != this) {
            return false;
        }

        const auto parentIndex = index.parent();
        return parentIndex.isValid() &&
               parentIndex.row() == USDef::PI_Singers &&
               parentIndex.parent().isValid() &&
               !parentIndex.parent().parent().isValid();
    }

    bool PackageListProxyModel::isInferenceIndex(const QModelIndex &index) const {
        if (!index.isValid() || index.model() != this) {
            return false;
        }

        const auto parentIndex = index.parent();
        return parentIndex.isValid() &&
               parentIndex.row() == USDef::PI_Inferences &&
               parentIndex.parent().isValid() &&
               !parentIndex.parent().parent().isValid();
    }

}
