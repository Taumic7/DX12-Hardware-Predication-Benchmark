// Shader model 5.0

struct GSOut
{
	float4 pos		: SV_POSITION;
};

float4 PS_main(GSOut input) : SV_TARGET0
{

	return float4(1.f, 0.f, 0.f, 1.f);
}