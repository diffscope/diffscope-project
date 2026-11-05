// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "ChildProcess.h"

#if defined(Q_OS_WIN)
#    include <cstddef>
#    include <memory>
#elif defined(Q_OS_LINUX)
#    include <signal.h>
#    include <sys/prctl.h>
#    include <unistd.h>
#    include <cerrno>
#endif

#include <QLoggingCategory>
#include <QProcess>
#include <QString>

#if defined(Q_OS_WIN)
#    include <qt_windows.h>
#endif

namespace Core {

#if defined(Q_OS_WIN)
    Q_STATIC_LOGGING_CATEGORY(lcChildProcess, "diffscope.core.childprocess")

    namespace {

        struct ChildProcessJob {
            HANDLE m_handle{};
            STARTUPINFOEXW m_startupInfo{};
            std::unique_ptr<std::byte[]> m_attributeStorage;

            ~ChildProcessJob() {
                if (m_startupInfo.lpAttributeList)
                    DeleteProcThreadAttributeList(m_startupInfo.lpAttributeList);
                if (m_handle)
                    CloseHandle(m_handle);
            }
        };

        bool reportWindowsError(const char *operation, QString *errorMessage) {
            const auto error = GetLastError();
            qCWarning(lcChildProcess) << operation << "failed with error" << error;
            if (errorMessage) {
                wchar_t buffer[1024]{};
                const auto length = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error, 0, buffer, sizeof(buffer) / sizeof(buffer[0]), nullptr);
                const auto message = length ? QString::fromWCharArray(buffer, length).trimmed() : QString::number(error);
                *errorMessage = QString::fromLatin1(operation) + QStringLiteral(": ") + message;
            }
            return false;
        }

    }
#endif

    bool configureChildProcess(QProcess *process, QString *errorMessage) {
        Q_ASSERT(process);
        Q_ASSERT(process->state() == QProcess::NotRunning);
        if (errorMessage)
            errorMessage->clear();
#if defined(Q_OS_WIN)
        auto job = std::make_shared<ChildProcessJob>();
        job->m_handle = CreateJobObjectW(nullptr, nullptr);
        if (!job->m_handle)
            return reportWindowsError("CreateJobObjectW", errorMessage);

        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
        limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if (!SetInformationJobObject(job->m_handle, JobObjectExtendedLimitInformation, &limits, sizeof(limits)))
            return reportWindowsError("SetInformationJobObject", errorMessage);

        SIZE_T attributeSize{};
        InitializeProcThreadAttributeList(nullptr, 1, 0, &attributeSize);
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            return reportWindowsError("InitializeProcThreadAttributeList", errorMessage);
        job->m_attributeStorage = std::make_unique<std::byte[]>(attributeSize);
        auto attributes = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(job->m_attributeStorage.get());
        if (!InitializeProcThreadAttributeList(attributes, 1, 0, &attributeSize))
            return reportWindowsError("InitializeProcThreadAttributeList", errorMessage);
        job->m_startupInfo.lpAttributeList = attributes;
        if (!UpdateProcThreadAttribute(attributes, 0, PROC_THREAD_ATTRIBUTE_JOB_LIST, &job->m_handle, sizeof(job->m_handle), nullptr, nullptr))
            return reportWindowsError("UpdateProcThreadAttribute", errorMessage);

        process->setCreateProcessArgumentsModifier([job](QProcess::CreateProcessArguments *arguments) {
            job->m_startupInfo.StartupInfo = *arguments->startupInfo;
            job->m_startupInfo.StartupInfo.cb = sizeof(STARTUPINFOEXW);
            arguments->startupInfo = &job->m_startupInfo.StartupInfo;
            arguments->flags |= EXTENDED_STARTUPINFO_PRESENT;
        });
#elif defined(Q_OS_LINUX)
        const auto parentPid = getpid();
        process->setChildProcessModifier([process, parentPid] {
            if (prctl(PR_SET_PDEATHSIG, static_cast<unsigned long>(SIGKILL), 0UL, 0UL, 0UL) == -1)
                process->failChildProcessModifier("prctl(PR_SET_PDEATHSIG)", errno);
            if (getppid() != parentPid)
                process->failChildProcessModifier("getppid", ESRCH);
        });
#else
        Q_UNUSED(process)
#endif
        return true;
    }

}
