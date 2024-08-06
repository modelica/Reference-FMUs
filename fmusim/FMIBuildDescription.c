#include <libxml/parser.h>
#include <libxml/xmlschemas.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

// to regenerate run
//
// python xsdflatten.py fmi3BuildDescription.xsd > fmi3BuildDescriptionMerged.xsd
//
// and
//
// xxd -i fmi3BuildDescriptionMerged.xsd > fmi3BuildDescriptionSchema.h
#include "fmi3BuildDescriptionSchema.h"

#include "FMIBuildDescription.h"
#include "FMIUtil.h"

#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)

static bool getBooleanAttribute(const xmlNodePtr node, const char* name) {
    char* literal = (char*)xmlGetProp(node, (xmlChar*)name);
    bool value = literal && (strcmp(literal, "true") == 0 || strcmp(literal, "1") == 0);
    xmlFree(literal);
    return value;
}

static const char* getStringAttribute(const xmlNodePtr node, const char* name) {

    char* value = NULL;

    char* prop = (char*)xmlGetProp(node, (xmlChar*)name);

    if (prop) {
        value = strdup(prop);
        xmlFree(prop);
    }

    return value;
}

static void logSchemaValidationError(void* ctx, const char* msg, ...) {
    (void)ctx; // unused
    va_list args;
    va_start(args, msg);
    logErrorMessage(msg, args);
    va_end(args);
}

#define APPEND_TO_ARRAY(a, n, s) \
    CALL(FMIRealloc(&a, (n + 1) * sizeof(void*))); \
    a[n] = s; \
    n++;

FMIBuildDescription* FMIReadBuildDescription(const char* filename) {

    FMIStatus status = FMIOK;

    xmlDocPtr doc = NULL;
    xmlNodePtr root = NULL;
    xmlSchemaPtr schema = NULL;
    xmlSchemaParserCtxtPtr pctxt = NULL;
    xmlSchemaValidCtxtPtr vctxt = NULL;

    doc = xmlParseFile(filename);

    if (!doc) {
        FMILogError("Invalid XML.");
        goto TERMINATE;
    }

    root = xmlDocGetRootElement(doc);

    if (root == NULL) {
        FMILogError("Empty document.");
        goto TERMINATE;
    }

    pctxt = xmlSchemaNewMemParserCtxt((char*)fmi3BuildDescriptionMerged_xsd, fmi3BuildDescriptionMerged_xsd_len);

    schema = xmlSchemaParse(pctxt);

    if (!schema) {
        FMILogError("Failed to parse XML schema.");
        goto TERMINATE;
    }

    vctxt = xmlSchemaNewValidCtxt(schema);

    if (!vctxt) {
        FMILogError("Failed to create schema validation context.");
        goto TERMINATE;
    }

    xmlSchemaSetValidErrors(vctxt, (xmlSchemaValidityErrorFunc)logSchemaValidationError, NULL, NULL);

    if (xmlSchemaValidateDoc(vctxt, doc)) {
        goto TERMINATE;
    }

    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(root->doc);

    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression((xmlChar*)"/fmiBuildDescription/BuildConfiguration", xpathCtx);

    FMIBuildDescription* buildDescription = NULL;
    CALL(FMICalloc((void**)&buildDescription, 1, sizeof(FMIBuildDescription)));
    
    buildDescription->nBuildConfigurations = xpathObj->nodesetval->nodeNr;
    CALL(FMICalloc((void**)&buildDescription->buildConfigurations, xpathObj->nodesetval->nodeNr, sizeof(FMIBuildConfiguration*)));

    for (int i = 0; i < xpathObj->nodesetval->nodeNr; i++) {

        FMIBuildConfiguration* buildConfiguration;
        CALL(FMICalloc((void**)&buildConfiguration, 1, sizeof(FMIBuildConfiguration)));

        buildDescription->buildConfigurations[i] = buildConfiguration;

        const xmlNodePtr buildConfigurationNode = xpathObj->nodesetval->nodeTab[i];

        buildConfiguration->modelIdentifier = getStringAttribute(buildConfigurationNode, "modelIdentifier");

        xmlXPathObjectPtr xpathObj2 = xmlXPathEvalExpression((xmlChar*)"/fmiBuildDescription/BuildConfiguration", xpathCtx);

        for (int j = 0; j < xpathObj->nodesetval->nodeNr; j++) {

            const xmlNodePtr buildConfigurationNode = xpathObj->nodesetval->nodeTab[i];

            xmlNodePtr sourceFileSetNode = buildConfigurationNode->children;

            while (sourceFileSetNode) {

                if (!strcmp(sourceFileSetNode->name, "SourceFileSet")) {

                    FMISourceFileSet* sourceFileSet;
                    CALL(FMICalloc((void**)&sourceFileSet, 1, sizeof(FMISourceFileSet)));

                    sourceFileSet->name            = getStringAttribute(sourceFileSetNode, "name");
                    sourceFileSet->language        = getStringAttribute(sourceFileSetNode, "language");
                    sourceFileSet->compiler        = getStringAttribute(sourceFileSetNode, "compiler");
                    sourceFileSet->compilerOptions = getStringAttribute(sourceFileSetNode, "compilerOptions");

                    APPEND_TO_ARRAY(buildConfiguration->sourceFileSets, buildConfiguration->nSourceFileSets, sourceFileSet);

                    xmlNodePtr sourceFileSetChildNode = sourceFileSetNode->children;

                    while (sourceFileSetChildNode) {

                        if (!strcmp(sourceFileSetChildNode->name, "SourceFile")) {

                            APPEND_TO_ARRAY(sourceFileSet->sourceFiles, sourceFileSet->nSourceFiles, getStringAttribute(sourceFileSetChildNode, "name"));

                        } else if (!strcmp(sourceFileSetChildNode->name, "PreprocessorDefinition")) {

                            FMIPreprocessorDefinition* preprocessorDefinition;
                            CALL(FMICalloc((void**)&preprocessorDefinition, 1, sizeof(FMIPreprocessorDefinition)));

                            preprocessorDefinition->name        = getStringAttribute(sourceFileSetChildNode, "name");
                            preprocessorDefinition->value       = getStringAttribute(sourceFileSetChildNode, "value");
                            preprocessorDefinition->optional    = getBooleanAttribute(sourceFileSetChildNode, "optional");
                            preprocessorDefinition->description = getStringAttribute(sourceFileSetChildNode, "description");

                            APPEND_TO_ARRAY(sourceFileSet->preprocessorDefinitions, sourceFileSet->nPreprocessorDefinitions, preprocessorDefinition);

                        }
                            
                        sourceFileSetChildNode = sourceFileSetChildNode->next;
                    }

                }

                sourceFileSetNode = sourceFileSetNode->next;
            }

        }

    }

    xmlXPathFreeObject(xpathObj);

TERMINATE:

    return buildDescription;
}

void FMIFreeBuildDescription(FMIBuildDescription* buildDescription) {

    if (!buildDescription) {
        return;
    }

    for (size_t i = 0; i < buildDescription->nBuildConfigurations; i++) {

        FMIBuildConfiguration* buildConfiguration = buildDescription->buildConfigurations[i];

        for (size_t j = 0; j < buildConfiguration->nSourceFileSets; j++) {

            FMISourceFileSet* sourceFileSet = buildConfiguration->sourceFileSets[j];

            for (size_t k = 0; k < sourceFileSet->nPreprocessorDefinitions; k++) {
                FMIPreprocessorDefinition* preprocessorDefinition = sourceFileSet->preprocessorDefinitions[k];
                FMIFree(&preprocessorDefinition->name);
                FMIFree(&preprocessorDefinition->value);
                FMIFree(&preprocessorDefinition->description);
                FMIFree(&preprocessorDefinition);
            }

            FMIFree((void**)&sourceFileSet->preprocessorDefinitions);

            for (size_t k = 0; k < sourceFileSet->nSourceFiles; k++) {
                FMIFree((void**)&sourceFileSet->sourceFiles[k]);
            }
            
            FMIFree((void**)&sourceFileSet->sourceFiles);

            for (size_t k = 0; k < sourceFileSet->nIncludeDirectories; k++) {
                FMIFree((void**)&sourceFileSet->includeDirectories[k]);
            }
            
            FMIFree((void**)&sourceFileSet->includeDirectories);

            FMIFree(&sourceFileSet);
        }

        FMIFree((void**)&buildConfiguration->sourceFileSets);

        FMIFree(&buildConfiguration);
    }

    FMIFree((void**)&buildDescription->buildConfigurations);

    FMIFree((void**)&buildDescription);
}
