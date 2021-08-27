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

	// std::uniform_real_distriution = ���������ɂ���͈͂̎����l���쐬
	std::uniform_real_distribution<float> rotation_dist(0, DirectX::XM_2PI);
	std::uniform_real_distribution<float> length_dist(0.0, 300.0);

	/// @brief �����_���ȃ��W�A���p���擾
	/// @return ���W�A���p
	float GetRandomRotation()
	{
		return rotation_dist(random_engine);
	}

	/// @brief �����_���Ȓ������擾
	/// @return 0 - 300 �̒l
	float GetRandomLength()
	{
		return length_dist(random_engine);
	}

	/// @brief �G�̃Z���f�[�^�̔z����擾
	/// @return std::vector<Cell>
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

	/// @brief �����p�̃Z���f�[�^�̔z����擾
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

		// �G�f�[�^�̏�����
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

	/// @brief �e�N�X�`���f�[�^���쐬
	/// @retval true ����
	/// @retval false ���s
	bool Game::CreateTexture()
	{
		const int texture_count = static_cast<int>(TextureType::SIZE);
		textures_.resize(texture_count);
		texture_views_.resize(texture_count);

		for (int i = 0; i < texture_count; i++)
		{
			textures_[i].Reset(texture_path_[i]);
		}

		//	�e�N�X�`�����[�h
		for (int i = 0; i < texture_count; i++)
		{
			if (!LoadTextureFromFile(device_context_->device(), &textures_[i]))
			{
				return false;
			}
		}

		// �e�N�X�`���A�b�v���[�h�R�}���h�쐬�J�n
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

		// �f�B�X�N���v�^�q�[�v�쐬
		D3D12_DESCRIPTOR_HEAP_DESC desc{};
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = 1024;
		desc.NodeMask = 0;
		if (!srv_cbv_heap_.Initialize(device_context_->device(), desc))
		{
			return false;
		}

		// �r���[�쐬
		for (int i = 0; i < texture_count; i++)
		{
			if (!texture_views_[i].Create(device_context_->device(), &srv_cbv_heap_, &textures_[i]))
			{
				return false;
			}
		}

		return true;
	}

	/// @brief �v���C���[�f�[�^�쐬
	void Game::MakePlayer()
	{
		{
			int type = static_cast<int>(TextureType::SPACESHIP);

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
				{shipSize * 2,0,shipSize,shipSize},
				{shipSize * 3,0,shipSize,shipSize},
				}
			);

			player_.sprite.animation_state(Sprite::AnimationState::Play);
			player_.sprite.is_loop(true);

			player_.sprite.position.x = device_context_->screen_viewport().Width * 0.5f;
			player_.sprite.position.y = device_context_->screen_viewport().Height * 0.5f;

			player_.collider.radius = shipSize * 0.5f;
		}
	}

	/// @brief �G�̃X�v���C�g�f�[�^�쐬 
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

	/// @brief �����̃X�v���C�g�f�[�^�쐬 
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
		sprite.cell_data(GetExplosionCellData());
		sprite.animation_state(Sprite::AnimationState::Play);
		sprite.is_loop(false);
	}

	/// @brief �v���C���[�̍X�V����
	/// @param delta_time �t���[���̃f���^�^�C��
	void Game::UpdatePlayer(float delta_time)
	{
		if (keyboard_->GetState().IsKeyDown(Keyboard::A))
		{
			player_.sprite.rotation += player_.rotation_speed * delta_time;
		}
		else if (keyboard_->GetState().IsKeyDown(Keyboard::D))
		{
			player_.sprite.rotation -= player_.rotation_speed * delta_time;
		}

		if (keyboard_->GetState().IsKeyDown(Keyboard::W))
		{
			auto r = player_.sprite.rotation + XM_PIDIV2;
			player_.sprite.position.x += player_.speed * XMScalarCos(r) * delta_time;
			player_.sprite.position.y -= player_.speed * XMScalarSin(r) * delta_time;
		}
		else if (keyboard_->GetState().IsKeyDown(Keyboard::S))
		{
			auto r = player_.sprite.rotation + XM_PIDIV2;
			player_.sprite.position.x -= player_.speed * XMScalarCos(r) * delta_time;
			player_.sprite.position.y += player_.speed * XMScalarSin(r) * delta_time;
		}

		player_.sprite.Update(delta_time);
	}

	/// @brief �G�̍X�V����
	/// @param delta_time �t���[���̃f���^�^�C��
	void Game::UpdateEnemy(float delta_time)
	{
		SpawnEnemy(delta_time);

		for (auto& enemy : enemys_)
		{
			if (!enemy.is_active)
			{
				continue;
			}

			enemy.sprite.Update(delta_time);

			switch (enemy.state)
			{
			case Enemy::State::Spawn:
			{
				enemy.timer -= delta_time;
				enemy.sprite.color.w += delta_time * 0.5f;
				if (enemy.timer <= 0.f)
				{
					enemy.state = Enemy::State::Battle;
					enemy.sprite.color.w = 1.0f;
					enemy.timer = 10.0f;
				}
			}
			break;

			case Enemy::State::Battle:
			{
				auto& sprite = enemy.sprite;

				sprite.position.x += DirectX::XMScalarCos(sprite.rotation - XM_PIDIV2);
				sprite.position.y -= DirectX::XMScalarSin(sprite.rotation - XM_PIDIV2);

				if (sprite.position.x < 0.f)
				{
					sprite.position.x = 0.f;
					sprite.rotation += DirectX::XM_PI;
				}
				else if (sprite.position.x > device_context_->screen_viewport().Width)
				{
					sprite.position.x = device_context_->screen_viewport().Width;
					sprite.rotation += DirectX::XM_PI;
				}

				if (sprite.position.y < 0.f)
				{
					sprite.position.y = 0.f;
					sprite.rotation += DirectX::XM_PI;
				}
				else if (sprite.position.y > device_context_->screen_viewport().Height)
				{
					sprite.position.y = device_context_->screen_viewport().Height;
					sprite.rotation += DirectX::XM_PI;
				}

				// �G����莞�Ԃŏ���
				enemy.timer -= delta_time;
				if (enemy.timer <= 0.f)
				{
					enemy.state = Enemy::State::Dead;
					MakeExplosionSprite(enemy.sprite);
				}
			}
			break;

			case Enemy::State::Dead:
			{
				if (enemy.sprite.animation_state() == Sprite::AnimationState::End)
				{
					enemy.is_active = false;
					enemy.state = Enemy::State::None;
				}
			}
			break;

			case Enemy::State::None:
			case Enemy::State::Size:
			default:
				assert(false);
				break;
			}
		}
	}

	/// @brief �G�̔�������
	/// @param delta_time �t���[���̃f���^�^�C��
	void Game::SpawnEnemy(float delta_time)
	{
		respawn_timer_ += delta_time;

		// ��莞�Ԗ��ɓG����
		if (respawn_timer_ > respawnTime)
		{
			// �G�̔����t���O
			bool is_spawn = false;

			// �I�z��𑖍����ċ󂢂Ă���G��������
			for (auto& enemy : enemys_)
			{
				if (enemy.is_active)
				{
					continue;
				}

				// �������̂Ńf�[�^������
				enemy.is_active = true;
				enemy.state = Enemy::State::Spawn;
				enemy.timer = 2.f;

				MakeEnemySprite(enemy.sprite);

				// Spawn�ʒu�ƌ������v�Z
				auto rot = GetRandomRotation();
				auto len = GetRandomLength();

				auto x = device_context_->screen_viewport().Width * 0.5f;
				auto y = device_context_->screen_viewport().Height * 0.5f;

				enemy.sprite.position = XMFLOAT3{
					x + len * DirectX::XMScalarCos(rot),
					y + len * DirectX::XMScalarSin(rot),
					0.6f
				};
				enemy.sprite.color.w = 0.0f;
				enemy.sprite.rotation = rot;

				is_spawn = true;
				break;
			}

			// �����������������Ԃ��O�ɂ���
			if (is_spawn)
			{
				respawn_timer_ = 0;
			}
		}
	}

	/// @brief �v���C���[�ƓG�̏Փ˃`�F�b�N
	void Game::CheckCollision()
	{
		for (auto& enemy : enemys_)
		{
			if (enemy.is_active && enemy.state == Enemy::State::Battle)
			{
				if (CollisionCircleToCircle(enemy.collider, enemy.sprite.position, player_.collider, player_.sprite.position))
				{
					// �Փ˂��Ă���G�𔚔���Ԃɂ���
					enemy.state = Enemy::State::Dead;
					MakeExplosionSprite(enemy.sprite);
				}
			}
		}
	}
#pragma endregion

} // namespace ncc