//���X�^���C�U���痬��Ă���f�[�^���󂯎�邽�߂̍\����
//��{�I�ɒ��_�V�F�[�_�[����̏o�͂Ɠ����t�H�[�}�b�g
struct PSIn {
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

float4 main(PSIn In) : SV_TARGET
{
	float4 color = In.color;

	return color;
}
