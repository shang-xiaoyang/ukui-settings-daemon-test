#include "a11y-keyboard-plugin.h"
#include "clib-syslog.h"

PluginInterface *A11yKeyboardPlugin::mInstance = nullptr;
A11yKeyboardManager *A11yKeyboardPlugin::UsdA11yManager= nullptr;

A11yKeyboardPlugin::A11yKeyboardPlugin()
{
    CT_SYSLOG(LOG_DEBUG,"A11yKeyboardPlugin initializing ");
    if(nullptr == UsdA11yManager)
        UsdA11yManager = A11yKeyboardManager::A11KeyboardManagerNew();

}
A11yKeyboardPlugin::~A11yKeyboardPlugin()
{
    if(UsdA11yManager){
        delete UsdA11yManager;
        UsdA11yManager = nullptr;
    }
}
void A11yKeyboardPlugin::activate()
{
    bool res;
    CT_SYSLOG(LOG_DEBUG,"ctivating A11y-Keyboard Plugin");
    res = UsdA11yManager->A11yKeyboardManagerStart();
    if(!res)
        CT_SYSLOG(LOG_ERR,"Unable to start A11y-Keyboard manager");
}

PluginInterface * A11yKeyboardPlugin::getInstance()
{
    if(nullptr == mInstance)
        mInstance = new A11yKeyboardPlugin();

    return mInstance;
}

void A11yKeyboardPlugin::deactivate()
{
    CT_SYSLOG(LOG_DEBUG,"Deactivating A11y-Keyboard plugin");
    UsdA11yManager->A11yKeyboardManagerStop();
}

PluginInterface *createSettingsPlugin()
{
    return A11yKeyboardPlugin::getInstance();
}
