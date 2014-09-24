#include <stdio.h>
#include <conio.h>
#include <assert.h>
#include <Windows.h>

#define TOBII_TX_DETAIL
#include <eyex\EyeX.h>

#pragma comment (lib, "Tobii.EyeX.Client.lib")

//#define ET_DEBUG
#define ET_PEEKABOO
#define ET_GAZESTREAM

static TX_CONTEXTHANDLE g_etContext = TX_EMPTY_HANDLE;
static TX_HANDLE g_hInteractorSnapshot = TX_EMPTY_HANDLE;
static const TX_STRING g_interactorId = "et";

void onStateReceived(TX_HANDLE p_hStateBag);
void onGazeEvent(TX_HANDLE p_hGazeDataBehavior);
// Callbacks:
void TX_CALLCONVENTION cbOnEngineConnectionStateChanged(
                        TX_CONNECTIONSTATE p_connectionState,
                        TX_USERPARAM p_userParam);
void TX_CALLCONVENTION cbOnPresenceStateChanged(
                        TX_CONSTHANDLE p_hAsyncData,
                        TX_USERPARAM p_userParam);
void TX_CALLCONVENTION cbOnEvent(TX_CONSTHANDLE p_hAsyncData,
                                 TX_USERPARAM p_userParam);
void TX_CALLCONVENTION cbOnSnapshotCommitted(TX_CONSTHANDLE p_hAsyncData,
                                             TX_USERPARAM p_param);

int main(int argc, int argv[]) {
    printf("Eye Tracking MWE using the Tobii EyeX SDK"
           " with the EyeX controller devkit.\n");

    TX_TICKET hEventHandlerTicket = TX_INVALID_TICKET;
    TX_TICKET hPresenceStateChangedTicket = TX_INVALID_TICKET;
    TX_TICKET hConnectionStateChangedTicket = TX_INVALID_TICKET;

    // Create EyeX engine context and register observers on state changes:
    TX_RESULT isInitialized = txInitializeEyeX(
                                             TX_EYEXCOMPONENTOVERRIDEFLAG_NONE,
                                             NULL, NULL, NULL, NULL);
    if(isInitialized==TX_RESULT_OK) {
        isInitialized = txCreateContext(&g_etContext, TX_FALSE);
    }
    if(isInitialized==TX_RESULT_OK) {
        TX_HANDLE hInteractor = TX_EMPTY_HANDLE;
        TX_GAZEPOINTDATAPARAMS params = {
            TX_GAZEPOINTDATAMODE_LIGHTLYFILTERED
        };
        bool success = txCreateGlobalInteractorSnapshot(
                        g_etContext, g_interactorId,
                        &g_hInteractorSnapshot, &hInteractor)==TX_RESULT_OK;
        success &= txCreateGazePointDataBehavior(hInteractor,
                                                 &params)==TX_RESULT_OK;
        isInitialized = success==true ? TX_RESULT_OK : TX_RESULT_UNKNOWN;
        txReleaseObject(&hInteractor);
    }
    if(isInitialized==TX_RESULT_OK) {
        isInitialized = txRegisterConnectionStateChangedHandler(
                         g_etContext, &hConnectionStateChangedTicket,
                         cbOnEngineConnectionStateChanged, NULL);
    }
    if(isInitialized==TX_RESULT_OK) {
        isInitialized = txRegisterStateChangedHandler(
                         g_etContext, &hPresenceStateChangedTicket,
                         TX_STATEPATH_USERPRESENCE, cbOnPresenceStateChanged,
                         NULL);
    }
    if(isInitialized==TX_RESULT_OK) {
        isInitialized = txRegisterEventHandler(g_etContext,
                                               &hEventHandlerTicket,
                                               cbOnEvent, NULL);
    }
    if(isInitialized==TX_RESULT_OK) {
        isInitialized = txEnableConnection(g_etContext);
    }
    if(isInitialized!=TX_RESULT_OK) {
        printf("Failed to initialize EyeX context. Aborting.\n");
        return -1;
    }

    printf("Press any key to exit...\n");
    _getch();
    printf("Exiting.\n");
    
    // Cleanup:
    txDisableConnection(g_etContext);
    txUnregisterConnectionStateChangedHandler(g_etContext,
                                              hConnectionStateChangedTicket);
    txUnregisterStateChangedHandler(g_etContext, hPresenceStateChangedTicket);
    txShutdownContext(g_etContext, TX_CLEANUPTIMEOUT_DEFAULT, TX_FALSE);
    txReleaseObject(&g_hInteractorSnapshot);
    txReleaseContext(&g_etContext);
    return 0;
}

void onStateReceived(TX_HANDLE p_hStateBag) {
    // Establish whether or not the device itself is up-and-running:
    TX_INTEGER status;
    TX_RESULT success = txGetStateValueAsInteger(p_hStateBag,
                                                 TX_STATEPATH_EYETRACKINGSTATE,
                                                 &status);
    assert(success==TX_RESULT_OK);
    switch(status) {
    case TX_EYETRACKINGDEVICESTATUS_TRACKING:
        printf("The EyeX controller is up and tracking.\n");
        break;
    default:
        printf("For some reason the EyeX controller is not tracking. This may"
               " be due to the device itself not being connected. Status code"
               " %d. Aborting...\n", status);
        assert(0);
        break;
    }

    // Retrieve screen size according to EyeX:
    TX_SIZE2 screenSize;
    success = txGetStateValueAsSize2(p_hStateBag, TX_STATEPATH_DISPLAYSIZE, 
                                     &screenSize);
    assert(success==TX_RESULT_OK);
    printf("EyeX interprets your screen size as %5.2f x %5.2f mm\n",
           screenSize.Width, screenSize.Height);

    // Retrieve screen dimensions according to EyeX:
    TX_SIZE2 screenDim;
    success = txGetStateValueAsSize2(p_hStateBag, TX_STATEPATH_SCREENBOUNDS,
                                     &screenDim);
    assert(success==TX_RESULT_OK);
    printf("EyeX interprets your resolution as %5.0f x %5.0f pixels.\n",
           screenDim.Width, screenDim.Height);

    txReleaseObject(&p_hStateBag);
}


void TX_CALLCONVENTION cbOnSnapshotCommitted(TX_CONSTHANDLE p_hAsyncData,
                                             TX_USERPARAM p_param) {
    TX_RESULT result = TX_RESULT_UNKNOWN;
    txGetAsyncDataResultCode(p_hAsyncData, &result);
    assert(result==TX_RESULT_OK || result==TX_RESULT_CANCELLED);
}

void TX_CALLCONVENTION cbOnEngineConnectionStateChanged(
                        TX_CONNECTIONSTATE p_connectionState,
                        TX_USERPARAM p_userParam) {
    if(p_connectionState==TX_CONNECTIONSTATE_CONNECTED) {
        TX_RESULT success = txCommitSnapshotAsync(g_hInteractorSnapshot,
                                                  cbOnSnapshotCommitted, NULL);
        assert(success==TX_RESULT_OK);

        TX_HANDLE hStateBag = TX_EMPTY_HANDLE;
        printf("Successfully initialized EyeX context; application is thus"
               " connected to the EyeX Engine.\n");
        txGetState(g_etContext, TX_STATEPATH_EYETRACKING, &hStateBag);
        onStateReceived(hStateBag);
    }
}
void TX_CALLCONVENTION cbOnPresenceStateChanged(TX_CONSTHANDLE p_hAsyncData,
                                                TX_USERPARAM p_userParam) {
#ifdef ET_PEEKABOO
    TX_RESULT result = TX_RESULT_UNKNOWN;
    TX_HANDLE hStateBag = TX_EMPTY_HANDLE;
    
    bool success = txGetAsyncDataResultCode(p_hAsyncData, &result)==TX_RESULT_OK
         && txGetAsyncDataContent(p_hAsyncData, &hStateBag)==TX_RESULT_OK;
    if(success==true) {
        TX_INTEGER userPresence;
        result = txGetStateValueAsInteger(hStateBag, TX_STATEPATH_USERPRESENCE, 
                                           &userPresence);
        if(result==TX_RESULT_OK) {
            if(userPresence==TX_USERPRESENCE_PRESENT) {
                printf("Pekaboo! EyeX has detected the presence of an"
                       " observer.\n");
            } else {
                printf("EyeX cannot recognize any present observer."
                       " Searching...\n");
            }
        }
    }
#endif // ET_PEEKABOO
}

void onGazeEvent(TX_HANDLE p_hGazeDataBehavior) {
    TX_GAZEPOINTDATAEVENTPARAMS eventParams;
    TX_RESULT success = txGetGazePointDataEventParams(p_hGazeDataBehavior,
                                                      &eventParams);
    assert(success==TX_RESULT_OK);
#ifdef ET_GAZESTREAM
    printf("%.0f ms: [%.1f, %.1f]\n", eventParams.Timestamp,
           eventParams.X, eventParams.Y);
#endif // ET_GAZESTREAM
}

void TX_CALLCONVENTION cbOnEvent(TX_CONSTHANDLE p_hAsyncData,
                                 TX_USERPARAM p_userParam) {
    TX_HANDLE hEvent = TX_EMPTY_HANDLE;
    txGetAsyncDataContent(p_hAsyncData, &hEvent);
#ifdef ET_DEBUG
    OutputDebugStringA(txDebugObject(hEvent));
#endif // ET_DEBUG
    TX_HANDLE hBehavior = TX_EMPTY_HANDLE;
    TX_RESULT success = txGetEventBehavior(hEvent, &hBehavior,
                                           TX_BEHAVIORTYPE_GAZEPOINTDATA);
    if(success==TX_RESULT_OK) {
        onGazeEvent(hBehavior);
        txReleaseObject(&hBehavior);
    }
    txReleaseObject(&hEvent);
}
