#ifndef PERIMETER_SOKOLRENDER_H
#define PERIMETER_SOKOLRENDER_H

#if defined(SOKOL_GLCORE) || defined(SOKOL_GLES3)
#define SOKOL_GL (1)
#endif

#include <sokol_gfx.h>
#include <SDL_video.h>

#include "SokolTypes.h"

const int PERIMETER_SOKOL_TEXTURES = 8;

struct SokolCommand {
    SokolCommand();
    ~SokolCommand();
    void CreateShaderParams();
    void Clear();
    void ClearDrawData();
    void ClearShaderParams();
    void SetTexture(size_t index, SokolResource<sg_image>* sokol_texture);
    void ClearTextures();
    NO_COPY_CONSTRUCTOR(SokolCommand)
    
    pipeline_id_t pipeline_id = 0;
    SOKOL_SHADER_ID shader_id = SOKOL_SHADER_ID_NONE;
    size_t base_elements = 0;
    size_t vertices = 0;
    size_t indices = 0;
    SokolResourceTexture* sokol_textures[PERIMETER_SOKOL_TEXTURES] = {};
    SokolResourceBuffer* vertex_buffer = nullptr;
    SokolResourceBuffer* index_buffer = nullptr;
    void* vs_params = nullptr;
    void* fs_params = nullptr;
    size_t vs_params_len = 0;
    size_t fs_params_len = 0;
    Vect2i viewport[2]; //0 Pos 1 Size
    Vect2i clip[2]; //0 Pos 1 Size
};

class cSokolRender: public cInterfaceRenderDevice {
private:
    //SDL context
#ifdef SOKOL_GL
    SDL_GLContext sdl_gl_context = nullptr;
#endif
#ifdef SOKOL_METAL
    friend void sokol_metal_render_callback();
#endif
    //D3D backend stuff
#ifdef SOKOL_D3D11
    struct sokol_d3d_context* d3d_context;
    
    void d3d_CreateDefaultRenderTarget();
    void d3d_DestroyDefaultRenderTarget();

    friend const void* sokol_d3d_render_target_view_cb();
    friend const void* sokol_d3d_depth_stencil_view_cb();
#endif
    
    //Stores resources for reusing
    std::unordered_multimap<uint64_t, SokolResourceBuffer*> bufferPool;
    
    //For swapchain pass that renders into final device
    sg_pass swapchain_pass;
    
    //Renderer state
    bool ActiveScene = false;
    bool isOrthographicProjSet = false;
    std::vector<SokolCommand*> commands;
    sg_sampler sampler;
    
    //Empty texture when texture slot is unused
    SokolTexture2D* emptyTexture = nullptr;

    //Pipelines
    std::unordered_map<std::string, sg_shader> shaders;
    std::unordered_map<pipeline_id_t, struct SokolPipeline*> pipelines;
    static pipeline_id_t GetPipelineID(PIPELINE_TYPE type, vertex_fmt_t vertex_fmt, const PIPELINE_MODE& mode);
    static void GetPipelineIDParts(pipeline_id_t id, PIPELINE_TYPE* type, vertex_fmt_t* vertex_fmt, PIPELINE_MODE* mode);
    void ClearPipelines();
    void ResetViewport();
    void RegisterPipeline(pipeline_id_t id);
    
    //Active pipeline/command state
    SokolCommand activeCommand;
    PIPELINE_TYPE activePipelineType = PIPELINE_TYPE_DEFAULT;
    PIPELINE_MODE activePipelineMode;
    Mat4f activeCommandVP;
    Mat4f activeCommandW;
    eColorMode activeCommandColorMode = COLOR_MOD;
    float activeCommandTex2Lerp = -1;
    eAlphaTestMode activeCommandAlphaTest = ALPHATEST_NONE;
    SOKOL_MATERIAL_TYPE activeMaterial;
    sColor4f activeCommandTileColor;
    sColor4f activeDiffuse;
    sColor4f activeAmbient;
    sColor4f activeSpecular;
    sColor4f activeEmissive;
    float activePower;
    bool activeGlobalLight = false;
    Vect3f activeLightDir;
    sColor4f activeLightDiffuse;
    sColor4f activeLightAmbient;
    sColor4f activeLightSpecular;
    Mat4f activeTextureTransform[PERIMETER_SOKOL_TEXTURES];

    //Commands handling
    void ClearCommands();
    void FinishActiveDrawBuffer();
    void CreateCommand(class VertexBuffer* vb, size_t vertices, class IndexBuffer* ib, size_t indices);
    void SetVPMatrix(const Mat4f* matrix);
    void SetTex2Lerp(float lerp);
    void SetColorMode(eColorMode color_mode);
    void SetMaterial(SOKOL_MATERIAL_TYPE material, const sColor4f& diffuse, const sColor4f& ambient,
                     const sColor4f& specular, const sColor4f& emissive, float power);

    ///Assigns unused sokol buffer to buffer_ptr with requested 
    void PrepareSokolBuffer(SokolBuffer*& buffer_ptr, MemoryResource* resource, size_t len, bool dynamic, sg_buffer_type type);

    //Updates internal state after init/resolution change
    int UpdateRenderMode();
    
    //Does actual drawing using sokol API
    void DoSokolRendering();

    //Set common VS/FS parameters
    template<typename T_VS, typename T_FS>
    void shader_set_common_params(T_VS* vs_params, T_FS* fs_params) {
        vs_params->un_mvp = isOrthographicProjSet ? orthoVP : (activeCommandW * activeCommandVP);
        switch (activeCommandAlphaTest) {
            default:
            case ALPHATEST_NONE:
                fs_params->un_alpha_test = -1.0f;
                break;
            case ALPHATEST_GT_0:
                fs_params->un_alpha_test = 0.0f;
                break;
            case ALPHATEST_GT_1:
                fs_params->un_alpha_test = (1.0f/255.0f);
                break;
            case ALPHATEST_GT_254:
                fs_params->un_alpha_test = (254.0f/255.0f);
                break;
        }
    }

public:
    cSokolRender();
    ~cSokolRender() override;

    // //// cInterfaceRenderDevice impls start ////

    eRenderDeviceSelection GetRenderSelection() const override;

    uint32_t GetWindowCreationFlags() const override;
    void SetActiveDrawBuffer(class DrawBuffer*) override;
    
    int Init(int xScr,int yScr,int mode,SDL_Window* wnd=nullptr,int RefreshRateInHz=0) override;
    bool ChangeSize(int xScr,int yScr,int mode) override;

    int GetClipRect(int *xmin,int *ymin,int *xmax,int *ymax) override;
    int SetClipRect(int xmin,int ymin,int xmax,int ymax) override;
    
    int Done() override;

    void SetDrawNode(cCamera *pDrawNode) override;
    void SetWorldMat4f(const Mat4f* matrix) override;
    void UseOrthographicProjection() override;
    void SetDrawTransform(class cCamera *DrawNode) override;

    int BeginScene() override;
    int EndScene() override;
    int Fill(int r,int g,int b,int a=255) override;
    void ClearZBuffer() override;
    int Flush(bool wnd=false) override;
    
    int SetGamma(float fGamma,float fStart=0.f,float fFinish=1.f) override;

    void DeleteVertexBuffer(class VertexBuffer &vb) override;
    void DeleteIndexBuffer(class IndexBuffer &ib) override;
    void SubmitDrawBuffer(class DrawBuffer* db, DrawRange* range) override;
    void SubmitBuffers(ePrimitiveType primitive, class VertexBuffer* vb, size_t vertices, class IndexBuffer* ib, size_t indices, DrawRange* range) override;
    int CreateTexture(class cTexture *Texture,class cFileImage *FileImage,bool enable_assert=true) override;
    int DeleteTexture(class cTexture *Texture) override;
    void* LockTexture(class cTexture *Texture, int& Pitch) override;
    void* LockTextureRect(class cTexture* Texture, int& Pitch, Vect2i pos, Vect2i size) override;
    void UnlockTexture(class cTexture *Texture) override;
    void SetTextureImage(uint32_t slot, struct TextureImage* texture_image) override;
    void SetTextureTransform(uint32_t slot, const Mat4f& transform) override;
    uint32_t GetMaxTextureSlots() override;

    void SetGlobalFog(const sColor4f &color,const Vect2f &v) override;

    void SetGlobalLight(Vect3f *vLight, sColor4f *Ambient = nullptr,
                        sColor4f *Diffuse = nullptr, sColor4f *Specular = nullptr) override;

    uint32_t GetRenderState(eRenderStateOption option) override;
    int SetRenderState(eRenderStateOption option,uint32_t value) override;

    void OutText(int x,int y,const char *string,const sColor4f& color,int align=-1,eBlendMode blend_mode=ALPHA_BLEND) override;
    void OutText(int x,int y,const char *string,const sColor4f& color,int align,eBlendMode blend_mode,
                 cTexture* pTexture,eColorMode mode,Vect2f uv,Vect2f duv,float phase=0,float lerp_factor=1) override;

    bool SetScreenShot(const char *fname) override;

    void DrawSprite2(int x,int y,int dx,int dy,float u,float v,float du,float dv,float u1,float v1,float du1,float dv1,
                     cTexture *Tex1,cTexture *Tex2,float lerp_factor,float alpha=1,float phase=0,eColorMode mode=COLOR_MOD,eBlendMode blend_mode=ALPHA_NONE) override;

    bool IsEnableSelfShadow() override;

    void SetNoMaterial(eBlendMode blend,float Phase=0,cTexture *Texture0=0,cTexture *Texture1=0,eColorMode color_mode=COLOR_MOD) override;
    void SetBlendState(eBlendMode blend) override;

    void BeginDrawMesh(bool obj_mesh, bool use_shadow) override;
    void EndDrawMesh() override;
    void SetSimplyMaterialMesh(cObjMesh* mesh, sDataRenderMaterial* data) override;
    void DrawNoMaterialMesh(cObjMesh* mesh, sDataRenderMaterial* data) override;

    void BeginDrawShadow(bool shadow_map) override;
    void EndDrawShadow() override;
    void SetSimplyMaterialShadow(cObjMesh* mesh, cTexture* texture) override;
    void DrawNoMaterialShadow(cObjMesh* mesh) override;
    SurfaceImage GetShadowZBuffer() override;

    void SetRenderTarget(cTexture* target, SurfaceImage zbuffer) override;
    void RestoreRenderTarget() override;

    void SetMaterialTilemap(cTileMap *TileMap) override;
    void SetMaterialTilemapShadow() override;
    void SetTileColor(sColor4f color) override;

    // //// cInterfaceRenderDevice impls end ////
};

#endif //PERIMETER_SOKOLRENDER_H
