#include <stdio.h>
#include <conio.h>
#include <assert.h>

#define TOBII_TX_DETAIL
#include <eyex\EyeX.h>

#pragma comment (lib, "Tobii.EyeX.Client.lib")

static TX_CONTEXTHANDLE g_etContext = TX_EMPTY_HANDLE;

void onStateReceived(TX_HANDLE p_hStateBag);
// Callbacks:
void TX_CALLCONVENTION cbOnEngineConnectionStateChanged(
                        TX_CONNECTIONSTATE p_connectionState,
                        TX_USERPARAM p_userParam);
void TX_CALLCONVENTION cbOnPresenceStateChanged(
                        TX_CONSTHANDLE p_hAsyncData,
                        TX_USERPARAM p_userParam);

int main(int argc, int argv[]) {
    printf("Eye Tracking MWE using the Tobii EyeX SDK"
           " with the EyeX controller devkit.\n");

    TX_TICKET hConnectionStateChangedTicket = TX_INVALID_TICKET;
    TX_TICKET hPresenceStateChangedTicket = TX_INVALID_TICKET;

    // Create EyeX engine context and register observers on state changes:
    TX_RESULT isInitialized = txInitializeEyeX(
                                             TX_EYEXCOMPONENTOVERRIDEFLAG_NONE,
                                             NULL, NULL, NULL, NULL);
    if(isInitialized==TX_RESULT_OK) {
        isInitialized = txCreateContext(&g_etContext, TX_FALSE);
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
    txUnregisterConnectionStateChangedHandler(g_etContext,
                                              hConnectionStateChangedTicket);
    txUnregisterStateChangedHandler(g_etContext, hPresenceStateChangedTicket);
    txShutdownContext(g_etContext, TX_CLEANUPTIMEOUT_DEFAULT, TX_FALSE);
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

void TX_CALLCONVENTION cbOnEngineConnectionStateChanged(
                        TX_CONNECTIONSTATE p_connectionState,
                        TX_USERPARAM p_userParam) {
    if(p_connectionState==TX_CONNECTIONSTATE_CONNECTED) {
        TX_HANDLE hStateBag = TX_EMPTY_HANDLE;
        printf("Successfully initialized EyeX context; application is thus"
               " connected to the EyeX Engine.\n");
        txGetState(g_etContext, TX_STATEPATH_EYETRACKING, &hStateBag);
        onStateReceived(hStateBag);
    }
}
void TX_CALLCONVENTION cbOnPresenceStateChanged(TX_CONSTHANDLE p_hAsyncData,
                                                TX_USERPARAM p_userParam) {
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
}
