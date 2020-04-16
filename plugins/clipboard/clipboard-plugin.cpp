#include "clipboard-plugin.h"
#include "clib-syslog.h"

ClipboardManager* ClipboardPlugin::mManager = nullptr;
PluginInterface* ClipboardPlugin::mInstance = nullptr;

ClipboardPlugin::~ClipboardPlugin()
{
    delete mManager;
    mManager = nullptr;
}

PluginInterface *ClipboardPlugin::getInstance()
{
    if (nullptr == mInstance) {
        mInstance = new ClipboardPlugin();
    }
    return mInstance;
}

void ClipboardPlugin::activate()
{
    if (nullptr != mManager) mManager->managerStart();
}

void ClipboardPlugin::deactivate()
{
    if (nullptr != mManager) mManager->managerStop();
    if (nullptr != mInstance) {
        delete mInstance;
        mInstance = nullptr;
    }
}

ClipboardPlugin::ClipboardPlugin()
{
    if ((nullptr == mManager)) {
        mManager = new ClipboardManager();
    }
}

PluginInterface* createSettingsPlugin()
{
    return ClipboardPlugin::getInstance();
}

