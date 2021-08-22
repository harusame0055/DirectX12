#include"game.h"

#include"keyboard.h"
#include"device_context.h"

#include<random>

using namespace DirectX;

namespace {

	using namespace ncc;

	constexpr int shipSize = 48;

	std::random_device seed;
	std::mt19937 random_engine(seed());

	// std::uniform_real_distriution = 乱数を元にある範囲の実数値を作成
	std::uniform_real_distribution<float> rotation_dist(0, DirectX::XM_2PI);
	std::uniform_real_distribution<float> length_dist(0.0, 300.0);

	/// @brief ランダムなラジアン角を取得
	/// @return ラジアン角
	float GetRandomRotation()
	{
		return rotation_dist(random_engine);
	}


	std::vector <Cell>GetEnemyCellData()
	{
		constexpr int base_x = 192;

		std::vector<Cell> cells = {
			{base_x,			0,		shipSize,shipSize},
			{base_x + shipSize,		0,		shipSize,shipSize},
			{base_x + (shipSize * 2),0,shipSize,shipSize},
			{base_x + (shipSize * 3),0,shipSize,shipSize},
		};
		return cells;
	}

	/// @brief 爆発用のセルデータの配列を取得
	/// @return std::vector<Cell>
	std::vector<Cell> GetExplosionCellData()
	{
		constexpr auto size = 80;
		constexpr auto cell_size = 11;

		std::vector<Cell> cells;
		for (int i = 0; i < cell_size; i++)
		{
			std::uint32_t x = i % size;
			std::uint32_t y = i / size;

			cells.emplace_back(
				Cell{
				x * size,
				y * size,
				size,
				size }
			);
		}
		return cells;
	}

} // namespace

namespace ncc {

#pragma once

#pragma region public
	bool Game::Initialize(ncc::DeviceContext* device_context,
		DirectX::Keyboard* keyboard)
	{
		assert(device_context && keyboard);
		device_context_ = device_context;
		keyboard_ = keyboard;

		if (!CreateTexture())
		{
			return false;
		}

		sprite_renderer_.Initialize(
			device_context->device(),
			device_context->backbuffer_size(),
			device_context->screen_viewport());

		MakePlayer();

		// 敵データの初期化
		{
			enemys_.resize(enemySize);
			for (auto& enemy : enemys_)
			{
				enemy.is_active = false;
				enemy.state = Enemy::State::None;
				enemy.timer = 0;
				enemy.collider.radius = shipSize * 0.5f;
			}
		}

		return true;
	}

	void Game::Finalize()
	{
		for (auto& view : texture_views_)
		{
			view.Release();
		}

		for (auto& tex : textures_)
		{
			tex.Release();
		}

		srv_cbv_heap_.Finalize();

		device_context_ = nullptr;
	}

	void Game::Update(ncc::FpsTimer& timer)
	{
		float delta_time = timer.delta_seconds();

		UpdatePlayer(delta_time);
		UpdateEnemy(delta_time);

		CheckCollision();
	}

	void Game::Render()
	{
		ID3D12DescriptorHeap* heaps[]{ srv_cbv_heap_.heap().Get() };
		device_context_->command_list()->SetDescriptorHeaps(_countof(heaps), heaps);

		sprite_renderer_.Begin(device_context_->backbuffer_index(), device_context_->command_list());
		{
			sprite_renderer_.Draw(player_.sprite);

			for (auto& enemy : enemys_)
			{
				if (enemy.is_active)
				{
					sprite_renderer_.Draw(enemy.sprite);
				}
			}
		}
		sprite_renderer_.End();
	}

#pragma endregion

#pragma region private

	/// @brief テクスチャデータを作成
	/// @retval true 成功
	/// @retval false 失敗
	bool Game::CreateTexture()
	{
		const int texture_count = static_cast<int>(TextureType::SIZE);
		textures_.resize(texture_count);
		texture_views_.resize(texture_count);

		for (int i = 0; i < texture_count; i++)
		{
			textures_[i].Reset(texture_path_[i]);
		}

		//	テクスチャロード
		for (int i = 0; i < texture_count; i++)
		{
			if (!LoadTextureFromFile(device_context_->device(), &textures_[i]))
			{
				return false;
			}
		}

		// テクスチャアップロードコマンド作成開始
		TextureUploadCommandList upload_command;
		if (!upload_command.BeginRecording(device_context_->device()))
		{
			return false;
		}

		for (int i = 0; i < texture_count; i++)
		{
			if (!upload_command.AddList(&textures_[i]))
			{
				return false;
			}
		}

		if (!upload_command.EndRecording())
		{
			return false;
		}
		if (!upload_command.Execute())
		{
			return false;
		}

		// ディスクリプタヒープ作成
		D3D12_DESCRIPTOR_HEAP_DESC desc{};
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = 1024;
		desc.NodeMask = 0;
		if (!srv_cbv_heap_.Initialize(device_context_->device(), desc))
		{
			return false;
		}

		// ビュー作成
		for (int i = 0; i < texture_count; i++)
		{
			if (!texture_views_[i].Create(device_context_->device(), &srv_cbv_heap_, &textures_[i]))
			{
				return false;
			}
		}

		return true;
	}

	/// @brief プレイヤーデータ作成
	void Game::MakePlayer()
	{
		{
			int type = static_cast<int>(TextureType::SHIPANIMATED);

			auto desc = textures_[type].resource()->GetDesc();
			XMUINT2 tex_size{
			static_cast<std::uint32_t>(desc.Width),
			static_cast<std::uint32_t>(desc.Height),
			};

			player_.sprite.SetTextureData(
				texture_views_[type].gpu_handle(),
				tex_size);

			player_.sprite.cell_data(
				{
				{0,	0,shipSize,shipSize},
				{shipSize,0,shipSize,shipSize},
				{shipSize * 2.0,shipSize,shipSize},
				{shipSize * 3.0,shipSize,shipSize},
				}
			);

			player_.sprite.animation_state(Sprite::AnimationState::Play);
			player_.sprite.is_loop(true);

			player_.sprite.position.x = device_context_->screen_viewport().Width * 0.5f;
			player_.sprite.position.y = device_context_->screen_viewport().Height * 0.5f;

			player_.collider.radius = shipSize * 0.5f;
		}
	}

	/// @brief 敵のスプライトデータ作成 
	void Game::MakeEnemySprite(Sprite& sprite)
	{
		const auto type = static_cast<int>(TextureType::SPACESHIP);
		auto desc = textures_[type].resource()->GetDesc();
		XMUINT2 tex_size{
			static_cast<std::uint32_t>(desc.Width),
			static_cast<std::uint32_t>(desc.Height),
		};

		sprite.SetTextureData(texture_views_[type].gpu_handle(),
			tex_size);
		sprite.cell_data(GetEnemyCellData());
		sprite.animation_state(Sprite::AnimationState::Play);
		sprite.is_loop(true);
	}

	/// @brief 爆発のスプライトデータ作成 
	void Game::MakeExplosionSprite(Sprite& sprite)
	{
		const auto type = static_cast<int>(TextureType::EXPLOSION);
		auto desc = textures_[type].resource()->GetDesc();
		XMUINT2 tex_size{
			static_cast<std::uint32_t>(desc.Width),
			static_cast<std::uint32_t>(desc.Height),
		};

		sprite.SetTextureData(texture_views_[type].gpu_handle(),
			tex_size);
		sprite.cell_data(GetEnemyCellData());
		sprite.animation_state(Sprite::AnimationState::Play);
		sprite.is_loop(false);
	}

	/// @brief プレイヤーの更新処理
	/// @param delta_time フレームのデルタタイム
	void Game::UpdatePlayer(float delta_time)
	{
		if (keyboard_->GetState().IsKeyDown(Keyboard::A))
		{
			player_.sprite.rotatin += player_.rotation_speed * delta_time;
		}
		else if (keyboard_->GetState().IsKeyDown(Keyboard::D))
		{
			player_.sprite.rotatin -= player_.rotation_speed * delta_time;
		}

		if (keyboard_->GetState().IsKeyDown(Keyboard::W))
		{
			auto r = player_.sprite.rotatin + XM_PIDIV2;
			player_.sprite.position.x += player_.speed * XMScalarCos(r) * delta_time;
			player_.sprite.position.y -= player_.speed * XMScalarSin(r) * delta_time;
		}
		else if (keyboard_->GetState().IsKeyDown(Keyboard::S))
		{

		}
	}
#pragma endregion

}