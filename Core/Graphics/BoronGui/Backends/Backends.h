#pragma once

#include "GLOBALS.h"
#include "BoronGuiTypes.h"
#include "Widgets/Frame.h"

namespace BoronGuiBackends {
	class Backends {
	public:
		virtual ~Backends() = default;
		virtual void RenderAFrame(Borongui::Frame frame) = 0;
		virtual void ReSizeViewport(GPUVector2 p_newSize) = 0;
		virtual void Init() = 0;
		virtual void SetBoronGuiNeeds(BoronGuiNeeds& p_boronGuiNeeds) = 0;
		virtual void UpdatePerFrameOBJ(PerFrameStuct& p_perFrameStuct) = 0;
		virtual void UploadBatch(const std::vector<GuiVertex>& vertices, const std::vector<uint32_t>& p_indices) = 0;
		virtual void DrawBatch() = 0;
	private:

	};
};