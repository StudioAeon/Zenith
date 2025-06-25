#pragma once

#include "Zenith/Asset/Asset.hpp"

namespace Zenith {

	using ResourceDescriptorInfo = void*;

	class RendererResource : public Asset
	{
	public:
		virtual ResourceDescriptorInfo GetDescriptorInfo() const = 0;
	};

}
