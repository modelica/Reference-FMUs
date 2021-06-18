#ifndef namespace_h
#define namespace_h

#define pasteA(a,b)          a ## b
#define pasteB(a,b)          pasteA(a,b)

// C-code FMUs have functions names prefixed with MODEL_IDENTIFIER_.
// Define DISABLE_PREFIX to build a binary FMU.

#ifndef DISABLE_PREFIX

#define FMI3_FUNCTION_PREFIX pasteB(MODEL_IDENTIFIER, _)
#define FMI2_FUNCTION_PREFIX pasteB(MODEL_IDENTIFIER, _)
#define FMI_FUNCTION_PREFIX pasteB(MODEL_IDENTIFIER, _)

#endif

#endif /*namespace_h*/