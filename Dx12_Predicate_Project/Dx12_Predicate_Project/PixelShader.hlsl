// Shader model 5.0

struct GSOut
{
	float4 pos		: SV_POSITION;
	float4 realPos	: POSITION;
};

float4 PS_main(GSOut input) : SV_TARGET0
{
	return float4((
		input.realPos.x + 1) / 2.f,
		(input.realPos.y + 1) / 2.f,
		(input.realPos.y * input.realPos.x  + 1) / 2.f,
		1.0f);
	//return float4(1.f, 0.f, 0.f, 1.f);
}