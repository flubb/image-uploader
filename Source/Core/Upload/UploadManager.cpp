#include "UploadManager.h"
#include "Func/Common.h"
#include "Core/HistoryManager.h"
#include "Core/ServiceLocator.h"
#include "Core/LocalFileCache.h"
#include "Core/Upload/FileUploadTask.h"
#include "Core/Upload/Filters/UserFilter.h"
#include "Core/Scripting/ScriptsManager.h"
#include "Core/Settings.h"
#include "UploadEngineManager.h"

UploadManager::UploadManager(UploadEngineManager* uploadEngineManager, ScriptsManager* scriptsManager, IUploadErrorHandler* uploadErrorHandler) :
                CFileQueueUploader(uploadEngineManager, scriptsManager, uploadErrorHandler),
                                                    userFilter(scriptsManager)
{
    uploadEngineManager_ = uploadEngineManager;
#ifdef IU_WTL
    addUploadFilter(&imageConverterFilter);
#endif
    addUploadFilter(&userFilter);
    addUploadFilter(&urlShorteningFilter);
    setMaxThreadCount(Settings.MaxThreads);
    Settings.addChangeCallback(BasicSettings::ChangeCallback(this, &UploadManager::settingsChanged));
    OnConfigureNetworkClient.bind(this, &UploadManager::configureNetwork);
}

bool UploadManager::shortenLinksInSession(std::shared_ptr<UploadSession> session)
{
    int taskCount = session->taskCount();

    bool res = false;
    for (int i = 0; i < taskCount; i++)
    {
        std::shared_ptr<UploadTask> task = session->getTask(i);
        if (task->shorteningStarted())
        {
            continue;
        }
        task->serverProfile().setShortenLinks(true);
        res = urlShorteningFilter.PostUpload(task.get()) || res;
    }
    return res;
}

void UploadManager::configureNetwork(CFileQueueUploader* uploader, NetworkClient* networkClient)
{
    IU_ConfigureProxy(*networkClient);
}

void UploadManager::sessionAdded(UploadSession* session)
{
    CFileQueueUploader::sessionAdded(session);
    session->addSessionFinishedCallback(UploadSession::SessionFinishedCallback(this, &UploadManager::onSessionFinished));
}

void UploadManager::onSessionFinished(UploadSession* uploadSession)
{
    uploadEngineManager_->resetFailedAuthorization();
}

void UploadManager::onTaskFinished(UploadTask* task, bool ok)
{
    UploadSession* uploadSession = task->session();
    CHistoryManager * mgr = ServiceLocator::instance()->historyManager();
    std::lock_guard<std::mutex> lock(uploadSession->historySessionMutex_);
    std::shared_ptr<CHistorySession> session = uploadSession->historySession_;
    if (!session) {
        session = mgr->newSession();
        uploadSession->historySession_ = session;
    }

    FileUploadTask* fileTask = dynamic_cast<FileUploadTask*>(task);
    if (!fileTask)
    {
        return ;
    }

    TempFileDeleter* tempFileDeleter = fileTask->tempFileDeleter(false);
    if (tempFileDeleter)
    {
        tempFileDeleter->cleanup();
    }
    if (!fileTask->uploadSuccess() || fileTask->status() != UploadTask::StatusFinished)
    {
        return ;
    }
    HistoryItem hi;
    hi.localFilePath = fileTask->originalFileName();
    hi.serverName = fileTask->serverProfile().serverName();
    UploadResult* uploadResult = fileTask->uploadResult();
    hi.directUrl = uploadResult->directUrl;
    hi.directUrlShortened = uploadResult->directUrlShortened;
    hi.thumbUrl = uploadResult->thumbUrl;
    hi.viewUrl = uploadResult->downloadUrl;
    hi.viewUrlShortened = uploadResult->downloadUrlShortened;
    hi.editUrl = uploadResult->editUrl;
    hi.deleteUrl = uploadResult->deleteUrl;
    hi.displayName = fileTask->getDisplayName();

    hi.uploadFileSize = fileTask->getDataLength();
    if (!hi.directUrl.empty())
    {
        LocalFileCache::instance()->addFile(hi.directUrl, hi.localFilePath);
    }
    session->AddItem(hi);
}

void UploadManager::taskAdded(UploadTask* task)
{
    FileUploadTask* fileTask = dynamic_cast<FileUploadTask*>(task);
    if (!fileTask)
    {
        return;
    }

    if (task->parentTask())
    {
        return ;
    }
    task->addTaskFinishedCallback(UploadTask::TaskFinishedCallback(this, &UploadManager::onTaskFinished));
}

void UploadManager::settingsChanged(BasicSettings* settings)
{
    setMaxThreadCount(Settings.MaxThreads);
}