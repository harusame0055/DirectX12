//���X�^���C�U���痬��Ă���f�[�^���󂯎�邽�߂̍\����
//��{�I�ɒ��_�V�F�[�_�[����̏o�͂Ɠ����t�H�[�}�b�g
struct PSIn {
	float4 position : SV_POSITION;
	float4 color : COLOR;
	float2 texcoord:TEXCOORD;
};

//�R�}���h�Őݒ肳�ꂽ�e�N�X�`����s�|�|���n�����
Texture2D color_tex : register(t0);

//�T���v���[�̓e�N�X�`�����Q�Ƃ��邽�߂̃I�u�W�F�N�g
SamplerState liner_sampler : register(s0);

float4 main(PSIn In) : SV_TARGET
{
	float4 color = In.color;

	return color_tex.Sample(liner_sampler,In.texcoord);
}
