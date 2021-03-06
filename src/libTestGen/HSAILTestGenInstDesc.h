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

#ifndef INCLUDED_HSAIL_TESTGEN_INST_DESC_H
#define INCLUDED_HSAIL_TESTGEN_INST_DESC_H

#include "HSAILTestGenProp.h"
#include "HSAILTestGenInstSetManager.h"
#include "HSAILUtilities.h"
#include "HSAILInstProps.h"

#include <cassert>
#include <algorithm>
#include <iterator>
#include <vector>

using std::vector;

namespace TESTGEN {

using namespace HSAIL_PROPS;

//==============================================================================
//==============================================================================
//==============================================================================

struct PropDeleter { void operator()(Prop* ptr) { delete ptr; } };

//==============================================================================
//==============================================================================
//==============================================================================
// Description of an HSAIL instruction. Consist of a set of properties and
// their values.

class InstDesc
{
private:
    // Mapping of abstract HDL property values (e.g. 'reg')
    // to actual TestGen values (e.g. '$c0', '$d1' etc).
    // Used for non-brig properties only.
    static map<unsigned, unsigned*> valMap;

    friend class Prop;

private:
    const unsigned opcode;  // Instruction opcode
    const unsigned format;  // Instruction format

    // Each instruction is described as a set of abstract properties each
    // of which has a set of valid and invalid values. Note that the set of valid values
    // may depend on on values of other properties. Consequently, all properties
    // fall into one of two categories: primary and secondary.
    // - set of valid values of a primary property depends only on values of other primary properties.
    // - set of valid values of a secondary property depends only on values of primary properties.
    // Order of primary properties specified by InstSetManager must be preserved;
    // this order is essential for proper test generation.

    vector<Prop*> prmProp;  // Primary properties
    vector<Prop*> secProp;  // Secondary propertiess

    //==========================================================================

private:
    InstDesc(const InstDesc&); // non-copyable
    const InstDesc &operator=(const InstDesc &);  // not assignable

    //==========================================================================

protected:
    InstDesc(unsigned fmt, unsigned opc) : opcode(opc), format(fmt) { initProps(); }

public:
    ~InstDesc()
    {
        std::for_each(prmProp.begin(), prmProp.end(), PropDeleter());
        std::for_each(secProp.begin(), secProp.end(), PropDeleter());
    }

    //==========================================================================

public:
    static unsigned getFormat(unsigned opcode) { return InstSetManager::getFormat(opcode); }

public:
    unsigned getOpcode() const { return opcode; }
    unsigned getFormat() const { return format; }

    //==========================================================================

protected:
    unsigned getPrmPropNum()        const { return static_cast<unsigned>(prmProp.size()); } //F: use iterators
    Prop*    getPrmProp(unsigned i) const { assert(i < prmProp.size()); return prmProp[i]; }
    unsigned getSecPropNum()        const { return static_cast<unsigned>(secProp.size()); }
    Prop*    getSecProp(unsigned i) const { assert(i < secProp.size()); return secProp[i]; }

    bool removeProp(unsigned propId) { return removeProp(propId, prmProp) || removeProp(propId, secProp); }

    //==========================================================================

private:

    void initProps()
    {
        unsigned prmPropsNum;
        unsigned secPropsNum;
        const unsigned* props = InstSetManager::getProps(opcode, prmPropsNum, secPropsNum);

        assert(props && prmPropsNum > 0 && secPropsNum > 0);

        for (unsigned i = 0; i < prmPropsNum + secPropsNum; ++i)
        {
            unsigned propId = props[i];
            assert(PROP_MINID <= propId && propId < PROP_MAXID);

            unsigned pValsNum;
            const unsigned* pVals = InstSetManager::getValidPropVals(opcode, propId, pValsNum);   // values supported by instruction
            assert(pVals && pValsNum > 0);

            unsigned nValsNum;
            const unsigned* nVals = InstSetManager::getAllPropVals(opcode, propId, nValsNum);     // all values of this property
            assert(nVals && nValsNum > 0);

            if (pValsNum == 1 && *pVals == OPERAND_VAL_NULL)                                // minimize tests number
            {
                nValsNum = 1;
                nVals = pVals;
            }

            Prop* p = Prop::create(propId, pVals, pValsNum, nVals, nValsNum);

            if (i < prmPropsNum)
            {
                prmProp.push_back(p);
            }
            else
            {
                secProp.push_back(p);
            }
        }
    }

    //==========================================================================

private:
    bool removeProp(unsigned propId, vector<Prop*> &prop)
    {
        assert(PROP_MINID <= propId && propId < PROP_MAXID);

        for (vector<Prop*>::iterator it = prop.begin(); it != prop.end(); ++it) //F replace with STL
        {
            if ((*it)->getPropId() == propId)
            {
                delete (*it);
                prop.erase(it);
                return true;
            }
        }
        return false;
    }

}; // class InstDesc

//==============================================================================
//==============================================================================
//==============================================================================

}; // namespace TESTGEN

#endif // INCLUDED_HSAIL_TESTGEN_INST_DESC_H