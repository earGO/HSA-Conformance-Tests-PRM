/*
   Copyright 2013-2015 Heterogeneous System Architecture (HSA) Foundation

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "HSAILValidatorBase.h"
#include "HSAILTestGenDump.h"
#include "HSAILTestGenInstSetManager.h"
#include "HSAILInstProps.h"

#include <sstream>
#include <string>
//#include <iostream> //FF

using std::string;
using std::ostringstream;
using HSAIL_ASM::getRegName;

using namespace HSAIL_ASM;
using namespace HSAIL_PROPS;


namespace TESTGEN {

//==============================================================================
//==============================================================================
//==============================================================================

class TestGenInstDump
{
private:
    ostringstream out;

public:
    TestGenInstDump(){}

    string operator()(Inst inst)
    {
        out << "==========================================\n";

        visitBrigProps(inst);

        for (int i = 0; i < inst.operands().size(); ++i)
        {
            dumpOperand(i, inst.operand(i));
        }

        out << "\n";

        return out.str();
    }

    //==============================================================================
private:

    void dumpProp(string propName, string propVal)
    {
        const char* fill = "          ";
        if (propName.length() < strlen(fill)) propName += (fill + propName.length());
        out << propName << "= " << propVal << "\n";
    }

    void dumpProp(unsigned propId, string propVal)
    {
        dumpProp(HSAIL_ASM::prop2key(propId), propVal);
    }

    //==============================================================================

    string getOperandName(unsigned idx)
    {
        ostringstream s;
        s << "operand " << idx;
        return s.str();
    }

    string OperandConstantBytes2str(OperandConstantBytes o)
    {
        ostringstream s;
        SRef data = o.bytes();

        s << "IMM(" << static_cast<unsigned>(data[0]);
        for (unsigned i = 1; i < o.byteCount(); ++i)
        {
            s << ", " << static_cast<unsigned>(data[i]);
        }
        s << ")";
        return s.str();
    }

    string operandVector2str(OperandOperandList o)
    {
        ostringstream s;

        s << "(";
        for (unsigned i = 0; i < o.elementCount(); ++i)
        {
            if (i > 0) s << ", ";

            if      (OperandRegister r = o.elements(i))        s << getRegName(r);
            else if (OperandConstantBytes imm = o.elements(i)) s << OperandConstantBytes2str(imm);
            else if (OperandWavesize(o.elements(i)))           s <<  "wavesize";
            else                                               s << "***UNKNOWN***";
        }
        s << ")";

        return s.str();
    }

    string operandAddress2str(OperandAddress o)
    {
        ostringstream s;

        if (o.symbol())                                  s << "[" << o.symbol().name() << "]";
        if (o.reg())                                     s << "[" << o.reg()           << "]";
        if (o.offset() != 0 || !(o.symbol() || o.reg())) s << "[" << o.offset()        << "]";

        return s.str();
    }

    string operandList2str(OperandCodeList o)
    {
        ostringstream s;

        s << "(";
        for (unsigned i = 0; i < o.elementCount(); ++i)
        {
            s << (i > 0? ", " : "") << getName(o.elements(i));
        }
        s << ")";

        return s.str();
    }

    string operandCodeRef2str(OperandCodeRef ref)
    {
        assert(ref);

        ostringstream s;
        Directive d = ref.ref();

        if      (DirectiveLabel             o = d) { s << o.name(); }
        else if (DirectiveFunction          o = d) { s << o.name(); }
        else if (DirectiveIndirectFunction  o = d) { s << o.name(); }
        else if (DirectiveSignature         o = d) { s << o.name(); }
        else if (DirectiveFbarrier          o = d) { s << o.name(); }
        else if (DirectiveKernel            o = d) { s << o.name(); }
        else s << "***UNKNOWN***";

        return s.str();
    }

    void dumpOperand(unsigned idx, Operand opr)
    {
        ostringstream s;

        if      (!opr)                         { s << "NULL";  }
        else if (OperandRegister      o = opr) { s << getRegName(o); }
        else if (OperandOperandList   o = opr) { s << operandVector2str(o); }
        else if (OperandAddress       o = opr) { s << operandAddress2str(o); }
        else if (OperandWavesize      o = opr) { s << "wavesize"; }
        else if (OperandCodeRef       o = opr) { s << operandCodeRef2str(o); }
        else if (OperandCodeList      o = opr) { s << operandList2str(o); }
        else if (OperandConstantBytes o = opr) { s << OperandConstantBytes2str(o); }
        else if (OperandAlign         o = opr) { s << "align(" << o.align() << ")"; }
        else                                   { s << "*UNKNOWN*, kind = " << opr.kind(); }

        dumpProp(getOperandName(idx), s.str());
    }

    //==========================================================================
private:

    void visitProp(Inst inst, unsigned propId, unsigned propVal)
    {
        if (propId == PROP_EQUIVCLASS)
        {
            ostringstream s;
            s << propVal;
            dumpProp(propId, s.str());
        }
        else
        {
            dumpProp(propId, propVal2str(propId, propVal));
        }
    }

    string propVal2str(unsigned prop, unsigned val)
    { 
        const char* str = InstSetManager::propVal2str(prop, val);
        if (str) return str;
        
        ostringstream s;
        s << "(" << val << ")";
        return s.str();
    }

    //==========================================================================
    // Dumping test properties (autogenerated)
private:

#include "HSAILBrigPropsVisitor_gen.hpp"

    //==========================================================================
};

//==============================================================================
//==============================================================================
//==============================================================================

string dumpTestInst(Inst inst)
{
    return TestGenInstDump()(inst);
}

}; // namespace TESTGEN
