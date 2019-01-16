#include "SeEvtCallback.h"
#include "EseUpdater.h"

void SeEvtCallback::evtCallback(__attribute__((unused)) SESTATUS evt) {
    EseUpdater::sendeSEUpdateState(ESE_JCOP_UPDATE_COMPLETED);
    EseUpdater::seteSEClientState(ESE_UPDATE_COMPLETED);
    EseUpdater::eSEUpdate_SeqHandler();
return;
}