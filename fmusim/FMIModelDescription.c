#include "FMIModelDescription.h"

#include <libxml/parser.h>
#include <libxml/xmlschemas.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <Windows.h>


FMIModelDescription* FMIReadModelDescription(const char* filename) {

    // TODO: add stream interface (see https://gitlab.gnome.org/GNOME/libxml2/-/wikis/Parser-interfaces)

    FMIModelDescription* modelDescription = (FMIModelDescription*)calloc(1, sizeof(FMIModelDescription));

    if (!modelDescription) return NULL;

    xmlDocPtr doc = xmlParseFile(filename);

    // xmlKeepBlanksDefault(0);
    // xmlDocDump(stdout, doc);

    if (!doc) {
        printf("Invalid XML.\n");
        return NULL;
    }

    xmlNodePtr root = xmlDocGetRootElement(doc);

    if (root == NULL) {
        printf("Empty document\n");
        return NULL;
    }

    modelDescription->fmiVersion = FMIVersion3; // xmlGetProp(root, "fmiVersion");

    xmlSchemaParserCtxtPtr pctxt;

    char path[2048] = "";
    GetModuleFileNameA(NULL, path, 2048);

    switch (modelDescription->fmiVersion) {
    case FMIVersion2:
        strcat(path, "\\..\\schema\\fmi2\\fmi2ModelDescription.xsd");
        break;
    case FMIVersion3:
        strcat(path, "\\..\\schema\\fmi3\\fmi3ModelDescription.xsd");
        break;
    default:
        return NULL;
    }

    pctxt = xmlSchemaNewParserCtxt(path);

    xmlSchemaPtr schema = xmlSchemaParse(pctxt);

    xmlSchemaFreeParserCtxt(pctxt);

    if (schema == NULL) {
        return NULL;
    }

    xmlSchemaValidCtxtPtr vctxt = xmlSchemaNewValidCtxt(schema);

    if (!vctxt) {
        xmlSchemaFree(schema);
        return NULL;
    }

    xmlSchemaSetValidErrors(vctxt, (xmlSchemaValidityErrorFunc)fprintf, (xmlSchemaValidityWarningFunc)fprintf, stderr);

    if (xmlSchemaValidateDoc(vctxt, doc) != 0) {
        return NULL;
    }

    modelDescription->instantiationToken = xmlGetProp(root, "instantiationToken");
    modelDescription->description        = xmlGetProp(root, "description");
    modelDescription->generationTool     = xmlGetProp(root, "generationTool");
    modelDescription->generationDate     = xmlGetProp(root, "generationDate");

    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression("/fmiModelDescription/CoSimulation", xpathCtx);

    xmlNodePtr coSimulation = xpathObj->nodesetval->nodeTab[0];

    modelDescription->modelIdentifier = xmlGetProp(coSimulation, "modelIdentifier");

    xpathObj = xmlXPathEvalExpression("/fmiModelDescription/ModelVariables/*[self::Float32 or self::Float64 or self::Int32]", xpathCtx);

    modelDescription->nModelVariables = xpathObj->nodesetval->nodeNr;
    modelDescription->modelVariables = calloc(xpathObj->nodesetval->nodeNr, sizeof(FMIModelVariable));

    for (size_t i = 0; i < xpathObj->nodesetval->nodeNr; i++) {

        xmlNodePtr node = xpathObj->nodesetval->nodeTab[i];

        modelDescription->modelVariables[i].name = xmlGetProp(node, "name");
        modelDescription->modelVariables[i].description = xmlGetProp(node, "description");

        FMIVariableType type;

        if (!strcmp(node->name, "Float32")) {
            type = FMIFloat32Type;
        } else if (!strcmp(node->name, "Float64") || !strcmp(node->name, "Real")) {
            type = FMIFloat64Type;
        } else if (!strcmp(node->name, "Int8")) {
            type = FMIInt8Type;
        } else if (!strcmp(node->name, "UInt8")) {
            type = FMIUInt8Type;
        } else if (!strcmp(node->name, "Int16")) {
            type = FMIInt16Type;
        } else if (!strcmp(node->name, "UInt16")) {
            type = FMIUInt16Type;
        } else if (!strcmp(node->name, "Int32") || !strcmp(node->name, "Integer")) {
            type = FMIInt32Type;
        } else if (!strcmp(node->name, "UInt32")) {
            type = FMIUInt32Type;
        } else if (!strcmp(node->name, "Int64")) {
            type = FMIInt64Type;
        } else if (!strcmp(node->name, "UInt64")) {
            type = FMIUInt64Type;
        } else if (!strcmp(node->name, "Boolean")) {
            type = FMIBooleanType;
        } else if (!strcmp(node->name, "String")) {
            type = FMIStringType;
        } else if (!strcmp(node->name, "Binary")) {
            type = FMIBinaryType;
        } else if (!strcmp(node->name, "Clock")) {
            type = FMIClockType;
        }
        
        modelDescription->modelVariables[i].type = type;
        
        const char* vr = xmlGetProp(node, "valueReference");
        
        modelDescription->modelVariables[i].valueReference = strtoul(vr, NULL, 0);

        const char* causality = xmlGetProp(node, "causality");

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

    }

    xpathObj = xmlXPathEvalExpression("/fmiModelDescription/ModelStructure/Output", xpathCtx);
    modelDescription->nOutputs = xpathObj->nodesetval->nodeNr;

    xpathObj = xmlXPathEvalExpression("/fmiModelDescription/ModelStructure/ContinuousStateDerivative", xpathCtx);
    modelDescription->nContinuousStates = xpathObj->nodesetval->nodeNr;

    xpathObj = xmlXPathEvalExpression("/fmiModelDescription/ModelStructure/InitialUnknown", xpathCtx);
    modelDescription->nInitialUnknowns = xpathObj->nodesetval->nodeNr;

    xpathObj = xmlXPathEvalExpression("/fmiModelDescription/ModelStructure/EventIndicator", xpathCtx);
    modelDescription->nEventIndicators = xpathObj->nodesetval->nodeNr;

    xmlFreeDoc(doc);

    return modelDescription;
}

void FMIFreeModelDescription(FMIModelDescription* modelDescription) {
    // TODO
}

void FMIDumpModelDescription(FMIModelDescription* modelDescription, FILE* file) {

    fprintf(file, "FMI Version        3.0\n");
    fprintf(file, "FMI Type           Co-Simulation\n");
    fprintf(file, "Model Name         %s\n", modelDescription->modelIdentifier);
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
