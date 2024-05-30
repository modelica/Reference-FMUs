#include "FMIModelDescription.h"

#include <string.h>
#include <stdint.h>

#include <libxml/parser.h>
#include <libxml/xmlschemas.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#include "structured_variable_name.tab.h"

// to regenerate run
//
// python xsdflatten.py fmi3ModelDescription.xsd > fmi3Merged.xsd
//
// and
//
// xxd -i xxd -i fmi3Merged.xsd > fmi3schema.h
#include "fmi1schema.h"
#include "fmi2schema.h"
#include "fmi3schema.h"


#include "FMIUtil.h"


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)

static bool getBooleanAttribute(const xmlNodePtr node, const char* name) {
    char* literal = (char*)xmlGetProp(node, (xmlChar*)name);
    bool value = literal && (strcmp(literal, "true") == 0 || strcmp(literal, "1") == 0);
    xmlFree(literal);
    return value;
}

static uint32_t getUInt32Attribute(const xmlNodePtr node, const char* name) {
    char* literal = (char*)xmlGetProp(node, (xmlChar*)name);
    uint32_t value = strtoul(literal, NULL, 0);
    xmlFree(literal);
    return value;
}

static FMIVariableNamingConvention getVariableNamingConvention(const xmlNodePtr node) {
    char* value = (char*)xmlGetProp(node, (xmlChar*)"variableNamingConvention");
    FMIVariableNamingConvention variableNamingConvention = (value && !strcmp(value, "structured")) ? FMIStructured : FMIFlat;
    xmlFree(value);
    return variableNamingConvention;
}

static FMIModelDescription* readModelDescriptionFMI1(xmlNodePtr root) {

    FMIStatus status = FMIOK;

    FMIModelDescription* modelDescription = NULL;

    CALL(FMICalloc((void**)&modelDescription, 1, sizeof(FMIModelDescription)));

    modelDescription->fmiVersion               = FMIVersion1;
    modelDescription->modelName                = (char*)xmlGetProp(root, (xmlChar*)"modelName");
    modelDescription->instantiationToken       = (char*)xmlGetProp(root, (xmlChar*)"guid");
    modelDescription->description              = (char*)xmlGetProp(root, (xmlChar*)"description");
    modelDescription->generationTool           = (char*)xmlGetProp(root, (xmlChar*)"generationTool");
    modelDescription->generationDate           = (char*)xmlGetProp(root, (xmlChar*)"generationDateAndTime");
    modelDescription->variableNamingConvention = getVariableNamingConvention(root);

    const char* numberOfContinuousStates = (char*)xmlGetProp(root, (xmlChar*)"numberOfContinuousStates");
    
    if (numberOfContinuousStates) {
        modelDescription->nContinuousStates = atoi(numberOfContinuousStates);
    }
    
    xmlFree((void*)numberOfContinuousStates);

    const char* numberOfEventIndicators = (char*)xmlGetProp(root, (xmlChar*)"numberOfEventIndicators");
    
    if (numberOfEventIndicators) {
        modelDescription->nEventIndicators = atoi(numberOfEventIndicators);
    }
    
    xmlFree((void*)numberOfEventIndicators);

    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(root->doc);

    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression((xmlChar*)"/fmiModelDescription/Implementation/CoSimulation_StandAlone", xpathCtx);
    
    if (xpathObj->nodesetval->nodeNr == 1) {
        CALL(FMICalloc((void**)&modelDescription->coSimulation, 1, sizeof(FMICoSimulationInterface)));
        modelDescription->coSimulation->modelIdentifier = (char*)xmlGetProp(root, (xmlChar*)"modelIdentifier");
    } else {
        CALL(FMICalloc((void**)&modelDescription->modelExchange, 1, sizeof(FMIModelExchangeInterface)));
        modelDescription->modelExchange->modelIdentifier = (char*)xmlGetProp(root, (xmlChar*)"modelIdentifier");
    }
    
    xmlXPathFreeObject(xpathObj);

    xpathObj = xmlXPathEvalExpression((xmlChar*)"/fmiModelDescription/DefaultExperiment", xpathCtx);
    
    if (xpathObj->nodesetval->nodeNr == 1) {
        CALL(FMICalloc((void**)&modelDescription->defaultExperiment, 1, sizeof(FMIDefaultExperiment)));
        const xmlNodePtr node = xpathObj->nodesetval->nodeTab[0];
        modelDescription->defaultExperiment->startTime = (char*)xmlGetProp(node, (xmlChar*)"startTime");
        modelDescription->defaultExperiment->stopTime = (char*)xmlGetProp(node, (xmlChar*)"stopTime");
    }
    
    xmlXPathFreeObject(xpathObj);

    xpathObj = xmlXPathEvalExpression((xmlChar*)"/fmiModelDescription/ModelVariables/ScalarVariable/*[self::Real or self::Integer or self::Enumeration or self::Boolean or self::String]", xpathCtx);

    modelDescription->nModelVariables = xpathObj->nodesetval->nodeNr;

    CALL(FMICalloc((void**)&modelDescription->modelVariables, xpathObj->nodesetval->nodeNr, sizeof(FMIModelVariable)));

    for (size_t i = 0; i < xpathObj->nodesetval->nodeNr; i++) {

        xmlNodePtr typeNode = xpathObj->nodesetval->nodeTab[i];
        xmlNodePtr variableNode = typeNode->parent;

        FMIModelVariable* variable = &modelDescription->modelVariables[i];

        variable->line = variableNode->line;
        variable->name = (char*)xmlGetProp(variableNode, (xmlChar*)"name");
        variable->description = (char*)xmlGetProp(variableNode, (xmlChar*)"description");

        const char* typeName = (char*)typeNode->name;

        const char* causality = (char*)xmlGetProp(variableNode, (xmlChar*)"causality");

        if (!causality) {
            // default
            variable->causality = FMILocal;
        } else if (!strcmp(causality, "input")) {
            variable->causality = FMIInput;
        } else if (!strcmp(causality, "output")) {
            variable->causality = FMIOutput;
        } else {
            // "internal" or "none"
            variable->causality = FMILocal;
        }

        xmlFree((void*)causality);
        
        const char* variability = (char*)xmlGetProp(variableNode, (xmlChar*)"variability");

        if (!variability) {
            // default
            variable->variability = FMIContinuous;
        } else if (!strcmp(variability, "constant")) {
            variable->variability = FMIConstant;
        } else if (!strcmp(variability, "parameter")) {
            variable->causality = FMIParameter;
            variable->variability = FMITunable;
        } else if (!strcmp(variability, "discrete")) {
            variable->variability = FMIDiscrete;
        } else if (!strcmp(variability, "continuous")) {
            variable->variability = FMIContinuous;
        }

        free((void*)variability);

        if (!strcmp(typeName, "Real")) {
            const char* variability = (char*)xmlGetProp(variableNode, (xmlChar*)"variability");
            if (variability && !strcmp(variability, "discrete")) {
                variable->type = FMIDiscreteRealType;
            } else {
                variable->type = FMIRealType;
            }
            free((void*)variability);
        } else if (!strcmp(typeName, "Integer") || !strcmp(typeName, "Enumeration")) {
            variable->type = FMIIntegerType;
        } else if (!strcmp(typeName, "Boolean")) {
            variable->type = FMIBooleanType;
        } else if (!strcmp(typeName, "String")) {
            variable->type = FMIStringType;
        } else {
            continue;
        }

        variable->valueReference = getUInt32Attribute(variableNode, "valueReference");

    }

    size_t nProblems = 0;

    // check variabilities
    for (size_t i = 0; i < modelDescription->nModelVariables; i++) {
        const FMIModelVariable* variable = &modelDescription->modelVariables[i];
        if (variable->type != FMIRealType && variable->type != FMIDiscreteRealType && variable->variability == FMIContinuous) {
            FMILogError("Variable \"%s\" is not of type Real but has variability = continuous.\n", variable->name);
            nProblems++;
        }
    }

    xmlXPathFreeObject(xpathObj);

    xmlXPathFreeContext(xpathCtx);

    if (nProblems > 0) {
        FMIFreeModelDescription(modelDescription);
        modelDescription = NULL;
    }

TERMINATE:

    if (status != FMIOK) {
        FMIFree((void**)&modelDescription);
    }

    return modelDescription;
}

static void readUnknownsFMI2(xmlXPathContextPtr xpathCtx, FMIModelDescription* modelDescription, const char* path, size_t* nUnkonwns, FMIUnknown** unknowns) {

    FMIStatus status = FMIOK;

    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression((xmlChar*)path, xpathCtx);
    
    *nUnkonwns = xpathObj->nodesetval->nodeNr;

    CALL(FMICalloc((void**)unknowns, *nUnkonwns, sizeof(FMIUnknown)));

    for (size_t i = 0; i < xpathObj->nodesetval->nodeNr; i++) {

        xmlNodePtr unkownNode = xpathObj->nodesetval->nodeTab[i];

        const char* indexLiteral = (char*)xmlGetProp(unkownNode, (xmlChar*)"index");

        (*unknowns)[i].modelVariable = FMIModelVariableForIndexLiteral(modelDescription, indexLiteral);

        free((void*)indexLiteral);
    }

    xmlXPathFreeObject(xpathObj);

TERMINATE:

    // TODO
    ;
}

static void readUnknownsFMI3(xmlXPathContextPtr xpathCtx, FMIModelDescription* modelDescription, const char* path, size_t* nUnkonwns, FMIUnknown** unknowns) {

    FMIStatus status = FMIOK;

    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression((xmlChar*)path, xpathCtx);

    *nUnkonwns = xpathObj->nodesetval->nodeNr;

    CALL(FMICalloc((void**)unknowns, *nUnkonwns, sizeof(FMIUnknown)));

    for (size_t i = 0; i < xpathObj->nodesetval->nodeNr; i++) {

        xmlNodePtr unknownNode = xpathObj->nodesetval->nodeTab[i];

        FMIValueReference valueReference = getUInt32Attribute(unknownNode, "valueReference");

        for (size_t j = 0; j < modelDescription->nModelVariables; j++) {
            FMIModelVariable* variable = &modelDescription->modelVariables[j];
            if (variable->valueReference == valueReference) {
                (*unknowns)[i].modelVariable = variable;
                break;
            }
        }
    }

    xmlXPathFreeObject(xpathObj);

TERMINATE:

    // TODO
    ;
}

static FMIModelDescription* readModelDescriptionFMI2(xmlNodePtr root) {

    FMIStatus status = FMIOK;

    FMIModelDescription* modelDescription = NULL;
    
    CALL(FMICalloc((void**)&modelDescription, 1, sizeof(FMIModelDescription)));

    modelDescription->fmiVersion = FMIVersion2;
    modelDescription->modelName = (char*)xmlGetProp(root, (xmlChar*)"modelName");
    modelDescription->instantiationToken = (char*)xmlGetProp(root, (xmlChar*)"guid");
    modelDescription->description = (char*)xmlGetProp(root, (xmlChar*)"description");
    modelDescription->generationTool = (char*)xmlGetProp(root, (xmlChar*)"generationTool");
    modelDescription->generationDate = (char*)xmlGetProp(root, (xmlChar*)"generationDate");
    modelDescription->variableNamingConvention = getVariableNamingConvention(root);

    const char* numberOfEventIndicators = (char*)xmlGetProp(root, (xmlChar*)"numberOfEventIndicators");

    if (numberOfEventIndicators) {
        modelDescription->nEventIndicators = atoi(numberOfEventIndicators);
    }

    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(root->doc);

    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression((xmlChar*)"/fmiModelDescription/CoSimulation", xpathCtx);
    if (xpathObj->nodesetval->nodeNr == 1) {
        CALL(FMICalloc((void**)&modelDescription->coSimulation, 1, sizeof(FMICoSimulationInterface)));
        modelDescription->coSimulation->modelIdentifier = (char*)xmlGetProp(xpathObj->nodesetval->nodeTab[0], (xmlChar*)"modelIdentifier");
    }
    xmlXPathFreeObject(xpathObj);

    xpathObj = xmlXPathEvalExpression((xmlChar*)"/fmiModelDescription/ModelExchange", xpathCtx);
    if (xpathObj->nodesetval->nodeNr == 1) {
        CALL(FMICalloc((void**)&modelDescription->modelExchange, 1, sizeof(FMIModelExchangeInterface)));
        const xmlNodePtr node = xpathObj->nodesetval->nodeTab[0];
        modelDescription->modelExchange->modelIdentifier = (char*)xmlGetProp(node, (xmlChar*)"modelIdentifier");
        modelDescription->modelExchange->providesDirectionalDerivatives = getBooleanAttribute(node, "providesDirectionalDerivative");
        modelDescription->modelExchange->needsCompletedIntegratorStep = !getBooleanAttribute(node, "completedIntegratorStepNotNeeded");
    }
    xmlXPathFreeObject(xpathObj);

    xpathObj = xmlXPathEvalExpression((xmlChar*)"/fmiModelDescription/DefaultExperiment", xpathCtx);
    if (xpathObj->nodesetval->nodeNr == 1) {
        CALL(FMICalloc((void**)&modelDescription->defaultExperiment, 1, sizeof(FMIDefaultExperiment)));
        const xmlNodePtr node = xpathObj->nodesetval->nodeTab[0];
        modelDescription->defaultExperiment->startTime = (char*)xmlGetProp(node, (xmlChar*)"startTime");
        modelDescription->defaultExperiment->stopTime = (char*)xmlGetProp(node, (xmlChar*)"stopTime");
        modelDescription->defaultExperiment->stepSize = (char*)xmlGetProp(node, (xmlChar*)"stepSize");
    }
    xmlXPathFreeObject(xpathObj);

    xpathObj = xmlXPathEvalExpression((xmlChar*)"/fmiModelDescription/ModelVariables/ScalarVariable/*[self::Real or self::Integer or self::Enumeration or self::Boolean or self::String]", xpathCtx);

    modelDescription->nModelVariables = xpathObj->nodesetval->nodeNr;
    CALL(FMICalloc((void**)&modelDescription->modelVariables, modelDescription->nModelVariables, sizeof(FMIModelVariable)));

    for (size_t i = 0; i < xpathObj->nodesetval->nodeNr; i++) {

        xmlNodePtr typeNode = xpathObj->nodesetval->nodeTab[i];
        xmlNodePtr variableNode = typeNode->parent;

        FMIModelVariable* variable = &modelDescription->modelVariables[i];

        variable->line = variableNode->line;
        variable->name = (char*)xmlGetProp(variableNode, (xmlChar*)"name");
        variable->description = (char*)xmlGetProp(variableNode, (xmlChar*)"description");

        variable->derivative = (FMIModelVariable*)xmlGetProp(typeNode, (xmlChar*)"derivative");

        const char* typeName = (char*)typeNode->name;

        const char* variability = (char*)xmlGetProp(variableNode, (xmlChar*)"variability");
        
        if (!variability) {
            variable->variability = FMIContinuous;
        } else if(!strcmp(variability, "constant")) {
            variable->variability = FMIConstant;
        } else if (!strcmp(variability, "fixed")) {
            variable->variability = FMIFixed;
        } else if (!strcmp(variability, "tunable")) {
            variable->variability = FMITunable;
        } else if (!strcmp(variability, "discrete")) {
            variable->variability = FMIDiscrete;
        } else {
            variable->variability = FMIContinuous;
        }

        free((void*)variability);

        if (!strcmp(typeName, "Real")) {
            variable->type = variable->variability == FMIDiscrete ? FMIDiscreteRealType : FMIRealType;
        } else if (!strcmp(typeName, "Integer") || !strcmp(typeName, "Enumeration")) {
            variable->type = FMIIntegerType;
        } else if (!strcmp(typeName, "Boolean")) {
            variable->type = FMIBooleanType;
        } else if (!strcmp(typeName, "String")) {
            variable->type = FMIStringType;
        } else {
            continue;
        }

        const char* vr = (char*)xmlGetProp(variableNode, (xmlChar*)"valueReference");

        variable->valueReference = FMIValueReferenceForLiteral(vr);

        free((void*)vr);
        
        const char* causality = (char*)xmlGetProp(variableNode, (xmlChar*)"causality");

        if (!causality) {
            variable->causality = FMILocal;
        } else if (!strcmp(causality, "parameter")) {
            variable->causality = FMIParameter;
        } else if (!strcmp(causality, "input")) {
            variable->causality = FMIInput;
        } else if (!strcmp(causality, "output")) {
            variable->causality = FMIOutput;
        } else if (!strcmp(causality, "independent")) {
            variable->causality = FMIIndependent;
        } else {
            variable->causality = FMILocal;
        }

        free((void*)causality);
    }

    xmlXPathFreeObject(xpathObj);

    readUnknownsFMI2(xpathCtx, modelDescription, "/fmiModelDescription/ModelStructure/Outputs/Unknown", &modelDescription->nOutputs, &modelDescription->outputs);
    readUnknownsFMI2(xpathCtx, modelDescription, "/fmiModelDescription/ModelStructure/Derivatives/Unknown", &modelDescription->nContinuousStates, &modelDescription->derivatives);
    readUnknownsFMI2(xpathCtx, modelDescription, "/fmiModelDescription/ModelStructure/InitialUnknowns/Unknown", &modelDescription->nInitialUnknowns, &modelDescription->initialUnknowns);

    xmlXPathFreeContext(xpathCtx);

    size_t nProblems = 0;

    // resolve derivatives
    for (size_t i = 0; i < modelDescription->nModelVariables; i++) {
        FMIModelVariable* variable = &modelDescription->modelVariables[i];
        if (variable->derivative) {
            char* literal = (char*)variable->derivative;
            variable->derivative = FMIModelVariableForIndexLiteral(modelDescription, literal);
            if (!variable->derivative) {
                FMILogError("Failed to resolve attribute derivative=\"%s\" for model variable \"%s\".", literal, variable->name);
                nProblems++;
            }
            free(literal);
        }
    }

    // check variabilities
    for (size_t i = 0; i < modelDescription->nModelVariables; i++) {
        FMIModelVariable* variable = &modelDescription->modelVariables[i];
        if (variable->type != FMIRealType && variable->type != FMIDiscreteRealType && variable->variability == FMIContinuous) {
            FMILogError("Variable \"%s\" is not of type Real but has variability = continuous.\n", variable->name);
            nProblems++;
        }
    }

    nProblems += FMIValidateModelDescription(modelDescription);

    if (nProblems > 0) {
        FMIFreeModelDescription(modelDescription);
        modelDescription = NULL;
    }

TERMINATE:
    
    // TODO
    
    return modelDescription;
}

static FMIModelDescription* readModelDescriptionFMI3(xmlNodePtr root) {

    FMIStatus status = FMIOK;
    
    FMIModelDescription* modelDescription = NULL;

    CALL(FMICalloc((void**)&modelDescription, 1, sizeof(FMIModelDescription)));

    modelDescription->fmiVersion               = FMIVersion3;
    modelDescription->modelName                = (char*)xmlGetProp(root, (xmlChar*)"modelName");
    modelDescription->instantiationToken       = (char*)xmlGetProp(root, (xmlChar*)"instantiationToken");
    modelDescription->description              = (char*)xmlGetProp(root, (xmlChar*)"description");
    modelDescription->generationTool           = (char*)xmlGetProp(root, (xmlChar*)"generationTool");
    modelDescription->generationDate           = (char*)xmlGetProp(root, (xmlChar*)"generationDate");
    modelDescription->variableNamingConvention = getVariableNamingConvention(root);

    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(root->doc);

    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression((xmlChar*)"/fmiModelDescription/CoSimulation", xpathCtx);
    if (xpathObj->nodesetval->nodeNr == 1) {
        CALL(FMICalloc((void**)&modelDescription->coSimulation, 1, sizeof(FMICoSimulationInterface)));
        const xmlNodePtr node = xpathObj->nodesetval->nodeTab[0];
        modelDescription->coSimulation->modelIdentifier = (char*)xmlGetProp(node, (xmlChar*)"modelIdentifier");
        modelDescription->coSimulation->hasEventMode = getBooleanAttribute(node, "hasEventMode");
    }
    xmlXPathFreeObject(xpathObj);

    xpathObj = xmlXPathEvalExpression((xmlChar*)"/fmiModelDescription/ModelExchange", xpathCtx);
    if (xpathObj->nodesetval->nodeNr == 1) {
        const xmlNodePtr node = xpathObj->nodesetval->nodeTab[0];
        CALL(FMICalloc((void**)&modelDescription->modelExchange, 1, sizeof(FMIModelExchangeInterface)));
        modelDescription->modelExchange->modelIdentifier = (char*)xmlGetProp(node, (xmlChar*)"modelIdentifier");
        modelDescription->modelExchange->providesDirectionalDerivatives = getBooleanAttribute(node, "providesDirectionalDerivatives");
        modelDescription->modelExchange->needsCompletedIntegratorStep = getBooleanAttribute(node, "needsCompletedIntegratorStep");
    }
    xmlXPathFreeObject(xpathObj);

    xpathObj = xmlXPathEvalExpression((xmlChar*)"/fmiModelDescription/DefaultExperiment", xpathCtx);
    if (xpathObj->nodesetval->nodeNr == 1) {
        CALL(FMICalloc((void**)&modelDescription->defaultExperiment, 1, sizeof(FMIDefaultExperiment)));
        const xmlNodePtr node = xpathObj->nodesetval->nodeTab[0];
        modelDescription->defaultExperiment->startTime = (char*)xmlGetProp(node, (xmlChar*)"startTime");
        modelDescription->defaultExperiment->stopTime  = (char*)xmlGetProp(node, (xmlChar*)"stopTime");
        modelDescription->defaultExperiment->stepSize  = (char*)xmlGetProp(node, (xmlChar*)"stepSize");
    }
    xmlXPathFreeObject(xpathObj);

    // unit definitions
    xpathObj = xmlXPathEvalExpression((xmlChar*)"/fmiModelDescription/UnitDefinitions/Unit", xpathCtx);

    modelDescription->nUnitDefinitions = xpathObj->nodesetval->nodeNr;
    CALL(FMICalloc((void**)&modelDescription->unitDefinitions, xpathObj->nodesetval->nodeNr, sizeof(FMIUnit)));

    for (size_t i = 0; i < xpathObj->nodesetval->nodeNr; i++) {

        FMIUnit* unit = &modelDescription->unitDefinitions[i];

        unit->factor = 1.0;

        xmlNodePtr unitNode = xpathObj->nodesetval->nodeTab[i];

        unit->name = (char*)xmlGetProp(unitNode, (xmlChar*)"name");
    }

    // type definitions
    xpathObj = xmlXPathEvalExpression((xmlChar*)"/fmiModelDescription/TypeDefinitions/"
      "*[self::Float32Type"
      " or self::Float64Type"
      " or self::Int8Type"
      " or self::UInt8Type"
      " or self::Int16Type"
      " or self::UInt16Type"
      " or self::Int32Type"
      " or self::UInt32Type"
      " or self::Int64Type"
      " or self::UInt64Type"
      " or self::EnumerationType"
      " or self::BooleanType"
      " or self::StringType"
      " or self::BinaryType"
      " or self::ClockType]",
      xpathCtx);

    if (xpathObj->nodesetval) {
        modelDescription->nTypeDefinitions = xpathObj->nodesetval->nodeNr;
        CALL(FMICalloc((void**)&modelDescription->typeDefinitions, xpathObj->nodesetval->nodeNr, sizeof(FMIModelVariable)));

        for (size_t i = 0; i < xpathObj->nodesetval->nodeNr; i++) {

            FMITypeDefinition* typeDefinition = &modelDescription->typeDefinitions[i];

            xmlNodePtr typeDefinitionNode = xpathObj->nodesetval->nodeTab[i];

            // TODO: type
            typeDefinition->name = (char*)xmlGetProp(typeDefinitionNode, (xmlChar*)"name");
            typeDefinition->quantity = (char*)xmlGetProp(typeDefinitionNode, (xmlChar*)"quantity");

            const char* unitName = (char*)xmlGetProp(typeDefinitionNode, (xmlChar*)"unit");

            typeDefinition->unit = FMIUnitForName(modelDescription, unitName);

            xmlFree((void*)unitName);

            typeDefinition->displayUnit = (char*)xmlGetProp(typeDefinitionNode, (xmlChar*)"displayUnit");
            typeDefinition->relativeQuantity = getBooleanAttribute(typeDefinitionNode, "relativeQuantity");
            typeDefinition->min = (char*)xmlGetProp(typeDefinitionNode, (xmlChar*)"min");
            typeDefinition->max = (char*)xmlGetProp(typeDefinitionNode, (xmlChar*)"max");
            typeDefinition->nominal = (char*)xmlGetProp(typeDefinitionNode, (xmlChar*)"nominal");
            typeDefinition->unbounded = getBooleanAttribute(typeDefinitionNode, "unbounded");
        }
    }

    // model variables
    xpathObj = xmlXPathEvalExpression((xmlChar*)"/fmiModelDescription/ModelVariables/"
        "*[self::Float32"
        " or self::Float64"
        " or self::Int8"
        " or self::UInt8"
        " or self::Int16"
        " or self::UInt16"
        " or self::Int32"
        " or self::UInt32"
        " or self::Int64"
        " or self::UInt64"
        " or self::Enumeration"
        " or self::Boolean"
        " or self::String"
        " or self::Binary"
        " or self::Clock]", 
        xpathCtx);

    modelDescription->nModelVariables = xpathObj->nodesetval->nodeNr;
    CALL(FMICalloc((void**)&modelDescription->modelVariables, xpathObj->nodesetval->nodeNr, sizeof(FMIModelVariable)));

    for (size_t i = 0; i < xpathObj->nodesetval->nodeNr; i++) {

        FMIModelVariable* variable = &modelDescription->modelVariables[i];

        xmlNodePtr variableNode = xpathObj->nodesetval->nodeTab[i];

        variable->line = variableNode->line;
        variable->name = (char*)xmlGetProp(variableNode, (xmlChar*)"name");
        variable->min = (char*)xmlGetProp(variableNode, (xmlChar*)"min");
        variable->max = (char*)xmlGetProp(variableNode, (xmlChar*)"max");
        variable->nominal = (char*)xmlGetProp(variableNode, (xmlChar*)"nominal");
        variable->start = (char*)xmlGetProp(variableNode, (xmlChar*)"start");
        variable->description = (char*)xmlGetProp(variableNode, (xmlChar*)"description");

        FMIVariableType type;
        
        const char* name = (char*)variableNode->name;

        const char* variability = (char*)xmlGetProp(variableNode, (xmlChar*)"variability");

        if (!variability) {
            variable->variability = -1;  // infer from type
        } else if (!strcmp(variability, "constant")) {
            variable->variability = FMIConstant;
        } else if (!strcmp(variability, "fixed")) {
            variable->variability = FMIFixed;
        } else if (!strcmp(variability, "tunable")) {
            variable->variability = FMITunable;
        } else if (!strcmp(variability, "discrete")) {
            variable->variability = FMIDiscrete;
        } else {
            variable->variability = FMIContinuous;
        }

        xmlFree((void*)variability);

        if (!strcmp(name, "Float32")) {
            switch (variable->variability) {
            case -1:
            case FMIContinuous:
                type = FMIFloat32Type;
                break;
            default:
                type = FMIDiscreteFloat32Type;
                break;
            }
        } else if (!strcmp(name, "Float64")) {
            switch (variable->variability) {
            case -1:
            case FMIContinuous:
                type = FMIFloat64Type;
                break;
            default:
                type = FMIDiscreteFloat64Type;
                break;
            }
        } else if (!strcmp(name, "Int8")) {
            type = FMIInt8Type;
        } else if (!strcmp(name, "UInt8")) {
            type = FMIUInt8Type;
        } else if (!strcmp(name, "Int16")) {
            type = FMIInt16Type;
        } else if (!strcmp(name, "UInt16")) {
            type = FMIUInt16Type;
        } else if (!strcmp(name, "Int32")) {
            type = FMIInt32Type;
        } else if (!strcmp(name, "UInt32")) {
            type = FMIUInt32Type;
        } else if (!strcmp(name, "Int64") || !strcmp(name, "Enumeration")) {
            type = FMIInt64Type;
        } else if (!strcmp(name, "UInt64")) {
            type = FMIUInt64Type;
        } else if (!strcmp(name, "Boolean")) {
            type = FMIBooleanType;
        } else if (!strcmp(name, "String")) {
            type = FMIStringType;
        } else if (!strcmp(name, "Binary")) {
            type = FMIBinaryType;
        } else if (!strcmp(name, "Clock")) {
            type = FMIClockType;
        } else {
            return NULL;
        }

        variable->type = type;

        if (variable->variability == -1) {
            switch (variable->type) {
            case FMIFloat32Type:
            case FMIFloat64Type:
                variable->variability = FMIContinuous;
                break;
            default:
                variable->variability = FMIDiscrete;
                break;
            }
        }

        const char* vr = (char*)xmlGetProp(variableNode, (xmlChar*)"valueReference");

        variable->valueReference = FMIValueReferenceForLiteral(vr);

        xmlFree((void*)vr);

        const char* causality = (char*)xmlGetProp(variableNode, (xmlChar*)"causality");

        if (!causality) {
            variable->causality = FMILocal;
        } else if (!strcmp(causality, "parameter")) {
            variable->causality = FMIParameter;
        } else if (!strcmp(causality, "calculatedParameter")) {
            variable->causality = FMICalculatedParameter;
        } else if (!strcmp(causality, "structuralParameter")) {
            variable->causality = FMIStructuralParameter;
        } else if (!strcmp(causality, "input")) {
            variable->causality = FMIInput;
        } else if (!strcmp(causality, "output")) {
            variable->causality = FMIOutput;
        } else if (!strcmp(causality, "independent")) {
            variable->causality = FMIIndependent;
        } else {
            variable->causality = FMILocal;
        }

        xmlFree((void*)causality);

        variable->derivative = (FMIModelVariable*)xmlGetProp(variableNode, (xmlChar*)"derivative");

        xmlXPathObjectPtr xpathObj2 = xmlXPathNodeEval(variableNode, (xmlChar*)".//Dimension", xpathCtx);

        for (size_t j = 0; j < xpathObj2->nodesetval->nodeNr; j++) {

            const xmlNodePtr dimensionNode = xpathObj2->nodesetval->nodeTab[j];

            const char* start = (char*)xmlGetProp(dimensionNode, (xmlChar*)"start");
            const char* valueReference = (char*)xmlGetProp(dimensionNode, (xmlChar*)"valueReference");

            variable->dimensions = realloc(variable->dimensions, (variable->nDimensions + 1) * sizeof(FMIDimension));

            if (!variable->dimensions) {
                return NULL;
            }

            FMIDimension* dimension = &variable->dimensions[variable->nDimensions];

            dimension->start = 0;
            dimension->variable = NULL;

            if (start) {
                dimension->start = atoi(start);
            } else if (valueReference) {
                const FMIValueReference vr = atoi(valueReference);
                dimension->variable = FMIModelVariableForValueReference(modelDescription, vr);
            } else {
                FMILogError("Dimension must have start or valueReference.\n");
                return NULL;
            }

            variable->nDimensions++;
        }

        xmlChar* declaredType = xmlGetProp(variableNode, (xmlChar*)"declaredType");

        variable->declaredType = FMITypeDefinitionForName(modelDescription, (char*)declaredType);

        xmlFree(declaredType);

        xmlXPathFreeObject(xpathObj2);
    }

    xmlXPathFreeObject(xpathObj);

    readUnknownsFMI3(xpathCtx, modelDescription, "/fmiModelDescription/ModelStructure/Output", &modelDescription->nOutputs, &modelDescription->outputs);
    readUnknownsFMI3(xpathCtx, modelDescription, "/fmiModelDescription/ModelStructure/ContinuousStateDerivative", &modelDescription->nContinuousStates, &modelDescription->derivatives);
    readUnknownsFMI3(xpathCtx, modelDescription, "/fmiModelDescription/ModelStructure/InitialUnknown", &modelDescription->nInitialUnknowns, &modelDescription->initialUnknowns);
    readUnknownsFMI3(xpathCtx, modelDescription, "/fmiModelDescription/ModelStructure/EventIndicator", &modelDescription->nEventIndicators, &modelDescription->eventIndicators);

    xmlXPathFreeContext(xpathCtx);

    size_t nProblems = 0;

    // check variabilities
    for (size_t i = 0; i < modelDescription->nModelVariables; i++) {

        FMIModelVariable* variable = &modelDescription->modelVariables[i];
        
        if (variable->type != FMIFloat32Type && variable->type != FMIFloat64Type && variable->variability == FMIContinuous) {
            FMILogError("Variable \"%s\" is not of type Float{32|64} but has variability = continuous.\n", variable->name);
            nProblems++;
        }
    }

    // resolve derivatives
    for (size_t i = 0; i < modelDescription->nModelVariables; i++) {

        FMIModelVariable* variable = &modelDescription->modelVariables[i];
        
        if (variable->derivative) {
            char* literal = (char*)variable->derivative;
            const FMIValueReference vr = FMIValueReferenceForLiteral(literal);
            variable->derivative = FMIModelVariableForValueReference(modelDescription, vr);
            if (!variable->derivative) {
                nProblems++;
                FMILogError("Failed to resolve attribute derivative=\"%s\" for model variable \"%s\".\n", literal, variable->name);
            }
            xmlFree(literal);
        }

    }

    nProblems += FMIValidateModelDescription(modelDescription);

    if (nProblems > 0) {
        FMIFreeModelDescription(modelDescription);
        modelDescription = NULL;
    }

TERMINATE:

    // TODO

    return modelDescription;
}

static void logSchemaValidationError(void* ctx, const char* msg, ...) {
    (void)ctx; // unused
    va_list args;
    va_start(args, msg);
    logErrorMessage(msg, args);
    va_end(args);
}

FMIModelDescription* FMIReadModelDescription(const char* filename) {

    // TODO: add stream interface (see https://gitlab.gnome.org/GNOME/libxml2/-/wikis/Parser-interfaces)

    xmlDocPtr doc = NULL;
    xmlNodePtr root = NULL;
    xmlSchemaPtr schema = NULL;
    xmlSchemaParserCtxtPtr pctxt = NULL;
    xmlSchemaValidCtxtPtr vctxt = NULL;
    FMIModelDescription* modelDescription = NULL;
    const char* version = NULL;
    FMIVersion fmiVersion;

    doc = xmlParseFile(filename);

    // xmlKeepBlanksDefault(0);
    // xmlDocDump(stdout, doc);

    if (!doc) {
        FMILogError("Invalid XML.\n");
        goto TERMINATE;
    }

    root = xmlDocGetRootElement(doc);

    if (root == NULL) {
        FMILogError("Empty document\n");
        goto TERMINATE;
    }

    version = (char*)xmlGetProp(root, (xmlChar*)"fmiVersion");

    if (!version) {
        FMILogError("Attribute fmiVersion is missing.\n");
        goto TERMINATE;
    } else if (!strcmp(version, "1.0")) {
        fmiVersion = FMIVersion1;
        pctxt = xmlSchemaNewMemParserCtxt((char*)fmi1Merged_xsd, fmi1Merged_xsd_len);
    } else if (!strcmp(version, "2.0")) {
        fmiVersion = FMIVersion2;
        pctxt = xmlSchemaNewMemParserCtxt((char*)fmi2Merged_xsd, fmi2Merged_xsd_len);
    } else if(!strncmp(version, "3.", 2)) {
        pctxt = xmlSchemaNewMemParserCtxt((char*)fmi3Merged_xsd, fmi3Merged_xsd_len);
        fmiVersion = FMIVersion3;
    } else {
        FMILogError("Unsupported FMI version: %s.\n", version);
        goto TERMINATE;
    }

    schema = xmlSchemaParse(pctxt);

    if (schema == NULL) {
        goto TERMINATE;
    }

    vctxt = xmlSchemaNewValidCtxt(schema);

    if (!vctxt) {
        goto TERMINATE;
    }

    xmlSchemaSetValidErrors(vctxt, (xmlSchemaValidityErrorFunc)logSchemaValidationError, NULL, NULL);

    if (xmlSchemaValidateDoc(vctxt, doc)) {
        goto TERMINATE;
    }

    if (fmiVersion == FMIVersion1) {
        modelDescription = readModelDescriptionFMI1(root);
    } else if (fmiVersion == FMIVersion2) {
        modelDescription = readModelDescriptionFMI2(root);
    } else {
        modelDescription = readModelDescriptionFMI3(root);
    }

TERMINATE:

    if (vctxt) {
        xmlSchemaFreeValidCtxt(vctxt);
    }
    
    if (schema) {
        xmlSchemaFree(schema);
    }

    if (pctxt) {
        xmlSchemaFreeParserCtxt(pctxt);
    }

    if (doc) {
        xmlFreeDoc(doc);
    }

    return modelDescription;
}

void FMIFreeModelDescription(FMIModelDescription* modelDescription) {

    if (!modelDescription) {
        return;
    }

    xmlFree((void*)modelDescription->modelName);
    xmlFree((void*)modelDescription->instantiationToken);
    xmlFree((void*)modelDescription->description);
    xmlFree((void*)modelDescription->generationTool);
    xmlFree((void*)modelDescription->generationDate);

    if (modelDescription->modelExchange) {
        xmlFree((void*)modelDescription->modelExchange->modelIdentifier);
        free(modelDescription->modelExchange);
    }

    if (modelDescription->coSimulation) {
        xmlFree((void*)modelDescription->coSimulation->modelIdentifier);
        free(modelDescription->coSimulation);
    }

    if (modelDescription->defaultExperiment) {
        xmlFree((void*)modelDescription->defaultExperiment->startTime);
        xmlFree((void*)modelDescription->defaultExperiment->stopTime);
        xmlFree((void*)modelDescription->defaultExperiment->stepSize);
        free(modelDescription->defaultExperiment);
    }

    for (size_t i = 0; i < modelDescription->nModelVariables; i++) {
        FMIModelVariable* variable = &modelDescription->modelVariables[i];
        xmlFree((void*)variable->name);
        xmlFree((void*)variable->start);
        xmlFree((void*)variable->description);
    }

    free(modelDescription->modelVariables);

    free(modelDescription);
}

FMIValueReference FMIValueReferenceForLiteral(const char* literal) {
    return (FMIValueReference)strtoul(literal, NULL, 0);
}

FMITypeDefinition* FMITypeDefinitionForName(const FMIModelDescription* modelDescription, const char* name) {

    if (!name) {
        return NULL;
    }

    for (size_t i = 0; i < modelDescription->nTypeDefinitions; i++) {

        FMITypeDefinition* typeDefinition = &modelDescription->typeDefinitions[i];

        if (!strcmp(typeDefinition->name, name)) {
            return typeDefinition;
        }
    }

    return NULL;
}

FMIUnit* FMIUnitForName(const FMIModelDescription* modelDescription, const char* name) {

    if (!name) {
        return NULL;
    }

    for (size_t i = 0; i < modelDescription->nUnitDefinitions; i++) {

        FMIUnit* unit = &modelDescription->unitDefinitions[i];

        if (!strcmp(unit->name, name)) {
            return unit;
        }
    }

    return NULL;
}

FMIModelVariable* FMIModelVariableForName(const FMIModelDescription* modelDescription, const char* name) {

    for (size_t i = 0; i < modelDescription->nModelVariables; i++) {

        FMIModelVariable* variable = &modelDescription->modelVariables[i];
        
        if (!strcmp(variable->name, name)) {
            return variable;
        }
    }

    return NULL;
}

FMIModelVariable* FMIModelVariableForValueReference(const FMIModelDescription* modelDescription, FMIValueReference valueReference) {

    for (size_t i = 0; i < modelDescription->nModelVariables; i++) {

        FMIModelVariable* variable = &modelDescription->modelVariables[i];

        if (variable->valueReference == valueReference) {
            return variable;
        }
    }

    return NULL;
}

FMIModelVariable* FMIModelVariableForIndexLiteral(const FMIModelDescription* modelDescription, const char* index) {

    const size_t i = strtoul(index, NULL, 0);

    if (i == 0 || i > modelDescription->nModelVariables) {
        return NULL;
    }

    return &modelDescription->modelVariables[i - 1];
}

static bool isLegalCombination(FMIModelVariable* variable) {

    //FMICausality causality, FMIVariability variability, FMIInitial initial;

    if (variable->causality == FMIStructuralParameter || variable->causality == FMIParameter) {
        
        if (variable->variability == FMIFixed || variable->variability == FMITunable) {

            if (variable->initial == FMIUndefined) {
                variable->initial = FMIExact;
            }
        
            if (variable->initial == FMIExact) {
                return true;
            }

        }

    } else if (variable->causality == FMICalculatedParameter) {
        
        if (variable->variability == FMIFixed || variable->variability == FMITunable) {

            if (variable->initial == FMIUndefined) {
                variable->initial = FMICalculated;
            }
        
            if (variable->initial == FMICalculated || variable->initial == FMIApprox) {
                return true;
            }

        }

    } else if (variable->causality == FMIInput) {
        
        if (variable->variability == FMIDiscrete || variable->variability == FMIContinuous) {
        
            if (variable->initial == FMIUndefined) {
                variable->initial = FMIExact;
            }

            if (variable->initial == FMIExact) {
                return true;
            }

        }

    } else if (variable->causality == FMIOutput) {

        if (variable->variability == FMIConstant) {

            if (variable->initial == FMIUndefined) {
                variable->initial = FMIExact;
            }

            if (variable->initial == FMIExact) {
                return true;
            }

        } else if (variable->variability == FMIDiscrete || variable->variability == FMIContinuous) {

            if (variable->initial == FMIUndefined) {
                variable->initial = FMICalculated;
            }

            if (variable->initial == FMICalculated || variable->initial == FMIExact || variable->initial == FMIApprox) {
                return true;
            }

        }

    } else if (variable->causality == FMILocal) {

        if (variable->variability == FMIConstant) {

            if (variable->initial == FMIUndefined) {
                variable->initial = FMIExact;
            }

            if (variable->initial == FMIExact) {
                return true;
            }

        } else if (variable->variability == FMIFixed || variable->variability == FMITunable) {

            if (variable->initial == FMIUndefined) {
                variable->initial = FMICalculated;
            }

            if (variable->initial == FMICalculated || variable->initial == FMIApprox) {
                return true;
            }

        } else if (variable->variability == FMIDiscrete || variable->variability == FMIContinuous) {

            if (variable->initial == FMIUndefined) {
                variable->initial = FMICalculated;
            }

            if (variable->initial == FMICalculated || variable->initial == FMIExact || variable->initial == FMIApprox) {
                return true;
            }

        }

    } else if (variable->causality == FMIIndependent) {

        if (variable->variability == FMIContinuous) {

            if (variable->initial == FMIUndefined) {
                return true;
            }

        }

    }

    return false;
}

void set_input_string(const char* in);

void end_lexical_scan(void);

void yyerror(const FMIModelVariable* variable, const char* s) {
    FMILogError("\"%s\" (line %d) is not a valid variable name for variableNamingConvention=\"structured\".\n", variable->name, variable->line);
}

size_t FMIValidateModelDescription(const FMIModelDescription* modelDescription) {

    size_t nProblems = 0;

    // validate structured variable names
    if (modelDescription->variableNamingConvention == FMIStructured) {

        for (size_t i = 0; i < modelDescription->nModelVariables; i++) {

            const FMIModelVariable* variable = &modelDescription->modelVariables[i];

            set_input_string(variable->name);

            if (yyparse((void*)variable)) {
                nProblems++;
            }

            end_lexical_scan();
        }
    }

    // check combinations of causality, variability, and initial
    for (size_t i = 0; i < modelDescription->nModelVariables; i++) {

        FMIModelVariable* variable = &modelDescription->modelVariables[i];

        if (!isLegalCombination(variable)) {
            nProblems++;
            FMILogError("The variable \"%s\" has an illegal combination of causality, variability, and initial.\n", variable->name);
        }
    }

    size_t nOutputs = 0;

    for (size_t i = 0; i < modelDescription->nModelVariables; i++) {

        FMIModelVariable* variable = &modelDescription->modelVariables[i];

        if (variable->causality == FMIOutput) {
            nOutputs++;
        }
    }

    if (nOutputs != modelDescription->nOutputs) {
        nProblems++;
        FMILogError("The number of model varialbes with causality=\"output\" (%zu) must match the number of outputs"
            " in the model structure (%zu).\n", nOutputs, modelDescription->nContinuousStates);
    }

    return nProblems;
}

void FMIDumpModelDescription(FMIModelDescription* modelDescription, FILE* file) {

    fprintf(file, "FMI Version        3.0\n");
    fprintf(file, "FMI Type           Co-Simulation\n");
    fprintf(file, "Model Name         %s\n", modelDescription->modelName);
    fprintf(file, "Description        %s\n", modelDescription->description ? modelDescription->description : "n/a");
    fprintf(file, "Continuous States  %zu\n", modelDescription->nContinuousStates);
    fprintf(file, "Event Indicators   %zu\n", modelDescription->nEventIndicators);
    fprintf(file, "Generation Tool    %s\n", modelDescription->generationTool ? modelDescription->generationTool : "n/a");
    fprintf(file, "Generation Date    %s\n", modelDescription->generationDate ? modelDescription->generationDate : "n/a");
    fprintf(file, "\n");
    fprintf(file, "Model Variables\n");
    fprintf(file, "\n");
    fprintf(file, "Name                           Description\n");
    fprintf(file, "\n");
    for (size_t i = 0; i < modelDescription->nModelVariables; i++) {
        fprintf(file, "%-30s %s\n", modelDescription->modelVariables[i].name, modelDescription->modelVariables[i].description);
    }

}
