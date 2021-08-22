#include"collision.h"

namespace ncc {

	using namespace DirectX;

	bool CollisionCircleToCircle(const CircleCollider& c1, const XMFLOAT3& p1, const CircleCollider& c2, const XMFLOAT3& p2)
	{
		auto x = (c1.center.x + p1.x) - (c2.center.x + p2.x);
		auto y = (c1.center.y + p1.y) - (c2.center.y + p2.y);
		// �R���C�_�[�Ԃ̋���
		auto length = x * x + y * y;

		// �R���C�_�[���m���ڐG���Ȃ�����
		auto r = (c1.radius + c2.radius) * (c1.radius + c2.radius);

		if (length <= r)
		{
			return true;
		}
		return false;
	}

}	//	namespace ncc