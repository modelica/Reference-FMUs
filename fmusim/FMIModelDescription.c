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

#include "calc.tab.h"

#include "fmi1schema.h"
#include "fmi2schema.h"
#include "fmi3schema.h"


static bool getBooleanAttribute(const xmlNodePtr node, const char* name) {
    char* literal = (char*)xmlGetProp(node, (xmlChar*)name);
    bool value = literal && (strcmp(literal, "true") == 0 || strcmp(literal, "1") == 0);
    free(literal);
    return value;
}

static uint32_t getUInt32Attribute(const xmlNodePtr node, const char* name) {
    char* literal = (char*)xmlGetProp(node, (xmlChar*)name);
    uint32_t value = strtoul(literal, NULL, 0);
    free(literal);
    return value;
}

static FMIVariableNamingConvention getVariableNamingConvention(const xmlNodePtr node) {
    const char* value = (char*)xmlGetProp(node, (xmlChar*)"variableNamingConvention");
    FMIVariableNamingConvention variableNamingConvention = (value && !strcmp(value, "structured")) ? FMIStructured : FMIFlat;
    free(value);
    return variableNamingConvention;
}

static FMIModelDescription* readModelDescriptionFMI1(xmlNodePtr root) {

    FMIModelDescription* modelDescription = (FMIModelDescription*)calloc(1, sizeof(FMIModelDescription));

    if (!modelDescription) {
        return NULL;
    }

    modelDescription->fmiVersion = FMIVersion1;
    modelDescription->modelName = (char*)xmlGetProp(root, (xmlChar*)"modelName");
    modelDescription->instantiationToken = (char*)xmlGetProp(root, (xmlChar*)"guid");
    modelDescription->description = (char*)xmlGetProp(root, (xmlChar*)"description");
    modelDescription->generationTool = (char*)xmlGetProp(root, (xmlChar*)"generationTool");
    modelDescription->generationDate = (char*)xmlGetProp(root, (xmlChar*)"generationDateAndTime");
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
        modelDescription->coSimulation = (FMICoSimulationInterface*)calloc(1, sizeof(FMICoSimulationInterface));
        modelDescription->coSimulation->modelIdentifier = (char*)xmlGetProp(root, (xmlChar*)"modelIdentifier");
    } else {
        modelDescription->modelExchange = (FMIModelExchangeInterface*)calloc(1, sizeof(FMIModelExchangeInterface));
        modelDescription->modelExchange->modelIdentifier = (char*)xmlGetProp(root, (xmlChar*)"modelIdentifier");
    }
    
    xmlXPathFreeObject(xpathObj);

    xpathObj = xmlXPathEvalExpression((xmlChar*)"/fmiModelDescription/DefaultExperiment", xpathCtx);
    
    if (xpathObj->nodesetval->nodeNr == 1) {
        modelDescription->defaultExperiment = (FMIDefaultExperiment*)calloc(1, sizeof(FMIDefaultExperiment));
        const xmlNodePtr node = xpathObj->nodesetval->nodeTab[0];
        modelDescription->defaultExperiment->startTime = (char*)xmlGetProp(node, (xmlChar*)"startTime");
        modelDescription->defaultExperiment->stopTime = (char*)xmlGetProp(node, (xmlChar*)"stopTime");
    }
    
    xmlXPathFreeObject(xpathObj);

    xpathObj = xmlXPathEvalExpression((xmlChar*)"/fmiModelDescription/ModelVariables/ScalarVariable/*[self::Real or self::Integer or self::Enumeration or self::Boolean or self::String]", xpathCtx);

    modelDescription->nModelVariables = xpathObj->nodesetval->nodeNr;
    modelDescription->modelVariables = calloc(xpathObj->nodesetval->nodeNr, sizeof(FMIModelVariable));

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
        FMIModelVariable* variable = &modelDescription->modelVariables[i];
        if (variable->type != FMIRealType && variable->type != FMIDiscreteRealType && variable->variability == FMIContinuous) {
            printf("Variable \"%s\" is not of type Real but has variability = continuous.\n", variable->name);
            nProblems++;
        }
    }

    xmlXPathFreeObject(xpathObj);

    xmlXPathFreeContext(xpathCtx);

    nProblems += FMIValidateVariableNames(modelDescription);

    if (nProblems > 0) {
        FMIFreeModelDescription(modelDescription);
        modelDescription = NULL;
    }

    return modelDescription;

    return modelDescription;
}

static void readUnknownsFMI2(xmlXPathContextPtr xpathCtx, FMIModelDescription* modelDescription, const char* path, size_t* nUnkonwns, FMIUnknown** unknowns) {

    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression((xmlChar*)path, xpathCtx);
    
    *nUnkonwns = xpathObj->nodesetval->nodeNr;
    *unknowns = calloc(*nUnkonwns, sizeof(FMIUnknown));

    for (size_t i = 0; i < xpathObj->nodesetval->nodeNr; i++) {

        xmlNodePtr unkownNode = xpathObj->nodesetval->nodeTab[i];

        const char* indexLiteral = (char*)xmlGetProp(unkownNode, (xmlChar*)"index");

        (*unknowns)[i].modelVariable = FMIModelVariableForIndexLiteral(modelDescription, indexLiteral);

        free(indexLiteral);
    }

    xmlXPathFreeObject(xpathObj);
}

static void readUnknownsFMI3(xmlXPathContextPtr xpathCtx, FMIModelDescription* modelDescription, const char* path, size_t* nUnkonwns, FMIUnknown** unknowns) {

    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression((xmlChar*)path, xpathCtx);

    *nUnkonwns = xpathObj->nodesetval->nodeNr;
    *unknowns = calloc(*nUnkonwns, sizeof(FMIUnknown));

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
}

static FMIModelDescription* readModelDescriptionFMI2(xmlNodePtr root) {

    FMIModelDescription* modelDescription = (FMIModelDescription*)calloc(1, sizeof(FMIModelDescription));

    if (!modelDescription) {
        return NULL;
    }

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
        modelDescription->coSimulation = (FMICoSimulationInterface*)calloc(1, sizeof(FMICoSimulationInterface));
        modelDescription->coSimulation->modelIdentifier = (char*)xmlGetProp(xpathObj->nodesetval->nodeTab[0], (xmlChar*)"modelIdentifier");
    }
    xmlXPathFreeObject(xpathObj);

    xpathObj = xmlXPathEvalExpression((xmlChar*)"/fmiModelDescription/ModelExchange", xpathCtx);
    if (xpathObj->nodesetval->nodeNr == 1) {
        modelDescription->modelExchange = (FMIModelExchangeInterface*)calloc(1, sizeof(FMIModelExchangeInterface));
        modelDescription->modelExchange->modelIdentifier = (char*)xmlGetProp(xpathObj->nodesetval->nodeTab[0], (xmlChar*)"modelIdentifier");
        modelDescription->modelExchange->providesDirectionalDerivatives = getBooleanAttribute(xpathObj->nodesetval->nodeTab[0], "providesDirectionalDerivative");
    }
    xmlXPathFreeObject(xpathObj);

    xpathObj = xmlXPathEvalExpression((xmlChar*)"/fmiModelDescription/DefaultExperiment", xpathCtx);
    if (xpathObj->nodesetval->nodeNr == 1) {
        modelDescription->defaultExperiment = (FMIDefaultExperiment*)calloc(1, sizeof(FMIDefaultExperiment));
        const xmlNodePtr node = xpathObj->nodesetval->nodeTab[0];
        modelDescription->defaultExperiment->startTime = (char*)xmlGetProp(node, (xmlChar*)"startTime");
        modelDescription->defaultExperiment->stopTime = (char*)xmlGetProp(node, (xmlChar*)"stopTime");
        modelDescription->defaultExperiment->stepSize = (char*)xmlGetProp(node, (xmlChar*)"stepSize");
    }
    xmlXPathFreeObject(xpathObj);

    xpathObj = xmlXPathEvalExpression((xmlChar*)"/fmiModelDescription/ModelVariables/ScalarVariable/*[self::Real or self::Integer or self::Enumeration or self::Boolean or self::String]", xpathCtx);

    modelDescription->nModelVariables = xpathObj->nodesetval->nodeNr;
    modelDescription->modelVariables = calloc(modelDescription->nModelVariables, sizeof(FMIModelVariable));

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

        free(vr);
        
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

        free(causality);
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
                printf("Failed to resolve attribute derivative=\"%s\" for model variable \"%s\".", literal, variable->name);
                nProblems++;
            }
            free(literal);
        }
    }

    // check variabilities
    for (size_t i = 0; i < modelDescription->nModelVariables; i++) {
        FMIModelVariable* variable = &modelDescription->modelVariables[i];
        if (variable->type != FMIRealType && variable->type != FMIDiscreteRealType && variable->variability == FMIContinuous) {
            printf("Variable \"%s\" is not of type Real but has variability = continuous.\n", variable->name);
            nProblems++;
        }
    }

    nProblems += FMIValidateModelStructure(modelDescription);

    nProblems += FMIValidateVariableNames(modelDescription);

    if (nProblems > 0) {
        FMIFreeModelDescription(modelDescription);
        modelDescription = NULL;
    }

    return modelDescription;
}

static FMIModelDescription* readModelDescriptionFMI3(xmlNodePtr root) {

    FMIModelDescription* modelDescription = (FMIModelDescription*)calloc(1, sizeof(FMIModelDescription));

    if (!modelDescription) {
        return NULL;
    }

    modelDescription->fmiVersion = FMIVersion3;
    modelDescription->modelName = (char*)xmlGetProp(root, (xmlChar*)"modelName");
    modelDescription->instantiationToken = (char*)xmlGetProp(root, (xmlChar*)"instantiationToken");
    modelDescription->description = (char*)xmlGetProp(root, (xmlChar*)"description");
    modelDescription->generationTool = (char*)xmlGetProp(root, (xmlChar*)"generationTool");
    modelDescription->generationDate = (char*)xmlGetProp(root, (xmlChar*)"generationDate");
    modelDescription->variableNamingConvention = getVariableNamingConvention(root);

    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(root->doc);

    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression((xmlChar*)"/fmiModelDescription/CoSimulation", xpathCtx);
    if (xpathObj->nodesetval->nodeNr == 1) {
        modelDescription->coSimulation = (FMICoSimulationInterface*)calloc(1, sizeof(FMICoSimulationInterface));
        modelDescription->coSimulation->modelIdentifier = (char*)xmlGetProp(xpathObj->nodesetval->nodeTab[0], (xmlChar*)"modelIdentifier");
    }
    xmlXPathFreeObject(xpathObj);

    xpathObj = xmlXPathEvalExpression((xmlChar*)"/fmiModelDescription/ModelExchange", xpathCtx);
    if (xpathObj->nodesetval->nodeNr == 1) {
        xmlNodePtr node = xpathObj->nodesetval->nodeTab[0];
        modelDescription->modelExchange = (FMIModelExchangeInterface*)calloc(1, sizeof(FMIModelExchangeInterface));
        modelDescription->modelExchange->modelIdentifier = (char*)xmlGetProp(node, (xmlChar*)"modelIdentifier");
        modelDescription->modelExchange->providesDirectionalDerivatives = getBooleanAttribute(node, "providesDirectionalDerivatives");
    }
    xmlXPathFreeObject(xpathObj);

    xpathObj = xmlXPathEvalExpression((xmlChar*)"/fmiModelDescription/DefaultExperiment", xpathCtx);
    if (xpathObj->nodesetval->nodeNr == 1) {
        modelDescription->defaultExperiment = (FMIDefaultExperiment*)calloc(1, sizeof(FMIDefaultExperiment));
        const xmlNodePtr node = xpathObj->nodesetval->nodeTab[0];
        modelDescription->defaultExperiment->startTime = (char*)xmlGetProp(node, (xmlChar*)"startTime");
        modelDescription->defaultExperiment->stopTime = (char*)xmlGetProp(node, (xmlChar*)"stopTime");
        modelDescription->defaultExperiment->stepSize = (char*)xmlGetProp(node, (xmlChar*)"stepSize");
    }
    xmlXPathFreeObject(xpathObj);

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
    modelDescription->modelVariables = (FMIModelVariable*)calloc(xpathObj->nodesetval->nodeNr, sizeof(FMIModelVariable));

    for (size_t i = 0; i < xpathObj->nodesetval->nodeNr; i++) {

        FMIModelVariable* variable = &modelDescription->modelVariables[i];

        xmlNodePtr variableNode = xpathObj->nodesetval->nodeTab[i];

        variable->line = variableNode->line;
        variable->name = (char*)xmlGetProp(variableNode, (xmlChar*)"name");
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

        free((void*)variability);

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

        free(vr);

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

        free(causality);

        variable->derivative = (FMIModelVariable*)xmlGetProp(variableNode, (xmlChar*)"derivative");

        xmlXPathObjectPtr xpathObj2 = xmlXPathNodeEval(variableNode, ".//Dimension", xpathCtx);

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
                printf("Dimension must have start or valueReference.\n");
                return NULL;
            }

            variable->nDimensions++;
        }

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
            printf("Variable \"%s\" is not of type Float{32|64} but has variability = continuous.\n", variable->name);
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
                printf("Failed to resolve attribute derivative=\"%s\" for model variable \"%s\".\n", literal, variable->name);
            }
            free(literal);
        }

    }

    nProblems += FMIValidateModelStructure(modelDescription);

    nProblems += FMIValidateVariableNames(modelDescription);

    if (nProblems > 0) {
        FMIFreeModelDescription(modelDescription);
        modelDescription = NULL;
    }

    return modelDescription;
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
        printf("Invalid XML.\n");
        goto TERMINATE;
    }

    root = xmlDocGetRootElement(doc);

    if (root == NULL) {
        printf("Empty document\n");
        goto TERMINATE;
    }

    version = (char*)xmlGetProp(root, (xmlChar*)"fmiVersion");

    if (!version) {
        printf("Attribute fmiVersion is missing.\n");
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
        printf("Unsupported FMI version: %s.\n", version);
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

    xmlSchemaSetValidErrors(vctxt, (xmlSchemaValidityErrorFunc)fprintf, (xmlSchemaValidityWarningFunc)fprintf, stderr);

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

    free((void*)modelDescription->modelName);
    free((void*)modelDescription->instantiationToken);
    free((void*)modelDescription->description);
    free((void*)modelDescription->generationTool);
    free((void*)modelDescription->generationDate);

    if (modelDescription->modelExchange) {
        free((void*)modelDescription->modelExchange->modelIdentifier);
        free(modelDescription->modelExchange);
    }

    if (modelDescription->coSimulation) {
        free((void*)modelDescription->coSimulation->modelIdentifier);
        free(modelDescription->coSimulation);
    }

    if (modelDescription->defaultExperiment) {
        free((void*)modelDescription->defaultExperiment->startTime);
        free((void*)modelDescription->defaultExperiment->stopTime);
        free((void*)modelDescription->defaultExperiment->stepSize);
        free(modelDescription->defaultExperiment);
    }

    for (size_t i = 0; i < modelDescription->nModelVariables; i++) {
        FMIModelVariable* variable = &modelDescription->modelVariables[i];
        free((void*)variable->name);
        free((void*)variable->start);
        free((void*)variable->description);
    }

    free(modelDescription->modelVariables);

    free(modelDescription);
}

FMIValueReference FMIValueReferenceForLiteral(const char* literal) {
    return (FMIValueReference)strtoul(literal, NULL, 0);
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

size_t FMIValidateModelStructure(const FMIModelDescription* modelDescription) {

    size_t nProblems = 0;
    size_t nOutputs = 0;

    for (size_t i = 0; i < modelDescription->nModelVariables; i++) {

        FMIModelVariable* variable = &modelDescription->modelVariables[i];

        if (variable->causality == FMIOutput) {
            nOutputs++;
        }
    }

    if (nOutputs != modelDescription->nOutputs) {
        nProblems++;
        printf("The number of model varialbes with causality=\"output\" (%zu) must match the number of outputs"
            " in the model structure (%zu).\n", nOutputs, modelDescription->nContinuousStates);
    }

    return nProblems;
}

/* Declarations */
void set_input_string(const char* in);
void end_lexical_scan(void);

void yyerror(char* name, const char* s) {
    printf("\"%s\" is not a valid variable name for variableNamingConvention=\"structured\".\n", name);
}

size_t FMIValidateVariableNames(const FMIModelDescription* modelDescription) {

    size_t nProblems = 0;

    if (modelDescription->variableNamingConvention == FMIStructured) {

        for (size_t i = 0; i < modelDescription->nModelVariables; i++) {

            set_input_string(modelDescription->modelVariables[i].name);
            
            if (yyparse(modelDescription->modelVariables[i].name)) {
                nProblems++;
            }
            
            end_lexical_scan();
        }
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
