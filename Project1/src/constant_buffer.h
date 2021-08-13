#pragma once

#include<cstdint>
#include<memory>
#include<string>
#include<vector>

#include<d3d12.h>
#include<wrl.h>

#include"descriptor_heap.h"

namespace ncc {

	/// @brief 定数バッファのリソースデータを管理するクラス
	/// 毎フレーム更新するような定数データのためにバッファリング対応
	class ConstantBuffer {
	public:
		ConstantBuffer() = default;

		/// @brief デストラクタ
		~ConstantBuffer();

		/// @brief 初期化。使う前に必ず実行
		/// @param device D3D12Device
		/// @param buffering_count 定数バッファのバッファリング数
		/// @param byte_size 書き込むデータサイズ
		/// @retval true 正常終了
		/// retval false 失敗
		bool Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device, int buffering_count, std::size_t byte_size);

		/// @brief 解放処理
		void Release();

		/// @brief バッファへデータ書き込み
		/// @param data 書き込みデータの先頭アドレス
		/// @param index バッファリングされているので、書き込むインデックスを指定
		void UpdateBuffer(const void* data, int index);

		/// @brief 定数バッファリソースのポインタを返す
		/// @param index 要素のインデックス
		/// @return ID3D12Resourceポインタ
		Microsoft::WRL::ComPtr<ID3D12Resource> resource(int index)
		{
			return resources_[index];
		}

		/// @brief 初期化時に作成したリソース数を返す
		/// @return 作成してあるバッファ数
		std::size_t buffer_count() const
		{
			return resources_.size();
		}

		/// @brief 定数バッファのD3D12_RESOURCE_DESC
		/// @return D3D12_RESOURCE_DESCのコピー
		D3D12_RESOURCE_DESC resource_desc() const
		{
			return resource_desc_;
		}

	private:
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> resources_;
		D3D12_HEAP_PROPERTIES heap_props_{};
		D3D12_RESOURCE_DESC resource_desc_{};
	};

	/// @brief 定数バッファビュー
	class ConstantBufferView {
	public:
		ConstantBufferView() = default;

		/// @brief デストラクタ
		~ConstantBufferView();

		/// @brief ビュー作成
		/// @param device ComPtr<ID3D12Device>
		/// @param heap ビュー作成するヒープ
		/// @param cbuffer ビューを作成するバッファ
		/// @retval true 成功
		/// @return false 失敗
		bool Create(Microsoft::WRL::ComPtr<ID3D12Device> device,
			DescriptorHeap* heap,
			ConstantBuffer* cbuffer);

		/// @brief 解放処理
		void Release();

		/// @brief 初期化時に作成したリソース数を返す
		/// @return 作成してあるバッファ数
		std::size_t view_count() const
		{
			return handles_.size();
		}

		/// @brief 定数バッファリソースのディスクリプタハンドルを返す
		/// @param index 取得する要素のインデックス
		/// @return DescriptorHandleのコピー
		DescriptorHandle handle(int index)
		{
			return handles_[index];
		}

	private:
		std::vector<DescriptorHandle> handles_;

		DescriptorHeap* heap_ = nullptr;
	};

}