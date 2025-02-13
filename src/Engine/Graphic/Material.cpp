
#include "Material.hpp"
#include "Engine/Shader.hpp"
#include <iostream>

Material::Material(std::string const &name, float shininess,
				   glm::vec3 const &ambient,
				   glm::vec3 const &diffuse, glm::vec3 const &specular) :
		name_(name),
		shininess_(shininess),
		ambient_(ambient),
		diffuse_(diffuse),
		specular_(specular)

{
}

void 		Material::putMaterialToShader(Shader const &shader) const {
	shader.setInt("uMaterial.isActive", 1);
	shader.setFloat("uMaterial.shininess", shininess_);
	shader.setVec3("uMaterial.diffuse", diffuse_);
	shader.setVec3("uMaterial.ambient", ambient_);
	shader.setVec3("uMaterial.specular", specular_);
}

void		Material::unsetMaterial(class Shader const &shader) {
	shader.setInt("uMaterial.isActive", 0);
}