#include "FMIModelDescription.h"

#include <string.h>

#include <libxml/parser.h>
#include <libxml/xmlschemas.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#include "fmi2schema.h"
#include "fmi3schema.h"

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

    xpathObj = xmlXPathEvalExpression((xmlChar*)"/fmiModelDescription/ModelExchange", xpathCtx);

    if (xpathObj->nodesetval->nodeNr == 1) {
        modelDescription->modelExchange = (FMIModelExchangeInterface*)calloc(1, sizeof(FMIModelExchangeInterface));
        modelDescription->modelExchange->modelIdentifier = (char*)xmlGetProp(xpathObj->nodesetval->nodeTab[0], (xmlChar*)"modelIdentifier");
    }

    xpathObj = xmlXPathEvalExpression((xmlChar*)"/fmiModelDescription/DefaultExperiment", xpathCtx);

    if (xpathObj->nodesetval->nodeNr == 1) {
        modelDescription->defaultExperiment = (FMIDefaultExperiment*)calloc(1, sizeof(FMIDefaultExperiment));
        const xmlNodePtr node = xpathObj->nodesetval->nodeTab[0];
        modelDescription->defaultExperiment->startTime = (char*)xmlGetProp(node, (xmlChar*)"startTime");
        modelDescription->defaultExperiment->stopTime = (char*)xmlGetProp(node, (xmlChar*)"stopTime");
        modelDescription->defaultExperiment->stepSize = (char*)xmlGetProp(node, (xmlChar*)"stepSize");
    }

    xpathObj = xmlXPathEvalExpression((xmlChar*)"/fmiModelDescription/ModelVariables/ScalarVariable/*[self::Real or self::Integer or self::Enumeration or self::Boolean or self::String]", xpathCtx);

    modelDescription->nModelVariables = xpathObj->nodesetval->nodeNr;
    modelDescription->modelVariables = calloc(xpathObj->nodesetval->nodeNr, sizeof(FMIModelVariable));

    for (size_t i = 0; i < xpathObj->nodesetval->nodeNr; i++) {

        xmlNodePtr typeNode = xpathObj->nodesetval->nodeTab[i];
        xmlNodePtr variableNode = typeNode->parent;

        modelDescription->modelVariables[i].name = (char*)xmlGetProp(variableNode, (xmlChar*)"name");
        modelDescription->modelVariables[i].description = (char*)xmlGetProp(variableNode, (xmlChar*)"description");

        FMIVariableType type;
        const char* typeName = (char*)typeNode->name;

        if (!strcmp(typeName, "Real")) {
            type = FMIRealType;
        } else if (!strcmp(typeName, "Integer") || !strcmp(typeName, "Enumeration")) {
            type = FMIIntegerType;
        } else if (!strcmp(typeName, "Boolean")) {
            type = FMIBooleanType;
        } else if (!strcmp(typeName, "String")) {
            type = FMIStringType;
        } else {
            continue;
        }

        modelDescription->modelVariables[i].type = type;

        const char* vr = (char*)xmlGetProp(variableNode, (xmlChar*)"valueReference");

        modelDescription->modelVariables[i].valueReference = (FMIValueReference)strtoul(vr, NULL, 0);

        const char* causality = (char*)xmlGetProp(variableNode, (xmlChar*)"causality");

        if (!causality) {
            modelDescription->modelVariables[i].causality = FMILocal;
        } else if (!strcmp(causality, "parameter")) {
            modelDescription->modelVariables[i].causality = FMIParameter;
        } else if (!strcmp(causality, "input")) {
            modelDescription->modelVariables[i].causality = FMIInput;
        } else if (!strcmp(causality, "output")) {
            modelDescription->modelVariables[i].causality = FMIOutput;
        } else if (!strcmp(causality, "independent")) {
            modelDescription->modelVariables[i].causality = FMIIndependent;
        } else {
            modelDescription->modelVariables[i].causality = FMILocal;
        }

    }

    xpathObj = xmlXPathEvalExpression((xmlChar*)"/fmiModelDescription/ModelStructure/Outputs/Unknown", xpathCtx);
    modelDescription->nOutputs = xpathObj->nodesetval->nodeNr;

    xpathObj = xmlXPathEvalExpression((xmlChar*)"/fmiModelDescription/ModelStructure/Derivatives/Unknown", xpathCtx);
    modelDescription->nContinuousStates = xpathObj->nodesetval->nodeNr;

    xpathObj = xmlXPathEvalExpression((xmlChar*)"/fmiModelDescription/ModelStructure/InitialUnknowns/Unknown", xpathCtx);
    modelDescription->nInitialUnknowns = xpathObj->nodesetval->nodeNr;

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

    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(root->doc);

    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression((xmlChar*)"/fmiModelDescription/CoSimulation", xpathCtx);

    if (xpathObj->nodesetval->nodeNr == 1) {
        modelDescription->coSimulation = (FMICoSimulationInterface*)calloc(1, sizeof(FMICoSimulationInterface));
        modelDescription->coSimulation->modelIdentifier = (char*)xmlGetProp(xpathObj->nodesetval->nodeTab[0], (xmlChar*)"modelIdentifier");
    }

    xpathObj = xmlXPathEvalExpression((xmlChar*)"/fmiModelDescription/ModelExchange", xpathCtx);

    if (xpathObj->nodesetval->nodeNr == 1) {
        modelDescription->modelExchange = (FMIModelExchangeInterface*)calloc(1, sizeof(FMIModelExchangeInterface));
        modelDescription->modelExchange->modelIdentifier = (char*)xmlGetProp(xpathObj->nodesetval->nodeTab[0], (xmlChar*)"modelIdentifier");
    }

    xpathObj = xmlXPathEvalExpression((xmlChar*)"/fmiModelDescription/DefaultExperiment", xpathCtx);

    if (xpathObj->nodesetval->nodeNr == 1) {
        modelDescription->defaultExperiment = (FMIDefaultExperiment*)calloc(1, sizeof(FMIDefaultExperiment));
        const xmlNodePtr node = xpathObj->nodesetval->nodeTab[0];
        modelDescription->defaultExperiment->startTime = (char*)xmlGetProp(node, (xmlChar*)"startTime");
        modelDescription->defaultExperiment->stopTime = (char*)xmlGetProp(node, (xmlChar*)"stopTime");
        modelDescription->defaultExperiment->stepSize = (char*)xmlGetProp(node, (xmlChar*)"stepSize");
    }

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
        " or self::UInt64"
        " or self::Boolean"
        " or self::String"
        " or self::Binary"
        " or self::Clock]", 
        xpathCtx);

    modelDescription->nModelVariables = xpathObj->nodesetval->nodeNr;
    modelDescription->modelVariables = calloc(xpathObj->nodesetval->nodeNr, sizeof(FMIModelVariable));

    for (size_t i = 0; i < xpathObj->nodesetval->nodeNr; i++) {

        xmlNodePtr node = xpathObj->nodesetval->nodeTab[i];

        modelDescription->modelVariables[i].name = (char*)xmlGetProp(node, (xmlChar*)"name");
        modelDescription->modelVariables[i].description = (char*)xmlGetProp(node, (xmlChar*)"description");

        FMIVariableType type;
        
        const char* name = (char*)node->name;

        if (!strcmp(name, "Float32")) {
            type = FMIFloat32Type;
        } else if (!strcmp(name, "Float64") || !strcmp(name, "Real")) {
            type = FMIFloat64Type;
        } else if (!strcmp(name, "Int8")) {
            type = FMIInt8Type;
        } else if (!strcmp(name, "UInt8")) {
            type = FMIUInt8Type;
        } else if (!strcmp(name, "Int16")) {
            type = FMIInt16Type;
        } else if (!strcmp(name, "UInt16")) {
            type = FMIUInt16Type;
        } else if (!strcmp(name, "Int32") || !strcmp(name, "Integer")) {
            type = FMIInt32Type;
        } else if (!strcmp(name, "UInt32")) {
            type = FMIUInt32Type;
        } else if (!strcmp(name, "Int64")) {
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
        }

        modelDescription->modelVariables[i].type = type;

        const char* vr = (char*)xmlGetProp(node, (xmlChar*)"valueReference");

        modelDescription->modelVariables[i].valueReference = (FMIValueReference)strtoul(vr, NULL, 0);

        const char* causality = (char*)xmlGetProp(node, (xmlChar*)"causality");

        if (!causality) {
            modelDescription->modelVariables[i].causality = FMILocal;
        } else if (!strcmp(causality, "parameter")) {
            modelDescription->modelVariables[i].causality = FMIParameter;
        } else if (!strcmp(causality, "calculatedParameter")) {
            modelDescription->modelVariables[i].causality = FMICalculatedParameter;
        } else if (!strcmp(causality, "structuralParameter")) {
            modelDescription->modelVariables[i].causality = FMIStructuralParameter;
        } else if (!strcmp(causality, "input")) {
            modelDescription->modelVariables[i].causality = FMIInput;
        } else if (!strcmp(causality, "output")) {
            modelDescription->modelVariables[i].causality = FMIOutput;
        } else if (!strcmp(causality, "independent")) {
            modelDescription->modelVariables[i].causality = FMIIndependent;
        } else {
            modelDescription->modelVariables[i].causality = FMILocal;
        }

        xmlXPathObjectPtr xpathObj2 = xmlXPathNodeEval(node, ".//Dimension", xpathCtx);

        FMIModelVariable* variable = &modelDescription->modelVariables[i];

        for (size_t j = 0; j < xpathObj2->nodesetval->nodeNr; j++) {

            const xmlNodePtr dimensionNode = xpathObj2->nodesetval->nodeTab[j];

            const char* start = (char*)xmlGetProp(dimensionNode, (xmlChar*)"start");
            const char* valueReference = (char*)xmlGetProp(dimensionNode, (xmlChar*)"valueReference");

            variable->dimensions = realloc(variable->dimensions, (variable->nDimensions) + 1 * sizeof(FMIDimension));

            FMIDimension* dimension = &variable->dimensions[variable->nDimensions];

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

            printf("");
        }

    }

    xpathObj = xmlXPathEvalExpression((xmlChar*)"/fmiModelDescription/ModelStructure/Output", xpathCtx);
    modelDescription->nOutputs = xpathObj->nodesetval->nodeNr;

    xpathObj = xmlXPathEvalExpression((xmlChar*)"/fmiModelDescription/ModelStructure/ContinuousStateDerivative", xpathCtx);
    modelDescription->nContinuousStates = xpathObj->nodesetval->nodeNr;

    xpathObj = xmlXPathEvalExpression((xmlChar*)"/fmiModelDescription/ModelStructure/InitialUnknown", xpathCtx);
    modelDescription->nInitialUnknowns = xpathObj->nodesetval->nodeNr;

    xpathObj = xmlXPathEvalExpression((xmlChar*)"/fmiModelDescription/ModelStructure/EventIndicator", xpathCtx);
    modelDescription->nEventIndicators = xpathObj->nodesetval->nodeNr;

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

    if (fmiVersion == FMIVersion2) {
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
        free((void*)modelDescription->modelVariables[i].name);
        free((void*)modelDescription->modelVariables[i].description);
    }
    free(modelDescription->modelVariables);

    free(modelDescription);
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

        const FMIModelVariable* variable = &modelDescription->modelVariables[i];

        if (variable->valueReference == valueReference) {
            return variable;
        }
    }

    return NULL;
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
