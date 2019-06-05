float4 PS(float4 pos : SV_POSITION, float4 color : COLOR, float4 normal : NORMAL) : SV_Target
{
	float4 la = {0.1,0.1,0.1,0.1};
	float4 ld = { 0.0,1.0,1.0,1.0 };
	float3 ldir = { 1.0,1.0,-1.5 };
	float cl = max(dot(ldir, normal.xyz), 0);
	float4 newcolor = la * color + float4(cl * (ld.rgb * color.rgb), ld.a * color.a);
	return newcolor;
}