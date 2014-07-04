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
        case EBranchL::L1: return "NL1";
        case EBranchL::L2: return "NL2";
        case EBranchL::L3: return "NL3";
        case EBranchL::L4: return "NL4";
        case EBranchL::L5: return "NL5";
        case EBranchL::L6: return "NL6";
        case EBranchL::L7: return "NL7";
        case EBranchL::L8: return "NL8";
        case EBranchL::L9: return "NL9";
        case EBranchL::L10: return "NL10";
        case EBranchL::L11: return "NL11";
        case EBranchL::L12: return "NL12";
        case EBranchL::L13: return "NL13";
        case EBranchL::L14: return "NL14";
        case EBranchL::L15: return "NL15";
        case EBranchL::L16: return "NL16";
        case EBranchL::L17: return "NL17";
        case EBranchL::L18: return "NL18";
        case EBranchL::L19: return "NL19";
        case EBranchL::L20: return "NL20";
        case EBranchL::L21: return "NL21";
        case EBranchL::L22: return "NL22";
        case EBranchL::L23: return "NL23";
        case EBranchL::L24: return "NL24";
        case EBranchL::L25: return "NL25";
        case EBranchL::L26: return "NL26";
        case EBranchL::L27: return "NL27";
        case EBranchL::L28: return "NL28";
        case EBranchL::L29: return "NL29";
        case EBranchL::L30: return "NL30";
        case EBranchL::L31: return "NL31";//tail
        case EBranchL::L32: return "NL31";//retargeting
        default: return "NL31";
    }
};
