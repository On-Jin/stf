#pragma once

#include <memory>
#include <list>
#include <Engine/Camera.hpp>

/// \breid Permit to Manage all Cameras
/// \details You can Add, Remove Cameras, associated with StringName
/// \todo Right THROW
class CameraManager {
public:

    /// \brief Constructor
    /// \details Create Singleton
    CameraManager();
    /// \details Create a default Camera, mapped with "Default" name
    void init();

    /// \brief Update all camera
    /// \details Do it when a Frame is started
    void update();

    /// \brief Set Focused Camera
    /// \brief Set Focused Camera for get him with :
    ///			CameraManager::Get().getFocus();
    ///			Camera::Focus;
    /// \{
    /// \param camera
    void	setFocus(Camera &camera);
    /// \param name : Name mapped to Camera to focus
    /// \throw It throws out_of_range if "Name" is not the key of an element in the Cameras map.
    void	setFocus(std::string const &name);
    /// \}

    /// \brief Get Camera focused
    /// \details Default camera is created in constructor of Cameramanager
    Camera &getFocus();

    void	addCamera(std::string const &name);

    void	removeCamera(std::string const &name);
    void	removeCamera(Camera const &camera);

    /// \throw It throws out_of_range if "Name" is not the key of an element in the Cameras map.
    Camera	&getCamera(std::string const &name);
    //void	removeCamera(std::string const &name);

    std::list<Camera > &getListCameras();

    static CameraManager &Get();


private:
    std::list<Camera> bufferCameras_;

    Camera *focus_;

    static std::unique_ptr<CameraManager> instance_;
};