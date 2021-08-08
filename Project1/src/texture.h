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

	/// @brief システムメモリにあるテクスチャデータをGPUメモリに
	/// アップロードするための処理を行う
	class TextureUploadComandList
	{
	public :
		TextureUploadComandList() = default;
		~TextureUploadComandList() = default;

		/// @brief テクスチャアップロードコマンド記録開始
		/// @param device ID3D12Deviceポインタ
		/// @return 成功ならtrue / 失敗ならfalse
		bool BeginRecording(Microsoft::WRL::ComPtr<ID3D12Device> device);

		/// @brief コマンド記録終了
		/// @return 成功ならtrue / 失敗ならfalse
		bool EndRecording();

		/// @brief バッファデータをGPUメモリに転送
		/// @return 成功ならtrue /　失敗ならfalse
		bool Execute();

		/// @brief GPUメモリにアップロードしたいTextureResourceをコマンドリストに追加する
		/// @param texture 対象のテクスチャ
		/// @return 成功ならtrue /　失敗ならfalse
		bool AddList(TextureResource* texture);

	private:
		Microsoft::WRL::ComPtr<ID3D12Device> device_;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator>allocator_;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list_;

		std::vector<TextureResource*>upload_list_;
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>buffers_;

		bool is_begin_recording_ = false;
	};

	/// @brief テクスチャリソースのシェーダリソースビューを処理するクラス
	class TextureView {
	public:
		TextureView() = default;

		/// @brief デストラクタ
		~TextureView();

		/// @brief ビュー作成
		/// @param device ComPtr<ID3D12Device>
		/// @param srv_heap ビューを書き込むヒープ
		/// @param texture 作成対象テクスチャリソース
		/// @retval true 成功
		/// @retval false 失敗
		bool Create(Microsoft::WRL::ComPtr<ID3D12Device> device,
			DescriptorHeap* srv_heap,
			TextureResource* texture);

		/// @brief 解放処理
		/// @retval true 成功
		/// @retval false 失敗
		bool Release();

		/// @brief ビューが指すリースを取得
		/// @return ComPtr<ID3D12Resource>
		Microsoft::WRL::ComPtr<ID3D12Resource> resource()
		{
			return texture_->resource();
		}

		/// @brief ビューのGPUハンドルを取得
		/// @return D3D12_GPU_DESCRIPTOR_HANDLE
		D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle()
		{
			return handle_.gpu_handle();
		}

		D3D12_CPU_DESCRIPTOR_HANDLE cpu_hadle()
		{
			return handle_.cpu_handle();
		}

	private:
		TextureResource* texture_ = nullptr;
		DescriptorHeap* heap_ = nullptr;
		DescriptorHandle handle_;
	};
}