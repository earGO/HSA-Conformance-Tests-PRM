/*
   Copyright 2014 Heterogeneous System Architecture (HSA) Foundation

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

#include "ImageRdTests.hpp"
#include "RuntimeContext.hpp"
#include "HCTests.hpp"
#include "MObject.hpp"
#include <math.h>

using namespace hexl::emitter;
using namespace hexl::scenario;
using namespace HSAIL_ASM;
using namespace hexl;

namespace hsail_conformance {

class ImageRdTest:  public Test {
private:
  Image imgobj;
  Sampler smpobj;

  ImageGeometry imageGeometry;
  BrigImageGeometry imageGeometryProp;
  BrigImageChannelOrder imageChannelOrder;
  BrigImageChannelType imageChannelType;
  BrigSamplerCoordNormalization samplerCoord;
  BrigSamplerFilter samplerFilter;
  BrigSamplerAddressing samplerAddressing;

public:
  ImageRdTest(Location codeLocation, 
      Grid geometry, BrigImageGeometry imageGeometryProp_, BrigImageChannelOrder imageChannelOrder_, BrigImageChannelType imageChannelType_, 
      BrigSamplerCoordNormalization samplerCoord_, BrigSamplerFilter samplerFilter_, BrigSamplerAddressing samplerAddressing_, unsigned Array_ = 1): Test(codeLocation, geometry), 
      imageGeometryProp(imageGeometryProp_), imageChannelOrder(imageChannelOrder_), imageChannelType(imageChannelType_), 
      samplerCoord(samplerCoord_), samplerFilter(samplerFilter_), samplerAddressing(samplerAddressing_)
  {
     imageGeometry = ImageGeometry(geometry->GridSize(0), geometry->GridSize(1), geometry->GridSize(2), Array_);
  }
  
  void Name(std::ostream& out) const {
    out << CodeLocationString() << '_' << geometry << '\\' << imageGeometry << "_" << ImageGeometryString(MObjectImageGeometry(imageGeometryProp)) << "_" << ImageChannelOrderString(MObjectImageChannelOrder(imageChannelOrder)) << "_" << ImageChannelTypeString(MObjectImageChannelType(imageChannelType)) << "_" <<
      SamplerCoordsString(MObjectSamplerCoords(samplerCoord)) << "_" << SamplerFilterString(MObjectSamplerFilter(samplerFilter)) << "_" << SamplerAddressingString(MObjectSamplerAddressing(samplerAddressing));
  }

  ImageCalc calc;

  void Init() {
   Test::Init();

   EImageSpec imageSpec(BRIG_SEGMENT_KERNARG, BRIG_TYPE_ROIMG);
   imageSpec.Geometry(imageGeometryProp);
   imageSpec.ChannelOrder(imageChannelOrder);
   imageSpec.ChannelType(imageChannelType);
   imageSpec.Width(imageGeometry.ImageWidth());
   imageSpec.Height(imageGeometry.ImageHeight());
   imageSpec.Depth(imageGeometry.ImageDepth());
   imageSpec.ArraySize(imageGeometry.ImageArray());
   imgobj = kernel->NewImage("%roimage", &imageSpec);
   imgobj->AddData(Value(MV_UINT8, 0xFF));
 
   ESamplerSpec samplerSpec(BRIG_SEGMENT_KERNARG);
   samplerSpec.CoordNormalization(samplerCoord);
   samplerSpec.Filter(samplerFilter);
   samplerSpec.Addresing(samplerAddressing);
   smpobj = kernel->NewSampler("%sampler", &samplerSpec);

   imgobj->InitImageCalculator(smpobj);
  }

  void ModuleDirectives() override {
    be.EmitExtensionDirective("IMAGE");
  }

  Value ExpectedResult() const {
    Value color[4];
    Value coords[3];
    coords[0] = Value(0.0f);
    coords[1] = Value(0.0f);
    coords[2] = Value(0.0f);
    imgobj->ReadColor(coords, color);
    
    switch (imageChannelType)
    {
    case BRIG_CHANNEL_TYPE_SNORM_INT16:
      if (samplerFilter == BRIG_FILTER_LINEAR)
      {
        if (samplerAddressing == BRIG_ADDRESSING_CLAMP_TO_BORDER)
        {
          switch (imageGeometryProp)
          {
          case BRIG_GEOMETRY_1D:
          case BRIG_GEOMETRY_1DA:
            return Value(MV_UINT32, 0xB7800100);
          case BRIG_GEOMETRY_2D:
          case BRIG_GEOMETRY_2DA:
            return Value(MV_UINT32, 0xB7000100);
          case BRIG_GEOMETRY_3D:
            return Value(MV_UINT32, 0);
          default:
            break;
          }
          return Value(MV_UINT32, 0xBB810204);
        }
      }
      return (imageChannelOrder == BRIG_CHANNEL_ORDER_A) ? Value(MV_UINT32, color[3].U32()) : Value(MV_UINT32, color[0].U32());
    case BRIG_CHANNEL_TYPE_UNORM_INT16:
      if (samplerFilter == BRIG_FILTER_LINEAR) {
        if (samplerAddressing == BRIG_ADDRESSING_CLAMP_TO_BORDER)
        {
          switch (imageGeometryProp)
          {
          case BRIG_GEOMETRY_1D:
          case BRIG_GEOMETRY_1DA:
            return Value(MV_UINT32, 0x3F000000);
          case BRIG_GEOMETRY_2D:
          case BRIG_GEOMETRY_2DA:
            return Value(MV_UINT32, 0x3E800000);
          case BRIG_GEOMETRY_3D:
            return Value(MV_UINT32, 0x3E000080);
          default:
            break;
          }
          return Value(MV_UINT32, 0x3F000000);
        }
      }
      return (imageChannelOrder == BRIG_CHANNEL_ORDER_A) ? Value(MV_UINT32, color[3].U32()) : Value(MV_UINT32, color[0].U32());
    case BRIG_CHANNEL_TYPE_HALF_FLOAT:
    case BRIG_CHANNEL_TYPE_FLOAT:
      return Value(MV_UINT32, 0xFFC00000);
    default:
      break;
    }
    return (imageChannelOrder == BRIG_CHANNEL_ORDER_A) ? Value(MV_UINT32, color[3].U32()) : Value(MV_UINT32, color[0].U32());
  }

  bool IsValid() const override {
    if (samplerFilter == BRIG_FILTER_LINEAR) //only f32 access type is supported for linear filter
    {
      switch (imageChannelType)
      {
      case BRIG_CHANNEL_TYPE_SIGNED_INT8:
      case BRIG_CHANNEL_TYPE_SIGNED_INT16:
      case BRIG_CHANNEL_TYPE_SIGNED_INT32:
      case BRIG_CHANNEL_TYPE_UNSIGNED_INT8:
      case BRIG_CHANNEL_TYPE_UNSIGNED_INT16:
      case BRIG_CHANNEL_TYPE_UNSIGNED_INT32:
      case BRIG_CHANNEL_TYPE_UNKNOWN:
      case BRIG_CHANNEL_TYPE_FIRST_USER_DEFINED:
        return false;
        break;
      default:
        break;
      }
    }
    return IsImageSupported(imageGeometryProp, imageChannelOrder, imageChannelType) && IsImageGeometrySupported(imageGeometryProp, imageGeometry) && (codeLocation != FUNCTION);
  }
 
  BrigType ResultType() const {
    return BRIG_TYPE_U32; 
  }

  size_t OutputBufferSize() const override {
    return imageGeometry.ImageSize()*4;
  }

  TypedReg Get1dCoord()
  {
    auto result = be.AddTReg(BRIG_TYPE_F32);
    auto x = be.EmitWorkitemAbsId(0, false);
    be.EmitMov(result, x->Reg());
    return result;
  }

  OperandOperandList Get2dCoord()
  {
    auto result = be.AddVec(BRIG_TYPE_F32, 2);
    auto x = be.EmitWorkitemAbsId(1, false);
    auto y = be.EmitWorkitemAbsId(0, false);
    be.EmitMov(result.elements(0), x->Reg(), 32);
    be.EmitMov(result.elements(1), y->Reg(), 32);
    return result;
  }

  OperandOperandList Get3dCoord()
  {
    auto result = be.AddVec(BRIG_TYPE_F32, 3);
    auto x = be.EmitWorkitemAbsId(2, false);
    auto y = be.EmitWorkitemAbsId(1, false);
    auto z = be.EmitWorkitemAbsId(0, false);
    be.EmitMov(result.elements(0), x->Reg(), 32);
    be.EmitMov(result.elements(1), y->Reg(), 32);
    be.EmitMov(result.elements(2), z->Reg(), 32);
    return result;
  }

  TypedReg Result() {
    auto result = be.AddTReg(BRIG_TYPE_U32);
    be.EmitMov(result, be.Immed(BRIG_TYPE_U32, 0));
   // Load input
    auto imageaddr = be.AddTReg(imgobj->Variable().type());
    be.EmitLoad(imgobj->Segment(), imageaddr->Type(), imageaddr->Reg(), be.Address(imgobj->Variable())); 

    auto sampleraddr = be.AddTReg(smpobj->Variable().type());
    be.EmitLoad(smpobj->Segment(), sampleraddr->Type(), sampleraddr->Reg(), be.Address(smpobj->Variable())); 

    OperandOperandList regs_dest;
    auto reg_dest = be.AddTReg(BRIG_TYPE_U32, 1);
    switch (imageGeometryProp)
    {
    case BRIG_GEOMETRY_1D:
    case BRIG_GEOMETRY_1DB:
      regs_dest = be.AddVec(BRIG_TYPE_U32, 4);
      imgobj->EmitImageRd(regs_dest, BRIG_TYPE_U32,  imageaddr, sampleraddr, Get1dCoord());
      break;
    case BRIG_GEOMETRY_1DA:
    case BRIG_GEOMETRY_2D:
      regs_dest = be.AddVec(BRIG_TYPE_U32, 4);
      imgobj->EmitImageRd(regs_dest, BRIG_TYPE_U32,  imageaddr, sampleraddr, Get2dCoord(), BRIG_TYPE_F32);
      break;
    case BRIG_GEOMETRY_2DDEPTH:
      imgobj->EmitImageRd(reg_dest, imageaddr, sampleraddr, Get2dCoord(), BRIG_TYPE_F32);
      break;
    case BRIG_GEOMETRY_3D:
    case BRIG_GEOMETRY_2DA:
      regs_dest = be.AddVec(BRIG_TYPE_U32, 4);
      imgobj->EmitImageRd(regs_dest, BRIG_TYPE_U32,  imageaddr, sampleraddr, Get3dCoord(), BRIG_TYPE_F32);
      break;
    case BRIG_GEOMETRY_2DADEPTH:
      imgobj->EmitImageRd(reg_dest, imageaddr, sampleraddr, Get3dCoord(), BRIG_TYPE_F32);
      break;
    default:
      assert(0);
    }

    if ((imageGeometryProp == BRIG_GEOMETRY_2DDEPTH) || (imageGeometryProp == BRIG_GEOMETRY_2DADEPTH)) {
      be.EmitMov(result, reg_dest);
    }
    else {
      if (imageChannelOrder == BRIG_CHANNEL_ORDER_A)
      {
        be.EmitMov(result, regs_dest.elements(3));
      }
      else
      {
        be.EmitMov(result, regs_dest.elements(0));
      }
    }
    return result;
  }
};

void ImageRdTestSet::Iterate(hexl::TestSpecIterator& it)
{
  CoreConfig* cc = CoreConfig::Get(context);
  Arena* ap = cc->Ap();
  TestForEach<ImageRdTest>(ap, it, "image_rd/basic", CodeLocations(), cc->Grids().ImagesSet(),
     cc->Images().ImageRdGeometryProp(), cc->Images().ImageSupportedChannelOrders(), cc->Images().ImageChannelTypes(), cc->Sampler().SamplerCoords(), cc->Sampler().SamplerFilters(), cc->Sampler().SamplerAddressings(), cc->Images().ImageArraySets());
}

} // hsail_conformance
