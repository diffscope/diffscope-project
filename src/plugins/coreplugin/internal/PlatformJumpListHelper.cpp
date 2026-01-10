#include "PlatformJumpListHelper.h"

#include <QApplication>

#ifdef Q_OS_WIN
#    include <ShObjIdl.h>
#    include <ShlObj.h>
#    include <atlbase.h>
#    include <propkey.h>
#    include <propvarutil.h>
#elif defined(Q_OS_MACOS)
// TODO: Add macOS jump list (dock menu) implementation when available
#endif

namespace Core::Internal {

    void PlatformJumpListHelper::initializePlatformJumpList() {
#ifdef Q_OS_WIN
        CoInitialize(nullptr);

        CComPtr<ICustomDestinationList> pcdl;
        HRESULT hr = pcdl.CoCreateInstance(CLSID_DestinationList, nullptr, CLSCTX_INPROC_SERVER);
        if (FAILED(hr)) {
            CoUninitialize();
            return;
        }

        UINT cMinSlots;
        CComPtr<IObjectArray> poaRemoved;
        hr = pcdl->BeginList(&cMinSlots, IID_PPV_ARGS(&poaRemoved));
        if (FAILED(hr)) {
            CoUninitialize();
            return;
        }

        CComPtr<IObjectCollection> poc;
        hr = poc.CoCreateInstance(CLSID_EnumerableObjectCollection, nullptr, CLSCTX_INPROC_SERVER);
        if (FAILED(hr)) {
            CoUninitialize();
            return;
        }

        CComPtr<IShellLink> psl;
        hr = psl.CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER);
        if (FAILED(hr)) {
            CoUninitialize();
            return;
        }

        auto appPath = QApplication::applicationFilePath().toStdWString();
        psl->SetPath(appPath.c_str());
        psl->SetArguments(L"--new");

        CComPtr<IPropertyStore> pps;
        hr = psl->QueryInterface(IID_PPV_ARGS(&pps));
        if (SUCCEEDED(hr)) {
            PROPVARIANT propvar;
            InitPropVariantFromString(L"New Project", &propvar);
            pps->SetValue(PKEY_Title, propvar);
            PropVariantClear(&propvar);
            pps->Commit();
        }

        poc->AddObject(psl);

        CComPtr<IObjectArray> poa;
        hr = poc->QueryInterface(IID_PPV_ARGS(&poa));
        if (SUCCEEDED(hr)) {
            pcdl->AddUserTasks(poa);
        }

        pcdl->CommitList();
        CoUninitialize();
#elif defined(Q_OS_MACOS)
        // TODO: Add macOS jump list (dock menu) implementation when available
#endif
    }

}
