#ifndef EBRANCH_H
#define EBRANCH_H

#include <string>

enum class EBranchT
{
    T1,
    T2,
    T3,
    T4,
    T5,
    TMAX
};

enum class EBranchL
{
    L1, L2, L3, L4, L5, L6, L7, L8, L9, L10, L11, L12, L13, L14, L15, L16, L17, L18, L19, L20, L21, L28, L29, L30, LMAX
};

extern const std::string EBranchL2String[(int)EBranchL::LMAX];

#endif // EBRANCH_H
