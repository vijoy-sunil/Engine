#ifndef DUMP_FORMATTING_H
#define DUMP_FORMATTING_H

namespace Collection {
    #define TAB_L1          "\t"
    #define TAB_L2          TAB_L1 "\t"
    #define TAB_L3          TAB_L2 "\t"
    #define TAB_L4          TAB_L3 "\t"
    #define TAB_L5          TAB_L4 "\t"
    #define TAB_L6          TAB_L5 "\t"

    #define OPEN_L1         TAB_L1 << "{" << "\n"
    #define OPEN_L3         TAB_L3 << "{" << "\n"
    #define OPEN_L5         TAB_L5 << "{" << "\n"

    #define CLOSE_L1        TAB_L1 << "}" << "\n"
    #define CLOSE_L3        TAB_L3 << "}" << "\n"
    #define CLOSE_L5        TAB_L5 << "}" << "\n"
}   // namespace Collection
#endif  // DUMP_FORMATTING_H