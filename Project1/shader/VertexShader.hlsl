struct VSIn {
	float3 position : POSITION;
	float4 color : COLOR;
	float2 texcoord :TEXCOORD;
};

struct PSIn {
	float4 position : SV_POSITION;
	float4 color : COLOR;
	float2 texcoord : TEXCOORD;
};

PSIn main(VSIn In)
{
	PSIn Out = (PSIn)0;

	Out.position = float4(In.position, 1.0f);
	Out.color = In.color;
	out.texcoord = In.texcoord;

	return Out;
}