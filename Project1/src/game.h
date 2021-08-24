#pragma once
#include<vector>

#include<d3d12.h>
#include<wrl.h>

#include"collision.h"
#include"descriptor_heap.h"
#include"fps_timer.h"
#include"sprite_renderer.h"
#include"texture.h"

namespace DirectX {
	class Keyboard;
}

namespace ncc {

	class DeviceContext;

	/// @brief �v���C���[�\����
	struct Player {
		Sprite sprite{};
		CircleCollider collider{};

		// �ړ����x
		float speed = 100.0f;
		// ��]���x
		float rotation_speed = DirectX::XM_PI;
	};

	/// @brief �o���b�g�\����
	struct Bullet {
		Sprite sprite{};
		CircleCollider collider{};

		// �e�̑��x
		float speed = 100.0f;
	};

	/// @brief �G�\����
	struct Enemy {
		// �o�����t�F�[�h�C�������Ɏg������
		static constexpr float fadeTime{ 2.0f };

		// �G�̏��
		enum class State {
			None = 0,
			Spawn,
			Battle,
			Dead,
			Size
		};

		Sprite sprite{};
		CircleCollider collider{};

		// ���̓G�f�[�^���g���Ă��邩�̃t���O
		bool is_active{ false };

		// ���݂̏��
		State state{ Enemy::State::Spawn };

		// �ėp�^�C�}�[
		float timer = 0;
	};

	/// @brief ���C���ƂȂ鏈�����W�߂��N���X
	class Game {
	public:
		/// @brief ������
		/// @param device_context DeviceContext*
		/// @param keyboard DirectX::Keyboard*
		/// @retval true ����
		/// @retval false ���s
		bool Initialize(ncc::DeviceContext* device_context,
			DirectX::Keyboard* keyboard);

		/// @brief �I������
		void Finalize();

		/// @brief �X�V����
		/// @param timer FpsTimer&
		void Update(ncc::FpsTimer& timer);

		/// @brief �`�揈��
		void Render();

	private:
		void CheckCollision();
		bool CreateTexture();
		void MakeEnemySprite(Sprite& sprite);
		void MakeExplosionSprite(Sprite& sprite);
		void MakePlayer();
		void SpawnEnemy(float delta_time);
		void UpdatePlayer(float delta_time);
		void UpdateEnemy(float delta_time);

		// �X�v���C�g�����_��
		SpriteRenderer sprite_renderer_;

		// �v���C���[
		Player player_;

		// �G�̔z��
		std::vector<Enemy>enemys_;
		// �G�̍ő吔
		static constexpr int enemySize = 10;

		// �G�͈�莞�ԗ��ɕ����A2.0f�Ŕ���
		static constexpr float respawnTime = 2.0f;

		// ���X�|�[�����Ԃ��Ǘ�
		float respawn_timer_ = 0.0f;

		//�e�N�X�`���̎��
		enum class TextureType : int {
			SHIPANIMATED,
			SPACESHIP,
			EXPLOSION,
			SIZE
		};

		// �t�@�C�����̔z��
		const std::vector<std::wstring>texture_path_{
			L"asset/shipmated.png",
			L"asset/Spaceship.png",
			L"asset/Explosion.png",
			L"asset/Bullet.png",
		};

		// �e�N�X�`�����\�[�X
		std::vector<TextureResource> textures_;
		// �e�N�X�`���r���[
		std::vector<TextureView>texture_views_;
		// �e�N�X�`���r���[�̃q�[�v
		DescriptorHeap srv_cbv_heap_;

		// �f�o�C�X
		DeviceContext* device_context_ = nullptr;
		// �L�[�{�[�h
		DirectX::Keyboard* keyboard_ = nullptr;

	};

}	// namespace ncc