#include "MainGraphicExtendModel.hpp"
#include <Engine/MainGraphic.hpp>
#include <Engine/Time.hpp>
#include <Engine/Display/DisplayWindow.hpp>
#include <Gui/WidgetRender.hpp>
#include <Engine/Camera.hpp>
#include <PathManager.hpp>
#include <Engine/ModelEngine/ModelManager.hpp>

MainGraphicExtendModel::MainGraphicExtendModel() :
        light_(glm::vec3(0.f, 0.f, 30.f)),
        model_(1.f)
{
    boost::filesystem::path &pathShaders = PathManager::Get().getPath("shaders");
    constructMaterialMap_();


    shader_.attach((pathShaders / "basic.vert").generic_string());
    shader_.attach((pathShaders / "basic.geom").generic_string());
    shader_.attach((pathShaders / "basic.frag").generic_string());
    shader_.link();

	ModelManager::Get().addModel("nanosuit", PathManager::Get().getPath("objects") / "nanosuit" / "nanosuit.obj");
    actor_.assign(&ModelManager::Get().getModel("nanosuit"));
/*
    modelRoom_.setModel((pathRoot / "resources" / "objects" / "room" / "interior.obj").generic_string());
    actorRoom_.assign(&modelRoom_);
    actorRoom_.resetTransform();
    actorRoom_.scale(10.f, 1.f);
    actorRoom_.setPosition(glm::vec3(60, 258.0f, -10.0f));
*/
}

void MainGraphicExtendModel::constructMaterialMap_() {
    vectorMap_.reserve(12);
    vectorMap_.emplace_back("GREEN");
    vectorMap_.emplace_back("GROUND", 0.f,
                            glm::vec3(0.0756f * 0.f, 0.4423f * 0.1f, 0.07568f * 0.1f),
                            glm::vec3(0.0756f, 0.5523f, 0.07568f),
                            glm::vec3(0.f, 0.f, 0.f));
    vectorMap_.emplace_back("BLUE", 31.f,
                            glm::vec3(0.0f, 0.1f, 0.06f),
                            glm::vec3(0.0f, 0.50980392f, 0.50980392f),
                            glm::vec3(0.50196078f, 0.50196078f, 0.50196078f));
    glm::vec3 purple((213.f / 255.f), 0.f, (249.f / 255));
    vectorMap_.emplace_back("PURPLE", 50.f, purple * 0.1f, purple, purple);
    glm::vec3 pink((255.f / 255.f), (64.f / 255.f), (129.f / 255));
    vectorMap_.emplace_back("PINK", 50.f, pink * 0.1f, pink, pink);
    vectorMap_.emplace_back("GREY", 89.6,
                            glm::vec3(0.23125f, 0.23125f, 0.23125f),
                            glm::vec3(0.2775f, 0.2775f, 0.2775f),
                            glm::vec3(0.773911f, 0.773911f, 0.773911f));
    vectorMap_.emplace_back("YELLOW", 50.6f,
                            glm::vec3(0.19f, 0.19f, 0.0545f),
                            glm::vec3(0.85f, 0.85f, 0.1f),
                            glm::vec3(0.628281f, 0.628281f, 0.628281f));
    glm::vec3 orange((244.f / 255.f), (81.f / 255.f), (30.f / 255));
    vectorMap_.emplace_back("ORANGE", 50.f, orange * 0.1f, orange, orange);
    vectorMap_.emplace_back("RED", 31.f,
                            glm::vec3(0.2f, 0.07f, 0.07f),
                            glm::vec3(0.7f, 0.07f, 0.07f),
                            glm::vec3(0.7f, 0.6f, 0.6f));
    vectorMap_.emplace_back("FOOD", 76.8f,
                            glm::vec3(0.1745f, 0.01175f, 0.01175f),
                            glm::vec3(0.61424f, 0.04136f, 0.04136f),
                            glm::vec3(0.727811f, 0.626959f, 0.626959f));
    vectorMap_.emplace_back("WALL", 0.f,
                            glm::vec3(0.05f, 0.05f, 0.05f),
                            glm::vec3(0.2f, 0.2f, 0.2f),
                            glm::vec3(0.2f, 0.2f, 0.2f));
    vectorMap_.emplace_back("NONE", 0.f, glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f));
}

float	inter_plan_private(glm::vec3 planePosition, glm::vec3 planeNormal, glm::vec3 ray, glm::vec3 origin)
{
    float		t;

    t = dot(glm::normalize(ray), planeNormal);
    if (fabs(t) < 0.00001f)
        return (0.f);
    t = (dot(planePosition - origin, planeNormal)) / t;
    if (t < 0.00001f)
        return (0.f);
    return (t);
}

void MainGraphicExtendModel::update(float deltaTime)
{

    MainGraphic &main = MainGraphic::Get();
    Camera &camera = *Camera::focus;

    if (DisplayWindow::Get().getKey(GLFW_KEY_G))
        distanceAttractor += deltaTime * 10;
    if (DisplayWindow::Get().getKey(GLFW_KEY_B))
        distanceAttractor -= deltaTime * 10;
    if (DisplayWindow::Get().getKey(GLFW_KEY_Q) == KeyState::kDown)
        updateAttractor = !updateAttractor;

    WidgetRender &wrender = WidgetRender::Get();


    ImVec2 screenToRender = ImVec2(static_cast<float>(main.getRenderBuffer().width) / static_cast<float>(DisplayWindow::Get().getWidthWindow()),
                                   static_cast<float>(main.getRenderBuffer().height) / static_cast<float>(DisplayWindow::Get().getHeightWindow()));
    float x = (2.0f * (DisplayWindow::lastX_ - wrender.widgetPosition.x)) / (DisplayWindow::Get().getWidthWindow() * screenToRender.x) - 1.0f;
    float y = 1.0f - (2.0f * (DisplayWindow::lastY_ - wrender.widgetPosition.y)) / (DisplayWindow::Get().getHeightWindow() * screenToRender.y);
    float z = 1.0f;

    glm::vec3 ray_nds(x, y, z);
    glm::vec4 ray_clip = glm::vec4(ray_nds.x, ray_nds.y, -1.0, 1.0);

    glm::vec4 ray_eye = inverse(Camera::focus->getProjectionMatrix()) * ray_clip;
    ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0, 0.0);
    glm::vec3 ray_wor = (inverse(Camera::focus->getViewMatrix()) * ray_eye);
    ray_wor = glm::normalize(ray_wor);

    if (updateAttractor) {
        attractorPoint = ray_wor * inter_plan_private(camera.getPosition() + glm::normalize(camera.getFront()) * distanceAttractor, camera.getFront(), ray_wor, camera.getPosition());
        attractorPoint += camera.getPosition();
    }

    shader_.activate();
    vectorMap_[1].putMaterialToShader(shader_);
    shader_.setInt("uBackground", 1);
    light_.putLightToShader(shader_);
    shader_.setMat4("projection", Camera::focus->getProjectionMatrix());
    shader_.setMat4("view", Camera::focus->getViewMatrix());
    shader_.setVec3("uCameraPosition", Camera::focus->getPosition());
    actor_.resetTransform();
    actor_.translate(attractorPoint);
    actor_.render(shader_);

//    actorRoom_.resetTransform();
//    actorRoom_.translate(attractorPoint);

//      actorRoom_.render(shader_);

    shader_.setInt("uBackground", 0);


    float time = static_cast<float>(Time::Get().sinceWorldStartProgram.count<std::chrono::milliseconds>()) / 1000.0f;
    //shader_.setFloat("uTime", time);
    Material::unsetMaterial(shader_);
}


MainGraphicExtendModel &MainGraphicExtendModel::Get() {
    if (!instance_)
        instance_ = std::make_unique<MainGraphicExtendModel>();
    return *instance_;
}

std::unique_ptr<MainGraphicExtendModel> MainGraphicExtendModel::instance_ = nullptr;