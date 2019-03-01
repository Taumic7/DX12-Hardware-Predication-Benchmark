
struct VSOut
{
	float4 pos		: SV_POSITION;
};

struct GSOut
{
	float4 pos		: SV_POSITION;
};

[maxvertexcount(4)]
void GS_main(point VSOut input[1], inout TriangleStream<GSOut> tristream) 
{
	float4 v[4];
	const float halfWidth = 0.005f;
	v[0] = input[0].pos + float4(-halfWidth, -halfWidth, 0.0f, 0.0f);
	v[1] = input[0].pos + float4(-halfWidth, halfWidth, 0.0f, 0.0f);
	v[2] = input[0].pos + float4(halfWidth, -halfWidth, 0.0f, 0.0f);
	v[3] = input[0].pos + float4(halfWidth, halfWidth, 0.0f, 0.0f);
	GSOut output = (GSOut)0;
	for (int i = 0; i < 4; i++)
	{
		output.pos = v[i];
		tristream.Append(output);
	}
}