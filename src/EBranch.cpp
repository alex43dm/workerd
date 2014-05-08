#include "EBranch.h"
/*
EBranch::EBranch()
{
    //ctor
}

EBranch::~EBranch()
{
    //dtor
}
*/

const std::string EBranchL2String(EBranchL b)
{
    switch(b)
    {
        case EBranchL::L1: return "L1";
        case EBranchL::L2: return "L2";
        case EBranchL::L3: return "L3";
        case EBranchL::L4: return "L4";
        case EBranchL::L5: return "L5";
        case EBranchL::L6: return "L6";
        case EBranchL::L7: return "L7";
        case EBranchL::L8: return "L8";
        case EBranchL::L9: return "L9";
        case EBranchL::L10: return "L10";
        case EBranchL::L11: return "L11";
        case EBranchL::L12: return "L12";
        case EBranchL::L13: return "L13";
        case EBranchL::L14: return "L14";
        case EBranchL::L15: return "L15";
        case EBranchL::L16: return "L16";
        case EBranchL::L17: return "L17";
        case EBranchL::L18: return "L18";
        case EBranchL::L19: return "L19";
        case EBranchL::L20: return "L20";
        case EBranchL::L21: return "L21";
        case EBranchL::L22: return "L22";
        case EBranchL::L23: return "L23";
        case EBranchL::L24: return "L24";
        case EBranchL::L25: return "L25";
        case EBranchL::L26: return "L26";
        case EBranchL::L27: return "L27";
        case EBranchL::L28: return "L28";
        case EBranchL::L29: return "L29";
        case EBranchL::L30: return "L30";
        default: return "L31";
    }
};
