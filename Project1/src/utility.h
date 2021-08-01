#pragma once
#include<string>

#include<d3d12.h>
#include<wrl.h>

namespace ncc {

	/// @brief HLSLをコンパイルして実行可能なオブジェクトを作る
	/// @param filename コンパイルするHLSLファイルパス
	/// @param profile シェーダーープロファイル
	/// @param shader_blob コンパイル後のHLSLデータを受け取る
	/// @param error_blob 失敗時はこっちにエラー情報オブジェクトを入れる
	/// @return 処理成功ならS_OK / そうでなければS_FALSE
	HRESULT CompileShaderFromFile(const std::wstring& filename,
		const std::wstring& profile,
		Microsoft::WRL::ComPtr<ID3DBlob>& shader_blob,
		Microsoft::WRL::ComPtr<ID3DBlob>& error_blob);

	/// @brief HLSLファイルからBlobオブジェクトを作る
	/// @param filename Blob化するHLSLファイルパス
	/// @param profile シェーダープロファイル
	/// @param shader_blob 処理成功ならtrue / そうでなければfalse
	/// @return 
	bool CreateShaderBlob(const std::wstring& filename,
		const std::wstring& profile,
		Microsoft::WRL::ComPtr<ID3DBlob>& shader_blob);

}
