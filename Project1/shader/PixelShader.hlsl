//ラスタライザから流れてくるデータを受け取るための構造体
//基本的に頂点シェーダーからの出力と同じフォーマット
struct PSIn {
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

float4 main(PSIn In) : SV_TARGET
{
	float4 color = In.color;

	return color;
}
