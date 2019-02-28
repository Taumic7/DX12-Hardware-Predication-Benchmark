// Shader model 5.0

struct VSIn
{
	float2 pos		: POS;
};

struct VSOut
{
	float4 pos		: SV_POSITION;
};

VSOut VS_main(VSIn input, uint index : SV_VertexID)
{
	VSOut output = (VSOut)0;
	output.pos = float4(input.pos,1.f,1.f);


	return output;
}