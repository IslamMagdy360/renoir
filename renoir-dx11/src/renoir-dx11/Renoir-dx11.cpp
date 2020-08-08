#include "renoir-dx11/Exports.h"

#include <renoir/Renoir.h>

#include <mn/Thread.h>
#include <mn/Pool.h>
#include <mn/Buf.h>
#include <mn/Defer.h>
#include <mn/Log.h>
#include <mn/Map.h>
#include <mn/Str.h>
#include <mn/Str_Intern.h>

#include <atomic>
#include <assert.h>

#include <d3d11.h>
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <dxgi.h>

inline static int
_renoir_buffer_type_to_dx(RENOIR_BUFFER type)
{
	switch(type)
	{
	case RENOIR_BUFFER_VERTEX: return D3D11_BIND_VERTEX_BUFFER;
	case RENOIR_BUFFER_UNIFORM: return D3D11_BIND_CONSTANT_BUFFER;
	case RENOIR_BUFFER_INDEX: return D3D11_BIND_INDEX_BUFFER;
	case RENOIR_BUFFER_COMPUTE: return D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	default: assert(false && "unreachable"); return 0;
	}
}

inline static D3D11_USAGE
_renoir_usage_to_dx(RENOIR_USAGE usage)
{
	switch(usage)
	{
	case RENOIR_USAGE_STATIC: return D3D11_USAGE_IMMUTABLE;
	case RENOIR_USAGE_DYNAMIC: return D3D11_USAGE_DYNAMIC;
	default: assert(false && "unreachable"); return D3D11_USAGE_DEFAULT;
	}
}

inline static int
_renoir_access_to_dx(RENOIR_ACCESS access)
{
	switch(access)
	{
	case RENOIR_ACCESS_NONE: return 0;
	case RENOIR_ACCESS_READ: return D3D11_CPU_ACCESS_READ;
	case RENOIR_ACCESS_WRITE: return D3D11_CPU_ACCESS_WRITE;
	case RENOIR_ACCESS_READ_WRITE: return D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
	default: assert(false && "unreachable"); return 0;
	}
}

inline static DXGI_FORMAT
_renoir_pixelformat_to_dx(RENOIR_PIXELFORMAT format)
{
	switch(format)
	{
	case RENOIR_PIXELFORMAT_RGBA8: return DXGI_FORMAT_R8G8B8A8_UNORM;
	case RENOIR_PIXELFORMAT_R16I: return DXGI_FORMAT_R16_SINT;
	case RENOIR_PIXELFORMAT_R16F: return DXGI_FORMAT_R16_FLOAT;
	case RENOIR_PIXELFORMAT_R32F: return DXGI_FORMAT_R32_FLOAT;
	case RENOIR_PIXELFORMAT_R32G32F: return DXGI_FORMAT_R32G32_FLOAT;
	case RENOIR_PIXELFORMAT_D24S8: return DXGI_FORMAT_R24G8_TYPELESS;
	case RENOIR_PIXELFORMAT_D32: return DXGI_FORMAT_R32_TYPELESS;
	case RENOIR_PIXELFORMAT_R8: return DXGI_FORMAT_R8_UNORM;
	default: assert(false && "unreachable"); return DXGI_FORMAT_R8G8B8A8_UNORM;
	}
}

inline static int
_renoir_pixelformat_to_size(RENOIR_PIXELFORMAT format)
{
	switch(format)
	{
	case RENOIR_PIXELFORMAT_RGBA8:
	case RENOIR_PIXELFORMAT_D32:
	case RENOIR_PIXELFORMAT_R32F:
	case RENOIR_PIXELFORMAT_D24S8:
		return 4;
	case RENOIR_PIXELFORMAT_R16I:
	case RENOIR_PIXELFORMAT_R16F:
		return 2;
	case RENOIR_PIXELFORMAT_R32G32F: return 8;
	case RENOIR_PIXELFORMAT_R8: return 1;
	default: assert(false && "unreachable"); return 0;
	}
}

inline static bool
_renoir_pixelformat_is_depth(RENOIR_PIXELFORMAT format)
{
	return (format == RENOIR_PIXELFORMAT_D32 ||
			format == RENOIR_PIXELFORMAT_D24S8);
}

inline static DXGI_FORMAT
_renoir_pixelformat_depth_to_dx_shader_view(RENOIR_PIXELFORMAT format)
{
	switch(format)
	{
	case RENOIR_PIXELFORMAT_D24S8: return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	case RENOIR_PIXELFORMAT_D32: return DXGI_FORMAT_R32_TYPELESS;
	default: assert(false && "unreachable"); return (DXGI_FORMAT)0;
	}
}

inline static DXGI_FORMAT
_renoir_pixelformat_depth_to_dx_depth_view(RENOIR_PIXELFORMAT format)
{
	switch(format)
	{
	case RENOIR_PIXELFORMAT_D24S8: return DXGI_FORMAT_D24_UNORM_S8_UINT;
	case RENOIR_PIXELFORMAT_D32: return DXGI_FORMAT_D32_FLOAT;
	default: assert(false && "unreachable"); return (DXGI_FORMAT)0;
	}
}

inline static D3D11_BLEND
_renoir_blend_to_dx(RENOIR_BLEND blend)
{
	switch(blend)
	{
	case RENOIR_BLEND_ZERO: return D3D11_BLEND_ZERO;
	case RENOIR_BLEND_ONE: return D3D11_BLEND_ONE;
	case RENOIR_BLEND_SRC_COLOR: return D3D11_BLEND_SRC_COLOR;
	case RENOIR_BLEND_ONE_MINUS_SRC_COLOR: return D3D11_BLEND_INV_SRC_COLOR;
	case RENOIR_BLEND_DST_COLOR: return D3D11_BLEND_DEST_COLOR;
	case RENOIR_BLEND_ONE_MINUS_DST_COLOR: return D3D11_BLEND_INV_DEST_COLOR;
	case RENOIR_BLEND_SRC_ALPHA: return D3D11_BLEND_SRC_ALPHA;
	case RENOIR_BLEND_ONE_MINUS_SRC_ALPHA: return D3D11_BLEND_INV_SRC_ALPHA;
	default: assert(false && "unreachable"); return D3D11_BLEND_ZERO;
	}
}

inline static D3D11_BLEND_OP
_renoir_blend_eq_to_dx(RENOIR_BLEND_EQ eq)
{
	switch(eq)
	{
	case RENOIR_BLEND_EQ_ADD: return D3D11_BLEND_OP_ADD;
	case RENOIR_BLEND_EQ_SUBTRACT: return D3D11_BLEND_OP_SUBTRACT;
	case RENOIR_BLEND_EQ_MIN: return D3D11_BLEND_OP_MIN;
	case RENOIR_BLEND_EQ_MAX: return D3D11_BLEND_OP_MAX;
	default: assert(false && "unreachable"); return D3D11_BLEND_OP_ADD;
	}
}

inline static D3D11_FILTER
_renoir_filter_to_dx(RENOIR_FILTER filter)
{
	switch(filter)
	{
	case RENOIR_FILTER_LINEAR: return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	case RENOIR_FILTER_POINT: return D3D11_FILTER_MIN_MAG_MIP_POINT;
	default: assert(false && "unreachable"); return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	}
}

inline static D3D11_TEXTURE_ADDRESS_MODE
_renoir_texmode_to_dx(RENOIR_TEXMODE m)
{
	switch(m)
	{
	case RENOIR_TEXMODE_WRAP: return D3D11_TEXTURE_ADDRESS_WRAP;
	case RENOIR_TEXMODE_CLAMP: return D3D11_TEXTURE_ADDRESS_CLAMP;
	case RENOIR_TEXMODE_BORDER: return D3D11_TEXTURE_ADDRESS_BORDER;
	case RENOIR_TEXMODE_MIRROR: return D3D11_TEXTURE_ADDRESS_MIRROR;
	default: assert(false && "unreachable"); return D3D11_TEXTURE_ADDRESS_WRAP;
	}
}

inline static D3D11_COMPARISON_FUNC
_renoir_compare_to_dx(RENOIR_COMPARE c)
{
	switch(c)
	{
	case RENOIR_COMPARE_LESS: return D3D11_COMPARISON_LESS;
	case RENOIR_COMPARE_EQUAL: return D3D11_COMPARISON_EQUAL;
	case RENOIR_COMPARE_LESS_EQUAL: return D3D11_COMPARISON_LESS_EQUAL;
	case RENOIR_COMPARE_GREATER: return D3D11_COMPARISON_GREATER;
	case RENOIR_COMPARE_NOT_EQUAL: return D3D11_COMPARISON_NOT_EQUAL;
	case RENOIR_COMPARE_GREATER_EQUAL: return D3D11_COMPARISON_GREATER_EQUAL;
	case RENOIR_COMPARE_NEVER: return D3D11_COMPARISON_NEVER;
	case RENOIR_COMPARE_ALWAYS: return D3D11_COMPARISON_ALWAYS;
	default: assert(false && "unreachable"); return D3D11_COMPARISON_LESS;
	}
}

inline static DXGI_FORMAT
_renoir_type_to_dx(RENOIR_TYPE type)
{
	switch(type)
	{
	case RENOIR_TYPE_UINT8: return DXGI_FORMAT_R8_UINT;
	case RENOIR_TYPE_UINT8_4: return DXGI_FORMAT_R8G8B8A8_UINT;
	case RENOIR_TYPE_UINT8_4N: return DXGI_FORMAT_R8G8B8A8_UNORM;
	case RENOIR_TYPE_UINT16: return DXGI_FORMAT_R16_UINT;
	case RENOIR_TYPE_INT16: return DXGI_FORMAT_R16_SINT;
	case RENOIR_TYPE_INT32: return DXGI_FORMAT_R32_SINT;
	case RENOIR_TYPE_FLOAT: return DXGI_FORMAT_R32_FLOAT;
	case RENOIR_TYPE_FLOAT_2: return DXGI_FORMAT_R32G32_FLOAT;
	case RENOIR_TYPE_FLOAT_3: return DXGI_FORMAT_R32G32B32_FLOAT;
	case RENOIR_TYPE_FLOAT_4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
	default: assert(false && "unreachable"); return (DXGI_FORMAT)0;
	}
}

inline static size_t
_renoir_type_to_size(RENOIR_TYPE type)
{
	size_t res = 0;
	switch (type)
	{
	case RENOIR_TYPE_UINT8:
		res = 1;
		break;
	case RENOIR_TYPE_UINT8_4:
	case RENOIR_TYPE_UINT8_4N:
	case RENOIR_TYPE_INT32:
	case RENOIR_TYPE_FLOAT:
		res = 4;
		break;
	case RENOIR_TYPE_INT16:
	case RENOIR_TYPE_UINT16:
		res = 2;
		break;
	case RENOIR_TYPE_FLOAT_2:
		res = 8;
		break;
	case RENOIR_TYPE_FLOAT_3:
		res = 12;
		break;
	case RENOIR_TYPE_FLOAT_4:
		res = 16;
		break;
	default:
		assert(false && "unreachable");
		break;
	}
	return res;
}

inline static int
_renoir_msaa_to_dx(RENOIR_MSAA_MODE msaa)
{
	switch(msaa)
	{
	case RENOIR_MSAA_MODE_NONE: return 1;
	case RENOIR_MSAA_MODE_2: return 2;
	case RENOIR_MSAA_MODE_4: return 4;
	case RENOIR_MSAA_MODE_8: return 8;
	default: assert(false && "unreachable"); return 0;
	}
}

struct Renoir_Command;

enum RENOIR_HANDLE_KIND
{
	RENOIR_HANDLE_KIND_NONE,
	RENOIR_HANDLE_KIND_SWAPCHAIN,
	RENOIR_HANDLE_KIND_PASS,
	RENOIR_HANDLE_KIND_BUFFER,
	RENOIR_HANDLE_KIND_TEXTURE,
	RENOIR_HANDLE_KIND_SAMPLER,
	RENOIR_HANDLE_KIND_PROGRAM,
	RENOIR_HANDLE_KIND_COMPUTE,
	RENOIR_HANDLE_KIND_PIPELINE,
};

struct Renoir_Handle
{
	RENOIR_HANDLE_KIND kind;
	std::atomic<int> rc;
	union
	{
		struct
		{
			int width;
			int height;
			void* window;
			IDXGISwapChain* swapchain;
			ID3D11RenderTargetView* render_target_view;
			ID3D11DepthStencilView *depth_stencil_view;
			ID3D11Texture2D* depth_buffer;
		} swapchain;

		struct
		{
			Renoir_Command *command_list_head;
			Renoir_Command *command_list_tail;
			// used when rendering is done on screen/window
			Renoir_Handle* swapchain;
			// used when rendering is done off screen
			ID3D11RenderTargetView* render_target_view[RENOIR_CONSTANT_COLOR_ATTACHMENT_SIZE];
			ID3D11DepthStencilView* depth_stencil_view;
			int width, height;
			Renoir_Pass_Offscreen_Desc offscreen;
		} pass;
		
		struct
		{
			ID3D11Buffer* buffer;
			RENOIR_BUFFER type;
			RENOIR_USAGE usage;
			RENOIR_ACCESS access;
			size_t size;
			ID3D11Buffer* buffer_staging;
		} buffer;

		struct
		{
			// normal texture part
			ID3D11Texture1D* texture1d;
			ID3D11Texture2D* texture2d;
			ID3D11Texture3D* texture3d;
			ID3D11ShaderResourceView* shader_view;
			Renoir_Size size;
			RENOIR_USAGE usage;
			RENOIR_ACCESS access;
			Renoir_Sampler_Desc default_sampler_desc;
			RENOIR_PIXELFORMAT pixel_format;
			// staging part (for fast CPU writes)
			ID3D11Texture1D* texture1d_staging;
			ID3D11Texture2D* texture2d_staging;
			ID3D11Texture3D* texture3d_staging;
			// render target part
			bool render_target;
			RENOIR_MSAA_MODE msaa;
			ID3D11Texture2D* render_color_buffer;
		} texture;

		struct
		{
			ID3D11SamplerState* sampler;
			Renoir_Sampler_Desc desc;
		} sampler;

		struct
		{
			ID3D11InputLayout* input_layout;
			ID3D11VertexShader* vertex_shader;
			ID3D10Blob* vertex_shader_blob;
			ID3D11PixelShader* pixel_shader;
			ID3D11GeometryShader* geometry_shader;
		} program;

		struct
		{
			ID3D11ComputeShader* compute_shader;
		} compute;

		struct
		{
			ID3D11DepthStencilState* depth_state;
			ID3D11RasterizerState* raster_state;
			ID3D11BlendState* blend_state;
		} pipeline;
	};
};

enum RENOIR_COMMAND_KIND
{
	RENOIR_COMMAND_KIND_NONE,
	RENOIR_COMMAND_KIND_INIT,
	RENOIR_COMMAND_KIND_SWAPCHAIN_NEW,
	RENOIR_COMMAND_KIND_SWAPCHAIN_FREE,
	RENOIR_COMMAND_KIND_SWAPCHAIN_RESIZE,
	RENOIR_COMMAND_KIND_PASS_SWAPCHAIN_NEW,
	RENOIR_COMMAND_KIND_PASS_OFFSCREEN_NEW,
	RENOIR_COMMAND_KIND_PASS_FREE,
	RENOIR_COMMAND_KIND_BUFFER_NEW,
	RENOIR_COMMAND_KIND_BUFFER_FREE,
	RENOIR_COMMAND_KIND_TEXTURE_NEW,
	RENOIR_COMMAND_KIND_TEXTURE_FREE,
	RENOIR_COMMAND_KIND_SAMPLER_NEW,
	RENOIR_COMMAND_KIND_SAMPLER_FREE,
	RENOIR_COMMAND_KIND_PROGRAM_NEW,
	RENOIR_COMMAND_KIND_PROGRAM_FREE,
	RENOIR_COMMAND_KIND_COMPUTE_NEW,
	RENOIR_COMMAND_KIND_COMPUTE_FREE,
	RENOIR_COMMAND_KIND_PIPELINE_NEW,
	RENOIR_COMMAND_KIND_PIPELINE_FREE,
	RENOIR_COMMAND_KIND_PASS_BEGIN,
	RENOIR_COMMAND_KIND_PASS_END,
	RENOIR_COMMAND_KIND_PASS_CLEAR,
	RENOIR_COMMAND_KIND_USE_PIPELINE,
	RENOIR_COMMAND_KIND_USE_PROGRAM,
	RENOIR_COMMAND_KIND_SCISSOR,
	RENOIR_COMMAND_KIND_BUFFER_WRITE,
	RENOIR_COMMAND_KIND_TEXTURE_WRITE,
	RENOIR_COMMAND_KIND_BUFFER_READ,
	RENOIR_COMMAND_KIND_TEXTURE_READ,
	RENOIR_COMMAND_KIND_BUFFER_BIND,
	RENOIR_COMMAND_KIND_TEXTURE_BIND,
	RENOIR_COMMAND_KIND_DRAW
};

struct Renoir_Command
{
	Renoir_Command *prev, *next;
	RENOIR_COMMAND_KIND kind;
	union
	{
		struct
		{
		} init;

		struct
		{
			Renoir_Handle* handle;
		} swapchain_new;

		struct
		{
			Renoir_Handle* handle;
		} swapchain_free;

		struct
		{
			Renoir_Handle* handle;
			int width, height;
		} swapchain_resize;

		struct
		{
			Renoir_Handle* handle;
			Renoir_Handle* swapchain;
		} pass_new;

		struct
		{
			Renoir_Handle* handle;
			Renoir_Pass_Offscreen_Desc desc;
		} pass_offscreen_new;

		struct
		{
			Renoir_Handle* handle;
		} pass_free;

		struct
		{
			Renoir_Handle* handle;
			Renoir_Buffer_Desc desc;
			bool owns_data;
		} buffer_new;

		struct
		{
			Renoir_Handle* handle;
		} buffer_free;

		struct
		{
			Renoir_Handle* handle;
			Renoir_Texture_Desc desc;
			bool owns_data;
		} texture_new;

		struct
		{
			Renoir_Handle* handle;
		} texture_free;

		struct
		{
			Renoir_Handle* handle;
			Renoir_Sampler_Desc desc;
		} sampler_new;

		struct
		{
			Renoir_Handle* handle;
		} sampler_free;

		struct
		{
			Renoir_Handle* handle;
			Renoir_Program_Desc desc;
			bool owns_data;
		} program_new;

		struct
		{
			Renoir_Handle* handle;
		} program_free;

		struct
		{
			Renoir_Handle* handle;
			Renoir_Compute_Desc desc;
			bool owns_data;
		} compute_new;

		struct
		{
			Renoir_Handle* handle;
		} compute_free;

		struct
		{
			Renoir_Handle* handle;
			Renoir_Pipeline_Desc desc;
		} pipeline_new;

		struct
		{
			Renoir_Handle* handle;
		} pipeline_free;

		struct
		{
			Renoir_Handle* handle;
		} pass_begin;

		struct
		{
			Renoir_Handle* handle;
		} pass_end;

		struct
		{
			Renoir_Clear_Desc desc;
		} pass_clear;

		struct
		{
			Renoir_Handle* pipeline;
		} use_pipeline;

		struct
		{
			Renoir_Handle* program;
		} use_program;

		struct
		{
			int x, y, w, h;
		} scissor;

		struct
		{
			Renoir_Handle* handle;
			size_t offset;
			void* bytes;
			size_t bytes_size;
		} buffer_write;

		struct
		{
			Renoir_Handle* handle;
			Renoir_Texture_Edit_Desc desc;
		} texture_write;

		struct
		{
			Renoir_Handle* handle;
			size_t offset;
			void* bytes;
			size_t bytes_size;
		} buffer_read;

		struct
		{
			Renoir_Handle* handle;
			Renoir_Texture_Edit_Desc desc;
		} texture_read;

		struct
		{
			Renoir_Handle* handle;
			RENOIR_SHADER shader;
			int slot;
		} buffer_bind;

		struct
		{
			Renoir_Handle* handle;
			RENOIR_SHADER shader;
			int slot;
			Renoir_Handle* sampler;
		} texture_bind;

		struct
		{
			Renoir_Draw_Desc desc;
		} draw;
	};
};

struct IRenoir
{
	mn::Mutex mtx;
	IDXGIFactory* factory;
	IDXGIAdapter* adapter;
	ID3D11Device* device;
	ID3D11DeviceContext* context;
	mn::Pool handle_pool;
	mn::Pool command_pool;
	Renoir_Settings settings;

	// global command list
	Renoir_Command *command_list_head;
	Renoir_Command *command_list_tail;

	// command execution context
	Renoir_Handle* current_pipeline;
	Renoir_Handle* current_program;
	Renoir_Handle* current_pass;

	// caches
	mn::Buf<Renoir_Handle*> sampler_cache;
};

static void
_renoir_dx11_command_execute(IRenoir* self, Renoir_Command* command);

static Renoir_Handle*
_renoir_dx11_handle_new(IRenoir* self, RENOIR_HANDLE_KIND kind)
{
	auto handle = (Renoir_Handle*)mn::pool_get(self->handle_pool);
	memset(handle, 0, sizeof(*handle));
	handle->kind = kind;
	handle->rc = 1;
	return handle;
}

static void
_renoir_dx11_handle_free(IRenoir* self, Renoir_Handle* h)
{
	mn::pool_put(self->handle_pool, h);
}

static Renoir_Handle*
_renoir_dx11_handle_ref(Renoir_Handle* h)
{
	h->rc.fetch_add(1);
	return h;
}

static bool
_renoir_dx11_handle_unref(Renoir_Handle* h)
{
	return h->rc.fetch_sub(1) == 1;
}

template<typename T>
static Renoir_Command*
_renoir_dx11_command_new(T* self, RENOIR_COMMAND_KIND kind)
{
	auto command = (Renoir_Command*)mn::pool_get(self->command_pool);
	memset(command, 0, sizeof(*command));
	command->kind = kind;
	return command;
}

template<typename T>
static void
_renoir_dx11_command_free(T* self, Renoir_Command* command)
{
	switch(command->kind)
	{
	case RENOIR_COMMAND_KIND_BUFFER_NEW:
	{
		if(command->buffer_new.owns_data)
			mn::free(mn::Block{(void*)command->buffer_new.desc.data, command->buffer_new.desc.data_size});
		break;
	}
	case RENOIR_COMMAND_KIND_TEXTURE_NEW:
	{
		if(command->texture_new.owns_data)
			mn::free(mn::Block{(void*)command->texture_new.desc.data, command->texture_new.desc.data_size});
		break;
	}
	case RENOIR_COMMAND_KIND_PROGRAM_NEW:
	{
		if(command->program_new.owns_data)
		{
			mn::free(mn::Block{(void*)command->program_new.desc.vertex.bytes, command->program_new.desc.vertex.size});
			mn::free(mn::Block{(void*)command->program_new.desc.pixel.bytes, command->program_new.desc.pixel.size});
			if (command->program_new.desc.geometry.bytes != nullptr)
				mn::free(mn::Block{(void*)command->program_new.desc.geometry.bytes, command->program_new.desc.geometry.size});
		}
		break;
	}
	case RENOIR_COMMAND_KIND_COMPUTE_NEW:
	{
		if(command->compute_new.owns_data)
		{
			mn::free(mn::Block{(void*)command->compute_new.desc.compute.bytes, command->compute_new.desc.compute.size});
		}
		break;
	}
	case RENOIR_COMMAND_KIND_BUFFER_WRITE:
	{
		mn::free(mn::Block{(void*)command->buffer_write.bytes, command->buffer_write.bytes_size});
		break;
	}
	case RENOIR_COMMAND_KIND_TEXTURE_WRITE:
	{
		mn::free(mn::Block{(void*)command->texture_write.desc.bytes, command->texture_write.desc.bytes_size});
		break;
	}
	case RENOIR_COMMAND_KIND_NONE:
	case RENOIR_COMMAND_KIND_INIT:
	case RENOIR_COMMAND_KIND_SWAPCHAIN_NEW:
	case RENOIR_COMMAND_KIND_SWAPCHAIN_FREE:
	case RENOIR_COMMAND_KIND_SWAPCHAIN_RESIZE:
	case RENOIR_COMMAND_KIND_PASS_SWAPCHAIN_NEW:
	case RENOIR_COMMAND_KIND_PASS_OFFSCREEN_NEW:
	case RENOIR_COMMAND_KIND_PASS_FREE:
	case RENOIR_COMMAND_KIND_BUFFER_FREE:
	case RENOIR_COMMAND_KIND_TEXTURE_FREE:
	case RENOIR_COMMAND_KIND_SAMPLER_NEW:
	case RENOIR_COMMAND_KIND_SAMPLER_FREE:
	case RENOIR_COMMAND_KIND_PROGRAM_FREE:
	case RENOIR_COMMAND_KIND_COMPUTE_FREE:
	case RENOIR_COMMAND_KIND_PIPELINE_NEW:
	case RENOIR_COMMAND_KIND_PIPELINE_FREE:
	case RENOIR_COMMAND_KIND_PASS_BEGIN:
	case RENOIR_COMMAND_KIND_PASS_END:
	case RENOIR_COMMAND_KIND_PASS_CLEAR:
	case RENOIR_COMMAND_KIND_USE_PIPELINE:
	case RENOIR_COMMAND_KIND_USE_PROGRAM:
	case RENOIR_COMMAND_KIND_SCISSOR:
	case RENOIR_COMMAND_KIND_BUFFER_READ:
	case RENOIR_COMMAND_KIND_TEXTURE_READ:
	case RENOIR_COMMAND_KIND_BUFFER_BIND:
	case RENOIR_COMMAND_KIND_TEXTURE_BIND:
	case RENOIR_COMMAND_KIND_DRAW:
	default:
		// do nothing
		break;
	}
	mn::pool_put(self->command_pool, command);
}

template<typename T>
static void
_renoir_dx11_command_push(T* self, Renoir_Command* command)
{
	if(self->command_list_tail == nullptr)
	{
		self->command_list_tail = command;
		self->command_list_head = command;
		return;
	}

	self->command_list_tail->next = command;
	command->prev = self->command_list_tail;
	self->command_list_tail = command;
}

static void
_renoir_dx11_command_process(IRenoir* self, Renoir_Command* command)
{
	if (self->settings.defer_api_calls)
	{
		_renoir_dx11_command_push(self, command);
	}
	else
	{
		_renoir_dx11_command_execute(self, command);
		_renoir_dx11_command_free(self, command);
	}
}

inline static void
_internal_renoir_dx11_swapchain_new(IRenoir* self, Renoir_Handle* h)
{
	auto dx_msaa = _renoir_msaa_to_dx(self->settings.msaa);

	// create swapchain itself
	DXGI_SWAP_CHAIN_DESC swapchain_desc{};
	swapchain_desc.BufferCount = 1;
	swapchain_desc.BufferDesc.Width = h->swapchain.width;
	swapchain_desc.BufferDesc.Height = h->swapchain.height;
	swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	if (self->settings.vsync == RENOIR_VSYNC_MODE_OFF)
	{
		swapchain_desc.BufferDesc.RefreshRate.Numerator = 0;
		swapchain_desc.BufferDesc.RefreshRate.Denominator = 1;
	}
	else
	{
		// find the numerator/denominator for correct vsync
		IDXGIOutput* output = nullptr;
		auto res = self->adapter->EnumOutputs(0, &output);
		assert(SUCCEEDED(res));
		mn_defer(output->Release());

		UINT modes_count = 0;
		res = output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &modes_count, NULL);
		assert(SUCCEEDED(res));

		auto modes = mn::buf_with_count<DXGI_MODE_DESC>(modes_count);
		mn_defer(mn::buf_free(modes));

		res = output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &modes_count, modes.ptr);
		assert(SUCCEEDED(res));

		for (const auto& mode: modes)
		{
			if (mode.Width == h->swapchain.width && mode.Height == h->swapchain.height)
			{
				swapchain_desc.BufferDesc.RefreshRate.Numerator = mode.RefreshRate.Numerator;
				swapchain_desc.BufferDesc.RefreshRate.Denominator = mode.RefreshRate.Denominator;
			}
		}
	}
	swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapchain_desc.OutputWindow = (HWND)h->swapchain.window;
	swapchain_desc.SampleDesc.Count = dx_msaa;
	swapchain_desc.Windowed = true;
	swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	auto res = self->factory->CreateSwapChain(self->device, &swapchain_desc, &h->swapchain.swapchain);
	assert(SUCCEEDED(res));

	// create render target view
	ID3D11Texture2D* color_buffer = nullptr;
	res = h->swapchain.swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&color_buffer);
	assert(SUCCEEDED(res));

	res = self->device->CreateRenderTargetView(color_buffer, nullptr, &h->swapchain.render_target_view);
	assert(SUCCEEDED(res));

	color_buffer->Release(); color_buffer = nullptr;

	// create depth buffer
	D3D11_TEXTURE2D_DESC depth_desc{};
	depth_desc.Width = h->swapchain.width;
	depth_desc.Height = h->swapchain.height;
	depth_desc.MipLevels = 1;
	depth_desc.ArraySize = 1;
	depth_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depth_desc.SampleDesc.Count = dx_msaa;
	depth_desc.Usage = D3D11_USAGE_DEFAULT;
	depth_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	res = self->device->CreateTexture2D(&depth_desc, nullptr, &h->swapchain.depth_buffer);
	assert(SUCCEEDED(res));

	// create depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view{};
	depth_stencil_view.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	if (self->settings.msaa == RENOIR_MSAA_MODE_NONE)
		depth_stencil_view.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	else
		depth_stencil_view.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
	res = self->device->CreateDepthStencilView(h->swapchain.depth_buffer, &depth_stencil_view, &h->swapchain.depth_stencil_view);
	assert(SUCCEEDED(res));
}

inline static void
_renoir_dx11_input_layout_create(IRenoir* self, Renoir_Handle* h, const Renoir_Draw_Desc& draw)
{
	mn_defer({
		h->program.vertex_shader_blob->Release();
		h->program.vertex_shader_blob = nullptr;
	});
	ID3D11ShaderReflection* reflection = nullptr;
	auto res = D3DReflect(
		h->program.vertex_shader_blob->GetBufferPointer(),
		h->program.vertex_shader_blob->GetBufferSize(),
		__uuidof(ID3D11ShaderReflection),
		(void**)&reflection
	);
	assert(SUCCEEDED(res));

	D3D11_SHADER_DESC shader_desc{};
	res = reflection->GetDesc(&shader_desc);
	assert(SUCCEEDED(res));

	assert(shader_desc.InputParameters < RENOIR_CONSTANT_DRAW_VERTEX_BUFFER_SIZE);
	D3D11_SIGNATURE_PARAMETER_DESC input_desc[RENOIR_CONSTANT_DRAW_VERTEX_BUFFER_SIZE];
	::memset(input_desc, 0, sizeof(input_desc));

	for (UINT i = 0; i < shader_desc.InputParameters; ++i)
	{
		res = reflection->GetInputParameterDesc(i, &input_desc[i]);
		assert(SUCCEEDED(res));
	}

	D3D11_INPUT_ELEMENT_DESC input_layout_desc[RENOIR_CONSTANT_DRAW_VERTEX_BUFFER_SIZE];
	int count = 0;
	::memset(input_layout_desc, 0, sizeof(input_layout_desc));
	for (int i = 0; i < RENOIR_CONSTANT_DRAW_VERTEX_BUFFER_SIZE; ++i)
	{
		if (draw.vertex_buffers[i].buffer.handle == nullptr)
			continue;

		auto dx_type = _renoir_type_to_dx(draw.vertex_buffers[i].type);

		auto& desc = input_layout_desc[count++];
		desc.SemanticName = input_desc[i].SemanticName;
		desc.SemanticIndex = input_desc[i].SemanticIndex;
		desc.Format = dx_type;
		desc.InputSlot = i;
		desc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		desc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		desc.InstanceDataStepRate = 0;
	}

	res = self->device->CreateInputLayout(
		input_layout_desc,
		count,
		h->program.vertex_shader_blob->GetBufferPointer(),
		h->program.vertex_shader_blob->GetBufferSize(),
		&h->program.input_layout
	);
	assert(SUCCEEDED(res));
}


static void
_renoir_dx11_command_execute(IRenoir* self, Renoir_Command* command)
{
	switch(command->kind)
	{
	case RENOIR_COMMAND_KIND_INIT:
	{
		break;
	}
	case RENOIR_COMMAND_KIND_SWAPCHAIN_NEW:
	{
		auto h = command->swapchain_new.handle;
		_internal_renoir_dx11_swapchain_new(self, h);
		break;
	}
	case RENOIR_COMMAND_KIND_SWAPCHAIN_FREE:
	{
		auto h = command->swapchain_free.handle;
		if (_renoir_dx11_handle_unref(h) == false)
			break;
		h->swapchain.swapchain->Release();
		h->swapchain.render_target_view->Release();
		h->swapchain.depth_stencil_view->Release();
		h->swapchain.depth_buffer->Release();
		_renoir_dx11_handle_free(self, h);
		break;
	}
	case RENOIR_COMMAND_KIND_SWAPCHAIN_RESIZE:
	{
		auto h = command->swapchain_resize.handle;
		h->swapchain.width = command->swapchain_resize.width;
		h->swapchain.height = command->swapchain_resize.height;

		// free the current swapchain
		h->swapchain.swapchain->Release();
		h->swapchain.render_target_view->Release();
		h->swapchain.depth_stencil_view->Release();
		h->swapchain.depth_buffer->Release();

		// reinitialize the swapchain
		_internal_renoir_dx11_swapchain_new(self, h);
		break;
	}
	case RENOIR_COMMAND_KIND_PASS_SWAPCHAIN_NEW:
	{
		auto h = command->pass_new.handle;
		h->pass.swapchain = command->pass_new.swapchain;
		break;
	}
	case RENOIR_COMMAND_KIND_PASS_OFFSCREEN_NEW:
	{
		auto h = command->pass_offscreen_new.handle;
		auto &desc = command->pass_offscreen_new.desc;
		h->pass.offscreen = desc;

		int width = -1;
		int height = -1;
		int msaa = -1;
		
		for (size_t i = 0; i < RENOIR_CONSTANT_COLOR_ATTACHMENT_SIZE; ++i)
		{
			auto color = (Renoir_Handle*)desc.color[i].handle;
			if (color == nullptr)
				continue;
			assert(color->texture.render_target);

			_renoir_dx11_handle_ref(color);
			if (color->texture.msaa != RENOIR_MSAA_MODE_NONE)
			{
				auto res = self->device->CreateRenderTargetView(color->texture.render_color_buffer, nullptr, &h->pass.render_target_view[i]);
				assert(SUCCEEDED(res));
			}
			else
			{
				auto res = self->device->CreateRenderTargetView(color->texture.texture2d, nullptr, &h->pass.render_target_view[i]);
				assert(SUCCEEDED(res));
			}

			// first time getting the width/height
			if (width == -1 && height == -1)
			{
				width = color->texture.size.width;
				height = color->texture.size.height;
			}
			else
			{
				assert(color->texture.size.width == width);
				assert(color->texture.size.height == height);
			}

			// check that all of them has the same msaa
			if (msaa == -1)
			{
				msaa = color->texture.msaa;
			}
			else
			{
				assert(msaa == color->texture.msaa);
			}
		}

		auto depth = (Renoir_Handle*)desc.depth_stencil.handle;
		if (depth)
		{
			assert(depth->texture.render_target);
			_renoir_dx11_handle_ref(depth);
			if (depth->texture.msaa != RENOIR_MSAA_MODE_NONE)
			{
				auto dx_format = _renoir_pixelformat_depth_to_dx_depth_view(depth->texture.pixel_format);
				D3D11_DEPTH_STENCIL_VIEW_DESC depth_view_desc{};
				depth_view_desc.Format = dx_format;
				depth_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
				auto res = self->device->CreateDepthStencilView(depth->texture.render_color_buffer, &depth_view_desc, &h->pass.depth_stencil_view);
				assert(SUCCEEDED(res));
			}
			else
			{
				auto res = self->device->CreateDepthStencilView(depth->texture.texture2d, nullptr, &h->pass.depth_stencil_view);
				assert(SUCCEEDED(res));
			}

			// first time getting the width/height
			if (width == -1 && height == -1)
			{
				width = depth->texture.size.width;
				height = depth->texture.size.height;
			}
			else
			{
				assert(depth->texture.size.width == width);
				assert(depth->texture.size.height == height);
			}

			// check that all of them has the same msaa
			if (msaa == -1)
			{
				msaa = depth->texture.msaa;
			}
			else
			{
				assert(msaa == depth->texture.msaa);
			}
		}
		h->pass.width = width;
		h->pass.height = height;
		break;
	}
	case RENOIR_COMMAND_KIND_PASS_FREE:
	{
		auto h = command->pass_free.handle;
		if (_renoir_dx11_handle_unref(h) == false)
			break;
		
		for(auto it = h->pass.command_list_head; it != NULL; it = it->next)
			_renoir_dx11_command_free(self, command);
		
		// free all the bound textures if it's a framebuffer pass
		if (h->pass.swapchain == nullptr)
		{
			for (size_t i = 0; i < RENOIR_CONSTANT_COLOR_ATTACHMENT_SIZE; ++i)
			{
				auto color = (Renoir_Handle*)h->pass.offscreen.color[i].handle;
				if (color == nullptr)
					continue;
				
				h->pass.render_target_view[0]->Release();
				// issue command to free the color texture
				auto command = _renoir_dx11_command_new(self, RENOIR_COMMAND_KIND_TEXTURE_FREE);
				command->texture_free.handle = color;
				_renoir_dx11_command_execute(self, command);
				_renoir_dx11_command_free(self, command);
			}

			auto depth = (Renoir_Handle*)h->pass.offscreen.depth_stencil.handle;
			if (depth)
			{
				h->pass.depth_stencil_view->Release();
				// issue command to free the depth texture
				auto command = _renoir_dx11_command_new(self, RENOIR_COMMAND_KIND_TEXTURE_FREE);
				command->texture_free.handle = depth;
				_renoir_dx11_command_execute(self, command);
				_renoir_dx11_command_free(self, command);
			}
		}

		_renoir_dx11_handle_free(self, h);
		break;
	}
	case RENOIR_COMMAND_KIND_BUFFER_NEW:
	{
		auto h = command->buffer_new.handle;
		auto& desc = command->buffer_new.desc;

		h->buffer.type = desc.type;
		h->buffer.usage = desc.usage;
		h->buffer.access = desc.access;
		h->buffer.size = desc.data_size;

		auto dx_buffer_type = _renoir_buffer_type_to_dx(desc.type);
		auto dx_usage = _renoir_usage_to_dx(desc.usage);
		auto dx_access = _renoir_access_to_dx(desc.access);

		D3D11_BUFFER_DESC buffer_desc{};
		buffer_desc.ByteWidth = desc.data_size;
		buffer_desc.BindFlags = dx_buffer_type;
		buffer_desc.Usage = dx_usage;
		buffer_desc.CPUAccessFlags = dx_access;

		if (desc.data)
		{
			D3D11_SUBRESOURCE_DATA data_desc{};
			data_desc.pSysMem = desc.data;
			auto res = self->device->CreateBuffer(&buffer_desc, &data_desc, &h->buffer.buffer);
			assert(SUCCEEDED(res));
		}
		else
		{
			auto res = self->device->CreateBuffer(&buffer_desc, nullptr, &h->buffer.buffer);
			assert(SUCCEEDED(res));
		}

		if (desc.usage == RENOIR_USAGE_DYNAMIC && (desc.access == RENOIR_ACCESS_WRITE || desc.access == RENOIR_ACCESS_READ_WRITE))
		{
			auto buffer_staging_desc = buffer_desc;
			buffer_staging_desc.Usage = D3D11_USAGE_STAGING;
			buffer_staging_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
			auto res = self->device->CreateBuffer(&buffer_desc, nullptr, &h->buffer.buffer_staging);
			assert(SUCCEEDED(res));
		}
		break;
	}
	case RENOIR_COMMAND_KIND_BUFFER_FREE:
	{
		auto h = command->buffer_free.handle;
		if (_renoir_dx11_handle_unref(h) == false)
			break;
		h->buffer.buffer->Release();
		if (h->buffer.buffer_staging) h->buffer.buffer_staging->Release();
		_renoir_dx11_handle_free(self, h);
		break;
	}
	case RENOIR_COMMAND_KIND_TEXTURE_NEW:
	{
		auto h = command->texture_new.handle;
		auto& desc = command->texture_new.desc;

		h->texture.access = desc.access;
		h->texture.pixel_format = desc.pixel_format;
		h->texture.usage = desc.usage;
		h->texture.size = desc.size;
		h->texture.render_target = desc.render_target;
		h->texture.msaa = desc.msaa;
		h->texture.default_sampler_desc = desc.sampler;

		auto dx_access = _renoir_access_to_dx(desc.access);
		auto dx_usage = _renoir_usage_to_dx(desc.usage);
		auto dx_pixelformat = _renoir_pixelformat_to_dx(desc.pixel_format);
		auto dx_pixelformat_size = _renoir_pixelformat_to_size(desc.pixel_format);

		assert(desc.size.width > 0 && "a texture must have at least width");

		if (desc.size.height == 0 && desc.size.depth == 0)
		{
			D3D11_TEXTURE1D_DESC texture_desc{};
			texture_desc.ArraySize = 1;
			texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			texture_desc.MipLevels = 1;
			texture_desc.Width = desc.size.width;
			texture_desc.CPUAccessFlags = dx_access;
			texture_desc.Usage = dx_usage;
			texture_desc.Format = dx_pixelformat;

			if (desc.data)
			{
				D3D11_SUBRESOURCE_DATA data_desc{};
				data_desc.pSysMem = desc.data;
				data_desc.SysMemPitch = desc.data_size;
				auto res = self->device->CreateTexture1D(&texture_desc, &data_desc, &h->texture.texture1d);
				assert(SUCCEEDED(res));
			}
			else
			{
				auto res = self->device->CreateTexture1D(&texture_desc, nullptr, &h->texture.texture1d);
				assert(SUCCEEDED(res));
			}

			D3D11_SHADER_RESOURCE_VIEW_DESC view_desc{};
			view_desc.Format = dx_pixelformat;
			view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
			view_desc.Texture1D.MipLevels = texture_desc.MipLevels;
			auto res = self->device->CreateShaderResourceView(h->texture.texture1d, &view_desc, &h->texture.shader_view);
			assert(SUCCEEDED(res));

			if (desc.usage == RENOIR_USAGE_DYNAMIC && (desc.access == RENOIR_ACCESS_WRITE || desc.access == RENOIR_ACCESS_READ_WRITE))
			{
				auto texture_staging_desc = texture_desc;
				texture_desc.Usage = D3D11_USAGE_STAGING;
				texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
				res = self->device->CreateTexture1D(&texture_staging_desc, nullptr, &h->texture.texture1d_staging);
				assert(SUCCEEDED(res));
			}
		}
		else if (desc.size.height > 0 && desc.size.depth == 0)
		{
			D3D11_TEXTURE2D_DESC texture_desc{};
			texture_desc.ArraySize = 1;
			if (h->texture.render_target)
			{
				if (_renoir_pixelformat_is_depth(desc.pixel_format) == false)
					texture_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
				else
					texture_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
			}
			else
			{
				texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			}
			texture_desc.MipLevels = 1;
			texture_desc.Width = desc.size.width;
			texture_desc.Height = desc.size.height;
			texture_desc.CPUAccessFlags = dx_access;
			texture_desc.Usage = h->texture.render_target ? D3D11_USAGE_DEFAULT : dx_usage;
			texture_desc.Format = dx_pixelformat;
			texture_desc.SampleDesc.Count = 1;

			if (desc.data)
			{
				D3D11_SUBRESOURCE_DATA data_desc{};
				data_desc.pSysMem = desc.data;
				data_desc.SysMemPitch = desc.size.width * dx_pixelformat_size;
				data_desc.SysMemSlicePitch = desc.data_size;
				auto res = self->device->CreateTexture2D(&texture_desc, &data_desc, &h->texture.texture2d);
				assert(SUCCEEDED(res));
			}
			else
			{
				auto res = self->device->CreateTexture2D(&texture_desc, nullptr, &h->texture.texture2d);
				assert(SUCCEEDED(res));
			}

			D3D11_SHADER_RESOURCE_VIEW_DESC view_desc{};
			if (_renoir_pixelformat_is_depth(desc.pixel_format) == false)
			{
				view_desc.Format = dx_pixelformat;
			}
			else
			{
				auto dx_shader_view_pixelformat = _renoir_pixelformat_depth_to_dx_shader_view(desc.pixel_format);
				view_desc.Format = dx_shader_view_pixelformat;
			}
			view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			view_desc.Texture2D.MipLevels = texture_desc.MipLevels;
			auto res = self->device->CreateShaderResourceView(h->texture.texture2d, &view_desc, &h->texture.shader_view);
			assert(SUCCEEDED(res));

			if (desc.render_target && desc.msaa != RENOIR_MSAA_MODE_NONE)
			{
				auto dx_msaa = _renoir_msaa_to_dx(desc.msaa);

				// create the msaa rendertarget
				D3D11_TEXTURE2D_DESC texture_desc{};
				texture_desc.ArraySize = 1;
				if (_renoir_pixelformat_is_depth(desc.pixel_format) == false)
					texture_desc.BindFlags = D3D11_BIND_RENDER_TARGET;
				else
					texture_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
				texture_desc.MipLevels = 1;
				texture_desc.Width = desc.size.width;
				texture_desc.Height = desc.size.height;
				texture_desc.CPUAccessFlags = dx_access;
				texture_desc.Usage = D3D11_USAGE_DEFAULT;
				texture_desc.Format = dx_pixelformat;
				texture_desc.SampleDesc.Count = dx_msaa;
				res = self->device->CreateTexture2D(&texture_desc, nullptr, &h->texture.render_color_buffer);
				assert(SUCCEEDED(res));
			}

			if (desc.usage == RENOIR_USAGE_DYNAMIC && (desc.access == RENOIR_ACCESS_WRITE || desc.access == RENOIR_ACCESS_READ_WRITE))
			{
				auto texture_staging_desc = texture_desc;
				texture_desc.Usage = D3D11_USAGE_STAGING;
				texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
				res = self->device->CreateTexture2D(&texture_staging_desc, nullptr, &h->texture.texture2d_staging);
				assert(SUCCEEDED(res));
			}
		}
		else if (desc.size.height > 0 && desc.size.depth > 0)
		{
			D3D11_TEXTURE3D_DESC texture_desc{};
			texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			texture_desc.MipLevels = 1;
			texture_desc.Width = desc.size.width;
			texture_desc.Height = desc.size.height;
			texture_desc.Depth = desc.size.depth;
			texture_desc.CPUAccessFlags = dx_access;
			texture_desc.Usage = dx_usage;
			texture_desc.Format = dx_pixelformat;

			if (desc.data)
			{
				D3D11_SUBRESOURCE_DATA data_desc{};
				data_desc.pSysMem = desc.data;
				data_desc.SysMemPitch = desc.size.width * dx_pixelformat_size;
				data_desc.SysMemSlicePitch = desc.size.height * data_desc.SysMemPitch;
				auto res = self->device->CreateTexture3D(&texture_desc, &data_desc, &h->texture.texture3d);
				assert(SUCCEEDED(res));
			}
			else
			{
				auto res = self->device->CreateTexture3D(&texture_desc, nullptr, &h->texture.texture3d);
				assert(SUCCEEDED(res));
			}

			D3D11_SHADER_RESOURCE_VIEW_DESC view_desc{};
			view_desc.Format = dx_pixelformat;
			view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
			view_desc.Texture3D.MipLevels = texture_desc.MipLevels;
			auto res = self->device->CreateShaderResourceView(h->texture.texture3d, &view_desc, &h->texture.shader_view);
			assert(SUCCEEDED(res));

			if (desc.usage == RENOIR_USAGE_DYNAMIC && (desc.access == RENOIR_ACCESS_WRITE || desc.access == RENOIR_ACCESS_READ_WRITE))
			{
				auto texture_staging_desc = texture_desc;
				texture_desc.Usage = D3D11_USAGE_STAGING;
				texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
				res = self->device->CreateTexture3D(&texture_staging_desc, nullptr, &h->texture.texture3d_staging);
				assert(SUCCEEDED(res));
			}
		}
		break;
	}
	case RENOIR_COMMAND_KIND_TEXTURE_FREE:
	{
		auto h = command->texture_free.handle;
		if (_renoir_dx11_handle_unref(h) == false)
			break;
		if (h->texture.texture1d) h->texture.texture1d->Release();
		if (h->texture.texture2d) h->texture.texture2d->Release();
		if (h->texture.texture3d) h->texture.texture3d->Release();
		if (h->texture.shader_view) h->texture.shader_view->Release();
		if (h->texture.texture1d_staging) h->texture.texture1d_staging->Release();
		if (h->texture.texture2d_staging) h->texture.texture2d_staging->Release();
		if (h->texture.texture3d_staging) h->texture.texture3d_staging->Release();
		if (h->texture.render_color_buffer) h->texture.render_color_buffer->Release();
		_renoir_dx11_handle_free(self, h);
		break;
	}
	case RENOIR_COMMAND_KIND_SAMPLER_NEW:
	{
		auto h = command->sampler_new.handle;
		auto& desc = command->sampler_new.desc;

		auto dx_filter = _renoir_filter_to_dx(desc.filter);
		auto dx_u = _renoir_texmode_to_dx(desc.u);
		auto dx_v = _renoir_texmode_to_dx(desc.v);
		auto dx_w = _renoir_texmode_to_dx(desc.w);
		auto dx_compare = _renoir_compare_to_dx(desc.compare);

		D3D11_SAMPLER_DESC sampler_desc{};
		sampler_desc.Filter = dx_filter;
		sampler_desc.AddressU = dx_u;
		sampler_desc.AddressV = dx_v;
		sampler_desc.AddressW = dx_w;
		sampler_desc.MipLODBias = 0;
		sampler_desc.MaxAnisotropy = 1;
		sampler_desc.ComparisonFunc = dx_compare;
		sampler_desc.BorderColor[0] = desc.border.r;
		sampler_desc.BorderColor[1] = desc.border.g;
		sampler_desc.BorderColor[2] = desc.border.b;
		sampler_desc.BorderColor[3] = desc.border.a;
		sampler_desc.MinLOD = 0;
		sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
		auto res = self->device->CreateSamplerState(&sampler_desc, &h->sampler.sampler);
		assert(SUCCEEDED(res));
		break;
	}
	case RENOIR_COMMAND_KIND_SAMPLER_FREE:
	{
		auto h = command->sampler_free.handle;
		if (_renoir_dx11_handle_unref(h) == false)
			break;
		h->buffer.buffer->Release();
		_renoir_dx11_handle_free(self, h);
		break;
	}
	case RENOIR_COMMAND_KIND_PROGRAM_NEW:
	{
		auto h = command->program_new.handle;
		auto& desc = command->program_new.desc;

		ID3D10Blob* error = nullptr;

		auto res = D3DCompile(
			desc.vertex.bytes,
			desc.vertex.size,
			NULL,
			NULL,
			NULL,
			"main",
			"vs_5_0",
			0,
			0,
			&h->program.vertex_shader_blob,
			&error
		);
		if (FAILED(res))
		{
			mn::log_error("vertex shader compile error\n{}", error->GetBufferPointer());
			break;
		}
		res = self->device->CreateVertexShader(
			h->program.vertex_shader_blob->GetBufferPointer(),
			h->program.vertex_shader_blob->GetBufferSize(),
			NULL,
			&h->program.vertex_shader
		);
		assert(SUCCEEDED(res));

		ID3D10Blob* pixel_shader_blob = nullptr;
		res = D3DCompile(
			desc.pixel.bytes,
			desc.pixel.size,
			NULL,
			NULL,
			NULL,
			"main",
			"ps_5_0",
			0,
			0,
			&pixel_shader_blob,
			&error
		);
		if (FAILED(res))
		{
			mn::log_error("pixel shader compile error\n{}", error->GetBufferPointer());
			break;
		}
		res = self->device->CreatePixelShader(
			pixel_shader_blob->GetBufferPointer(),
			pixel_shader_blob->GetBufferSize(),
			NULL,
			&h->program.pixel_shader
		);
		assert(SUCCEEDED(res));
		pixel_shader_blob->Release();

		if (desc.geometry.bytes)
		{
			ID3D10Blob* geometry_shader_blob = nullptr;
			auto res = D3DCompile(
				desc.geometry.bytes,
				desc.geometry.size,
				NULL,
				NULL,
				NULL,
				"main",
				"gs_5_0",
				0,
				0,
				&geometry_shader_blob,
				&error
			);
			if (FAILED(res))
			{
				mn::log_error("geometry shader compile error\n{}", error->GetBufferPointer());
				break;
			}
			res = self->device->CreateGeometryShader(
				geometry_shader_blob->GetBufferPointer(),
				geometry_shader_blob->GetBufferSize(),
				NULL,
				&h->program.geometry_shader
			);
			assert(SUCCEEDED(res));
			geometry_shader_blob->Release();
		}
		break;
	}
	case RENOIR_COMMAND_KIND_PROGRAM_FREE:
	{
		auto h = command->program_free.handle;
		if (_renoir_dx11_handle_unref(h) == false)
			break;
		if (h->program.vertex_shader) h->program.vertex_shader->Release();
		if (h->program.vertex_shader_blob) h->program.vertex_shader_blob->Release();
		if (h->program.pixel_shader) h->program.pixel_shader->Release();
		if (h->program.geometry_shader) h->program.geometry_shader->Release();
		if (h->program.input_layout) h->program.input_layout->Release();
		_renoir_dx11_handle_free(self, h);
		break;
	}
	case RENOIR_COMMAND_KIND_COMPUTE_NEW:
	{
		auto h = command->compute_new.handle;
		auto& desc = command->compute_new.desc;

		ID3D10Blob* error = nullptr;

		ID3D10Blob* compute_shader_blob = nullptr;
		auto res = D3DCompile(
			desc.compute.bytes,
			desc.compute.size,
			NULL,
			NULL,
			NULL,
			"main",
			"cs_5_0",
			0,
			0,
			&compute_shader_blob,
			&error
		);
		if (FAILED(res))
		{
			mn::log_error("compute shader compile error\n{}", error->GetBufferPointer());
			break;
		}
		res = self->device->CreateComputeShader(
			compute_shader_blob->GetBufferPointer(),
			compute_shader_blob->GetBufferSize(),
			NULL,
			&h->compute.compute_shader
		);
		assert(SUCCEEDED(res));
		compute_shader_blob->Release();
		break;
	}
	case RENOIR_COMMAND_KIND_COMPUTE_FREE:
	{
		auto h = command->compute_free.handle;
		if (_renoir_dx11_handle_unref(h) == false)
			break;
		if (h->compute.compute_shader) h->compute.compute_shader->Release();
		_renoir_dx11_handle_free(self, h);
		break;
	}
	case RENOIR_COMMAND_KIND_PIPELINE_NEW:
	{
		auto h = command->pipeline_new.handle;
		auto& desc = command->pipeline_new.desc;

		D3D11_DEPTH_STENCIL_DESC depth_desc{};

		depth_desc.DepthEnable = desc.depth == RENOIR_SWITCH_ENABLE;
		depth_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		depth_desc.DepthFunc = D3D11_COMPARISON_LESS;

		depth_desc.StencilEnable = false;
		depth_desc.StencilReadMask = 0xFF;
		depth_desc.StencilWriteMask = 0xFF;
		depth_desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		depth_desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
		depth_desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		depth_desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		depth_desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		depth_desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
		depth_desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		depth_desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		auto res = self->device->CreateDepthStencilState(&depth_desc, &h->pipeline.depth_state);
		assert(SUCCEEDED(res));

		D3D11_RASTERIZER_DESC raster_desc{};
		raster_desc.AntialiasedLineEnable = true;
		if (desc.cull == RENOIR_SWITCH_ENABLE)
		{
			switch(desc.cull_face)
			{
			case RENOIR_FACE_BACK:
				raster_desc.CullMode = D3D11_CULL_BACK;
				break;
			case RENOIR_FACE_FRONT:
				raster_desc.CullMode = D3D11_CULL_FRONT;
				break;
			case RENOIR_FACE_FRONT_BACK:
				break;
			default:
				assert(false && "unreachable");
				break;
			}
		}
		else
		{
			raster_desc.CullMode = D3D11_CULL_NONE;
		}
		raster_desc.DepthBias = 0;
		raster_desc.DepthBiasClamp = 0.0f;
		raster_desc.DepthClipEnable = true;
		raster_desc.FillMode = D3D11_FILL_SOLID;
		raster_desc.FrontCounterClockwise = desc.cull_front == RENOIR_ORIENTATION_CCW;
		raster_desc.MultisampleEnable = true;
		raster_desc.ScissorEnable = desc.scissor == RENOIR_SWITCH_ENABLE;
		raster_desc.SlopeScaledDepthBias = 0.0f;
		res = self->device->CreateRasterizerState(&raster_desc, &h->pipeline.raster_state);
		assert(SUCCEEDED(res));

		D3D11_BLEND_DESC blend_desc{};
		blend_desc.AlphaToCoverageEnable = false;
		blend_desc.IndependentBlendEnable = false;
		blend_desc.RenderTarget[0].BlendEnable = desc.blend == RENOIR_SWITCH_ENABLE;
		blend_desc.RenderTarget[0].SrcBlend = _renoir_blend_to_dx(desc.src_rgb);
		blend_desc.RenderTarget[0].DestBlend = _renoir_blend_to_dx(desc.dst_rgb);
		blend_desc.RenderTarget[0].BlendOp = _renoir_blend_eq_to_dx(desc.eq_rgb);
		blend_desc.RenderTarget[0].SrcBlendAlpha = _renoir_blend_to_dx(desc.src_alpha);
		blend_desc.RenderTarget[0].DestBlendAlpha = _renoir_blend_to_dx(desc.dst_alpha);
		blend_desc.RenderTarget[0].BlendOpAlpha = _renoir_blend_eq_to_dx(desc.eq_alpha);
		blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		res = self->device->CreateBlendState(&blend_desc, &h->pipeline.blend_state);
		assert(SUCCEEDED(res));
		break;
	}
	case RENOIR_COMMAND_KIND_PIPELINE_FREE:
	{
		auto h = command->pipeline_free.handle;
		if (_renoir_dx11_handle_unref(h) == false)
			break;
		if (h->pipeline.depth_state) h->pipeline.depth_state->Release();
		if (h->pipeline.raster_state) h->pipeline.raster_state->Release();
		if (h->pipeline.blend_state) h->pipeline.blend_state->Release();
		_renoir_dx11_handle_free(self, h);
		break;
	}
	case RENOIR_COMMAND_KIND_PASS_BEGIN:
	{
		auto h = command->pass_begin.handle;

		self->current_pass = h;

		// if this is an on screen/window
		if (auto swapchain = h->pass.swapchain)
		{
			self->context->OMSetRenderTargets(1, &swapchain->swapchain.render_target_view, swapchain->swapchain.depth_stencil_view);
			D3D11_VIEWPORT viewport{};
			viewport.Width = swapchain->swapchain.width;
			viewport.Height = swapchain->swapchain.height;
			viewport.MinDepth = 0.0f;
			viewport.MaxDepth = 1.0f;
			viewport.TopLeftX = 0.0f;
			viewport.TopLeftY = 0.0f;
			self->context->RSSetViewports(1, &viewport);
			D3D11_RECT scissor{};
			scissor.left = 0;
			scissor.right = viewport.Width;
			scissor.top = 0;
			scissor.bottom = viewport.Height;
			self->context->RSSetScissorRects(1, &scissor);
		}
		// this is an off screen
		else
		{
			self->context->OMSetRenderTargets(RENOIR_CONSTANT_COLOR_ATTACHMENT_SIZE, h->pass.render_target_view, h->pass.depth_stencil_view);
			D3D11_VIEWPORT viewport{};
			viewport.Width = h->pass.width;
			viewport.Height = h->pass.height;
			viewport.MinDepth = 0.0f;
			viewport.MaxDepth = 1.0f;
			viewport.TopLeftX = 0.0f;
			viewport.TopLeftY = 0.0f;
			self->context->RSSetViewports(1, &viewport);
			D3D11_RECT scissor{};
			scissor.left = 0;
			scissor.right = viewport.Width;
			scissor.top = 0;
			scissor.bottom = viewport.Height;
			self->context->RSSetScissorRects(1, &scissor);
		}
		break;
	}
	case RENOIR_COMMAND_KIND_PASS_END:
	{
		auto h = command->pass_end.handle;
		// if this is an off screen view with msaa we'll need to issue a read command to move the data
		// from renderbuffer to the texture
		for (size_t i = 0; i < RENOIR_CONSTANT_COLOR_ATTACHMENT_SIZE; ++i)
		{
			auto color = (Renoir_Handle*)h->pass.offscreen.color[i].handle;
			if (color == nullptr)
				continue;

			// only resolve msaa textures
			if (color->texture.msaa == RENOIR_MSAA_MODE_NONE)
				continue;

			auto dx_pixel_format = _renoir_pixelformat_to_dx(color->texture.pixel_format);
			self->context->ResolveSubresource(
				color->texture.texture2d,
				0,
				color->texture.render_color_buffer,
				0,
				dx_pixel_format
			);
		}

		// resolve depth textures as well
		auto depth = (Renoir_Handle*)h->pass.offscreen.depth_stencil.handle;
		if (depth && depth->texture.msaa != RENOIR_MSAA_MODE_NONE)
		{
			auto dx_pixel_format = _renoir_pixelformat_to_dx(depth->texture.pixel_format);
			self->context->ResolveSubresource(
				depth->texture.texture2d,
				0,
				depth->texture.render_color_buffer,
				0,
				dx_pixel_format
			);
		}
		break;
	}
	case RENOIR_COMMAND_KIND_PASS_CLEAR:
	{
		auto& desc = command->pass_clear.desc;

		if (desc.flags & RENOIR_CLEAR_COLOR)
		{
			if (auto swapchain = self->current_pass->pass.swapchain)
			{
				self->context->ClearRenderTargetView(swapchain->swapchain.render_target_view, &desc.color.r);
			}
			else
			{
				for (size_t i = 0; i < RENOIR_CONSTANT_COLOR_ATTACHMENT_SIZE; ++i)
				{
					auto render_target = self->current_pass->pass.render_target_view[i];
					if (render_target == nullptr)
						continue;
					self->context->ClearRenderTargetView(render_target, &desc.color.r);
				}
			}
		}

		if (desc.flags & RENOIR_CLEAR_DEPTH)
		{
			if (auto swapchain = self->current_pass->pass.swapchain)
			{
				self->context->ClearDepthStencilView(swapchain->swapchain.depth_stencil_view, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, desc.depth, desc.stencil);
			}
			else
			{
				auto depth_stencil = self->current_pass->pass.depth_stencil_view;
				self->context->ClearDepthStencilView(depth_stencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, desc.depth, desc.stencil);
			}
		}
		break;
	}
	case RENOIR_COMMAND_KIND_USE_PIPELINE:
	{
		self->current_pipeline = command->use_pipeline.pipeline;

		auto h = self->current_pipeline;
		self->context->OMSetBlendState(h->pipeline.blend_state, nullptr, 0xFFFFFF);
		self->context->OMSetDepthStencilState(h->pipeline.depth_state, 1);
		self->context->RSSetState(h->pipeline.raster_state);
		break;
	}
	case RENOIR_COMMAND_KIND_USE_PROGRAM:
	{
		auto h = command->use_program.program;
		self->current_program = h;
		self->context->VSSetShader(h->program.vertex_shader, NULL, 0);
		self->context->PSSetShader(h->program.pixel_shader, NULL, 0);
		if (h->program.geometry_shader)
			self->context->GSSetShader(h->program.geometry_shader, NULL, 0);
		if (h->program.input_layout)
			self->context->IASetInputLayout(h->program.input_layout);
		break;
	}
	case RENOIR_COMMAND_KIND_SCISSOR:
	{
		D3D11_RECT scissor{};
		scissor.left = command->scissor.x;
		scissor.right = command->scissor.x + command->scissor.w;
		scissor.top = command->scissor.y;
		scissor.bottom = command->scissor.y + command->scissor.h;
		self->context->RSSetScissorRects(1, &scissor);
		break;
	}
	case RENOIR_COMMAND_KIND_BUFFER_WRITE:
	{
		auto h = command->buffer_write.handle;

		// total buffer update
		if (command->buffer_write.offset == 0 && command->buffer_write.bytes_size == h->buffer.size)
		{
			D3D11_MAPPED_SUBRESOURCE mapped_resource{};
			auto res = self->context->Map(h->buffer.buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
			assert(SUCCEEDED(res));
			::memcpy(
				(char*)mapped_resource.pData + command->buffer_write.offset,
				command->buffer_write.bytes,
				command->buffer_write.bytes_size
			);
			self->context->Unmap(h->buffer.buffer, 0);
		}
		// partial buffer update
		else
		{
			D3D11_MAPPED_SUBRESOURCE mapped_resource{};
			auto res = self->context->Map(h->buffer.buffer_staging, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
			assert(SUCCEEDED(res));
			::memcpy(
				(char*)mapped_resource.pData + command->buffer_write.offset,
				command->buffer_write.bytes,
				command->buffer_write.bytes_size
			);
			self->context->Unmap(h->buffer.buffer_staging, 0);

			D3D11_BOX src_box{};
			src_box.left = command->buffer_write.offset;
			src_box.right = command->buffer_write.offset + command->buffer_write.bytes_size;
			src_box.bottom = 1;
			src_box.back = 1;
			self->context->CopySubresourceRegion(
				h->buffer.buffer,
				0,
				command->buffer_write.offset,
				0,
				0,
				h->buffer.buffer_staging,
				0,
				&src_box
			);
		}
		break;
	}
	case RENOIR_COMMAND_KIND_TEXTURE_WRITE:
	{
		auto h = command->texture_write.handle;
		auto& desc = command->texture_write.desc;

		auto dx_pixel_size = _renoir_pixelformat_to_size(h->texture.pixel_format);

		if (h->texture.texture1d)
		{
			D3D11_MAPPED_SUBRESOURCE mapped_resource{};
			auto res = self->context->Map(h->texture.texture1d_staging, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
			assert(SUCCEEDED(res));
			::memcpy(
				(char*)mapped_resource.pData + desc.x * dx_pixel_size,
				desc.bytes,
				desc.bytes_size
			);
			self->context->Unmap(h->texture.texture1d_staging, 0);

			D3D11_BOX src_box{};
			src_box.left = desc.x;
			src_box.right = desc.x + desc.width;
			src_box.bottom = 1;
			src_box.back = 1;
			self->context->CopySubresourceRegion(
				h->texture.texture1d,
				0,
				desc.x,
				0,
				0,
				h->texture.texture1d_staging,
				0,
				&src_box
			);
		}
		else if (h->texture.texture2d)
		{
			D3D11_MAPPED_SUBRESOURCE mapped_resource{};
			auto res = self->context->Map(h->texture.texture2d_staging, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
			assert(SUCCEEDED(res));

			char* write_ptr = (char*)mapped_resource.pData;
			write_ptr += mapped_resource.RowPitch * desc.y;
			char* read_ptr = (char*)desc.bytes;
			for (size_t i = 0; i < desc.height; ++i)
			{
				::memcpy(
					write_ptr + desc.x * dx_pixel_size,
					read_ptr,
					desc.width * dx_pixel_size
				);
				write_ptr += mapped_resource.RowPitch;
				read_ptr += desc.width * dx_pixel_size;
			}
			self->context->Unmap(h->texture.texture2d_staging, 0);

			D3D11_BOX src_box{};
			src_box.left = desc.x;
			src_box.right = desc.x + desc.width;
			src_box.top = desc.y;
			src_box.bottom = desc.y + desc.height;
			src_box.back = 1;
			self->context->CopySubresourceRegion(
				h->texture.texture2d,
				0,
				desc.x,
				desc.y,
				0,
				h->texture.texture2d_staging,
				0,
				&src_box
			);
		}
		else if (h->texture.texture3d)
		{
			D3D11_MAPPED_SUBRESOURCE mapped_resource{};
			auto res = self->context->Map(h->texture.texture3d_staging, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
			assert(SUCCEEDED(res));

			char* write_ptr = (char*)mapped_resource.pData;
			write_ptr += mapped_resource.DepthPitch * desc.z + mapped_resource.RowPitch * desc.y;
			char* read_ptr = (char*)desc.bytes;
			for (size_t i = 0; i < desc.depth; ++i)
			{
				auto write_2d_ptr = write_ptr;
				for (size_t j = 0; j < desc.height; ++j)
				{
					::memcpy(
						write_2d_ptr + desc.x * dx_pixel_size,
						read_ptr,
						desc.width * dx_pixel_size
					);
					write_2d_ptr += mapped_resource.RowPitch;
					read_ptr += desc.width * dx_pixel_size;
				}
				write_ptr += mapped_resource.DepthPitch;
			}
			self->context->Unmap(h->texture.texture3d_staging, 0);

			D3D11_BOX src_box{};
			src_box.left = desc.x;
			src_box.right = desc.x + desc.width;
			src_box.top = desc.y;
			src_box.bottom = desc.y + desc.height;
			src_box.front = desc.z;
			src_box.back = desc.z + desc.depth;
			self->context->CopySubresourceRegion(
				h->texture.texture3d,
				0,
				desc.x,
				desc.y,
				desc.z,
				h->texture.texture3d_staging,
				0,
				&src_box
			);
		}
		break;
	}
	case RENOIR_COMMAND_KIND_BUFFER_READ:
	{
		auto h = command->buffer_read.handle;

		D3D11_MAPPED_SUBRESOURCE mapped_resource{};
		self->context->Map(h->buffer.buffer, 0, D3D11_MAP_READ, 0, &mapped_resource);
		::memcpy(
			command->buffer_read.bytes,
			(char*)mapped_resource.pData + command->buffer_read.offset,
			command->buffer_read.bytes_size
		);
		self->context->Unmap(h->buffer.buffer, 0);
		break;
	}
	case RENOIR_COMMAND_KIND_TEXTURE_READ:
	{
		auto h = command->texture_read.handle;
		auto& desc = command->texture_read.desc;

		auto dx_pixel_size = _renoir_pixelformat_to_size(h->texture.pixel_format);

		if (h->texture.texture1d)
		{
			D3D11_MAPPED_SUBRESOURCE mapped_resource{};
			self->context->Map(h->texture.texture1d, 0, D3D11_MAP_READ, 0, &mapped_resource);
			::memcpy(
				desc.bytes,
				(char*)mapped_resource.pData + desc.x * dx_pixel_size,
				desc.bytes_size
			);
			self->context->Unmap(h->texture.texture1d, 0);
		}
		else if (h->texture.texture2d)
		{
			D3D11_MAPPED_SUBRESOURCE mapped_resource{};
			self->context->Map(h->texture.texture2d, 0, D3D11_MAP_READ, 0, &mapped_resource);

			char* read_ptr = (char*)mapped_resource.pData;
			read_ptr += mapped_resource.RowPitch * desc.y;
			char* write_ptr = (char*)desc.bytes;
			for(size_t i = 0; i < desc.height; ++i)
			{
				::memcpy(
					write_ptr,
					read_ptr + desc.x * dx_pixel_size,
					desc.width * dx_pixel_size
				);
				read_ptr += mapped_resource.RowPitch;
				write_ptr += desc.width * dx_pixel_size;
			}
			self->context->Unmap(h->texture.texture2d, 0);
		}
		else if (h->texture.texture3d)
		{
			D3D11_MAPPED_SUBRESOURCE mapped_resource{};
			self->context->Map(h->texture.texture3d, 0, D3D11_MAP_READ, 0, &mapped_resource);

			char* read_ptr = (char*)mapped_resource.pData;
			read_ptr += mapped_resource.DepthPitch * desc.z + mapped_resource.RowPitch * desc.y;
			char* write_ptr = (char*)desc.bytes;
			for(size_t i = 0; i < desc.depth; ++i)
			{
				auto read_2d_ptr = read_ptr;
				for(size_t j = 0; j < desc.height; ++j)
				{
					::memcpy(
						write_ptr,
						read_ptr + desc.x * dx_pixel_size,
						desc.width * dx_pixel_size
					);
					read_2d_ptr += mapped_resource.RowPitch;
					write_ptr += desc.width * dx_pixel_size;
				}
				read_ptr += mapped_resource.DepthPitch;
			}
			self->context->Unmap(h->texture.texture3d, 0);
		}
		break;
	}
	case RENOIR_COMMAND_KIND_BUFFER_BIND:
	{
		auto h = command->buffer_bind.handle;
		assert(h->buffer.type == RENOIR_BUFFER_UNIFORM || h->buffer.type == RENOIR_BUFFER_COMPUTE);
		switch(command->buffer_bind.shader)
		{
		case RENOIR_SHADER_VERTEX:
			self->context->VSSetConstantBuffers(command->buffer_bind.slot, 1, &h->buffer.buffer);
			break;
		case RENOIR_SHADER_PIXEL:
			self->context->PSSetConstantBuffers(command->buffer_bind.slot, 1, &h->buffer.buffer);
			break;
		case RENOIR_SHADER_GEOMETRY:
			self->context->GSSetConstantBuffers(command->buffer_bind.slot, 1, &h->buffer.buffer);
			break;
		case RENOIR_SHADER_COMPUTE:
			self->context->CSSetConstantBuffers(command->buffer_bind.slot, 1, &h->buffer.buffer);
			break;
		default:
			assert(false && "unreachable");
			break;
		}
		break;
	}
	case RENOIR_COMMAND_KIND_TEXTURE_BIND:
	{
		auto h = command->texture_bind.handle;
		switch(command->texture_bind.shader)
		{
		case RENOIR_SHADER_VERTEX:
			self->context->VSSetShaderResources(command->texture_bind.slot, 1, &h->texture.shader_view);
			self->context->VSSetSamplers(command->texture_bind.slot, 1, &command->texture_bind.sampler->sampler.sampler);
			break;
		case RENOIR_SHADER_PIXEL:
			self->context->PSSetShaderResources(command->texture_bind.slot, 1, &h->texture.shader_view);
			self->context->PSSetSamplers(command->texture_bind.slot, 1, &command->texture_bind.sampler->sampler.sampler);
			break;
		case RENOIR_SHADER_GEOMETRY:
			self->context->GSSetShaderResources(command->texture_bind.slot, 1, &h->texture.shader_view);
			self->context->GSSetSamplers(command->texture_bind.slot, 1, &command->texture_bind.sampler->sampler.sampler);
			break;
		case RENOIR_SHADER_COMPUTE:
			self->context->CSSetShaderResources(command->texture_bind.slot, 1, &h->texture.shader_view);
			self->context->CSSetSamplers(command->texture_bind.slot, 1, &command->texture_bind.sampler->sampler.sampler);
			break;
		default:
			assert(false && "unreachable");
			break;
		}
		break;
	}
	case RENOIR_COMMAND_KIND_DRAW:
	{
		auto& desc = command->draw.desc;
		auto hprogram = self->current_program;
		if (hprogram->program.input_layout == nullptr)
			_renoir_dx11_input_layout_create(self, hprogram, desc);

		self->context->IASetInputLayout(hprogram->program.input_layout);
		switch(desc.primitive)
		{
		case RENOIR_PRIMITIVE_POINTS:
			self->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
			break;
		case RENOIR_PRIMITIVE_LINES:
			self->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
			break;
		case RENOIR_PRIMITIVE_TRIANGLES:
			self->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			break;
		default:
			assert(false && "unreachable");
			break;
		}

		for (size_t i = 0; i < RENOIR_CONSTANT_DRAW_VERTEX_BUFFER_SIZE; ++i)
		{
			auto& vertex_buffer = desc.vertex_buffers[i];
			if (vertex_buffer.buffer.handle == nullptr)
				continue;
			
			// calculate the default stride for the vertex buffer
			if (vertex_buffer.stride == 0)
				vertex_buffer.stride = _renoir_type_to_size(vertex_buffer.type);

			auto hbuffer = (Renoir_Handle*)vertex_buffer.buffer.handle;
			UINT offset = vertex_buffer.offset;
			UINT stride = vertex_buffer.stride;
			self->context->IASetVertexBuffers(i, 1, &hbuffer->buffer.buffer, &stride, &offset);
		}

		if (desc.index_buffer.handle != nullptr)
		{
			if (desc.index_type == RENOIR_TYPE_NONE)
				desc.index_type = RENOIR_TYPE_UINT16;
			
			auto dx_type = _renoir_type_to_dx(desc.index_type);
			auto dx_type_size = _renoir_type_to_size(desc.index_type);
			auto hbuffer = (Renoir_Handle*)desc.index_buffer.handle;
			self->context->IASetIndexBuffer(hbuffer->buffer.buffer, dx_type, desc.base_element * dx_type_size);

			if (desc.instances_count > 1)
			{
				self->context->DrawIndexedInstanced(
					desc.elements_count,
					desc.instances_count,
					0,
					0,
					0
				);
			}
			else
			{
				self->context->DrawIndexed(
					desc.elements_count,
					0,
					0
				);
			}
		}
		else
		{
			if (desc.instances_count > 1)
			{
				self->context->DrawInstanced(
					desc.elements_count,
					desc.instances_count,
					desc.base_element,
					0
				);
			}
			else
			{
				self->context->Draw(desc.elements_count, desc.base_element);
			}
		}
		break;
	}
	default:
		assert(false && "unreachable");
		break;
	}
}

inline static Renoir_Handle*
_renoir_dx11_sampler_new(IRenoir* self, Renoir_Sampler_Desc desc)
{
	auto h = _renoir_dx11_handle_new(self, RENOIR_HANDLE_KIND_SAMPLER);
	auto command = _renoir_dx11_command_new(self, RENOIR_COMMAND_KIND_SAMPLER_NEW);
	command->sampler_new.handle = h;
	command->sampler_new.desc = desc;
	_renoir_dx11_command_process(self, command);
	return h;
}

inline static void
_renoir_dx11_sampler_free(IRenoir* self, Renoir_Handle* h)
{
	auto command = _renoir_dx11_command_new(self, RENOIR_COMMAND_KIND_SAMPLER_FREE);
	command->sampler_free.handle = h;
	_renoir_dx11_command_process(self, command);
}

inline static bool
operator==(const Renoir_Sampler_Desc& a, const Renoir_Sampler_Desc& b)
{
	return (
		a.filter == b.filter &&
		a.u == b.u &&
		a.v == b.v &&
		a.w == b.w &&
		a.compare == b.compare &&
		a.border.r == b.border.r &&
		a.border.g == b.border.g &&
		a.border.b == b.border.b &&
		a.border.a == b.border.a
	);
}

inline static Renoir_Handle*
_renoir_dx11_sampler_get(IRenoir* self, Renoir_Sampler_Desc desc)
{
	size_t best_ix = self->sampler_cache.count;
	size_t first_empty_ix = self->sampler_cache.count;
	for (size_t i = 0; i < self->sampler_cache.count; ++i)
	{
		auto hsampler = self->sampler_cache[i];
		if (hsampler == nullptr)
		{
			if (first_empty_ix == self->sampler_cache.count)
				first_empty_ix = i;
			continue;
		}

		if (desc == hsampler->sampler.desc)
		{
			best_ix = i;
			break;
		}
	}

	// we found what we were looking for
	if (best_ix < self->sampler_cache.count)
	{
		auto res = self->sampler_cache[best_ix];
		// reorder the cache
		for (size_t i = 0; i + 1 < best_ix; ++i)
		{
			auto index = best_ix - i - 1;
			self->sampler_cache[index] = self->sampler_cache[index - 1];
		}
		self->sampler_cache[0] = res;
		return res;
	}

	// we didn't find a matching sampler, so create new one
	size_t sampler_ix = first_empty_ix;

	// we didn't find an empty slot for the new sampler so we'll have to make one for it
	if (sampler_ix == self->sampler_cache.count)
	{
		auto to_be_evicted = mn::buf_top(self->sampler_cache);
		for (size_t i = 0; i + 1 < self->sampler_cache.count; ++i)
		{
			auto index = self->sampler_cache.count - i - 1;
			self->sampler_cache[index] = self->sampler_cache[index - 1];
		}
		_renoir_dx11_sampler_free(self, to_be_evicted);
		sampler_ix = 0;
	}

	// create the new sampler and put it at the head of the cache
	auto sampler = _renoir_dx11_sampler_new(self, desc);
	self->sampler_cache[sampler_ix] = sampler;
	return sampler;
}

// API
static bool
_renoir_dx11_init(Renoir* api, Renoir_Settings settings, void*)
{
	static_assert(RENOIR_CONSTANT_SAMPLER_CACHE_SIZE > 0, "sampler cache size should be > 0");

	IDXGIFactory* factory = nullptr;
	IDXGIAdapter* adapter = nullptr;
	ID3D11Device* device = nullptr;
	ID3D11DeviceContext* context = nullptr;
	mn_defer({
		if (factory) factory->Release();
		if (adapter) adapter->Release();
		if (device) device->Release();
		if (context) context->Release();
	});
	if (settings.external_context == false)
	{
		// create dx11 factory
		auto res = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
		if (FAILED(res))
			return false;

		// choose adapter/GPU
		res = factory->EnumAdapters(0, &adapter);
		if (FAILED(res))
			return false;

		const D3D_FEATURE_LEVEL feature_levels[] = {
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0
		};

		// create device and device context
		res = D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			0,
			feature_levels,
			2,
			D3D11_SDK_VERSION,
			&device,
			nullptr,
			&context
		);
		if (FAILED(res))
			return false;
	}

	auto self = mn::alloc_zerod<IRenoir>();
	self->mtx = mn::mutex_new("renoir dx11");
	self->factory = factory; factory = nullptr;
	self->adapter = adapter; adapter = nullptr;
	self->device = device; device = nullptr;
	self->context = context; context = nullptr;
	self->handle_pool = mn::pool_new(sizeof(Renoir_Handle), 128);
	self->command_pool = mn::pool_new(sizeof(Renoir_Command), 128);
	self->settings = settings;
	self->sampler_cache = mn::buf_new<Renoir_Handle*>();
	mn::buf_resize_fill(self->sampler_cache, RENOIR_CONSTANT_SAMPLER_CACHE_SIZE, nullptr);

	auto command = _renoir_dx11_command_new(self, RENOIR_COMMAND_KIND_INIT);
	_renoir_dx11_command_process(self, command);

	api->ctx = self;
	return true;
}

static void
_renoir_dx11_dispose(Renoir* api)
{
	auto self = api->ctx;
	mn::mutex_free(self->mtx);
	self->factory->Release();
	self->adapter->Release();
	self->device->Release();
	self->context->Release();
	mn::pool_free(self->handle_pool);
	mn::pool_free(self->command_pool);
	mn::buf_free(self->sampler_cache);
	mn::free(self);
}

static const char*
_renoir_dx11_name()
{
	return "dx11";
}

static RENOIR_TEXTURE_ORIGIN
_renoir_dx11_texture_origin()
{
	return RENOIR_TEXTURE_ORIGIN_TOP_LEFT;
}

static void
_renoir_dx11_handle_ref(Renoir* api, void* handle)
{
	auto h = (Renoir_Handle*)handle;
	h->rc.fetch_add(1);
}

static void
_renoir_dx11_flush(Renoir* api)
{
	auto self = api->ctx;

	mn::mutex_lock(self->mtx);
	mn_defer(mn::mutex_unlock(self->mtx));

	// process commands
	for(auto it = self->command_list_head; it != nullptr; it = it->next)
	{
		_renoir_dx11_command_execute(self, it);
		_renoir_dx11_command_free(self, it);
	}

	self->command_list_head = nullptr;
	self->command_list_tail = nullptr;
}

static Renoir_Swapchain
_renoir_dx11_swapchain_new(Renoir* api, int width, int height, void* window, void*)
{
	auto self = api->ctx;

	mn::mutex_lock(self->mtx);
	mn_defer(mn::mutex_unlock(self->mtx));

	auto h = _renoir_dx11_handle_new(self, RENOIR_HANDLE_KIND_SWAPCHAIN);
	h->swapchain.width = width;
	h->swapchain.height = height;
	h->swapchain.window = window;

	auto command = _renoir_dx11_command_new(self, RENOIR_COMMAND_KIND_SWAPCHAIN_NEW);
	command->swapchain_new.handle = h;
	_renoir_dx11_command_process(self, command);
	return Renoir_Swapchain{h};
}

static void
_renoir_dx11_swapchain_free(Renoir* api, Renoir_Swapchain swapchain)
{
	auto self = api->ctx;
	auto h = (Renoir_Handle*)swapchain.handle;

	mn::mutex_lock(self->mtx);
	mn_defer(mn::mutex_unlock(self->mtx));

	auto command = _renoir_dx11_command_new(self, RENOIR_COMMAND_KIND_SWAPCHAIN_FREE);
	command->swapchain_free.handle = h;
	_renoir_dx11_command_process(self, command);
}

static void
_renoir_dx11_swapchain_resize(Renoir* api, Renoir_Swapchain swapchain, int width, int height)
{
	auto self = api->ctx;
	auto h = (Renoir_Handle*)swapchain.handle;

	mn::mutex_lock(self->mtx);
	mn_defer(mn::mutex_unlock(self->mtx));

	auto command = _renoir_dx11_command_new(self, RENOIR_COMMAND_KIND_SWAPCHAIN_RESIZE);
	command->swapchain_resize.handle = h;
	command->swapchain_resize.width = width;
	command->swapchain_resize.height = height;
	_renoir_dx11_command_process(self, command);
}

static void
_renoir_dx11_swapchain_present(Renoir* api, Renoir_Swapchain swapchain)
{
	auto self = api->ctx;
	auto h = (Renoir_Handle*)swapchain.handle;

	mn::mutex_lock(self->mtx);
	mn_defer(mn::mutex_unlock(self->mtx));

	// process commands
	for(auto it = self->command_list_head; it != nullptr; it = it->next)
	{
		_renoir_dx11_command_execute(self, it);
		_renoir_dx11_command_free(self, it);
	}

	self->command_list_head = nullptr;
	self->command_list_tail = nullptr;

	if (self->settings.vsync == RENOIR_VSYNC_MODE_ON)
		h->swapchain.swapchain->Present(1, 0);
	else
		h->swapchain.swapchain->Present(0, 0);
}

static Renoir_Buffer
_renoir_dx11_buffer_new(Renoir* api, Renoir_Buffer_Desc desc)
{
	if (desc.usage == RENOIR_USAGE_NONE)
		desc.usage = RENOIR_USAGE_STATIC;

	if (desc.usage == RENOIR_USAGE_DYNAMIC && desc.access == RENOIR_ACCESS_NONE)
	{
		assert(false && "a dynamic buffer with cpu access set to none is a static buffer");
	}

	if (desc.usage == RENOIR_USAGE_STATIC && desc.data == nullptr)
	{
		assert(false && "a static buffer should have data to initialize it");
	}

	if (desc.type == RENOIR_BUFFER_UNIFORM && desc.data_size % 16 != 0)
	{
		assert(false && "uniform buffers should be aligned to 16 bytes");
	}

	auto self = api->ctx;

	mn::mutex_lock(self->mtx);
	mn_defer(mn::mutex_unlock(self->mtx));

	auto h = _renoir_dx11_handle_new(self, RENOIR_HANDLE_KIND_BUFFER);
	auto command = _renoir_dx11_command_new(self, RENOIR_COMMAND_KIND_BUFFER_NEW);
	command->buffer_new.handle = h;
	command->buffer_new.desc = desc;
	if (self->settings.defer_api_calls)
	{
		if (desc.data)
		{
			command->buffer_new.desc.data = mn::alloc(desc.data_size, alignof(char)).ptr;
			::memcpy(command->buffer_new.desc.data, desc.data, desc.data_size);
			command->buffer_new.owns_data = true;
		}
	}
	_renoir_dx11_command_process(self, command);
	return Renoir_Buffer{h};
}

static void
_renoir_dx11_buffer_free(Renoir* api, Renoir_Buffer buffer)
{
	auto self = api->ctx;
	auto h = (Renoir_Handle*)buffer.handle;

	mn::mutex_lock(self->mtx);
	mn_defer(mn::mutex_unlock(self->mtx));
	auto command = _renoir_dx11_command_new(self, RENOIR_COMMAND_KIND_BUFFER_FREE);
	command->buffer_free.handle = h;
	_renoir_dx11_command_process(self, command);
}

static Renoir_Texture
_renoir_dx11_texture_new(Renoir* api, Renoir_Texture_Desc desc)
{
	if (desc.usage == RENOIR_USAGE_NONE)
		desc.usage = RENOIR_USAGE_STATIC;

	if (desc.usage == RENOIR_USAGE_DYNAMIC && desc.access == RENOIR_ACCESS_NONE)
	{
		assert(false && "a dynamic texture with cpu access set to none is a static texture");
	}

	if (desc.render_target == false && desc.usage == RENOIR_USAGE_STATIC && desc.data == nullptr)
	{
		assert(false && "a static texture should have data to initialize it");
	}

	auto self = api->ctx;

	mn::mutex_lock(self->mtx);
	mn_defer(mn::mutex_unlock(self->mtx));

	auto h = _renoir_dx11_handle_new(self, RENOIR_HANDLE_KIND_TEXTURE);
	auto command = _renoir_dx11_command_new(self, RENOIR_COMMAND_KIND_TEXTURE_NEW);
	command->texture_new.handle = h;
	command->texture_new.desc = desc;
	if (self->settings.defer_api_calls)
	{
		if (desc.data)
		{
			command->texture_new.desc.data = mn::alloc(desc.data_size, alignof(char)).ptr;
			::memcpy(command->texture_new.desc.data, desc.data, desc.data_size);
			command->texture_new.owns_data = true;
		}
	}
	_renoir_dx11_command_process(self, command);
	return Renoir_Texture{h};
}

static void
_renoir_dx11_texture_free(Renoir* api, Renoir_Texture texture)
{
	auto self = api->ctx;
	auto h = (Renoir_Handle*)texture.handle;

	mn::mutex_lock(self->mtx);
	mn_defer(mn::mutex_unlock(self->mtx));
	auto command = _renoir_dx11_command_new(self, RENOIR_COMMAND_KIND_TEXTURE_FREE);
	command->texture_free.handle = h;
	_renoir_dx11_command_process(self, command);
}

static void*
_renoir_dx11_texture_native_handle(Renoir* api, Renoir_Texture texture)
{
	auto h = (Renoir_Handle*)texture.handle;
	if (h->texture.texture1d)
		return h->texture.texture1d;
	else if (h->texture.texture2d)
		return h->texture.texture2d;
	else if (h->texture.texture3d)
		return h->texture.texture3d;
	return nullptr;
}

static Renoir_Size
_renoir_dx11_texture_size(Renoir* api, Renoir_Texture texture)
{
	auto h = (Renoir_Handle*)texture.handle;
	return h->texture.size;
}

static bool
_renoir_dx11_program_check(Renoir* api,
	RENOIR_SHADER stage,
	const char* bytes,
	size_t bytes_size,
	char* error,
	size_t error_size)
{
	ID3D10Blob* shader_blob = nullptr;
	ID3D10Blob* error_blob = nullptr;
	mn_defer({
		if (error_blob) error_blob->Release();
		if (shader_blob) shader_blob->Release();
	});

	const char* target = "";
	switch(stage)
	{
	case RENOIR_SHADER_VERTEX:
		target = "vs_5_0";
		break;
	case RENOIR_SHADER_PIXEL:
		target = "ps_5_0";
		break;
	case RENOIR_SHADER_GEOMETRY:
		target = "gs_5_0";
		break;
	case RENOIR_SHADER_COMPUTE:
		target = "cs_5_0";
		break;
	default:
		assert(false && "unreachable");
		break;
	}

	auto res = D3DCompile(bytes, bytes_size, NULL, NULL, NULL, "main", target, 0, 0, &shader_blob, &error_blob);
	if (FAILED(res))
	{
		if (error_size > 0)
		{
			auto blob_error_size = error_blob->GetBufferSize();
			auto size = blob_error_size > error_size ? error_size : blob_error_size;
			::memcpy(error, error_blob->GetBufferPointer(), size - 1);
			error[size] = '\0';
		}
		return false;
	}

	if (stage == RENOIR_SHADER_VERTEX)
	{
		ID3D11ShaderReflection* reflection = nullptr;
		res = D3DReflect(
			shader_blob->GetBufferPointer(),
			shader_blob->GetBufferSize(),
			__uuidof(ID3D11ShaderReflection),
			(void**)&reflection
		);
		if (FAILED(res))
			return false;

		D3D11_SHADER_DESC shader_desc{};
		res = reflection->GetDesc(&shader_desc);
		if (FAILED(res))
			return false;

		if (shader_desc.InputParameters >= RENOIR_CONSTANT_DRAW_VERTEX_BUFFER_SIZE)
			return false;
	}
	return true;
}

static Renoir_Program
_renoir_dx11_program_new(Renoir* api, Renoir_Program_Desc desc)
{
	assert(desc.vertex.bytes != nullptr && desc.pixel.bytes != nullptr);
	if (desc.vertex.size == 0)
		desc.vertex.size = ::strlen(desc.vertex.bytes);
	if (desc.pixel.size == 0)
		desc.pixel.size = ::strlen(desc.pixel.bytes);
	if (desc.geometry.bytes != nullptr && desc.geometry.size == 0)
		desc.geometry.size = ::strlen(desc.geometry.bytes);

	auto self = api->ctx;

	mn::mutex_lock(self->mtx);
	mn_defer(mn::mutex_unlock(self->mtx));

	auto h = _renoir_dx11_handle_new(self, RENOIR_HANDLE_KIND_PROGRAM);
	auto command = _renoir_dx11_command_new(self, RENOIR_COMMAND_KIND_PROGRAM_NEW);
	command->program_new.handle = h;
	command->program_new.desc = desc;
	if (self->settings.defer_api_calls)
	{
		command->program_new.desc.vertex.bytes = (char*)mn::alloc(command->program_new.desc.vertex.size, alignof(char)).ptr;
		::memcpy((char*)command->program_new.desc.vertex.bytes, desc.vertex.bytes, desc.vertex.size);

		command->program_new.desc.pixel.bytes = (char*)mn::alloc(command->program_new.desc.pixel.size, alignof(char)).ptr;
		::memcpy((char*)command->program_new.desc.pixel.bytes, desc.pixel.bytes, desc.pixel.size);

		if (command->program_new.desc.geometry.bytes != nullptr)
		{
			command->program_new.desc.geometry.bytes = (char*)mn::alloc(command->program_new.desc.geometry.size, alignof(char)).ptr;
			::memcpy((char*)command->program_new.desc.geometry.bytes, desc.geometry.bytes, desc.geometry.size);
		}

		command->program_new.owns_data = true;
	}
	_renoir_dx11_command_process(self, command);
	return Renoir_Program{h};
}

static void
_renoir_dx11_program_free(Renoir* api, Renoir_Program program)
{
	auto self = api->ctx;
	auto h = (Renoir_Handle*)program.handle;

	mn::mutex_lock(self->mtx);
	mn_defer(mn::mutex_unlock(self->mtx));

	auto command = _renoir_dx11_command_new(self, RENOIR_COMMAND_KIND_PROGRAM_FREE);
	command->program_free.handle = h;
	_renoir_dx11_command_process(self, command);
}

static Renoir_Compute
_renoir_dx11_compute_new(Renoir* api, Renoir_Compute_Desc desc)
{
	assert(desc.compute.bytes != nullptr);
	if (desc.compute.size == 0)
		desc.compute.size = ::strlen(desc.compute.bytes);

	auto self = api->ctx;

	mn::mutex_lock(self->mtx);
	mn_defer(mn::mutex_unlock(self->mtx));

	auto h = _renoir_dx11_handle_new(self, RENOIR_HANDLE_KIND_COMPUTE);
	auto command = _renoir_dx11_command_new(self, RENOIR_COMMAND_KIND_COMPUTE_NEW);
	command->compute_new.handle = h;
	command->compute_new.desc = desc;
	if (self->settings.defer_api_calls)
	{
		command->compute_new.desc.compute.bytes = (char*)mn::alloc(command->compute_new.desc.compute.size, alignof(char)).ptr;
		::memcpy((char*)command->compute_new.desc.compute.bytes, desc.compute.bytes, desc.compute.size);

		command->program_new.owns_data = true;
	}
	_renoir_dx11_command_process(self, command);
	return Renoir_Compute{h};
}

static void
_renoir_dx11_compute_free(Renoir* api, Renoir_Compute compute)
{
	auto self = api->ctx;
	auto h = (Renoir_Handle*)compute.handle;

	mn::mutex_lock(self->mtx);
	mn_defer(mn::mutex_unlock(self->mtx));

	auto command = _renoir_dx11_command_new(self, RENOIR_COMMAND_KIND_COMPUTE_FREE);
	command->compute_free.handle = h;
	_renoir_dx11_command_process(self, command);
}

static Renoir_Pipeline
_renoir_dx11_pipeline_new(Renoir* api, Renoir_Pipeline_Desc desc)
{
	auto self = api->ctx;

	if (desc.cull == RENOIR_SWITCH_DEFAULT)
		desc.cull = RENOIR_SWITCH_ENABLE;
	if (desc.cull_face == RENOIR_FACE_NONE)
		desc.cull_face = RENOIR_FACE_BACK;
	if (desc.cull_front == RENOIR_ORIENTATION_NONE)
		desc.cull_front = RENOIR_ORIENTATION_CCW;

	if (desc.depth == RENOIR_SWITCH_DEFAULT)
		desc.depth = RENOIR_SWITCH_ENABLE;

	if (desc.blend == RENOIR_SWITCH_DEFAULT)
		desc.blend = RENOIR_SWITCH_ENABLE;
	if (desc.src_rgb == RENOIR_BLEND_NONE)
		desc.src_rgb = RENOIR_BLEND_SRC_ALPHA;
	if (desc.dst_rgb == RENOIR_BLEND_NONE)
		desc.dst_rgb = RENOIR_BLEND_ONE_MINUS_SRC_ALPHA;
	if (desc.src_alpha == RENOIR_BLEND_NONE)
		desc.src_alpha = RENOIR_BLEND_ZERO;
	if (desc.dst_alpha == RENOIR_BLEND_NONE)
		desc.dst_alpha = RENOIR_BLEND_ONE;
	if (desc.eq_rgb == RENOIR_BLEND_EQ_NONE)
		desc.eq_rgb = RENOIR_BLEND_EQ_ADD;
	if (desc.eq_alpha == RENOIR_BLEND_EQ_NONE)
		desc.eq_alpha = RENOIR_BLEND_EQ_ADD;

	if (desc.scissor == RENOIR_SWITCH_DEFAULT)
		desc.scissor = RENOIR_SWITCH_DISABLE;

	mn::mutex_lock(self->mtx);
	mn_defer(mn::mutex_unlock(self->mtx));

	auto h = _renoir_dx11_handle_new(self, RENOIR_HANDLE_KIND_PIPELINE);
	auto command = _renoir_dx11_command_new(self, RENOIR_COMMAND_KIND_PIPELINE_NEW);
	command->pipeline_new.handle = h;
	command->pipeline_new.desc = desc;
	_renoir_dx11_command_process(self, command);
	return Renoir_Pipeline{h};
}

static void
_renoir_dx11_pipeline_free(Renoir* api, Renoir_Pipeline pipeline)
{
	auto self = api->ctx;
	auto h = (Renoir_Handle*)pipeline.handle;

	mn::mutex_lock(self->mtx);
	mn_defer(mn::mutex_unlock(self->mtx));
	auto command = _renoir_dx11_command_new(self, RENOIR_COMMAND_KIND_PIPELINE_FREE);
	command->pipeline_free.handle = h;
	_renoir_dx11_command_process(self, command);
}

static Renoir_Pass
_renoir_dx11_pass_swapchain_new(Renoir* api, Renoir_Swapchain swapchain)
{
	auto self = api->ctx;

	mn::mutex_lock(self->mtx);
	mn_defer(mn::mutex_unlock(self->mtx));

	auto h = _renoir_dx11_handle_new(self, RENOIR_HANDLE_KIND_PASS);
	auto command = _renoir_dx11_command_new(self, RENOIR_COMMAND_KIND_PASS_SWAPCHAIN_NEW);
	command->pass_new.handle = h;
	command->pass_new.swapchain = (Renoir_Handle*)swapchain.handle;
	_renoir_dx11_command_process(self, command);
	return Renoir_Pass{h};
}

static Renoir_Pass
_renoir_dx11_pass_offscreen_new(Renoir* api, Renoir_Pass_Offscreen_Desc desc)
{
	auto self = api->ctx;

	// check that all sizes match
	Renoir_Size size{-1, -1, -1};
	for (int i = 0; i < RENOIR_CONSTANT_COLOR_ATTACHMENT_SIZE; ++i)
	{
		if (desc.color[i].handle == nullptr)
			continue;

		if (size.width == -1)
		{
			auto h = (Renoir_Handle*)desc.color[i].handle;
			size = h->texture.size;
		}
		else
		{
			auto h = (Renoir_Handle*)desc.color[i].handle;
			assert(size.width == h->texture.size.width &&
				   size.height == h->texture.size.height);
		}
	}
	if (desc.depth_stencil.handle && size.width != -1)
	{
		auto h = (Renoir_Handle*)desc.depth_stencil.handle;
		assert(size.width == h->texture.size.width &&
			   size.height == h->texture.size.height);
	}

	mn::mutex_lock(self->mtx);
	mn_defer(mn::mutex_unlock(self->mtx));

	auto h = _renoir_dx11_handle_new(self, RENOIR_HANDLE_KIND_PASS);
	auto command = _renoir_dx11_command_new(self, RENOIR_COMMAND_KIND_PASS_OFFSCREEN_NEW);
	command->pass_offscreen_new.handle = h;
	command->pass_offscreen_new.desc = desc;
	_renoir_dx11_command_process(self, command);
	return Renoir_Pass{h};
}

static void
_renoir_dx11_pass_free(Renoir* api, Renoir_Pass pass)
{
	auto self = api->ctx;

	mn::mutex_lock(self->mtx);
	mn_defer(mn::mutex_unlock(self->mtx));

	auto h = (Renoir_Handle*)pass.handle;
	auto command = _renoir_dx11_command_new(self, RENOIR_COMMAND_KIND_PASS_FREE);
	command->pass_free.handle = h;
	_renoir_dx11_command_process(self, command);
}

static Renoir_Size
_renoir_dx11_pass_size(Renoir* api, Renoir_Pass pass)
{
	Renoir_Size res{};
	auto h = (Renoir_Handle*)pass.handle;
	// if this is an on screen/window
	if (auto swapchain = h->pass.swapchain)
	{
		res.width = swapchain->swapchain.width;
		res.height = swapchain->swapchain.height;
	}
	// this is an off screen
	else
	{
		res.width = h->pass.width;
		res.height = h->pass.height;
	}
	return res;
}

// Graphics Commands
static void
_renoir_dx11_pass_begin(Renoir* api, Renoir_Pass pass)
{
	auto self = api->ctx;
	auto h = (Renoir_Handle*)pass.handle;
	h->pass.command_list_head = nullptr;
	h->pass.command_list_tail = nullptr;

	mn::mutex_lock(self->mtx);
	mn_defer(mn::mutex_unlock(self->mtx));

	auto command = _renoir_dx11_command_new(self, RENOIR_COMMAND_KIND_PASS_BEGIN);
	command->pass_begin.handle = h;
	_renoir_dx11_command_push(&h->pass, command);
}

static void
_renoir_dx11_pass_end(Renoir* api, Renoir_Pass pass)
{
	auto self = api->ctx;
	auto h = (Renoir_Handle*)pass.handle;

	if (h->pass.command_list_head != nullptr)
	{
		mn::mutex_lock(self->mtx);

		// push the pass end command
		auto command = _renoir_dx11_command_new(self, RENOIR_COMMAND_KIND_PASS_END);
		command->pass_end.handle = h;
		_renoir_dx11_command_push(&h->pass, command);

		// push the commands to the end of command list, if the user requested to defer api calls
		if (self->settings.defer_api_calls)
		{
			if (self->command_list_tail == nullptr)
			{
				self->command_list_head = h->pass.command_list_head;
				self->command_list_tail = h->pass.command_list_tail;
			}
			else
			{
				self->command_list_tail->next = h->pass.command_list_head;
				self->command_list_tail = h->pass.command_list_tail;
			}
		}
		// other than this just process the command
		else
		{
			for(auto it = h->pass.command_list_head; it != nullptr; it = it->next)
			{
				_renoir_dx11_command_execute(self, it);
				_renoir_dx11_command_free(self, it);
			}
		}
		mn::mutex_unlock(self->mtx);
	}
	h->pass.command_list_head = nullptr;
	h->pass.command_list_tail = nullptr;
}

static void
_renoir_dx11_clear(Renoir* api, Renoir_Pass pass, Renoir_Clear_Desc desc)
{
	auto self = api->ctx;
	auto h = (Renoir_Handle*)pass.handle;

	mn::mutex_lock(self->mtx);
	auto command = _renoir_dx11_command_new(self, RENOIR_COMMAND_KIND_PASS_CLEAR);
	mn::mutex_unlock(self->mtx);

	command->pass_clear.desc = desc;
	_renoir_dx11_command_push(&h->pass, command);
}

static void
_renoir_dx11_use_pipeline(Renoir* api, Renoir_Pass pass, Renoir_Pipeline pipeline)
{
	auto self = api->ctx;
	auto h = (Renoir_Handle*)pass.handle;

	mn::mutex_lock(self->mtx);
	auto command = _renoir_dx11_command_new(self, RENOIR_COMMAND_KIND_USE_PIPELINE);
	mn::mutex_unlock(self->mtx);

	command->use_pipeline.pipeline = (Renoir_Handle*)pipeline.handle;
	_renoir_dx11_command_push(&h->pass, command);
}

static void
_renoir_dx11_use_program(Renoir* api, Renoir_Pass pass, Renoir_Program program)
{
	auto self = api->ctx;
	auto h = (Renoir_Handle*)pass.handle;

	mn::mutex_lock(self->mtx);
	auto command = _renoir_dx11_command_new(self, RENOIR_COMMAND_KIND_USE_PROGRAM);
	mn::mutex_unlock(self->mtx);

	command->use_program.program = (Renoir_Handle*)program.handle;
	_renoir_dx11_command_push(&h->pass, command);
}

static void
_renoir_dx11_scissor(Renoir* api, Renoir_Pass pass, int x, int y, int width, int height)
{
	auto self = api->ctx;
	auto h = (Renoir_Handle*)pass.handle;

	mn::mutex_lock(self->mtx);
	auto command = _renoir_dx11_command_new(self, RENOIR_COMMAND_KIND_SCISSOR);
	mn::mutex_unlock(self->mtx);

	command->scissor.x = x;
	command->scissor.y = y;
	command->scissor.w = width;
	command->scissor.h = height;
	_renoir_dx11_command_push(&h->pass, command);
}

static void
_renoir_dx11_buffer_write(Renoir* api, Renoir_Pass pass, Renoir_Buffer buffer, size_t offset, void* bytes, size_t bytes_size)
{
	// this means he's trying to write nothing so no-op
	if (bytes_size == 0)
		return;

	auto self = api->ctx;
	auto h = (Renoir_Handle*)pass.handle;

	assert(h->buffer.usage != RENOIR_USAGE_STATIC);

	mn::mutex_lock(self->mtx);
	auto command = _renoir_dx11_command_new(self, RENOIR_COMMAND_KIND_BUFFER_WRITE);
	mn::mutex_unlock(self->mtx);

	command->buffer_write.handle = (Renoir_Handle*)buffer.handle;
	command->buffer_write.offset = offset;
	command->buffer_write.bytes = mn::alloc(bytes_size, alignof(char)).ptr;
	command->buffer_write.bytes_size = bytes_size;
	::memcpy(command->buffer_write.bytes, bytes, bytes_size);

	_renoir_dx11_command_push(&h->pass, command);
}

static void
_renoir_dx11_texture_write(Renoir* api, Renoir_Pass pass, Renoir_Texture texture, Renoir_Texture_Edit_Desc desc)
{
	// this means he's trying to write nothing so no-op
	if (desc.bytes_size == 0)
		return;

	auto self = api->ctx;
	auto h = (Renoir_Handle*)pass.handle;

	assert(h->texture.usage != RENOIR_USAGE_STATIC);

	mn::mutex_lock(self->mtx);
	auto command = _renoir_dx11_command_new(self, RENOIR_COMMAND_KIND_TEXTURE_WRITE);
	mn::mutex_unlock(self->mtx);

	command->texture_write.handle = (Renoir_Handle*)texture.handle;
	command->texture_write.desc = desc;
	command->texture_write.desc.bytes = mn::alloc(desc.bytes_size, alignof(char)).ptr;
	::memcpy(command->texture_write.desc.bytes, desc.bytes, desc.bytes_size);

	_renoir_dx11_command_push(&h->pass, command);
}

static void
_renoir_dx11_buffer_read(Renoir* api, Renoir_Buffer buffer, size_t offset, void* bytes, size_t bytes_size)
{
	// this means he's trying to read nothing so no-op
	if (bytes_size == 0)
		return;

	auto self = api->ctx;

	Renoir_Command command{};
	command.kind = RENOIR_COMMAND_KIND_BUFFER_READ;
	command.buffer_read.handle = (Renoir_Handle*)buffer.handle;
	command.buffer_read.offset = offset;
	command.buffer_read.bytes = bytes;
	command.buffer_read.bytes_size = bytes_size;

	mn::mutex_lock(self->mtx);
	_renoir_dx11_command_execute(self, &command);
	mn::mutex_unlock(self->mtx);
}

static void
_renoir_dx11_texture_read(Renoir* api, Renoir_Texture texture, Renoir_Texture_Edit_Desc desc)
{
	// this means he's trying to read nothing so no-op
	if (desc.bytes_size == 0)
		return;

	auto self = api->ctx;

	Renoir_Command command{};
	command.kind = RENOIR_COMMAND_KIND_TEXTURE_READ;
	command.texture_read.handle = (Renoir_Handle*)texture.handle;
	command.texture_read.desc = desc;

	mn::mutex_lock(self->mtx);
	_renoir_dx11_command_execute(self, &command);
	mn::mutex_unlock(self->mtx);
}

static void
_renoir_dx11_buffer_bind(Renoir* api, Renoir_Pass pass, Renoir_Buffer buffer, RENOIR_SHADER shader, int slot)
{
	auto self = api->ctx;
	auto h = (Renoir_Handle*)pass.handle;

	mn::mutex_lock(self->mtx);
	auto command = _renoir_dx11_command_new(self, RENOIR_COMMAND_KIND_BUFFER_BIND);
	mn::mutex_unlock(self->mtx);

	command->buffer_bind.handle = (Renoir_Handle*)buffer.handle;
	command->buffer_bind.shader = shader;
	command->buffer_bind.slot = slot;

	_renoir_dx11_command_push(&h->pass, command);
}

static void
_renoir_dx11_texture_bind(Renoir* api, Renoir_Pass pass, Renoir_Texture texture, RENOIR_SHADER shader, int slot)
{
	auto self = api->ctx;
	auto h = (Renoir_Handle*)pass.handle;

	auto htex = (Renoir_Handle*)texture.handle;

	mn::mutex_lock(self->mtx);
	auto hsampler = _renoir_dx11_sampler_get(self, htex->texture.default_sampler_desc);
	auto command = _renoir_dx11_command_new(self, RENOIR_COMMAND_KIND_TEXTURE_BIND);
	mn::mutex_unlock(self->mtx);

	command->texture_bind.handle = htex;
	command->texture_bind.shader = shader;
	command->texture_bind.slot = slot;
	command->texture_bind.sampler = hsampler;

	_renoir_dx11_command_push(&h->pass, command);
}

static void
_renoir_dx11_texture_sampler_bind(Renoir* api, Renoir_Pass pass, Renoir_Texture texture, RENOIR_SHADER shader, int slot, Renoir_Sampler_Desc sampler)
{
	auto self = api->ctx;
	auto h = (Renoir_Handle*)pass.handle;

	auto htex = (Renoir_Handle*)texture.handle;

	mn::mutex_lock(self->mtx);
	auto hsampler = _renoir_dx11_sampler_get(self, sampler);
	auto command = _renoir_dx11_command_new(self, RENOIR_COMMAND_KIND_TEXTURE_BIND);
	mn::mutex_unlock(self->mtx);

	command->texture_bind.handle = htex;
	command->texture_bind.shader = shader;
	command->texture_bind.slot = slot;
	command->texture_bind.sampler = hsampler;

	_renoir_dx11_command_push(&h->pass, command);
}

static void
_renoir_dx11_draw(Renoir* api, Renoir_Pass pass, Renoir_Draw_Desc desc)
{
	auto self = api->ctx;
	auto h = (Renoir_Handle*)pass.handle;

	mn::mutex_lock(self->mtx);
	auto command = _renoir_dx11_command_new(self, RENOIR_COMMAND_KIND_DRAW);
	mn::mutex_unlock(self->mtx);

	command->draw.desc = desc;

	_renoir_dx11_command_push(&h->pass, command);
}

inline static void
_renoir_load_api(Renoir* api)
{
	api->init = _renoir_dx11_init;
	api->dispose = _renoir_dx11_dispose;

	api->name = _renoir_dx11_name;
	api->texture_origin = _renoir_dx11_texture_origin;

	api->handle_ref = _renoir_dx11_handle_ref;
	api->flush = _renoir_dx11_flush;

	api->swapchain_new = _renoir_dx11_swapchain_new;
	api->swapchain_free = _renoir_dx11_swapchain_free;
	api->swapchain_resize = _renoir_dx11_swapchain_resize;
	api->swapchain_present = _renoir_dx11_swapchain_present;

	api->buffer_new = _renoir_dx11_buffer_new;
	api->buffer_free = _renoir_dx11_buffer_free;

	api->texture_new = _renoir_dx11_texture_new;
	api->texture_free = _renoir_dx11_texture_free;
	api->texture_native_handle = _renoir_dx11_texture_native_handle;
	api->texture_size = _renoir_dx11_texture_size;

	api->program_check = _renoir_dx11_program_check;
	api->program_new = _renoir_dx11_program_new;
	api->program_free = _renoir_dx11_program_free;

	api->compute_new = _renoir_dx11_compute_new;
	api->compute_free = _renoir_dx11_compute_free;

	api->pipeline_new = _renoir_dx11_pipeline_new;
	api->pipeline_free = _renoir_dx11_pipeline_free;

	api->pass_swapchain_new = _renoir_dx11_pass_swapchain_new;
	api->pass_offscreen_new = _renoir_dx11_pass_offscreen_new;
	api->pass_free = _renoir_dx11_pass_free;
	api->pass_size = _renoir_dx11_pass_size;

	api->pass_begin = _renoir_dx11_pass_begin;
	api->pass_end = _renoir_dx11_pass_end;
	api->clear = _renoir_dx11_clear;
	api->use_pipeline = _renoir_dx11_use_pipeline;
	api->use_program = _renoir_dx11_use_program;
	api->scissor = _renoir_dx11_scissor;
	api->buffer_write = _renoir_dx11_buffer_write;
	api->texture_write = _renoir_dx11_texture_write;
	api->buffer_read = _renoir_dx11_buffer_read;
	api->texture_read = _renoir_dx11_texture_read;
	api->buffer_bind = _renoir_dx11_buffer_bind;
	api->texture_bind = _renoir_dx11_texture_bind;
	api->texture_sampler_bind = _renoir_dx11_texture_sampler_bind;
	api->draw = _renoir_dx11_draw;
}

Renoir*
renoir_api()
{
	static Renoir _api;
	_renoir_load_api(&_api);
	return &_api;
}

extern "C" RENOIR_DX11_EXPORT void*
rad_api(void* api, bool reload)
{
	if (api == nullptr)
	{
		auto self = mn::alloc_zerod<Renoir>();
		_renoir_load_api(self);
		return self;
	}
	else if (api != nullptr && reload)
	{
		auto self = (Renoir*)api;
		_renoir_load_api(self);
		return api;
	}
	else if (api != nullptr && reload == false)
	{
		mn::free((Renoir*)api);
		return nullptr;
	}
	return nullptr;
}