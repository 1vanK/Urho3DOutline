#pragma once


class Game : public Application
{
    OBJECT(Game);

public:
    Game(Context* context);

    virtual void Setup();
    virtual void Start();

protected:
    void SetLogoVisible(bool enable);
    SharedPtr<Scene> scene_;
    SharedPtr<Scene> outlineScene_;
    SharedPtr<Node> outlineCameraNode_;
    SharedPtr<Node> cameraNode_;
    SharedPtr<Texture2D> renderTexture_;
    float yaw_;
    float pitch_;
    SharedPtr<Node> outlineNode_;
    SharedPtr<RenderSurface> surface_;
    SharedPtr<Viewport> outlineViewport;
    SharedPtr<RenderPath> rp;

private:
	void CreateScene();
	void SetupViewport();
	void MoveCamera(float timeStep);
	void SubscribeToEvents();
	void HandleUpdate(StringHash eventType, VariantMap& eventData);
};
