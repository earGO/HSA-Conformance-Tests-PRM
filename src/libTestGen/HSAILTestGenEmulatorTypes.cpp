
#include "HSAILTestGenEmulatorTypes.h"
#include "HSAILTestGenFpEmulator.h"
#include "HSAILTestGenVal.h"

using namespace HSAIL_ASM;

namespace TESTGEN {

//=============================================================================
//=============================================================================
//=============================================================================
// Min and max values for integer types (used for saturating rounding)

static const s64_t S8IB  = 0x7f;
static const s64_t S16IB = 0x7fff;
static const s64_t S32IB = 0x7fffffff;
static const s64_t S64IB = 0x7fffffffffffffffLL;
static const u64_t U8IB  = 0xff;
static const u64_t U16IB = 0xffff;
static const u64_t U32IB = 0xffffffff;
static const u64_t U64IB = 0xffffffffffffffffLL;

u64_t getIntBoundary(unsigned type, bool low)
{
    using namespace Brig; 

    switch (type)
    {
    case BRIG_TYPE_S8:  return low? -S8IB  - 1 : S8IB;
    case BRIG_TYPE_S16: return low? -S16IB - 1 : S16IB;
    case BRIG_TYPE_S32: return low? -S32IB - 1 : S32IB;
    case BRIG_TYPE_S64: return low? -S64IB - 1 : S64IB;
    case BRIG_TYPE_U8:  return low? 0          : U8IB;
    case BRIG_TYPE_U16: return low? 0          : U16IB;
    case BRIG_TYPE_U32: return low? 0          : U32IB;
    case BRIG_TYPE_U64: return low? 0          : U64IB;

    default:
        assert(false);
        return 0;
    }
}

//=============================================================================
//=============================================================================
//=============================================================================
//

static const u32_t MAX_U32_F32H = 0x4f7fffff; // max float value (4294967040.0f) which does not exceed U32IB = 4294967295
static const u32_t MAX_U64_F32H = 0x5f7fffff; // max float value (18446742974197923840.0f) which does not exceed U64IB = 18446744073709551615

static const u32_t MAX_S32_F32H = 0x4effffff; // max float value (2147483520.0f) which does not exceed S32IB = 2147483647
static const u32_t MAX_S64_F32H = 0x5effffff; // max float value (18446742974197923840.0f) which does not exceed S64IB = 9223372036854775807

static const u32_t MIN_S32_F32H = 0xcf000000; // min float value (2147483520.0f) which is greater than or equal to -(S32IB+1) = -2147483648
static const u32_t MIN_S64_F32H = 0xdf000000; // min float value (18446742974197923840.0f) which is greater than or equal to -(S64IB+1) = -9223372036854775808

static const u64_t MAX_U64_F64H = 0x43efffffffffffff; // max float value which does not exceed U64IB = 18446744073709551615
static const u64_t MAX_S64_F64H = 0x43dfffffffffffff; // max float value which does not exceed S64IB = 9223372036854775807
static const u64_t MIN_S64_F64H = 0xc3e0000000000000; // min float value which is greater than or equal to -(S64IB+1) = -9223372036854775808

static const float MIN_S8_F32  = -128.0f;
static const float MAX_S8_F32  =  127.0f;

static const float MIN_S16_F32 = -32768.0f;
static const float MAX_S16_F32 =  32767.0f;

static const float MIN_S32_F32 = HEX2F32(MIN_S32_F32H);
static const float MAX_S32_F32 = HEX2F32(MAX_S32_F32H);

static const float MIN_S64_F32 = HEX2F32(MIN_S64_F32H);
static const float MAX_S64_F32 = HEX2F32(MAX_S64_F32H);

static const float MAX_U8_F32  = 255.0f;
static const float MAX_U16_F32 = 65535.0f;
static const float MAX_U32_F32 = HEX2F32(MAX_U32_F32H);
static const float MAX_U64_F32 = HEX2F32(MAX_U64_F32H);

static const float MIN_U8_F32  = 0.0f;
static const float MIN_U16_F32 = 0.0f;
static const float MIN_U32_F32 = 0.0f;
static const float MIN_U64_F32 = 0.0f;

static const double MIN_S8_F64  = -128.0;
static const double MAX_S8_F64  =  127.0;

static const double MIN_S16_F64 = -32768.0;
static const double MAX_S16_F64 =  32767.0;

static const double MIN_S32_F64 = -2147483648.0;
static const double MAX_S32_F64 =  2147483647.0;

static const double MIN_S64_F64 = HEX2F64(MIN_S64_F64H);
static const double MAX_S64_F64 = HEX2F64(MAX_S64_F64H);

static const double MAX_U8_F64  = 255.0;
static const double MAX_U16_F64 = 65535.0;
static const double MAX_U32_F64 = 4294967295.0;
static const double MAX_U64_F64 = HEX2F64(MAX_U64_F64H);

static const double MIN_U8_F64  = 0.0;
static const double MIN_U16_F64 = 0.0;
static const double MIN_U32_F64 = 0.0;
static const double MIN_U64_F64 = 0.0;

static const f16_t MIN_S8_F16 (-128.0);   
static const f16_t MAX_S8_F16 ( 127.0);   

static const f16_t MIN_S16_F16(-32768.0); 
static const f16_t MAX_S16_F16( 32767.0); 

static const f16_t MIN_S32_F16(-65504.0); 
static const f16_t MAX_S32_F16( 65504.0); 

static const f16_t MIN_S64_F16(-65504.0); 
static const f16_t MAX_S64_F16( 65504.0); 

static const f16_t MAX_U8_F16 (255.0);    
static const f16_t MAX_U16_F16(65504.0);  
static const f16_t MAX_U32_F16(65504.0);  
static const f16_t MAX_U64_F16(65504.0);  

static const f16_t MIN_U8_F16 (0.0);
static const f16_t MIN_U16_F16(0.0);
static const f16_t MIN_U32_F16(0.0);
static const f16_t MIN_U64_F16(0.0);

//=============================================================================
//=============================================================================
//=============================================================================
//

f16_t getTypeBoundary_f16(unsigned type, bool isLo)
{
    using namespace Brig; 

    switch (type)
    {
    case BRIG_TYPE_S8:   return isLo? MIN_S8_F16  : MAX_S8_F16;
    case BRIG_TYPE_S16:  return isLo? MIN_S16_F16 : MAX_S16_F16;
    case BRIG_TYPE_S32:  return isLo? MIN_S32_F16 : MAX_S32_F16;
    case BRIG_TYPE_S64:  return isLo? MIN_S64_F16 : MAX_S64_F16;
    case BRIG_TYPE_U8:   return isLo? MIN_U8_F16  : MAX_U8_F16;
    case BRIG_TYPE_U16:  return isLo? MIN_U16_F16 : MAX_U16_F16;
    case BRIG_TYPE_U32:  return isLo? MIN_U32_F16 : MAX_U32_F16;
    case BRIG_TYPE_U64:  return isLo? MIN_U64_F16 : MAX_U64_F16;

    default:
        assert(false);
        return f16_t();
    }
}

float getTypeBoundary_f32(unsigned type, bool isLo)
{
    using namespace Brig; 

    switch (type)
    {
    case BRIG_TYPE_S8:   return isLo? MIN_S8_F32  : MAX_S8_F32;
    case BRIG_TYPE_S16:  return isLo? MIN_S16_F32 : MAX_S16_F32;
    case BRIG_TYPE_S32:  return isLo? MIN_S32_F32 : MAX_S32_F32;
    case BRIG_TYPE_S64:  return isLo? MIN_S64_F32 : MAX_S64_F32;
    case BRIG_TYPE_U8:   return isLo? MIN_U8_F32  : MAX_U8_F32;
    case BRIG_TYPE_U16:  return isLo? MIN_U16_F32 : MAX_U16_F32;
    case BRIG_TYPE_U32:  return isLo? MIN_U32_F32 : MAX_U32_F32;
    case BRIG_TYPE_U64:  return isLo? MIN_U64_F32 : MAX_U64_F32;

    default:
        assert(false);
        return 0;
    }
}

double getTypeBoundary_f64(unsigned type, bool isLo)
{
    using namespace Brig; 

    switch (type)
    {
    case BRIG_TYPE_S8:   return isLo? MIN_S8_F64  : MAX_S8_F64;
    case BRIG_TYPE_S16:  return isLo? MIN_S16_F64 : MAX_S16_F64;
    case BRIG_TYPE_S32:  return isLo? MIN_S32_F64 : MAX_S32_F64;
    case BRIG_TYPE_S64:  return isLo? MIN_S64_F64 : MAX_S64_F64;
    case BRIG_TYPE_U8:   return isLo? MIN_U8_F64  : MAX_U8_F64;
    case BRIG_TYPE_U16:  return isLo? MIN_U16_F64 : MAX_U16_F64;
    case BRIG_TYPE_U32:  return isLo? MIN_U32_F64 : MAX_U32_F64;
    case BRIG_TYPE_U64:  return isLo? MIN_U64_F64 : MAX_U64_F64;

    default:
        assert(false);
        return false;
    }
}

//=============================================================================
//=============================================================================
//=============================================================================
// Helpers for generation of tests for rounding modes

static const unsigned ROUNDING_TESTS_NUM = 12;

unsigned getRoundingTestsNum(unsigned dstType)
{
    return (isSignedType(dstType) || isUnsignedType(dstType))? ROUNDING_TESTS_NUM : 1;
}

template<typename T>
const void makeRoundingTestsData(unsigned dstType, AluMod aluMod, T* dst)
{
    if (getRoundingTestsNum(dstType) == 1)
    {
        dst[0] = (T)0.0; // dummy test data (cannot return empty list)
        return;
    }

    T lo = getTypeBoundary<T>(dstType, true);
    T hi = getTypeBoundary<T>(dstType, false);

    switch(aluMod.getRounding())
    {
    case AluMod::ROUNDING_NEARI:
    case AluMod::ROUNDING_NEARI_SAT:
    case AluMod::ROUNDING_SNEARI:
    case AluMod::ROUNDING_SNEARI_SAT:
                                        lo += (T)0.5;
                                        hi += (T)0.5;
                                        break;

    case AluMod::ROUNDING_ZEROI:
    case AluMod::ROUNDING_ZEROI_SAT:
    case AluMod::ROUNDING_SZEROI:
    case AluMod::ROUNDING_SZEROI_SAT:
                                        if (lo > (T)0.0) lo += (T)1.0;
                                        if (hi > (T)0.0) hi += (T)1.0;
                                        break;

    case AluMod::ROUNDING_DOWNI:
    case AluMod::ROUNDING_DOWNI_SAT:
    case AluMod::ROUNDING_SDOWNI:
    case AluMod::ROUNDING_SDOWNI_SAT:
                                        lo += (T)1.0;
                                        hi += (T)1.0;
                                        break;

    case AluMod::ROUNDING_UPI:
    case AluMod::ROUNDING_UPI_SAT:
    case AluMod::ROUNDING_SUPI:
    case AluMod::ROUNDING_SUPI_SAT:
                                        break;

    default:
        assert(false);
        break;
    }

    dst[0] = lo - (T)1.0;
    dst[1] = Val(lo - (T)1.0).ulp(1);
    dst[2] = Val(lo).ulp(-1);
    dst[3] = lo;
    dst[4] = Val(lo).ulp(1);
    dst[5] = lo + (T)1.0;
    dst[6] = hi - (T)1.0;
    dst[7] = Val(hi).ulp(-1);
    dst[8] = hi;
    dst[9] = Val(hi).ulp(1);
    dst[10] = Val(hi + (T)1.0).ulp(-1);
    dst[11] = hi + (T)1.0;

}

static f16_t  f16_rounding_tests[ROUNDING_TESTS_NUM];
static float  f32_rounding_tests[ROUNDING_TESTS_NUM];
static double f64_rounding_tests[ROUNDING_TESTS_NUM];

const f16_t*  getF16RoundingTestsData(unsigned dstType, AluMod aluMod)
{
    makeRoundingTestsData(dstType, aluMod, f16_rounding_tests);
    return f16_rounding_tests;
}

const float* getF32RoundingTestsData(unsigned dstType, AluMod aluMod)
{
    makeRoundingTestsData(dstType, aluMod, f32_rounding_tests);
    return f32_rounding_tests;
}

const double* getF64RoundingTestsData(unsigned dstType, AluMod aluMod)
{
    makeRoundingTestsData(dstType, aluMod, f64_rounding_tests);
    return f64_rounding_tests;
}

//==============================================================================
//==============================================================================
//==============================================================================

}; // namespace TESTGEN
