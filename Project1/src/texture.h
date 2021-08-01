#pragma once

#include<cstddef>
#include<memory>
#include<string>
#include<vector>

#include<d3d12.h>
#include<wrl.h>

#include"descriptor_heap.h"

namespace ncc {

	class TextureResource;

	/// @brief 画像ファイルをシステムメモリにロードして
	///	GPUメモリにリソース領域を確保する. データ転送はしないので注意
	/// @param device ID3D12Deviceポインタ
	/// @param texture TextureResourceの参照
	/// @return 成功ならtrue,失敗ならfalse.
	bool LoadTextureFromFile(Microsoft::WRL::ComPtr<ID3D12Device> device,
		TextureResource* texture);

	/// @brief テクスチャリソースを管理するクラス
	class TextureResource {
	public:
		TextureResource() = default;


		/// @brief コピーコンストラクタ
		/// @param o コピー元
		TextureResource(const TextureResource& o) : TextureResource(o.name_)
		{}

		TextureResource(const std::wstring& name)
			:name_(name)
		{}

		~TextureResource();

		/// @brief テクスチャデータがGPUに未アップロード済みか
		/// @return アップロード済みならtrue, そうでなければfalse.
		bool IsUploaded()
		{
			return !is_upload_wait_;
		}

		bool IsValid()
		{
			if (resource_ == nullptr ||
				is_upload_wait_ == true ||
				is_enabled_ == false)
			{
				return false;
			}
			return true;
		}

		/// @brief 現在のテクスチャを開放して新しいテクスチャリソースを作れるようにする
		/// @param name 新しいテクスチャ名
		void Reset(const std::wstring& name)
		{
			Release();
			name_ = name;
		}

		/// @brief オブジェクトのデータを開放する
		void Release()
		{
			name_.clear();

			is_upload_wait_ = false;
			is_enabled_ = false;

			subresource_ = D3D12_SUBRESOURCE_DATA{};
			data_.reset();
			resource_.Reset();
		}

		/// @brief GPUメモリに作成されたリソースを返す
		/// @return テクスチャリソース
		Microsoft::WRL::ComPtr<ID3D12Resource> resource()
		{
			return resource_;
		}

		/// @brief テクスチャ名取得
		/// @return テクスチャ名
		const std::wstring& name()
		{
			return name_;
		}

	private:
		friend bool LoadTextureFromFile(Microsoft::WRL::ComPtr<ID3D12Device> device,
			TextureResource* texture);
		friend class TextureUploadComandList;

		Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
		std::wstring name_;
		bool is_upload_wait_ = false;
		bool is_enabled_ = false;
		D3D12_SUBRESOURCE_DATA subresource_{};
		std::unique_ptr<std::uint8_t[]> data_;

	};


}