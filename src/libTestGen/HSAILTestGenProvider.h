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

#ifndef INCLUDED_HSAIL_TESTGEN_PROVIDER_H
#define INCLUDED_HSAIL_TESTGEN_PROVIDER_H

#include "HSAILTestGenSample.h"
#include "HSAILTestGenContext.h"
#include "HSAILTestGenInstSetManager.h"
#include "HSAILTestGenInstDesc.h"
#include "HSAILTestGenDump.h"

#include <cassert>

using HSAIL_ASM::validateProp;
using HSAIL_ASM::getSegment;
using HSAIL_ASM::isAddressableSeg;

namespace TESTGEN {

//==============================================================================
//==============================================================================
//==============================================================================
// Implementation of optimal and exhaustive test search
//
// There was two possibilities during TestGen design.
//
// The first and simplest possibility was to generate all possible combinations
// of values for HDL properties, generate Brig instruction for each combination
// and use InstValidator to select positive and negative tests. However this
// approach proved itself extremely inefficient because it caused about 50 times
// slowdown comparing with original handwritten TestGen.
//
// There are several reasons for this slowdown. First, most combinations (90% or so)
// are invalid and the total number of combinations is so great it is not useful
// even for negative testing due to time limitations. Second, validation using
// InstValidator is a slow operation because each instruction has many properties
// to validate.
//
// An alternative approach was to eliminate most invalid combinations by checking
// only a minimum set of so-called 'primary' properties. Even though this approach
// requires generation of all combinations of primary properties (exhaustive search)
// this number of combinations is dramatically less than that required by the first
// approach. An additional speedup is possible by validating each property independently
// right after assigning it a new value. These checks are very lightweight and allow
// quickly filter out invalid combinations of properties.
//
// The second approach proved itself very efficient and comparable in performance
// with manually-written TestGen. However it required tricky implementation due to
// the following reasons:
//
// 1. HDLProcessor had to analyze instructions requirements and identify primary
//    properties (which might affect values of other properties) and secondary
//    properties (which do not affect values of other properties). This was implemented
//    by analyzing conditionals in requirements:
//        { <primary> ? <secondary> }
//
// 2. HDL description had to be augmented with 'DependsOn' and 'Affects' statements.
//    This was necessary do describe hidden dependencies between properties which
//    could not be identified automatically.
//
// 3. It was very important to keep proper order of dependent properties.
//    When property A depends on property B, B must be assigned before A.
//    This is important because validation of A may check value of B.
//
// 4. HDLProcessor had to generate independent validation code for each property by
//    selecting only relevant checks from requirements. This validation includes
//    only basic conditions required to filter out invalid combinations quickly.
//    It was decided to perform complete validation of all primary properties
//    as a part of individual validation of last primary property.

class TestGen : public InstDesc
{
private:
    static bool isOptimalSearch;

private:
    // Samples are necessary to check property values because these checks
    // are performed on instructions (not on properties themselves).
    // Also we have to ensure that when checking property 'i',
    // all previous properties are initialized with valid values.

    // Positive and negative samples are used for generation of positive and negative tests.
    // Note that TestGen has a special version of instruction validator which can
    // validate each property independently of each other but assumes sertain order
    // of validation. Namely, primary properties must be assigned and validated
    // in the same order as specified by InstSetManager::getProps because validation of some
    // properties may include implicit reading of other properties.
    Sample positiveSample;
    Sample negativeSample;

    unsigned prmPropCurrent; // Index of current primary position. Used for validation only

    unsigned prmPropPos;     // Index of last returned primary property
    unsigned secPropPos;     // Index of last returned secondary property

    const bool isBasic;      // true if this is an InstBasic version generated automatically from InstMod

    //==========================================================================
private:
    TestGen(const TestGen&); // non-copyable
    const TestGen &operator=(const TestGen &);  // not assignable

private:
    TestGen(unsigned format, unsigned opcode, bool isBasicVariant)
        : InstDesc(format, opcode), prmPropCurrent(0), isBasic(isBasicVariant)
    {
        positiveSample = getPlayground()->createSample(format, opcode);
        negativeSample = getPlayground()->createSample(format, opcode);
    }

    static Context* getPlayground();

    //==========================================================================
public:
    static TestGen* create(unsigned opcode, bool isBasicFormat = false)
    {
        assert(!isBasicFormat || getFormat(opcode) == BRIG_KIND_INST_MOD);

        unsigned format = isBasicFormat? BRIG_KIND_INST_BASIC : getFormat(opcode);

        TestGen* test = new TestGen(format, opcode, isBasicFormat);

        if (isBasicFormat) test->removeBasicProps();

        return test;
    }

    string dump() { return dumpTestInst(positiveSample.getInst()); }

    static void init(bool isOpt);
    static void clean();
    //==========================================================================
public:
    // Generate next valid set of values for all primary properties;
    // initialize values of all secondary properties
    bool nextPrimarySet(bool start)
    {
        for (bool found = false; !found; )
        {
            if (!setAllPrimary(start)) return false;
            found = setAllSecondary();
        }

        secPropPos = 0;
        return true;
    }

    // Generate next valid set of values for all secondary properties
    bool nextSecondarySet()
    {
        return isOptimalSearch? nextSecondarySetOptimal() : nextSecondarySetExaustive();
    }

    bool nextSecondarySetOptimal()
    {
        unsigned secPropNum = getSecPropNum();
        for (; secPropPos < secPropNum; ++secPropPos)
        {
            if (getNextPositiveSecondary(secPropPos)) return true;

            // Restore first valid value
            getSecProp(secPropPos)->resetPositive();
            bool found = getNextPositiveSecondary(secPropPos);
            validate(found);
        }
        return false;
    }

    bool nextSecondarySetExaustive() //F code duplication
    {
        unsigned secPropNum = getSecPropNum();
        if (secPropNum == 0) return false;

        unsigned pIdx = secPropNum - 1;
        bool found = getNextPositiveSecondary(pIdx);

        for(;;)
        {
            assert(0 <= pIdx && pIdx < secPropNum);

            for (; found && ++pIdx < secPropNum; )
            {
                getSecProp(pIdx)->resetPositive();
                found = getNextPositiveSecondary(pIdx);
            }

            if (found) return true;

            assert(0 <= pIdx && pIdx < secPropNum);
            assert(!found);

            // Set next combination of primary propeties
            // Search for the last primary property we could change (if any)
            for (; !found && pIdx-- > 0; )
            {
                found = getNextPositiveSecondary(pIdx);
            }

            if (!found) return false;
        }
    }

    // Prepare for generation of negative tests
    void resetNegativeSet()
    {
        unsigned prmPropNum = getPrmPropNum();
        unsigned secPropNum = getSecPropNum();

        for (prmPropPos = 0; prmPropPos < prmPropNum; ++prmPropPos) getPrmProp(prmPropPos)->resetNegative();
        for (secPropPos = 0; secPropPos < secPropNum; ++secPropPos) getSecProp(secPropPos)->resetNegative();

        prmPropPos = 0;
        secPropPos = 0;
    }

    // Generate next set of invalid values
    bool nextNegativeSet(unsigned *id, unsigned* val)
    {
        unsigned prmPropNum = getPrmPropNum();
        unsigned secPropNum = getSecPropNum();

        for (; prmPropPos < prmPropNum; ++prmPropPos)
        {
            if (getNextNegativePrimary(prmPropPos))
            {
                *id = getPrmProp(prmPropPos)->getPropId();
                *val = getPrmProp(prmPropPos)->getCurrentNegative();
                return true;
            }
        }

        for (; secPropPos < secPropNum; ++secPropPos)
        {
            if (getNextNegativeSecondary(secPropPos))
            {
                *id = getSecProp(secPropPos)->getPropId();
                *val = getSecProp(secPropPos)->getCurrentNegative();
                return true;
            }
        }

        return false;
    }

    const Sample getPositiveSample() const { return positiveSample; }
    const Sample getNegativeSample() const { return negativeSample; }

    bool isBasicVariant()            const { return isBasic; }

    //==========================================================================
private:
    bool isValidPrimary(unsigned idx)
    {
        assert(idx < getPrmPropNum());

        unsigned propId = getPrmProp(idx)->getPropId();
        unsigned val    = getPrmProp(idx)->getCurrentPositive();

        assert(validatePrimaryPosition(idx));
        positiveSample.set(propId, val);
        return isValidProp(positiveSample.getInst(), propId);
    }

    bool isValidSecondary(unsigned idx)
    {
        assert(idx < getSecPropNum());

        unsigned propId = getSecProp(idx)->getPropId();
        unsigned val    = getSecProp(idx)->getCurrentPositive();

        positiveSample.set(propId, val);
        return isValidProp(positiveSample.getInst(), propId);
    }

    bool isInvalidPrimary(unsigned idx)
    {
        assert(idx < getPrmPropNum());

        unsigned propId = getPrmProp(idx)->getPropId();
        unsigned val    = getPrmProp(idx)->getCurrentNegative();

        assert(idx <= prmPropCurrent);
        assert(InstSetManager::isValidInst(positiveSample.getInst()));

        negativeSample.copyFrom(positiveSample);
        negativeSample.set(propId, val);
        return !InstSetManager::validatePrimaryProps(negativeSample.getInst());
    }

    bool isInvalidSecondary(unsigned idx)
    {
        assert(idx < getSecPropNum());

        unsigned propId = getSecProp(idx)->getPropId();
        unsigned val    = getSecProp(idx)->getCurrentNegative();

        assert(InstSetManager::isValidInst(positiveSample.getInst()));

        negativeSample.copyFrom(positiveSample);
        negativeSample.set(propId, val);
        return !isValidProp(negativeSample.getInst(), propId);
    }

    //==========================================================================

    bool getNextPositivePrimary(unsigned idx)
    {
        assert(idx < getPrmPropNum());

        for (; getPrmProp(idx)->findNextPositive(); )
        {
            if (isValidPrimary(idx)) return true;
        }
        return false;
    }

    bool getNextPositiveSecondary(unsigned idx)
    {
        assert(idx < getSecPropNum());

        for (; getSecProp(idx)->findNextPositive(); )
        {
            if (isValidSecondary(idx)) return true;
        }
        return false;
    }

    bool getNextNegativePrimary(unsigned idx)
    {
        assert(idx < getPrmPropNum());

        for (; getPrmProp(idx)->findNextNegative(); )
        {
            if (isInvalidPrimary(idx)) return true;
        }
        return false;
    }

    bool getNextNegativeSecondary(unsigned idx)
    {
        assert(idx < getSecPropNum());

        for (; getSecProp(idx)->findNextNegative(); )
        {
            if (isInvalidSecondary(idx)) return true;
        }
        return false;
    }

    //==========================================================================
private:
    // Set first/next combination of primary properties (both explic and implicit)
    bool setAllPrimary(bool start = false)
    {
        if (start) getPrmProp(0)->resetPositive();

        // This loop is required for InstBasic versions of InstMod instructions.
        // These instructions have additional implicit properties which cannot
        // be checked automatically by setExplicitPrimary. So this loop is
        // intended to skip combinations of primary properties for which
        // default 'rounding', 'packing' and 'ftz' values are not valid.
        // This is checked by validateBasicProps
        for(; setExplicitPrimary(start);)
        {
            if (!isBasicVariant()) return true;
            if (validateBasicProps()) return true;
            start = false;
        }

        return false;
    }

    // Set first/next combination of primary properties explicitly defined by TestGen.
    // NB: there may be hidden properties (e.g. when generating InstBasic variants)
    bool setExplicitPrimary(bool start)
    {
        unsigned prmPropNum = getPrmPropNum();
        unsigned pIdx = start? 0 : prmPropNum - 1;
        bool found = getNextPositivePrimary(pIdx);

        for(;;)
        {
            // Forward pass: try generating positive values for all primary properties

            assert(0 <= pIdx && pIdx < prmPropNum);

            for (; found && ++pIdx < prmPropNum; )
            {
                getPrmProp(pIdx)->resetPositive();
                found = getNextPositivePrimary(pIdx);
            }

            if (found) return true;

            // Backward pass: search for the nearest primary property we could change (if any)

            assert(0 <= pIdx && pIdx < prmPropNum);
            assert(!found);

            for (; !found && pIdx-- > 0; )
            {
                found = getNextPositivePrimary(pIdx);
            }

            if (!found) return false;
        }
    }

    // Initialize values for all secondary properties
    bool setAllSecondary()
    {
        unsigned secPropNum = getSecPropNum();
        for (unsigned i = 0; i < secPropNum; ++i)
        {
            getSecProp(i)->resetPositive();
            if (!getNextPositiveSecondary(i)) 
            {
                // TestGen cannot yet generate tests for ld/st with arg and spill segments
                assert(!isAddressableSeg(getSegment(positiveSample.getInst())));
                return false;
            }
        }
        return true;
    }

    //==========================================================================
private:

    // Explicitly validate values of InstMod properties for InstBase instruction.
    // This is necessary because these properties were removed from instruction
    // description for InstBase format, however, they logically present there
    // and must be checked
    bool validateBasicProps() const
    {
        assert(isBasicVariant());

        return isValidProp(positiveSample.getInst(), PROP_FTZ)   &&
               isValidProp(positiveSample.getInst(), PROP_ROUND) &&
               isValidProp(positiveSample.getInst(), PROP_PACK);
    }

    // Remove InstMod-specific properties which are not present in instBasic format.
    // This is necessary to avoid assignment to non-existing fields in InstBasic format.
    void removeBasicProps()
    {
        assert(isBasicVariant());

        bool res = removeProp(PROP_FTZ) && removeProp(PROP_ROUND) && removeProp(PROP_PACK);
        validate(res);
    }

    bool validatePrimaryPosition(unsigned idx)
    {
        assert(idx < getPrmPropNum());
        bool res = idx <= prmPropCurrent + 1;
        prmPropCurrent = idx;
        return res;
    }

    //==========================================================================
private:

    static bool isValidProp(Inst inst, unsigned propId)
    {
        if (!InstSetManager::isValidProp(inst, propId)) return false;
        return validateProp(inst, propId, BrigSettings::getModel(), BrigSettings::getProfile(), InstSetManager::isEnabled("IMAGE")) == 0;
    }

    void validate(bool flag) { assert(flag); } // used to avoid warnings in release builds

    //==========================================================================

}; // class TestGen

//==============================================================================
//==============================================================================
//==============================================================================

}; // namespace TESTGEN

#endif // INCLUDED_HSAIL_TESTGEN_PROVIDER_H
