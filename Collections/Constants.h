#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <string>

// dump formatting
#define TAB_L1                          "\t"
#define TAB_L2                          TAB_L1 "\t"
#define TAB_L3                          TAB_L2 "\t"
#define TAB_L4                          TAB_L3 "\t"
#define TAB_L5                          TAB_L4 "\t"
#define TAB_L6                          TAB_L5 "\t"

#define OPEN_L1                         TAB_L1 << "{" << "\n"
#define OPEN_L3                         TAB_L3 << "{" << "\n"
#define OPEN_L5                         TAB_L5 << "{" << "\n"

#define CLOSE_L1                        TAB_L1 << "}" << "\n"
#define CLOSE_L3                        TAB_L3 << "}" << "\n"
#define CLOSE_L5                        TAB_L5 << "}" << "\n"

/* reserved id use cases (these ids are not to be used by user as instance ids)
 * RESERVED_0                           used as offset instance id for the buffer used in LOG. An offset instance id is an
 *                                      instance id obtained by adding the offset to its parent instance id
*/
#define RESERVED_0                      99
#endif  // CONSTANTS_H