
struct VSOut
{
	float4 pos		: SV_POSITION;
};

struct GSOut
{
	float4 pos		: SV_POSITION;
};

[maxvertexcount(4)]
void GS_main(VSOut input, inout TriangleStream<GSOut> tristream) : SV_TARGET0
{
	float4 v[4];
	float halfWidth = 0.05f;
	v[0] = input.pos + float4(halfWidth, -halfWidth, 0.0f, 0.0f);
	v[1] = input.pos + float4(-halfWidth, -halfWidth, 0.0f, 0.0f);
	v[2] = input.pos + float4(-halfWidth, halfWidth, 0.0f, 0.0f);
	v[3] = input.pos + float4(halfWidth, halfWidth, 0.0f, 0.0f);
	GSOut output = (GSOut)0;
	for (int i = 0; i < 4; i++)
	{
		output.pos = v[i];
		tristream.Append(output);
	}

	return output;
}