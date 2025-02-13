#pragma  once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <vector>
#include "Engine/ModelEngine/ActorModel.hpp"

class Light : public ActorModel {
public:

	Light(Model const *model, glm::vec3 const &position = glm::vec3(0.f), float intensity = 1.f);
	Light(glm::vec3 const &position = glm::vec3(0.f), float intensity = 1.f);
	~Light() override;

	void 	putLightToShader(class Shader const &shader) const;

private:
	std::string		name_;
	float			intensity_;

	Light(Light const &) = delete;
	Light &operator=(Light const &) = delete;
};