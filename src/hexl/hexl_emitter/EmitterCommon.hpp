/*
   Copyright 2014-2015 Heterogeneous System Architecture (HSA) Foundation

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

#ifndef EMITTER_COMMON_HPP
#define EMITTER_COMMON_HPP

#include "Arena.hpp"
#include "Grid.hpp"
#include "Sequence.hpp"
#include "HSAILBrigantine.h"

namespace hexl {
  namespace emitter {
    enum EmitterScope {
      ES_MODULE,
      ES_FUNCARG,
      ES_LOCAL,
      ES_ARG,
    };

    enum Location {
      LOCATION_BEGIN = 0,
      AUTO = LOCATION_BEGIN,
      MODULE,
      KERNEL,
      FUNCTION,
      ARGSCOPE,
      HOST,
      LOCATION_END,
    };

    enum AnnotationLocation {
      ANNOTATION_LOCATION_BEGIN = 0,
      BEFORE_VERSION = ANNOTATION_LOCATION_BEGIN,
      AFTER_VERSION,
      END_MODULE,
      BEFORE_MODULE_VARIABLE,
      AFTER_MODULE_VARIABLE,
      START_KERNEL,
      END_KERNEL,
      MIDDLE_KERNEL,
      START_FUNCTION,
      END_FUNCTION,
      MIDDLE_FUNCTION,
      START_ARG_BLOCK,
      END_ARG_BLOCK,
      MIDDLE_ARG_BLOCK,
      ANNOTATION_LOCATION_END
    };

    std::string AnnotationLocationString(AnnotationLocation location);

    enum UserModeQueueType {
      SOURCE_START = 0,
      SEPARATE_QUEUE = SOURCE_START, // Queue created on the host separate from dispatch.
      SOURCE_END,
      DISPATCH_SERVICE_QUEUE, // Queue created on the host and passed as service_queue of dispatch queue.
///      DISPATCH_QUEUE, // Dispatch queue.
      USER_PROVIDED = SOURCE_START,
    };
    
    enum ConditionType {
      COND_BINARY,
      COND_TYPE_START = COND_BINARY,
      COND_SWITCH,
      COND_TYPE_END,
    };

    enum ConditionInput {
      COND_HOST_INPUT,
      COND_INPUT_START = COND_HOST_INPUT,
      COND_IMM_PATH0,
      COND_IMM_PATH1,
      COND_WAVESIZE,
      COND_REG,
      COND_INPUT_END,
    };

    const char *LocationString(Location location);

    class TestEmitter;
    class ETypedReg;
    class ETypedRegList;
    class EPointerReg;
    class EVariableSpec;
    class EVariable;
    class EFBarrier;
    class EAddressSpec;
    class EAddress;
    class EControlDirectives;
    class EBuffer;
    class EUserModeQueue;
    class ESignal;
    class EKernel;
    class EFunction;
    class EModule;
    class EDispatch;
    class ECondition;
    class EImageSpec;
    class EImage;
    class ESamplerSpec;
    class ESampler;
    class EImageCalc;

    typedef ETypedRegList* TypedRegList;
    typedef ETypedReg* TypedReg;
    typedef EPointerReg* PointerReg;
    typedef EBuffer* Buffer;
    typedef EVariableSpec* VariableSpec;
    typedef EVariable* Variable;
    typedef EFBarrier* FBarrier;
    typedef EAddressSpec* AddressSpec;
    typedef EAddress* Address;
    typedef EControlDirectives* ControlDirectives;
    typedef EUserModeQueue* UserModeQueue;
    typedef ESignal* Signal;
    typedef EKernel* Kernel;
    typedef EFunction* Function;
    typedef EModule* Module;
    typedef EDispatch* Dispatch;
    typedef ECondition* Condition;
    typedef EImageSpec* ImageSpec;
    typedef EImage* Image;
    typedef ESamplerSpec* SamplerSpec;
    typedef ESampler* Sampler;
    typedef EImageCalc* ImageCalc;
  }
}

#endif // EMITTER_COMMON_HPP
