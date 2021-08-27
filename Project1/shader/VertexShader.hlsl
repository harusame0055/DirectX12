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

// �V�[���萔: �Q�[���̕`��I�u�W�F�N�g�ŋ��ʂ���f�[�^
cbuffer SceneConstants : register(b0)
{
	float4x4 view_proj;
};

PSIn main(VSIn In)
{
	PSIn Out = (PSIn)0;

	Out.position = mul(float4(In.position, 1.0f), view_proj);
	Out.color = In.color;
	Out.texcoord = In.texcoord;

	return Out;
}