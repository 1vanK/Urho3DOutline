#include "Urho3DAll.h"
#include "Game.h"


DEFINE_APPLICATION_MAIN(Game)


Game::Game(Context* context) :
    Application(context),
    yaw_(0.0f),
    pitch_(0.0f)
{
    outlineNode_ = nullptr;
}


void Game::Setup()
{
	engineParameters_["WindowTitle"] = GetTypeName();
	//engineParameters_["LogName"] = GetSubsystem<FileSystem>()->GetAppPreferencesDir("urho3d", "logs") + GetTypeName() + ".log";
	engineParameters_["FullScreen"] = false;
	engineParameters_["Headless"] = false;
	engineParameters_["WindowWidth"] = 800;
	engineParameters_["WindowHeight"] = 600;
	engineParameters_["ResourcePaths"] = "Data;CoreData;MyData";
}


void Game::Start()
{
	CreateScene();
	SetupViewport();
	SubscribeToEvents();
	
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	XMLFile* xmlFile = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
	DebugHud* debugHud = engine_->CreateDebugHud();
	debugHud->SetDefaultStyle(xmlFile);
}


void Game::SetupViewport()
{
	Renderer* renderer = GetSubsystem<Renderer>();
	SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);
	ResourceCache* cache = GetSubsystem<ResourceCache>();
    SharedPtr<RenderPath> effectRenderPath = viewport->GetRenderPath()->Clone(); // обязательно SharedPtr, если просто указатель, то уничтожается, я думаю это баг
    effectRenderPath->Append(cache->GetResource<XMLFile>("PostProcess/Outline.xml"));
    effectRenderPath->Append(cache->GetResource<XMLFile>("PostProcess/FXAA3.xml"));
    viewport->SetRenderPath(effectRenderPath);


    Graphics* g = context_->GetSubsystem<Graphics>();
    int w = g->GetWidth();
    int h = g->GetHeight();

    renderTexture_ = new Texture2D(context_);
    renderTexture_->SetSize(w, h, Graphics::GetRGBFormat(), TEXTURE_RENDERTARGET);
    renderTexture_->SetFilterMode(FILTER_NEAREST);
    renderTexture_->SetName("OutlineMask");
    cache->AddManualResource(renderTexture_);

    surface_ = renderTexture_->GetRenderSurface();
    surface_->SetUpdateMode(RenderSurfaceUpdateMode::SURFACE_UPDATEALWAYS);
    outlineViewport = new Viewport(context_, outlineScene_, outlineCameraNode_->GetComponent<Camera>());
    surface_->SetViewport(0, outlineViewport);

}


void Game::CreateScene()
{
	ResourceCache* cache = GetSubsystem<ResourceCache>();

	scene_ = new Scene(context_);
	scene_->CreateComponent<Octree>();

	Node* planeNode = scene_->CreateChild("Plane");
	planeNode->SetScale(Vector3(100.0f, 1.0f, 100.0f));
	StaticModel* planeObject = planeNode->CreateComponent<StaticModel>();
	planeObject->SetModel(cache->GetResource<Model>("Models/Plane.mdl"));
	planeObject->SetMaterial(cache->GetResource<Material>("Materials/StoneTiled.xml"));

	Node* lightNode = scene_->CreateChild("DirectionalLight");
	lightNode->SetDirection(Vector3(0.6f, -1.0f, 0.8f));
	Light* light = lightNode->CreateComponent<Light>();
	light->SetColor(Color(0.6f, 0.5f, 0.2f));
	light->SetLightType(LIGHT_DIRECTIONAL);
	light->SetCastShadows(true);
	light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
	light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));
	//light->SetShadowIntensity(0.5f);

	Node* zoneNode = scene_->CreateChild("Zone");
	Zone* zone = zoneNode->CreateComponent<Zone>();
	zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
	zone->SetAmbientColor(Color(0.4f, 0.5f, 0.8f));
	zone->SetFogColor(Color(0.4f, 0.5f, 0.8f));
	zone->SetFogStart(100.0f);
	zone->SetFogEnd(300.0f);

    Node* ouNode = scene_->CreateChild("Mushroom");
    ouNode->SetPosition(Vector3(0.0f, 0.0f, 20.0f));
    ouNode->SetRotation(Quaternion(0.0f, Random(360.0f), 0.0f));
    ouNode->SetScale(0.5f + Random(2.0f));
    StaticModel* ouObject = ouNode->CreateComponent<StaticModel>();
    ouObject->SetModel(cache->GetResource<Model>("Models/Mushroom.mdl"));
    ouObject->SetMaterial(cache->GetResource<Material>("Materials/Mushroom.xml"));

	cameraNode_ = scene_->CreateChild("Camera");
	cameraNode_->CreateComponent<Camera>();
	cameraNode_->SetPosition(Vector3(0.0f, 5.0f, 0.0f));

    outlineScene_ = new Scene(context_);
    outlineScene_->CreateComponent<Octree>();

    outlineCameraNode_ = cameraNode_->Clone();
    outlineCameraNode_->SetParent(outlineScene_);

}


void Game::MoveCamera(float timeStep)
{
	Input* input = GetSubsystem<Input>();

	const float MOVE_SPEED = 20.0f;
	const float MOUSE_SENSITIVITY = 0.1f;

	IntVector2 mouseMove = input->GetMouseMove();
	yaw_ += MOUSE_SENSITIVITY * mouseMove.x_;
	pitch_ += MOUSE_SENSITIVITY * mouseMove.y_;
	pitch_ = Clamp(pitch_, -90.0f, 90.0f);

	cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));

	if (input->GetKeyDown('W'))
		cameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
	if (input->GetKeyDown('S'))
		cameraNode_->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
	if (input->GetKeyDown('A'))
		cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
	if (input->GetKeyDown('D'))
		cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);
		
	if (input->GetKeyPress(KEY_F2))
		GetSubsystem<DebugHud>()->ToggleAll();

}


void Game::SubscribeToEvents()
{
	SubscribeToEvent(E_UPDATE, HANDLER(Game, HandleUpdate));
}


void Game::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
	using namespace Update;

	float timeStep = eventData[P_TIMESTEP].GetFloat();

	MoveCamera(timeStep);

    outlineCameraNode_->SetPosition(cameraNode_->GetPosition());
    outlineCameraNode_->SetRotation(cameraNode_->GetRotation());
    outlineCameraNode_->SetScale(cameraNode_->GetScale());

    if (outlineNode_)
        outlineNode_->Remove();

    outlineNode_ = scene_->GetChild("Mushroom")->Clone();
    StaticModel* stModel = outlineNode_->GetComponent<StaticModel>();
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    stModel->SetMaterial(cache->GetResource<Material>("Materials/White.xml"));
    outlineNode_->SetParent(outlineScene_);

}
