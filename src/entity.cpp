
#include "entity.h"
#include "includes.h"
#include "scene.h"
#include "camera.h"
#include "application.h"

#include <iostream>

#include "mesh.h"

#include "shader.h"

class Application;

PrefabEntity::PrefabEntity(GTR::Prefab* pPrefab_)
{
	id = Scene::getInstance()->numPrefabEntities;
	Scene::getInstance()->numPrefabEntities++;
	entity_type = eType::PREFAB;
	Matrix44 model_;
	name = "Prefab";
	model = model_;
	visible = true;
	selected = false;
	pPrefab = pPrefab_;
	factor = 1;
}

void PrefabEntity::render(Camera* camera, GTR::Renderer* renderer) {
	renderer->renderPrefab(model, this->pPrefab, camera);
}

void PrefabEntity::renderInMenu()
{
	ImGui::Text("Name: %s", name.c_str());

	ImGui::Checkbox("Active", &visible);

	if (ImGui::Button("Select"))
		Scene::getInstance()->gizmoEntity = this;

	if (ImGui::TreeNode((void*)this, "Model"))
	{
		float matrixTranslation[3], matrixRotation[3], matrixScale[3];
		ImGuizmo::DecomposeMatrixToComponents(model.m, matrixTranslation, matrixRotation, matrixScale);
		ImGui::DragFloat3("Position", matrixTranslation, 0.1f);
		ImGui::DragFloat3("Rotation", matrixRotation, 0.1f);
		ImGui::DragFloat3("Scale", matrixScale, 0.1f);
		ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, model.m);

		ImGui::TreePop();
	}

	//Light color
	//ImGui::ColorEdit4("Color", (float*)&this->pPrefab->root.material->color);

//	ImGui::PopStyleColor();
}

Light::Light(lightType type_)
{
	light_type = type_;
	entity_type = eType::LIGHT;
	
	intensity = 1.0f;
	maxDist = 100.0f;
	color = Vector3(1, 1, 1);
	angleCutoff = 30;
	spotExponent = 0;

	visible = true;
	show_shadowMap = false;
	show_camera = false;
	far_directional_shadowmap_updated = false;
	is_cascade = false;
	renderedHighShadow = false;

	fbo = NULL;
	shadowMap = NULL;	
	mesh = new Mesh();
	mesh->createPyramid();

	camera = new Camera();
	camera->projection_matrix = model;
	camera->lookAt(model.getTranslation(), model.getTranslation() + model.frontVector(), Vector3(0, 1, 0));

	if (type_ == lightType::AMBIENT) { name = "Ambient light"; }
	else if (type_ == lightType::SPOT) {
		name = "Spot light";
		camera->setPerspective(
			angleCutoff * 2,
			Application::instance->window_width / (float)Application::instance->window_height,
			1.0f, 1000.0f);
	}
	else if (type_ == lightType::POINT_LIGHT) { 
		name = "Point light"; 
		camera->setPerspective(
			angleCutoff * 2,
			Application::instance->window_width / (float)Application::instance->window_height,
			1.0f, 1000.0f);
	}
	else {
		name = "Directional light";
		camera->setOrthographic(-256, 256, -256, 256, -500, 5000);
	}
}

void Light::renderInMenu()
{
	ImGui::Text("Name: %s", name.c_str()); // Edit 3 floats representing a color

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.75f, 0.75f, 1.0f));

	ImGui::Checkbox("Active", &visible);

	ImGui::DragFloat("Intensity", &intensity);
	ImGui::DragFloat("Max Distance", &maxDist);

	if (ImGui::Button("Selected"))
		Scene::getInstance()->gizmoEntity = this;

	//Model edit
	if (this->light_type != lightType::AMBIENT)	//except for ambient light, which does not have a model
	{
		ImGui::Checkbox("Shadow map", &show_shadowMap);
		ImGui::Checkbox("Show camera", &show_camera);
		if (this->light_type == lightType::DIRECTIONAL)
			ImGui::Checkbox("Activate cascade", &this->is_cascade);

		if (ImGui::TreeNode(camera, "Camera light")) {
			camera->renderInMenu();
			ImGui::TreePop();
		}

		if (ImGui::TreeNode((void*)this, "Model"))
		{
			float matrixTranslation[3], matrixRotation[3], matrixScale[3];
			ImGuizmo::DecomposeMatrixToComponents(model.m, matrixTranslation, matrixRotation, matrixScale);
			ImGui::DragFloat3("Position", matrixTranslation, 0.1f);
			ImGui::DragFloat3("Rotation", matrixRotation, 0.1f);
			ImGui::DragFloat3("Scale", matrixScale, 0.1f);
			ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, model.m);
			
			ImGui::TreePop();
		}
	}
	else	//to change ambient light intensity
	{
		ImGui::Checkbox("Ambient Light", &visible);
	}

	//Light color
	ImGui::ColorEdit4("Color", (float*)&color);

	ImGui::PopStyleColor();
}

void Light::setPosition(Vector3 pos) { this->model.translate(pos.x, pos.y, pos.z); }
void Light::setPosition(float x, float y, float z) { this->model.translate(x, y, z); }
void Light::setColor(float r, float g, float b) { this->color = Vector3(r, g, b); }

void Light::updateDirectional(Camera* user_camera)
{
	this->camera->eye = user_camera->eye + target_vector;
	this->camera->lookAt(camera->eye, user_camera->eye, Vector3(0, 1, 0));
}

void Light::renderShadowMap(GTR::Renderer* renderer)
{
	if (this->light_type != lightType::SPOT && this->light_type != lightType::DIRECTIONAL &&
		this->light_type != lightType::POINT_LIGHT)
		return;

	int w = 1024;
	int h = 1024;

	renderer->shadow = true;

	if (!this->fbo)
	{
		if (light_type != lightType::POINT_LIGHT)
		{
			this->fbo = new FBO();
			this->fbo->setDepthOnly(w, h);
		}
		else
		{
			this->fbo = new FBO();
			this->fbo->setDepthOnly(w / 2, h * 3);
		}

	}

	if (!this->camera)
		return;

	this->fbo->bind();

	this->camera->enable();

	if (light_type == lightType::SPOT)
	{
		renderSpotShadowMap(renderer);
	}
	else if( light_type == lightType::DIRECTIONAL){

		renderDirectionalShadowMap(renderer, is_cascade);
	}

	this->fbo->unbind();

	renderer->shadow = false;

	glDisable(GL_DEPTH_TEST);

	this->shadowMap = this->fbo->depth_texture;
}

void Light::renderSpotShadowMap(GTR::Renderer* renderer)
{
	int w = this->fbo->depth_texture->width;
	int h = this->fbo->depth_texture->height;

	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, w, h);

	for (auto& entity : Scene::getInstance()->prefabEntities)
	{
		renderer->renderPrefab(entity->model, entity->pPrefab, this->camera);
	}
}

void Light::renderDirectionalShadowMap(GTR::Renderer* renderer, bool is_cascade)
{

	float texture_width = this->fbo->depth_texture->width;
	float texture_height = this->fbo->depth_texture->height;
	float w = 512.0f;
	float h = 512.0f;
	float grid;

	//if (!far_directional_shadowmap_updated)
		glClear(GL_DEPTH_BUFFER_BIT);

	Texture* background = Texture::getWhiteTexture();

	if (!is_cascade)
	{
		this->camera->setOrthographic(-w / 2.0f, w / 2.0f, -h / 2.0f, h / 2.0f,
			this->camera->near_plane, this->camera->far_plane);

		grid = w / (texture_width * 0.5f);
		camera->view_matrix.M[3][1] = round(camera->view_matrix.M[3][1] / grid) * grid;
		camera->view_matrix.M[3][0] = round(camera->view_matrix.M[3][0] / grid) * grid;

		this->camera->viewprojection_matrix = camera->view_matrix * camera->projection_matrix;

		for (auto& entity : Scene::getInstance()->prefabEntities)
		{
			renderer->renderPrefab(entity->model, entity->pPrefab, this->camera);
		}
	}
	else {

		//first quadrant
		//--------------
		this->camera->setOrthographic(-w / 4.0f, w / 4.0f, -h / 4.0f, h / 4.0f,
			this->camera->near_plane, this->camera->far_plane);

		this->camera->updateProjectionMatrix();

		glViewport(0, 0, texture_width / 2.0f, texture_height / 2.0f);

		//in order to find the size of each pixel in world coordinates we need to take the width of the frustum
		//divided by the texture size. Since the texture is an atlas texture we have to divide it by the size of 
		//each real texture and not the whole texture (in this case just the half of the whole texture since each
		//texture occupies a quarter of the whole)
		//once the calculations are done, we round the position of the camera to make it fit into the grid
		grid = (w * 0.5f) / (texture_width * 0.5f);
		camera->view_matrix.M[3][1] = round(camera->view_matrix.M[3][1] / grid) * grid;
		camera->view_matrix.M[3][0] = round(camera->view_matrix.M[3][0] / grid) * grid;
		this->camera->viewprojection_matrix = camera->view_matrix * camera->projection_matrix;

		this->shadow_viewprojection[0] = camera->viewprojection_matrix;
		//this->shadow_viewprojection[0] = camera->viewprojection_matrix;

		for (auto& entity : Scene::getInstance()->prefabEntities)
		{
			renderer->renderPrefab(entity->model, entity->pPrefab, this->camera);
		}

		//second quadrant
		//---------------
		this->camera->setOrthographic(-w / 2.0f, w / 2.0f, -h / 2.0f, h / 2.0f,
			this->camera->near_plane, this->camera->far_plane);

		glViewport(texture_width / 2.0f, 0, texture_width / 2.0f, texture_height / 2.0f);

		grid = w / (this->fbo->depth_texture->width / 2);
		camera->view_matrix.M[3][1] = round(camera->view_matrix.M[3][1] / grid) * grid;
		camera->view_matrix.M[3][0] = round(camera->view_matrix.M[3][0] / grid) * grid;
		camera->viewprojection_matrix = camera->view_matrix * camera->projection_matrix;

		this->shadow_viewprojection[1] = camera->viewprojection_matrix;

		for (auto& entity : Scene::getInstance()->prefabEntities)
		{
			renderer->renderPrefab(entity->model, entity->pPrefab, this->camera);
		}

		//third quadrant
		//--------------
		this->camera->setOrthographic(-w, w, -h, h, this->camera->near_plane, this->camera->far_plane);

		glViewport(0, texture_height / 2, texture_width / 2, texture_height / 2);

		grid = (w * 2.0) / (this->fbo->depth_texture->width / 2);
		camera->view_matrix.M[3][1] = round(camera->view_matrix.M[3][1] / grid) * grid;
		camera->view_matrix.M[3][0] = round(camera->view_matrix.M[3][0] / grid) * grid;
		camera->viewprojection_matrix = camera->view_matrix * camera->projection_matrix;

		shadow_viewprojection[2] = camera->viewprojection_matrix;

		for (auto& entity : Scene::getInstance()->prefabEntities)
		{
			renderer->renderPrefab(entity->model, entity->pPrefab, this->camera);
		}

		//fourth quadrant
		//---------------
		this->camera->setOrthographic(-2 * w, 2 * w, -2 * h, 2 * h / 2,
			this->camera->near_plane, this->camera->far_plane);

		glViewport(texture_width / 2, texture_height / 2, texture_width / 2, texture_height / 2);

		grid = (w * 4.0) / (this->fbo->depth_texture->width / 2);
		camera->view_matrix.M[3][1] = round(camera->view_matrix.M[3][1] / grid) * grid;
		camera->view_matrix.M[3][0] = round(camera->view_matrix.M[3][0] / grid) * grid;
		camera->viewprojection_matrix = camera->view_matrix * camera->projection_matrix;

		if (!renderedHighShadow) {
			this->shadow_viewprojection[3] = camera->viewprojection_matrix;
			renderedHighShadow = true;
		}

		for (auto& entity : Scene::getInstance()->prefabEntities)
		{
			renderer->renderPrefab(entity->model, entity->pPrefab, this->camera);
		}

		far_directional_shadowmap_updated = true;
	}
}