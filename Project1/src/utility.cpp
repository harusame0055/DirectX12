#include"utility.h"

#include<filesystem>
#include<fstream>

#include<dxcapi.h>

namespace ncc {
	using namespace Microsoft::WRL;

	HRESULT CompileShaderFromFile(const std::wstring& filename,
		const std::wstring& profile,
		ComPtr<ID3DBlob>& shader_blob,
		ComPtr<ID3DBlob>& error_blob)
	{
		HRESULT hr = S_FALSE;

		// ファイル名からパスを作る
		std::filesystem::path file_path(filename);
		//ファイルを読み取るオブジェクト
		std::ifstream in_file(file_path);

		// ファイルがなければ失敗
		if (!in_file)
		{
			return hr;
		}

		// ファイルを書き込む動的配列
		std::vector<char> shader_src;
		// shader_srcのサイズをファイルに合わせる
		std::size_t file_size = static_cast<std::size_t>
			(in_file.seekg(0, in_file.end).tellg());
		shader_src.resize(file_size);

		// ファイル全体をshader_srcに読み込む
		in_file.seekg(0, in_file.beg).read(shader_src.data(), shader_src.size());

		// 読み込んだファイル(バイトデータ)をシェーダーソースに変換するオブジェクト
		ComPtr<IDxcLibrary> library;
		DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library));

		// シェーダソースを受け取るオブジェクト
		ComPtr<IDxcBlobEncoding> source;
		// バイトデータからシェーダーソースに変換
		library->CreateBlobWithEncodingFromPinned(
			shader_src.data(),
			static_cast<UINT32>(shader_src.size()),
			CP_UTF8,	// ファイルの文字コード, CP_UTF8 は UTF-8
			&source);	// 受け取る変数

		//DXCコンパイルフラグ
		LPCWSTR compiler_flags[]{
#if _DEBUG
			// デバッグ用フラグ
			L"/Zi",L"/O0",	// デバッグ情報生成・最適化なし
#else
			L"/O2"	// リソースビルドは最適化
#endif
		};
		// シェーダー内のincludeを解決してくれる
		ComPtr<IDxcIncludeHandler> include_handler;
		library->CreateIncludeHandler(include_handler.ReleaseAndGetAddressOf());

		// コンパイルの結果を受け取るオブジェクト
		ComPtr<IDxcOperationResult> result;

		// シェーダーコンパイラ
		ComPtr<IDxcCompiler> compiler;
		DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));

		// シェーダーデータを実行可能な状態にコンパイル
		compiler->Compile(source.Get(),	//シェーダーソース
			file_path.wstring().c_str(),		// ファイルパス
			L"main",	// コンパイル対象のエントリーポイント
			profile.c_str(),		// シェーダーのプロファイル
			compiler_flags,	// コンパイルフラグ
			_countof(compiler_flags),	// フラグの要素数
			nullptr,						//基本的にはnullptrでOK
			0,							// 0でOK
			include_handler.Get(),	// IDxcIncludeHandler
			&result);					// コンパイル結果

		// コンパイルの結果をチェック
		result->GetStatus(&hr);
		if (SUCCEEDED(hr))
		{
			// コンパイル成功, shader_blobにコンパイル後のデータを入れる
			result->GetResult(
				reinterpret_cast<IDxcBlob**>(shader_blob.GetAddressOf()));
		}
		else
		{
			//コンパイル失敗, エラーを渡す
			result->GetErrorBuffer(
				reinterpret_cast<IDxcBlobEncoding**>(error_blob.GetAddressOf()));
		}

		return hr;
	}

	bool CreateShaderBlob(const std::wstring& filename, const std::wstring& profile,
		ComPtr<ID3DBlob>& shader_blob)
	{
		bool ret = true;

		// シェーダープログラムは事前コンパイルしておく方法もあるが今回は都度コンパイル
		ComPtr<ID3DBlob> error;
		// 上で作った関数デコンパイル
		auto hr = CompileShaderFromFile(filename, profile, shader_blob, error);

		// 結果をチェック
		if (FAILED(hr))
		{
			OutputDebugStringA((const char*)error->GetBufferPointer());
			OutputDebugStringA("CompileShaderFromFile Failed.");
			ret = false;
		}
		return ret;
	}
}	// namespace ncc