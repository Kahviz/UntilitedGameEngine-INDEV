#include "Engine.h"
#include <stdexcept>
#include <chrono>
#include <iostream>
#include "GLOBALS.h"
#include "Camera/CameraControl.h"
#include "BGE_ASSERTS.h"
#include "BoronGui.h"
#include <thread>

#ifdef _WIN32
    #define GLFW_EXPOSE_NATIVE_WIN32
#endif

#if VULKAN == 1
    #include "backends/BoronGui_implVulkan.h"
#endif

#include <GLFW/glfw3native.h>

#include "backends/imgui_impl_glfw.h"
#include "GraphicsBackends.h"
#include "Components.h"
#include <random>
#include "Texture.h"

Engine::Engine()
    : window(1280, 800, "BoronEngine")
{
    CreateWarning("========================");
    CreateWarning("BoronEngine Version: " + std::to_string(BGE_VERSION));
    CreateWarning("BoronMathLibrary Version: " + std::to_string(BORONMATHversion));
    CreateWarning("========================");

    dcPresence.Initialize();
    m_ecs.init(&m_componentManager);

    if (!ImGuiInited) {
        std::cout << "ImGui version: " << IMGUI_VERSION << std::endl;
        IMGUI_CHECKVERSION();
        ImGuiContext* context = ImGui::CreateContext();

        ImGui::SetCurrentContext(context);

        ImGui::StyleColorsDark();

        GLFWwindow* glfwWindow = window.GetWindow();

        #if DIRECTX11 == 1
            ID3D11Device* device = window.GetGraphics().GetDevice();
            ID3D11DeviceContext* contextDX11 = window.GetGraphics().GetpContext();
        #endif

        if (!ImGui_ImplGlfw_InitForOther(glfwWindow, true)) {
            CreateError("Failed to initialize ImGui GLFW backend");
        }

        #if DIRECTX11 == 1 
            if (!ImGui_ImplDX11_Init(device, contextDX11)) {
                CreateError("Failed to initialize ImGui DX11 backend");
            }
            else {
                ImGuiInited = true;
            }
        #endif
#if VULKAN == 1
        auto& vk = static_cast<VulkanAdapter&>(window.GetGraphics().GetRenderer());

        VkRenderPass renderPass = vk.GetRenderPass();
        if (renderPass == VK_NULL_HANDLE) {
            CreateError("Render pass is nullptr!");
        }

        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.ApiVersion = VK_API_VERSION_1_2;
        init_info.Instance = vk.GetInstance();
        init_info.PhysicalDevice = vk.GetPhysicalDevice();
        init_info.Device = vk.GetDevice();
        init_info.QueueFamily = vk.GetGraphicsFamilyIndex();
        init_info.Queue = vk.GetGraphicsQueue();
        init_info.DescriptorPool = vk.GetImGuiPool();
        init_info.MinImageCount = 2;
        init_info.ImageCount = static_cast<uint32_t>(vk.GetSwapChainImageViews().size());
        init_info.PipelineCache = VK_NULL_HANDLE;

        init_info.PipelineInfoMain.RenderPass = renderPass;
        init_info.PipelineInfoMain.Subpass = 0;

        init_info.PipelineInfoForViewports = init_info.PipelineInfoMain;
        init_info.UseDynamicRendering = false;
        init_info.Allocator = nullptr;
        init_info.CheckVkResultFn = [](VkResult err) {
            if (err != VK_SUCCESS) {
                std::cerr << "Vulkan Error: " << err << std::endl;
            }
        };

        if (!ImGui_ImplVulkan_Init(&init_info)) {
            CreateError("Failed to initialize ImGui Vulkan backend");
        }

        ImGuiInited = true;
#endif
        ImGuiIO& io = ImGui::GetIO();
        std::string fontPath = fonts + "\\RobotoFont.ttf";
        ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 16.0f);

        if (font == nullptr) {
            CreateError("Could not load font, using default font");
            io.Fonts->AddFontDefault();
        }

        #ifdef _DEBUG
            CreateSuccess("ImGui initialized successfully!");
        #endif

        window.SetWindowIcon(window.GetWindow());
        ImGuiIO& IO = ImGui::GetIO();
        IO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        IO.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;
    }
}

Engine::~Engine()
{
    if (ImGuiInited) {
        #if DIRECTX11 == 1
            ImGui_ImplDX11_Shutdown();
        #endif //Vulkan Does it in CleanUp

        #if VULKAN == 1
            auto& vk = static_cast<VulkanAdapter&>(window.GetGraphics().GetRenderer());

            m_ecs.Each<TextureComponent>(
                [&](EntityECS entity, TextureComponent& textureComponent)
                {
                    if (textureComponent.texture->IsLoaded()) {
                        textureComponent.texture->Cleanup(vk.GetDevice());
                    }
                }
            );

            window.GetGraphics().GetRenderer().CleanUp();
        #endif

        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        ImGuiInited = false;
    }
}

int Engine::EngineRun()
{
    GLFWwindow* glfwWND = window.GetWindow();

    using clock = std::chrono::high_resolution_clock;
    ImGuiIO& IO = ImGui::GetIO();

    #if INEDITOR == 0
        InProject = true;
    #endif
    
    auto lastFrameTime = clock::now();

    BoronGuiNeeds boronGuiNeeds{};

    #if VULKAN == 1
        auto& vk = static_cast<VulkanAdapter&>(window.GetGraphics().GetRenderer());

        #if INEDITOR == 1
            vk.initViewport();
        #endif

        boronGuiNeeds.commandPool = vk.GetCommandPool();
        boronGuiNeeds.graphicsQueue = vk.GetGraphicsQueue();
        boronGuiNeeds.physicalDevice = vk.GetPhysicalDevice();
        boronGuiNeeds.swapchainExtent = vk.GetSwapchainExtent();
        boronGuiNeeds.device = vk.GetDevice();
        boronGuiNeeds.renderPass = vk.GetRenderPass();
    #endif

    BoronGui::InitBoronGui(boronGuiNeeds);

    while (!glfwWindowShouldClose(glfwWND))
    {
        glfwPollEvents();

        auto now = clock::now();

        float deltaTime =
            std::chrono::duration<float>(now - lastFrameTime).count();

        lastFrameTime = now;

        EngineDoFrame(&window, deltaTime);
    }

    SaveProject::Save(m_ecs);

    profiler.PrintInformation();
    
    return 0;
}

EntityECS Engine::AddAMesh(ECS& ecs, const std::string& Path, const std::string& Name,
    BML::Vector3 pos, BML::Vector3 Size, bool Selec, bool LiteralPath, bool UsesTexture)
{
    EntityECS entity = ecs.createEntity();

    BasicInfoComponent basicComp;
    ColorComponent colorComp;
    TransformComponent transformComp;
    PhysicsComponent physicsComp;
    ObjectComponent objectComp;
    EditorSettingsComponent editorComp;
    HierarchyComponent hierarcyComp;
    TextureComponent textureComp;
    InstanceTypeComponent instTypeComp;

    instTypeComp.InstanceType = Boron::Enums::InstanceType::Instance;

    editorComp.isVisibleInExplorer = true;
    editorComp.selected = false;
    basicComp.Name = Name;

    colorComp.color = BML::Int3(
        static_cast<int>(Color3.x() * 255.0f),
        static_cast<int>(Color3.y() * 255.0f),
        static_cast<int>(Color3.z() * 255.0f)
    );
    Transform transform;
    transform.Size = Size;
    transform.Position = pos;

    transformComp.transform = transform;
    physicsComp.anchored = true;

#if VULKAN == 1
    auto& vk = static_cast<VulkanAdapter&>(
        window.GetGraphics().GetRenderer()
        );

    if (!LiteralPath) {
        objectComp.OBJmesh = Mesh::Load(
            assets + Path,
            vk.GetDevice(),
            vk.GetPhysicalDevice(),
            vk.GetCommandPool(),
            vk.GetGraphicsQueue()
        );
    }
    else {
        objectComp.OBJmesh = Mesh::Load(
            Path,
            vk.GetDevice(),
            vk.GetPhysicalDevice(),
            vk.GetCommandPool(),
            vk.GetGraphicsQueue()
        );
    }
#endif
#if DIRECTX11 == 1
    auto* device = window.GetGraphics().GetDevice();

    if (!LiteralPath) {
        objectComp.OBJmesh = Mesh::Load(
            assets + Path,
            device
        );
    }
    else {
        objectComp.OBJmesh = Mesh::Load(
            Path,
            device
        );
    }
#endif
    std::string fullPath = textures + "\\TestTexture.png";

    if (UsesTexture) {
        textureComp.texture = new Texture();

        #if VULKAN == 1
            textureComp.texture->LoadVK(fullPath, window.GetGraphics().GetRenderer());
        #endif
        #if DIRECTX11 == 1
            textureComp.texture->Load(fullPath, window.GetGraphics().GetRenderer());
        #endif

        #if VULKAN == 1
            vk.UpdateDescriptorSet(ecs);
        #endif
    }

    hierarcyComp.parent = world;

    ecs.AddComponent(entity, basicComp);
    ecs.AddComponent(entity, colorComp);
    ecs.AddComponent(entity, transformComp);
    ecs.AddComponent(entity, physicsComp);
    ecs.AddComponent(entity, objectComp);
    ecs.AddComponent(entity, hierarcyComp);
    ecs.AddComponent(entity, editorComp);
    ecs.AddComponent(entity, instTypeComp);
    ecs.AddComponent(entity, textureComp);

    return entity;
}

void ScreenResizerDetector(Window* wnd) {
    static int lastWidth = 0, lastHeight = 0;
    glfwGetFramebufferSize(wnd->GetWindow(), &screen_width, &screen_height);

    if (screen_width != lastWidth || screen_height != lastHeight) {
        wnd->GetGraphics().ReSizeWindow(screen_width, screen_height, wnd);
        lastWidth = screen_width;
        lastHeight = screen_height;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)screen_width, (float)screen_height);

    int window_width, window_height;
    glfwGetWindowSize(wnd->GetWindow(), &window_width, &window_height);
    io.DisplayFramebufferScale = ImVec2(
        window_width > 0 ? (float)screen_width / (float)window_width : 1.0f,
        window_height > 0 ? (float)screen_height / (float)window_height : 1.0f
    );
}

float GetRandomFloat(float min, float max) { //Mathlib
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(min, max);
    return dist(gen);
}

void ImGui_Impl_NewFrame() {
    #if DIRECTX11 == 1
        ImGui_ImplDX11_NewFrame();
    #endif

    #if VULKAN == 1
        ImGui_ImplVulkan_NewFrame();
    #endif

    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}
void Engine::EngineDoFrame(Window* wnd, float deltatime)
{
    dcPresence.Update();
    Keyboard::Init(wnd->GetWindow());
    Mouse::updateMouse(wnd);

    #if INEDITOR == 1
        if (ImGuiInited) {
            ImGui_Impl_NewFrame();
        }
    #endif

    bool ctrlPressed = Keyboard::isHeld(window.GetWindow(), Boron::Keys::LeftCtrl);
    bool RctrlPressed = Keyboard::isHeld(window.GetWindow(), Boron::Keys::RightCtrl);

    static int cubes = 0;

    if (ctrlPressed) {
        AddAMesh(m_ecs, "\\Cube.obj", "Cube", { GetRandomFloat(-50,50),GetRandomFloat(-50,50),GetRandomFloat(-50,50) }, { 1,1,1 }, false, false, true);
        m_console.write("Creating cube", Boron::Enums::ConsoleLineType::Info);

        cubes++;
    }

    if (RctrlPressed) {
        AddAMesh(m_ecs, "\\Cylinder.obj", "Cylinder", { GetRandomFloat(-50,50),GetRandomFloat(-50,50),GetRandomFloat(-50,50) }, { 1,1,1 }, false,false, false);

        cubes++;

        std::cout << "FPS: " << (1.0f / deltatime) << '\n';
        std::cout << "Cubes: " << (cubes) << '\n';
    }


    if (ImGui::GetCurrentContext() == nullptr) {
        std::cerr << "ERROR: No ImGui context set!" << std::endl;
        return;
    }

    static bool Init = false;

    Graphics& graphics = wnd->GetGraphics();

    ScreenResizerDetector(wnd);

    wnd->GetGraphics().SetRenderTargetToBackBuffer();
    graphics.ClearBuffer(0.0f, 0.0f, 1.0f);

    if (InProject) {
        if (!Init) {
            CreateSuccess("Initing World");

            BasicInfoComponent basicInfoComp;
            basicInfoComp.Name = "World";
            std::cout << world << std::endl;
            InstanceTypeComponent instTypeComp;
            instTypeComp.InstanceType = Boron::Enums::InstanceType::World;

            EditorSettingsComponent editorComp;
            editorComp.isVisibleInExplorer = true;
            editorComp.selected = false;

            m_ecs.AddComponent(world, basicInfoComp);
            m_ecs.AddComponent(world, instTypeComp);
            m_ecs.AddComponent(world, editorComp);
            CreateSuccess("Starting loading the project!");

            SaveProject::Load(m_ecs, window, world);

            CreateSuccess("Ended loading the project!");

            #if VULKAN == 1
                auto& vk = static_cast<VulkanAdapter&>(
                    window.GetGraphics().GetRenderer()
                );
                vk.UpdateDescriptorSet(m_ecs);
            #endif

            m_ecs.Each<HierarchyComponent>(
                [&](EntityECS entity, HierarchyComponent& hierarchy)
                {
                    if (hierarchy.parent == 0) {
                        hierarchy.parent = world;
                    }
                }
            );

            if (m_ecs.getNumberOfEntities() <= 2) {
                AddAMesh(m_ecs, "\\Cube.obj", "Cube", { 0,-2,0 }, { 10,1,10 }, false, false,false);
                AddAMesh(m_ecs, "\\Cube.obj", "Cube2", { 0,2,0 }, { 1,2,1 }, false, false,true);
            }
            
            wnd->GetGraphics().GetCamera().SetPosition(5, 5, 5);
            wnd->GetGraphics().GetCamera().SetRotation(-0.615f, -2.356f, 0.0f);
            Init = true;
        }
    }
#if INEDITOR == 1
    wnd->GetGraphics().SetRenderTargetToScene();
    wnd->GetGraphics().ClearSceneBuffer(0.1f, 0.1f, 0.1f);
#endif


#if INEDITOR == 1
    if (InProject && ImGuiInited) {
        makeGui.MakeIMGui(
            m_ecs,
            *wnd,
            reinterpret_cast<float*>(&Color3),
            false,
            this,
            world,
            &graphics.GetRenderer()
        );
        makeGui.MakeIMViewPort(*wnd);
    }
    else {
        if (makeGui.MakeDashBoard(&graphics.GetRenderer())) {
            CreateInfo("Opened a project");
            InProject = true;
        }
    }

    BoronGui::ReSizeViewport({ static_cast<float>(screen_width), static_cast<float>(screen_height) });

    Borongui::Frame frame{};
    frame.setPosition({ 100,100 });
    frame.setSize({ 200,200 });
    frame.setColor({ 0,0,255 });

    BoronGui::SubmitWidget(frame);

    makeGui.RenderPopUps(deltatime); //Here last so thay be rendered always and on top of everything
#endif

#if VULKAN == 1
    m_ecs.Each<TransformComponent, ObjectComponent>(
        [&](EntityECS entity,
            TransformComponent& transform,
            ObjectComponent& object)
        {
            if (!object.canDraw)
                return;

            wnd->GetGraphics().RenderAMesh(
                m_ecs,
                entity
            );
        }
    );
#endif
    //Physics
    std::thread physicsThread([&]() {
        m_ecs.Each<PhysicsComponent>(
            [&](EntityECS entity, PhysicsComponent& physicsComp)
            {
                if (physicsComp.anchored)
                    return;

                physics.ApplyPhysics(m_ecs, entity, deltatime);
            }
        );
    });

    physicsThread.join();
    
    #if DIRECTX11 == 1
        wnd->GetGraphics().DrawAFrame(deltatime,m_ecs);
    #endif

    if (!ctrlPressed && !g_Typing) {
        camC.MakeCameraControls(*wnd, deltatime);
    }

    #if INEDITOR == 1
        if (ImGuiInited) {
            wnd->GetGraphics().SetRenderTargetToBackBuffer();

            ImGui::Render();

            #if DIRECTX11 == 1
                ImGui_ImplDX11_RenderDrawData(
                    ImGui::GetDrawData()
                );
            #endif
        }
    #endif
        
    float fps = 1.0f / deltatime;
    profiler.AddFPS(fps);

    #if VULKAN == 1
        wnd->GetGraphics().DrawAFrame(
            deltatime,
            m_ecs
        );
    #endif

    m_console.update();
    wnd->GetGraphics().EndFrame();

    BoronGui::EndFrame();
}