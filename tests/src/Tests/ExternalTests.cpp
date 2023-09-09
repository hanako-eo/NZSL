#include <Tests/ShaderUtils.hpp>
#include <NZSL/ShaderBuilder.hpp>
#include <NZSL/Parser.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <cctype>

TEST_CASE("external", "[Shader]")
{
	SECTION("Texture 2D")
	{
		std::string_view nzslSource = R"(
[nzsl_version("1.0")]
module;

external
{
	[tag("Color map")]
	[binding(0)] tex: sampler2D[f32]
}

[entry(frag)]
fn main()
{
	let value = tex.Sample(vec2[f32](0.0, 0.0));
}
)";

		nzsl::Ast::ModulePtr shaderModule = nzsl::Parse(nzslSource);
		shaderModule = SanitizeModule(*shaderModule);

		ExpectGLSL(*shaderModule, R"(
// fragment shader - this file was generated by NZSL compiler (Nazara Shading Language)

precision highp int;
#if GL_FRAGMENT_PRECISION_HIGH
precision highp float;
precision highp sampler2D;
#else
precision mediump float;
precision mediump sampler2D;
#endif

// header end

// external var tag: Color map
uniform sampler2D tex;

void main()
{
	vec4 value = texture(tex, vec2(0.0, 0.0));
}
)");

		ExpectNZSL(*shaderModule, R"(
external
{
	[set(0), binding(0), tag("Color map")] tex: sampler2D[f32]
}

[entry(frag)]
fn main()
{
	let value: vec4[f32] = tex.Sample(vec2[f32](0.0, 0.0));
})");

		ExpectSPIRV(*shaderModule, R"(
 %1 = OpTypeFloat 32
 %2 = OpTypeImage %1 Dim(Dim2D) 0 0 0 1 ImageFormat(Unknown)
 %3 = OpTypeSampledImage %2
 %4 = OpTypePointer StorageClass(UniformConstant) %3
 %6 = OpTypeVoid
 %7 = OpTypeFunction %6
 %8 = OpConstant %1 f32(0)
 %9 = OpTypeVector %1 2
%10 = OpTypeVector %1 4
%11 = OpTypePointer StorageClass(Function) %10
 %5 = OpVariable %4 StorageClass(UniformConstant)
%12 = OpFunction %6 FunctionControl(0) %7
%13 = OpLabel
%14 = OpVariable %11 StorageClass(Function)
%15 = OpLoad %3 %5
%16 = OpCompositeConstruct %9 %8 %8
%17 = OpImageSampleImplicitLod %10 %15 %16
      OpStore %14 %17
      OpReturn
      OpFunctionEnd)", {}, {}, true);
	}
	
	SECTION("Arrays of texture")
	{
		std::string_view nzslSource = R"(
[nzsl_version("1.0")]
module;

external
{
	[binding(0)] tex: array[sampler_cube[f32], 5]
}

[entry(frag)]
fn main()
{
	let value = tex[2].Sample(vec3[f32](0.0, 0.0, 0.0));
}
)";

		nzsl::Ast::ModulePtr shaderModule = nzsl::Parse(nzslSource);
		shaderModule = SanitizeModule(*shaderModule);

		ExpectGLSL(*shaderModule, R"(
// fragment shader - this file was generated by NZSL compiler (Nazara Shading Language)

precision highp int;
#if GL_FRAGMENT_PRECISION_HIGH
precision highp float;
precision highp samplerCube;
#else
precision mediump float;
precision mediump samplerCube;
#endif

// header end

uniform samplerCube tex[5];

void main()
{
	vec4 value = texture(tex[2], vec3(0.0, 0.0, 0.0));
}
)");

		ExpectNZSL(*shaderModule, R"(
external
{
	[set(0), binding(0)] tex: array[sampler_cube[f32], 5]
}

[entry(frag)]
fn main()
{
	let value: vec4[f32] = tex[2].Sample(vec3[f32](0.0, 0.0, 0.0));
})");

		ExpectSPIRV(*shaderModule, R"(
 %1 = OpTypeFloat 32
 %2 = OpTypeImage %1 Dim(Cube) 0 0 0 1 ImageFormat(Unknown)
 %3 = OpTypeSampledImage %2
 %4 = OpTypeInt 32 0
 %5 = OpConstant %4 u32(5)
 %6 = OpTypeArray %3 %5
 %7 = OpTypePointer StorageClass(UniformConstant) %6
 %9 = OpTypeVoid
%10 = OpTypeFunction %9
%11 = OpTypeInt 32 1
%12 = OpConstant %11 i32(2)
%13 = OpConstant %1 f32(0)
%14 = OpTypeVector %1 3
%15 = OpTypeVector %1 4
%16 = OpTypePointer StorageClass(Function) %15
%20 = OpTypePointer StorageClass(UniformConstant) %3
 %8 = OpVariable %7 StorageClass(UniformConstant)
%17 = OpFunction %9 FunctionControl(0) %10
%18 = OpLabel
%19 = OpVariable %16 StorageClass(Function)
%21 = OpAccessChain %20 %8 %12
%22 = OpLoad %3 %21
%23 = OpCompositeConstruct %14 %13 %13 %13
%24 = OpImageSampleImplicitLod %15 %22 %23
      OpStore %19 %24
      OpReturn
      OpFunctionEnd)", {}, {}, true);
	}

	SECTION("Uniform buffers")
	{
		std::string_view nzslSource = R"(
[nzsl_version("1.0")]
module;

[tag("DataStruct")]
struct Data
{
	[tag("Values")]
	values: array[f32, 47]
}

external
{
	[binding(0)] data: uniform[Data]
}

[entry(frag)]
fn main()
{
	let value = data.values[42];
}
)";

		nzsl::Ast::ModulePtr shaderModule = nzsl::Parse(nzslSource);
		shaderModule = SanitizeModule(*shaderModule);

		ExpectGLSL(*shaderModule, R"(
// struct tag: DataStruct
uniform _nzslBindingdata
{
	// member tag: Values
	float values[47];
} data;

void main()
{
	float value = data.values[42];
}
)");

		ExpectNZSL(*shaderModule, R"(
[tag("DataStruct")]
struct Data
{
	[tag("Values")] values: array[f32, 47]
}

external
{
	[set(0), binding(0)] data: uniform[Data]
}

[entry(frag)]
fn main()
{
	let value: f32 = data.values[42];
})");

		ExpectSPIRV(*shaderModule, R"(
      OpDecorate %9 Decoration(Binding) 0
      OpDecorate %9 Decoration(DescriptorSet) 0
      OpMemberDecorate %5 0 Decoration(Offset) 0
      OpDecorate %6 Decoration(ArrayStride) 16
      OpDecorate %7 Decoration(Block)
      OpMemberDecorate %7 0 Decoration(Offset) 0
 %1 = OpTypeFloat 32
 %2 = OpTypeInt 32 0
 %3 = OpConstant %2 u32(47)
 %4 = OpTypeArray %1 %3
 %5 = OpTypeStruct %4
 %6 = OpTypeArray %1 %3
 %7 = OpTypeStruct %6
 %8 = OpTypePointer StorageClass(Uniform) %7
%10 = OpTypeVoid
%11 = OpTypeFunction %10
%12 = OpTypeInt 32 1
%13 = OpConstant %12 i32(0)
%14 = OpConstant %12 i32(42)
%15 = OpTypePointer StorageClass(Function) %1
%19 = OpTypePointer StorageClass(Uniform) %1
 %9 = OpVariable %8 StorageClass(Uniform)
%16 = OpFunction %10 FunctionControl(0) %11
%17 = OpLabel
%18 = OpVariable %15 StorageClass(Function)
%20 = OpAccessChain %19 %9 %13 %14
%21 = OpLoad %1 %20
      OpStore %18 %21
      OpReturn
      OpFunctionEnd)", {}, {}, true);
	}

	SECTION("Storage buffers")
	{
		SECTION("With fixed-size array")
		{
			std::string_view nzslSource = R"(
[nzsl_version("1.0")]
module;

struct Data
{
	values: array[f32, 47]
}

external
{
	[binding(0)] data: storage[Data]
}

[entry(frag)]
fn main()
{
	let value = data.values[42];
}
)";

			nzsl::Ast::ModulePtr shaderModule = nzsl::Parse(nzslSource);
			shaderModule = SanitizeModule(*shaderModule);

			nzsl::GlslWriter::Environment glslEnv;
			glslEnv.glMajorVersion = 3;
			glslEnv.glMinorVersion = 1;

			ExpectGLSL(*shaderModule, R"(
buffer _nzslBindingdata
{
	float values[47];
} data;

void main()
{
	float value = data.values[42];
}
)", {}, glslEnv);

			ExpectNZSL(*shaderModule, R"(
struct Data
{
	values: array[f32, 47]
}

external
{
	[set(0), binding(0)] data: storage[Data]
}

[entry(frag)]
fn main()
{
	let value: f32 = data.values[42];
})");

			WHEN("Generating SPIR-V 1.0")
			{
				nzsl::SpirvWriter::Environment spirvEnv;
				ExpectSPIRV(*shaderModule, R"(
      OpDecorate %9 Decoration(Binding) 0
      OpDecorate %9 Decoration(DescriptorSet) 0
      OpMemberDecorate %5 0 Decoration(Offset) 0
      OpDecorate %6 Decoration(ArrayStride) 16
      OpDecorate %7 Decoration(BufferBlock)
      OpMemberDecorate %7 0 Decoration(Offset) 0
 %1 = OpTypeFloat 32
 %2 = OpTypeInt 32 0
 %3 = OpConstant %2 u32(47)
 %4 = OpTypeArray %1 %3
 %5 = OpTypeStruct %4
 %6 = OpTypeArray %1 %3
 %7 = OpTypeStruct %6
 %8 = OpTypePointer StorageClass(Uniform) %7
%10 = OpTypeVoid
%11 = OpTypeFunction %10
%12 = OpTypeInt 32 1
%13 = OpConstant %12 i32(0)
%14 = OpConstant %12 i32(42)
%15 = OpTypePointer StorageClass(Function) %1
%19 = OpTypePointer StorageClass(Uniform) %1
 %9 = OpVariable %8 StorageClass(Uniform)
%16 = OpFunction %10 FunctionControl(0) %11
%17 = OpLabel
%18 = OpVariable %15 StorageClass(Function)
%20 = OpAccessChain %19 %9 %13 %14
%21 = OpLoad %1 %20
      OpStore %18 %21
      OpReturn
      OpFunctionEnd)", {}, spirvEnv, true);
			}

			WHEN("Generating SPIR-V 1.3")
			{
				nzsl::SpirvWriter::Environment spirvEnv;
				spirvEnv.spvMajorVersion = 1;
				spirvEnv.spvMinorVersion = 3;

				ExpectSPIRV(*shaderModule, R"(
      OpDecorate %9 Decoration(Binding) 0
      OpDecorate %9 Decoration(DescriptorSet) 0
      OpMemberDecorate %5 0 Decoration(Offset) 0
      OpDecorate %6 Decoration(ArrayStride) 16
      OpDecorate %7 Decoration(Block)
      OpMemberDecorate %7 0 Decoration(Offset) 0
 %1 = OpTypeFloat 32
 %2 = OpTypeInt 32 0
 %3 = OpConstant %2 u32(47)
 %4 = OpTypeArray %1 %3
 %5 = OpTypeStruct %4
 %6 = OpTypeArray %1 %3
 %7 = OpTypeStruct %6
 %8 = OpTypePointer StorageClass(StorageBuffer) %7
%10 = OpTypeVoid
%11 = OpTypeFunction %10
%12 = OpTypeInt 32 1
%13 = OpConstant %12 i32(0)
%14 = OpConstant %12 i32(42)
%15 = OpTypePointer StorageClass(Function) %1
%19 = OpTypePointer StorageClass(StorageBuffer) %1
 %9 = OpVariable %8 StorageClass(StorageBuffer)
%16 = OpFunction %10 FunctionControl(0) %11
%17 = OpLabel
%18 = OpVariable %15 StorageClass(Function)
%20 = OpAccessChain %19 %9 %13 %14
%21 = OpLoad %1 %20
      OpStore %18 %21
      OpReturn
      OpFunctionEnd)", {}, spirvEnv, true);
			}
		}
		
		SECTION("With dynamically sized arrays")
		{
			std::string_view nzslSource = R"(
[nzsl_version("1.0")]
module;

struct Data
{
	data: u32,
	values: dyn_array[f32]
}

external
{
	[binding(0)] data: storage[Data]
}

[entry(frag)]
fn main()
{
	let value = data.values[42];
	let size = data.values.Size();
}
)";

			nzsl::Ast::ModulePtr shaderModule = nzsl::Parse(nzslSource);
			shaderModule = SanitizeModule(*shaderModule);

			nzsl::GlslWriter::Environment glslEnv;
			glslEnv.glMajorVersion = 3;
			glslEnv.glMinorVersion = 1;

			ExpectGLSL(*shaderModule, R"(
buffer _nzslBindingdata
{
	uint data;
	float values[];
} data;

void main()
{
	float value = data.values[42];
	uint size = uint(data.values.length());
}
)", {}, glslEnv);

			ExpectNZSL(*shaderModule, R"(
struct Data
{
	data: u32,
	values: dyn_array[f32]
}

external
{
	[set(0), binding(0)] data: storage[Data]
}

[entry(frag)]
fn main()
{
	let value: f32 = data.values[42];
	let size: u32 = data.values.Size();
})");

			WHEN("Generating SPIR-V 1.0")
			{
				nzsl::SpirvWriter::Environment spirvEnv;

				// Notice how runtime array is actually present twice
				// this is because the struct is registered by its declaration before being redeclared as a storage buffer
				// this could be fixed in a way similar to GLSL
				ExpectSPIRV(*shaderModule, R"(
      OpDecorate %8 Decoration(Binding) 0
      OpDecorate %8 Decoration(DescriptorSet) 0
      OpMemberDecorate %4 0 Decoration(Offset) 0
      OpMemberDecorate %4 1 Decoration(Offset) 16
      OpDecorate %5 Decoration(ArrayStride) 16
      OpDecorate %6 Decoration(BufferBlock)
      OpMemberDecorate %6 0 Decoration(Offset) 0
      OpMemberDecorate %6 1 Decoration(Offset) 16
 %1 = OpTypeInt 32 0
 %2 = OpTypeFloat 32
 %3 = OpTypeRuntimeArray %2
 %4 = OpTypeStruct %1 %3
 %5 = OpTypeRuntimeArray %2
 %6 = OpTypeStruct %1 %5
 %7 = OpTypePointer StorageClass(Uniform) %6
 %9 = OpTypeVoid
%10 = OpTypeFunction %9
%11 = OpTypeInt 32 1
%12 = OpConstant %11 i32(1)
%13 = OpConstant %11 i32(42)
%14 = OpTypePointer StorageClass(Function) %2
%15 = OpTypePointer StorageClass(Function) %1
%20 = OpTypePointer StorageClass(Uniform) %2
 %8 = OpVariable %7 StorageClass(Uniform)
%16 = OpFunction %9 FunctionControl(0) %10
%17 = OpLabel
%18 = OpVariable %14 StorageClass(Function)
%19 = OpVariable %15 StorageClass(Function)
%21 = OpAccessChain %20 %8 %12 %13
%22 = OpLoad %2 %21
      OpStore %18 %22
%23 = OpArrayLength %1 %8 1
      OpStore %19 %23
      OpReturn
      OpFunctionEnd)", {}, spirvEnv, true);
			}

			WHEN("Generating SPIR-V 1.3")
			{
				nzsl::SpirvWriter::Environment spirvEnv;
				spirvEnv.spvMajorVersion = 1;
				spirvEnv.spvMinorVersion = 3;

				ExpectSPIRV(*shaderModule, R"(
      OpDecorate %8 Decoration(Binding) 0
      OpDecorate %8 Decoration(DescriptorSet) 0
      OpMemberDecorate %4 0 Decoration(Offset) 0
      OpMemberDecorate %4 1 Decoration(Offset) 16
      OpDecorate %5 Decoration(ArrayStride) 16
      OpDecorate %6 Decoration(Block)
      OpMemberDecorate %6 0 Decoration(Offset) 0
      OpMemberDecorate %6 1 Decoration(Offset) 16
 %1 = OpTypeInt 32 0
 %2 = OpTypeFloat 32
 %3 = OpTypeRuntimeArray %2
 %4 = OpTypeStruct %1 %3
 %5 = OpTypeRuntimeArray %2
 %6 = OpTypeStruct %1 %5
 %7 = OpTypePointer StorageClass(StorageBuffer) %6
 %9 = OpTypeVoid
%10 = OpTypeFunction %9
%11 = OpTypeInt 32 1
%12 = OpConstant %11 i32(1)
%13 = OpConstant %11 i32(42)
%14 = OpTypePointer StorageClass(Function) %2
%15 = OpTypePointer StorageClass(Function) %1
%20 = OpTypePointer StorageClass(StorageBuffer) %2
 %8 = OpVariable %7 StorageClass(StorageBuffer)
%16 = OpFunction %9 FunctionControl(0) %10
%17 = OpLabel
%18 = OpVariable %14 StorageClass(Function)
%19 = OpVariable %15 StorageClass(Function)
%21 = OpAccessChain %20 %8 %12 %13
%22 = OpLoad %2 %21
      OpStore %18 %22
%23 = OpArrayLength %1 %8 1
      OpStore %19 %23
      OpReturn
      OpFunctionEnd)", {}, spirvEnv, true);
			}
		}
	}

	SECTION("Primitive external")
	{
		std::string_view nzslSource = R"(
[nzsl_version("1.0")]
[feature(primitive_externals)]
module;

[tag("External set tag")]
external
{
	[tag("Scalars")]
	[binding(0)] bVal: bool,
	[binding(1)] fVal: f32,
	[binding(2)] iVal: i32,
	[binding(3)] uVal: u32,

	[tag("Vectors")]
	[binding(4)] bVec: vec4[bool],
	[binding(5)] fVec: vec4[f32],
	[binding(6)] iVec: vec4[i32],
	[binding(7)] uVec: vec4[u32],

	[tag("Matrices")]
	[binding(8)] fMat: mat4[f32],

	[tag("Arrays of matrices")]
	[binding(9)] fArrayOfMat: array[mat4[f32], 5]
}

[entry(frag)]
fn main()
{
	let value: bool = bVec[1] && bVal;
	let value: vec4[f32] = fVal * fVec;
	let value: vec4[i32] = iVal.xxxx + iVec;
	let value: vec4[u32] = uVal.xxxx - uVec;
	let value: vec4[f32] = fMat * fVec;
	let value: vec4[f32] = fArrayOfMat[2] * fVec;
}
)";

		nzsl::Ast::ModulePtr shaderModule = nzsl::Parse(nzslSource);
		shaderModule = SanitizeModule(*shaderModule);

		ExpectGLSL(*shaderModule, R"(
// external block tag: External set tag
// external var tag: Scalars
uniform bool bVal;
uniform float fVal;
uniform int iVal;
uniform uint uVal;
// external var tag: Vectors
uniform bvec4 bVec;
uniform vec4 fVec;
uniform ivec4 iVec;
uniform uvec4 uVec;
// external var tag: Matrices
uniform mat4 fMat;
// external var tag: Arrays of matrices
uniform mat4 fArrayOfMat[5];

void main()
{
	bool value = bVec[1] && bVal;
	vec4 value_2 = fVal * fVec;
	ivec4 value_3 = (ivec4(iVal, iVal, iVal, iVal)) + iVec;
	uvec4 value_4 = (uvec4(uVal, uVal, uVal, uVal)) - uVec;
	vec4 value_5 = fMat * fVec;
	vec4 value_6 = fArrayOfMat[2] * fVec;
}
)");

		ExpectNZSL(*shaderModule, R"(
[tag("External set tag")]
external
{
	[set(0), binding(0), tag("Scalars")] bVal: bool,
	[set(0), binding(1)] fVal: f32,
	[set(0), binding(2)] iVal: i32,
	[set(0), binding(3)] uVal: u32,
	[set(0), binding(4), tag("Vectors")] bVec: vec4[bool],
	[set(0), binding(5)] fVec: vec4[f32],
	[set(0), binding(6)] iVec: vec4[i32],
	[set(0), binding(7)] uVec: vec4[u32],
	[set(0), binding(8), tag("Matrices")] fMat: mat4[f32],
	[set(0), binding(9), tag("Arrays of matrices")] fArrayOfMat: array[mat4[f32], 5]
}

[entry(frag)]
fn main()
{
	let value: bool = bVec[1] && bVal;
	let value: vec4[f32] = fVal * fVec;
	let value: vec4[i32] = (iVal.xxxx) + iVec;
	let value: vec4[u32] = (uVal.xxxx) - uVec;
	let value: vec4[f32] = fMat * fVec;
	let value: vec4[f32] = fArrayOfMat[2] * fVec;
})");

		WHEN("Generating SPIR-V 1.0 (which doesn't support primitive externals)")
		{
			nzsl::SpirvWriter spirvWriter;
			CHECK_THROWS_WITH(spirvWriter.Generate(*shaderModule), "unsupported type used in external block (SPIR-V doesn't allow primitive types as uniforms)");
		}
	}


	SECTION("Auto binding generation")
	{
		std::string_view nzslSource = R"(
[nzsl_version("1.0")]
module;

struct Foo
{
}

[auto_binding]
external
{
	tex1: sampler2D[f32],
	tex2: sampler2D[f32],
	foo : push_constant[Foo],
	[binding(4)] tex3: sampler2D[f32],
	[binding(0)] tex4: sampler2D[f32]
}

[auto_binding(true)]
external
{
	tex5: array[sampler2D[f32], 3],
	tex6: sampler_cube[f32]
}

[auto_binding(false)]
external
{
	[binding(8)] tex7: sampler2D[f32]
}

[entry(frag)]
fn main()
{
}
)";

		nzsl::Ast::ModulePtr shaderModule = nzsl::Parse(nzslSource);

		WHEN("Performing a full compilation")
		{
			shaderModule = SanitizeModule(*shaderModule);

			ExpectNZSL(*shaderModule, R"(
struct Foo
{

}

[auto_binding(true)]
external
{
	[set(0), binding(1)] tex1: sampler2D[f32],
	[set(0), binding(2)] tex2: sampler2D[f32],
	foo: push_constant[Foo],
	[set(0), binding(4)] tex3: sampler2D[f32],
	[set(0), binding(0)] tex4: sampler2D[f32]
}

[auto_binding(true)]
external
{
	[set(0), binding(5)] tex5: array[sampler2D[f32], 3],
	[set(0), binding(3)] tex6: sampler_cube[f32]
}

[auto_binding(false)]
external
{
	[set(0), binding(8)] tex7: sampler2D[f32]
}

[entry(frag)]
fn main()
{

})");
		}

		WHEN("Performing a partial compilation")
		{
			nzsl::Ast::SanitizeVisitor::Options options;
			options.partialSanitization = true;

			shaderModule = SanitizeModule(*shaderModule, options);

			ExpectNZSL(*shaderModule, R"(
struct Foo
{

}

[auto_binding(true)]
external
{
	[set(0)] tex1: sampler2D[f32],
	[set(0)] tex2: sampler2D[f32],
	foo: push_constant[Foo],
	[set(0), binding(4)] tex3: sampler2D[f32],
	[set(0), binding(0)] tex4: sampler2D[f32]
}

[auto_binding(true)]
external
{
	[set(0)] tex5: array[sampler2D[f32], 3],
	[set(0)] tex6: sampler_cube[f32]
}

[auto_binding(false)]
external
{
	[set(0), binding(8)] tex7: sampler2D[f32]
}

[entry(frag)]
fn main()
{

})");
		}

		WHEN("Performing a partial compilation and forcing auto_binding resolve")
		{
			nzsl::Ast::SanitizeVisitor::Options options;
			options.forceAutoBindingResolve = true;
			options.partialSanitization = true;

			shaderModule = SanitizeModule(*shaderModule, options);

			ExpectNZSL(*shaderModule, R"(
struct Foo
{

}

[auto_binding(true)]
external
{
	[set(0), binding(1)] tex1: sampler2D[f32],
	[set(0), binding(2)] tex2: sampler2D[f32],
	foo: push_constant[Foo],
	[set(0), binding(4)] tex3: sampler2D[f32],
	[set(0), binding(0)] tex4: sampler2D[f32]
}

[auto_binding(true)]
external
{
	[set(0), binding(5)] tex5: array[sampler2D[f32], 3],
	[set(0), binding(3)] tex6: sampler_cube[f32]
}

[auto_binding(false)]
external
{
	[set(0), binding(8)] tex7: sampler2D[f32]
}

[entry(frag)]
fn main()
{

})");
		}
	}

	SECTION("Push constant generation")
	{
		std::string_view nzslSource = R"(
[nzsl_version("1.0")]
module;

struct Data
{
	index: i32
}

external
{
	data: push_constant[Data]
}

[entry(frag)]
fn main()
{
}
)";

		nzsl::Ast::ModulePtr shaderModule = nzsl::Parse(nzslSource);
		shaderModule = SanitizeModule(*shaderModule);

		ExpectGLSL(*shaderModule, R"(
struct Data
{
	int index;
};

uniform Data data;

void main()
{

}
)");

		ExpectNZSL(*shaderModule, R"(
struct Data
{
	index: i32
}

external
{
	data: push_constant[Data]
}

[entry(frag)]
fn main()
{

})");

	ExpectSPIRV(*shaderModule, R"(
 %1 = OpTypeInt 32 1
 %2 = OpTypeStruct %1
 %3 = OpTypeStruct %1
 %4 = OpTypePointer StorageClass(PushConstant) %3
 %6 = OpTypeVoid
 %7 = OpTypeFunction %6
 %5 = OpVariable %4 StorageClass(PushConstant)
 %8 = OpFunction %6 FunctionControl(0) %7
 %9 = OpLabel
      OpReturn
      OpFunctionEnd)", {}, {}, true);
	}


	SECTION("Incompatible structs test")
	{
		std::string_view nzslSource = R"(
[nzsl_version("1.0")]
module;

const MaxLightCascadeCount = 4;
const MaxLightCount = 3;

[layout(std140)]
struct DirectionalLight
{
	color: vec3[f32],
	direction: vec3[f32],
	invShadowMapSize: vec2[f32],	
	ambientFactor: f32,
	diffuseFactor: f32,
	cascadeCount: u32,
	cascadeDistances: array[f32, MaxLightCascadeCount],
	viewProjMatrices: array[mat4[f32], MaxLightCascadeCount],
}

[layout(std140)]
struct LightData
{
	directionalLights: array[DirectionalLight, MaxLightCount],
	directionalLightCount: u32
}

external
{
	[binding(0)] lightData: uniform[LightData]
}

[entry(frag)]
fn main()
{
	for lightIndex in u32(0) -> lightData.directionalLightCount
	{
		// light struct is not compatible with DirectionalLight (because of the ArrayStride), this forces a per-member copy in SPIR-V
		let light = lightData.directionalLights[lightIndex];

		// struct are compatibles, a direct copy is performed
		let lightCopy = light;
	}
}
)";

		nzsl::Ast::ModulePtr shaderModule = nzsl::Parse(nzslSource);

		shaderModule = SanitizeModule(*shaderModule);

		ExpectGLSL(*shaderModule, R"(
struct DirectionalLight
{
	vec3 color;
	vec3 direction;
	vec2 invShadowMapSize;
	float ambientFactor;
	float diffuseFactor;
	uint cascadeCount;
	float cascadeDistances[4];
	mat4 viewProjMatrices[4];
};

// struct LightData omitted (used as UBO/SSBO)

layout(std140) uniform _nzslBindinglightData
{
	DirectionalLight directionalLights[3];
	uint directionalLightCount;
} lightData;

void main()
{
	{
		uint lightIndex = uint(0);
		uint to = lightData.directionalLightCount;
		while (lightIndex < to)
		{
			DirectionalLight light = lightData.directionalLights[lightIndex];
			DirectionalLight lightCopy = light;
			lightIndex += 1u;
		}

	}

}
)");

		ExpectNZSL(*shaderModule, R"(
const MaxLightCascadeCount: i32 = 4;

const MaxLightCount: i32 = 3;

[layout(std140)]
struct DirectionalLight
{
	color: vec3[f32],
	direction: vec3[f32],
	invShadowMapSize: vec2[f32],
	ambientFactor: f32,
	diffuseFactor: f32,
	cascadeCount: u32,
	cascadeDistances: array[f32, 4],
	viewProjMatrices: array[mat4[f32], 4]
}

[layout(std140)]
struct LightData
{
	directionalLights: array[DirectionalLight, 3],
	directionalLightCount: u32
}

external
{
	[set(0), binding(0)] lightData: uniform[LightData]
}

[entry(frag)]
fn main()
{
	for lightIndex in u32(0) -> lightData.directionalLightCount
	{
		let light: DirectionalLight = lightData.directionalLights[lightIndex];
		let lightCopy: DirectionalLight = light;
	}

})");

		ExpectSPIRV(*shaderModule, R"(
 %36 = OpFunction %21 FunctionControl(0) %22
 %37 = OpLabel
 %38 = OpVariable %25 StorageClass(Function)
 %39 = OpVariable %25 StorageClass(Function)
 %40 = OpVariable %28 StorageClass(Function)
 %41 = OpVariable %28 StorageClass(Function)
 %42 = OpBitcast %4 %24
       OpStore %38 %42
 %44 = OpAccessChain %43 %20 %26
 %45 = OpLoad %4 %44
       OpStore %39 %45
       OpBranch %46
 %46 = OpLabel
 %50 = OpLoad %4 %38
 %51 = OpLoad %4 %39
 %52 = OpULessThan %27 %50 %51
       OpLoopMerge %48 %49 LoopControl(0)
       OpBranchConditional %52 %47 %48
 %47 = OpLabel
 %53 = OpLoad %4 %38
 %55 = OpAccessChain %54 %20 %24 %53 %24
 %56 = OpLoad %2 %55
 %57 = OpAccessChain %58 %40 %24
       OpStore %57 %56
 %59 = OpLoad %4 %38
 %60 = OpAccessChain %54 %20 %24 %59 %26
 %61 = OpLoad %2 %60
 %62 = OpAccessChain %58 %40 %26
       OpStore %62 %61
 %63 = OpLoad %4 %38
 %65 = OpAccessChain %64 %20 %24 %63 %29
 %66 = OpLoad %3 %65
 %67 = OpAccessChain %68 %40 %29
       OpStore %67 %66
 %69 = OpLoad %4 %38
 %71 = OpAccessChain %70 %20 %24 %69 %30
 %72 = OpLoad %1 %71
 %73 = OpAccessChain %74 %40 %30
       OpStore %73 %72
 %75 = OpLoad %4 %38
 %76 = OpAccessChain %70 %20 %24 %75 %31
 %77 = OpLoad %1 %76
 %78 = OpAccessChain %74 %40 %31
       OpStore %78 %77
 %79 = OpLoad %4 %38
 %80 = OpAccessChain %43 %20 %24 %79 %32
 %81 = OpLoad %4 %80
 %82 = OpAccessChain %25 %40 %32
       OpStore %82 %81
 %83 = OpLoad %4 %38
 %84 = OpAccessChain %70 %20 %24 %83 %33 %24
 %85 = OpLoad %1 %84
 %86 = OpAccessChain %87 %40 %33
 %88 = OpAccessChain %74 %86 %24
       OpStore %88 %85
 %89 = OpLoad %4 %38
 %90 = OpAccessChain %70 %20 %24 %89 %33 %26
 %91 = OpLoad %1 %90
 %92 = OpAccessChain %87 %40 %33
 %93 = OpAccessChain %74 %92 %26
       OpStore %93 %91
 %94 = OpLoad %4 %38
 %95 = OpAccessChain %70 %20 %24 %94 %33 %29
 %96 = OpLoad %1 %95
 %97 = OpAccessChain %87 %40 %33
 %98 = OpAccessChain %74 %97 %29
       OpStore %98 %96
 %99 = OpLoad %4 %38
%100 = OpAccessChain %70 %20 %24 %99 %33 %30
%101 = OpLoad %1 %100
%102 = OpAccessChain %87 %40 %33
%103 = OpAccessChain %74 %102 %30
       OpStore %103 %101
%104 = OpLoad %4 %38
%106 = OpAccessChain %105 %20 %24 %104 %34 %24
%107 = OpLoad %8 %106
%108 = OpAccessChain %109 %40 %34
%110 = OpAccessChain %111 %108 %24
       OpStore %110 %107
%112 = OpLoad %4 %38
%113 = OpAccessChain %105 %20 %24 %112 %34 %26
%114 = OpLoad %8 %113
%115 = OpAccessChain %109 %40 %34
%116 = OpAccessChain %111 %115 %26
       OpStore %116 %114
%117 = OpLoad %4 %38
%118 = OpAccessChain %105 %20 %24 %117 %34 %29
%119 = OpLoad %8 %118
%120 = OpAccessChain %109 %40 %34
%121 = OpAccessChain %111 %120 %29
       OpStore %121 %119
%122 = OpLoad %4 %38
%123 = OpAccessChain %105 %20 %24 %122 %34 %30
%124 = OpLoad %8 %123
%125 = OpAccessChain %109 %40 %34
%126 = OpAccessChain %111 %125 %30
       OpStore %126 %124
%127 = OpLoad %10 %40
       OpStore %41 %127
%128 = OpLoad %4 %38
%129 = OpIAdd %4 %128 %35
       OpStore %38 %129
       OpBranch %49
 %49 = OpLabel
       OpBranch %46
 %48 = OpLabel
       OpReturn
       OpFunctionEnd)", {}, {}, true);
	}
}
