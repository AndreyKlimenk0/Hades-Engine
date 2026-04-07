#include "gpu_data.h"
#include "ui_storage.h"
#include "../sys/utils.h"
#include "../sys/sys.h" // call print
#include <imgui/imgui.h>

void UI_Storage::init(Render_Device *_render_device)
{
	render_device = _render_device;
}

void UI_Storage::upload_ui()
{
	ImDrawData *draw_data = ImGui::GetDrawData();

	if (!vertex_buffer || ((vertex_buffer->size() < (sizeof(ImDrawVert) * (u64)draw_data->TotalVtxCount)) && (draw_data->TotalVtxCount > 0))) {
		DELETE_PTR(vertex_buffer);
		Buffer_Desc buffer_desc;
		buffer_desc.usage = RESOURCE_USAGE_UPLOAD;
		buffer_desc.count = draw_data->TotalVtxCount;
		buffer_desc.stride = sizeof(ImDrawVert);
		buffer_desc.name = "ImGUI Vertex Buffer";

		vertex_buffer = render_device->create_buffer(&buffer_desc);
	}

	if (!index_buffer || ((index_buffer->size() < (sizeof(u32) *(u64)draw_data->TotalIdxCount)) && (draw_data->TotalIdxCount > 0))) {
		DELETE_PTR(index_buffer);
		Buffer_Desc buffer_desc;
		buffer_desc.usage = RESOURCE_USAGE_UPLOAD;
		buffer_desc.count = draw_data->TotalIdxCount;
		buffer_desc.stride = sizeof(ImDrawIdx);
		buffer_desc.name = "ImGUI Index Buffer";

		index_buffer = render_device->create_buffer(&buffer_desc);
	}

	draw_commands.reset();

	u32 vertex_offset = 0;
	u32 index_offset = 0;

	Vector2 clipOffset = { draw_data->DisplayPos.x, draw_data->DisplayPos.y };;

	for (s32 cmdListIdx = 0; cmdListIdx < draw_data->CmdListsCount; ++cmdListIdx) {
		const ImDrawList *imCommandList = draw_data->CmdLists[cmdListIdx];

		vertex_buffer->write(imCommandList->VtxBuffer.Data, sizeof(ImDrawVert) * imCommandList->VtxBuffer.Size, vertex_offset);
		index_buffer->write(imCommandList->IdxBuffer.Data, sizeof(ImDrawIdx) * imCommandList->IdxBuffer.Size, index_offset);

		print("vertex_offset", vertex_offset);
		print("vertex_offset", vertex_offset);

		for (s32 cmdBufferIdx = 0; cmdBufferIdx < imCommandList->CmdBuffer.Size; ++cmdBufferIdx) {
			const ImDrawCmd *imCommand = &imCommandList->CmdBuffer[cmdBufferIdx];

			UI_Draw_Command draw_command;
			draw_command.vertex_buffer_offset = vertex_offset;
			draw_command.index_buffer_offset = index_offset;
			draw_command.index_count = imCommand->ElemCount;

			if (imCommand->TexRef._TexData) {
				if ((imCommand->TexRef._TexData->Status == ImTextureStatus_WantCreate)) {
					if (imCommand->TexRef._TexData->Format == ImTextureFormat_RGBA32) {

					}
					int x = 0;
				}
			}

			draw_command.texture = imCommand->GetTexID() != ImTextureID_Invalid ? (Texture *)imCommand->GetTexID() : NULL;
			draw_command.clip_rect = static_cast<Rect_u32>(Rect_f32{ imCommand->ClipRect.x - clipOffset.x, imCommand->ClipRect.y - clipOffset.y,
																	 imCommand->ClipRect.z - clipOffset.x, imCommand->ClipRect.w - clipOffset.y });

			draw_commands.push(draw_command);
			index_offset += imCommand->ElemCount;
		}

		vertex_offset += imCommandList->VtxBuffer.Size;
	}
}

void UI_Storage::upload_font()
{
	ImGuiIO *io = &ImGui::GetIO();

	uint8_t *pixels;
	int width, height;
	io->Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	if (!font_texture) {
		Texture_Desc font_texture_desc;
		font_texture_desc.dimension = TEXTURE_DIMENSION_2D;
		font_texture_desc.width = width;
		font_texture_desc.height = height;
		font_texture_desc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
		font_texture_desc.data = (void *)pixels;

		font_texture = render_device->create_texture(&font_texture_desc);
	}

	Texture_Desc desc = font_texture->get_texture_desc();
	if (desc.width != width || desc.height != height) {
		DELETE_PTR(font_texture);
		Texture_Desc font_texture_desc;
		font_texture_desc.dimension = TEXTURE_DIMENSION_2D;
		font_texture_desc.width = width;
		font_texture_desc.height = height;
		font_texture_desc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
		font_texture_desc.data = (void *)pixels;

		font_texture = render_device->create_texture(&font_texture_desc);
	}
	io->Fonts->SetTexID(ImTextureRef((void *)font_texture));
	io->Fonts->TexRef._TexData->SetStatus(ImTextureStatus_OK);
	int x = 0;
}

void UI_Storage::prepare_for_rendering()
{
	upload_font();
	upload_ui();

	if (!ui_data_buffer) {
		Buffer_Desc buffer_desc;
		buffer_desc.usage = RESOURCE_USAGE_UPLOAD;
		buffer_desc.stride = sizeof(GPU_UI_Frame_Data);
		buffer_desc.name = "UI Data Buffer";

		ui_data_buffer = render_device->create_buffer(&buffer_desc);
	}
	ImDrawData *draw_data = ImGui::GetDrawData();

	float L = draw_data->DisplayPos.x;
	float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
	float T = draw_data->DisplayPos.y;
	float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;

	Matrix4 projection_matrix = {
		2.0f / (R - L),     0.0f,              0.0f,       0.0f,
		0.0f,               2.0f / (T - B),    0.0f,       0.0f,
		0.0f,               0.0f,              0.5f,       0.0f ,
		(R + L) / (L - R),  (T + B) / (B - T), 0.5f,       1.0f ,
	};

	GPU_UI_Frame_Data ui_data;
	//ui_data.projection_matrix = projection_matrix;
	ui_data.projection_matrix = make_orthographic_matrix(L, R, B, T, 1.0f, 1000.0f);
	ui_data.font_texture_idx = font_texture->shader_resource_descriptor()->index();

	ui_data_buffer->write((void *)&ui_data, sizeof(GPU_UI_Frame_Data));
}
