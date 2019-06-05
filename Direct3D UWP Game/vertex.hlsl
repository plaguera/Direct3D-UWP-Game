cbuffer cb : register(b0)
{

	float4x4 transform;

}

void VS(float3 pos : POSITION, float4 color : COLOR,
	out float4 opos : SV_POSITION, out float4 ocolor : COLOR)
{

	opos = mul(float4(pos, 1.0f), transform);
	ocolor = color;


}