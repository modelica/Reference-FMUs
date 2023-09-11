#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "fmi1Functions.h"
#include "FMI.h"

struct FMI1Functions_ {

    /***************************************************
     Common Functions for FMI 1.0
    ****************************************************/

    fmi1CallbackFunctions   callbacks;

    fmi1SetRealTYPE         *fmi1SetReal;
    fmi1SetIntegerTYPE      *fmi1SetInteger;
    fmi1SetBooleanTYPE      *fmi1SetBoolean;
    fmi1SetStringTYPE       *fmi1SetString;
    fmi1GetRealTYPE         *fmi1GetReal;
    fmi1GetIntegerTYPE      *fmi1GetInteger;
    fmi1GetBooleanTYPE      *fmi1GetBoolean;
    fmi1GetStringTYPE       *fmi1GetString;
    fmi1SetDebugLoggingTYPE *fmi1SetDebugLogging;

    /***************************************************
     FMI 1.0 for Model Exchange Functions
    ****************************************************/

    fmi1GetModelTypesPlatformTYPE      *fmi1GetModelTypesPlatform;
    fmi1GetVersionTYPE                 *fmi1GetVersion;
    fmi1InstantiateModelTYPE           *fmi1InstantiateModel;
    fmi1FreeModelInstanceTYPE          *fmi1FreeModelInstance;
    fmi1SetTimeTYPE                    *fmi1SetTime;
    fmi1SetContinuousStatesTYPE        *fmi1SetContinuousStates;
    fmi1CompletedIntegratorStepTYPE    *fmi1CompletedIntegratorStep;
    fmi1InitializeTYPE                 *fmi1Initialize;
    fmi1GetDerivativesTYPE             *fmi1GetDerivatives;
    fmi1GetEventIndicatorsTYPE         *fmi1GetEventIndicators;
    fmi1EventUpdateTYPE                *fmi1EventUpdate;
    fmi1GetContinuousStatesTYPE        *fmi1GetContinuousStates;
    fmi1GetNominalContinuousStatesTYPE *fmi1GetNominalContinuousStates;
    fmi1GetStateValueReferencesTYPE    *fmi1GetStateValueReferences;
    fmi1TerminateTYPE                  *fmi1Terminate;

    /***************************************************
     FMI 1.0 for Co-Simulation Functions
    ****************************************************/

    fmi1GetTypesPlatformTYPE         *fmi1GetTypesPlatform;
    fmi1InstantiateSlaveTYPE         *fmi1InstantiateSlave;
    fmi1InitializeSlaveTYPE          *fmi1InitializeSlave;
    fmi1TerminateSlaveTYPE           *fmi1TerminateSlave;
    fmi1ResetSlaveTYPE               *fmi1ResetSlave;
    fmi1FreeSlaveInstanceTYPE        *fmi1FreeSlaveInstance;
    fmi1SetRealInputDerivativesTYPE  *fmi1SetRealInputDerivatives;
    fmi1GetRealOutputDerivativesTYPE *fmi1GetRealOutputDerivatives;
    fmi1CancelStepTYPE               *fmi1CancelStep;
    fmi1DoStepTYPE                   *fmi1DoStep;
    fmi1GetStatusTYPE                *fmi1GetStatus;
    fmi1GetRealStatusTYPE            *fmi1GetRealStatus;
    fmi1GetIntegerStatusTYPE         *fmi1GetIntegerStatus;
    fmi1GetBooleanStatusTYPE         *fmi1GetBooleanStatus;
    fmi1GetStringStatusTYPE          *fmi1GetStringStatus;

};

/***************************************************
 Common Functions for FMI 1.0
****************************************************/
FMI_STATIC FMIStatus    FMI1SetReal         (FMIInstance *instance, const fmi1ValueReference vr[], size_t nvr, const fmi1Real    value[]);
FMI_STATIC FMIStatus    FMI1SetInteger      (FMIInstance *instance, const fmi1ValueReference vr[], size_t nvr, const fmi1Integer value[]);
FMI_STATIC FMIStatus    FMI1SetBoolean      (FMIInstance *instance, const fmi1ValueReference vr[], size_t nvr, const fmi1Boolean value[]);
FMI_STATIC FMIStatus    FMI1SetString       (FMIInstance *instance, const fmi1ValueReference vr[], size_t nvr, const fmi1String  value[]);
FMI_STATIC FMIStatus    FMI1GetReal         (FMIInstance *instance, const fmi1ValueReference vr[], size_t nvr,       fmi1Real    value[]);
FMI_STATIC FMIStatus    FMI1GetInteger      (FMIInstance *instance, const fmi1ValueReference vr[], size_t nvr,       fmi1Integer value[]);
FMI_STATIC FMIStatus    FMI1GetBoolean      (FMIInstance *instance, const fmi1ValueReference vr[], size_t nvr,       fmi1Boolean value[]);
FMI_STATIC FMIStatus    FMI1GetString       (FMIInstance *instance, const fmi1ValueReference vr[], size_t nvr,       fmi1String  value[]);
FMI_STATIC FMIStatus    FMI1SetDebugLogging (FMIInstance *instance, fmi1Boolean loggingOn);

/***************************************************
 FMI 1.0 for Model Exchange Functions
****************************************************/
FMI_STATIC const char*  FMI1GetModelTypesPlatform      (FMIInstance *instance);
FMI_STATIC const char*  FMI1GetVersion                 (FMIInstance *instance);
FMI_STATIC FMIStatus    FMI1InstantiateModel           (FMIInstance *instance, fmi1String modelIdentifier, fmi1String GUID, fmi1Boolean loggingOn);
FMI_STATIC void         FMI1FreeModelInstance          (FMIInstance *instance);
FMI_STATIC FMIStatus    FMI1SetTime                    (FMIInstance *instance, fmi1Real time);
FMI_STATIC FMIStatus    FMI1SetContinuousStates        (FMIInstance *instance, const fmi1Real x[], size_t nx);
FMI_STATIC FMIStatus    FMI1CompletedIntegratorStep    (FMIInstance *instance, fmi1Boolean* callEventUpdate);
FMI_STATIC FMIStatus    FMI1Initialize                 (FMIInstance *instance, fmi1Boolean toleranceControlled, fmi1Real relativeTolerance, fmi1EventInfo* eventInfo);
FMI_STATIC FMIStatus    FMI1GetDerivatives             (FMIInstance *instance, fmi1Real derivatives[], size_t nx);
FMI_STATIC FMIStatus    FMI1GetEventIndicators         (FMIInstance *instance, fmi1Real eventIndicators[], size_t ni);
FMI_STATIC FMIStatus    FMI1EventUpdate                (FMIInstance *instance, fmi1Boolean intermediateResults, fmi1EventInfo* eventInfo);
FMI_STATIC FMIStatus    FMI1GetContinuousStates        (FMIInstance *instance, fmi1Real states[], size_t nx);
FMI_STATIC FMIStatus    FMI1GetNominalContinuousStates (FMIInstance *instance, fmi1Real x_nominal[], size_t nx);
FMI_STATIC FMIStatus    FMI1GetStateValueReferences    (FMIInstance *instance, fmi1ValueReference vrx[], size_t nx);
FMI_STATIC FMIStatus    FMI1Terminate                  (FMIInstance *instance);

/***************************************************
 FMI 1.0 for Co-Simulation Functions
****************************************************/
FMI_STATIC const char*  FMI1GetTypesPlatform         (FMIInstance *instance);
FMI_STATIC FMIStatus    FMI1InstantiateSlave         (FMIInstance *instance, fmi1String modelIdentifier, fmi1String fmuGUID, fmi1String fmuLocation, fmi1String  mimeType, fmi1Real timeout, fmi1Boolean visible, fmi1Boolean interactive, fmi1Boolean loggingOn);
FMI_STATIC FMIStatus    FMI1InitializeSlave          (FMIInstance *instance, fmi1Real tStart, fmi1Boolean StopTimeDefined, fmi1Real tStop);
FMI_STATIC FMIStatus    FMI1TerminateSlave           (FMIInstance *instance);
FMI_STATIC FMIStatus    FMI1ResetSlave               (FMIInstance *instance);
FMI_STATIC void         FMI1FreeSlaveInstance        (FMIInstance *instance);
FMI_STATIC FMIStatus    FMI1SetRealInputDerivatives  (FMIInstance *instance, const fmi1ValueReference vr[], size_t nvr, const fmi1Integer order[], const fmi1Real value[]);
FMI_STATIC FMIStatus    FMI1GetRealOutputDerivatives (FMIInstance *instance, const fmi1ValueReference vr[], size_t nvr, const fmi1Integer order[],       fmi1Real value[]);
FMI_STATIC FMIStatus    FMI1CancelStep               (FMIInstance *instance);
FMI_STATIC FMIStatus    FMI1DoStep                   (FMIInstance *instance, fmi1Real currentCommunicationPoint, fmi1Real communicationStepSize, fmi1Boolean newStep);
FMI_STATIC FMIStatus    FMI1GetStatus                (FMIInstance *instance, const fmi1StatusKind s, fmi1Status*  value);
FMI_STATIC FMIStatus    FMI1GetRealStatus            (FMIInstance *instance, const fmi1StatusKind s, fmi1Real*    value);
FMI_STATIC FMIStatus    FMI1GetIntegerStatus         (FMIInstance *instance, const fmi1StatusKind s, fmi1Integer* value);
FMI_STATIC FMIStatus    FMI1GetBooleanStatus         (FMIInstance *instance, const fmi1StatusKind s, fmi1Boolean* value);
FMI_STATIC FMIStatus    FMI1GetStringStatus          (FMIInstance *instance, const fmi1StatusKind s, fmi1String*  value);

#ifdef __cplusplus
}  /* end of extern "C" { */
#endif
