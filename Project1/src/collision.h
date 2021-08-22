#pragma once
#include<DirectXMath.h>

namespace ncc {

	/// @brief �~�`�R���C�_�[
	struct CircleCollider {
		// �~�̔��a
		float radius = 0;
		// �~�̌��_
		DirectX::XMFLOAT2 center{};
	};

	/// @brief �~�Ɖ~�̏Փ˔���
	/// @param c1 �R���C�_�[�P
	/// @param p1 �R���C�_�[�P�̍��W
	/// @param c2 �R���C�_�[�Q
	/// @param p2 �R���C�_�[�Q�̍��W
	/// @retval true �Փ˂��Ă���
	/// @retval false �Փ˂��ĂȂ�
	bool CollisionCircleToCircle(const CircleCollider& c1, const DirectX::XMFLOAT3& p1, const CircleCollider& c2, const DirectX::XMFLOAT3& p2);
}