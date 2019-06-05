cbuffer cb : register(b0)
{
	float4x4 transform;
	float4x4 normaltransform;
}

void VS(float3 pos : POSITION, float4 color : COLOR, float3 normal : NORMAL,
	out float4 opos : SV_POSITION, out float4 ocolor : COLOR, out float4 onormal : NORMAL)
{
	opos = mul(float4(pos, 1.0f), transform);
	onormal = mul(float4(normal, 0.0f), normaltransform);
	ocolor = color;
}