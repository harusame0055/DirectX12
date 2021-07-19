//ラスタライザから流れてくるデータを受け取るための構造体
//基本的に頂点シェーダーからの出力と同じフォーマット
struct PSIn {
	float4 position : SV_POSITION;
	float4 color : COLOR;
	float2 texcoord:TEXCOORD;
};

//コマンドで設定されたテクスチャりs－酢が渡される
Texture2D color_tex : register(t0);

//サンプラーはテクスチャを参照するためのオブジェクト
SamplerState liner_sampler : register(s0);

float4 main(PSIn In) : SV_TARGET
{
	float4 color = In.color;

	return color_tex.Sample(liner_sampler,In.texcoord);
}
