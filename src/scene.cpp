#include "scene.h"
#include "mesh.h"
#include "prefab.h"
#include "texture.h"

#include "camera.h"

Scene* Scene::instance = nullptr;

Scene::Scene()
{
	Matrix44 model;
	numLightEntities = 0;
	numPrefabEntities = 0;
	ambientLight = Vector3(0.1f, 0.1f, 0.1f);
	gizmoEntity = nullptr;
}

void Scene::render(Camera* camera, GTR::Renderer* renderer) {

	//check if ambient light is activated
	if (!ambient_light)
		Scene::getInstance()->ambientLight = Vector3();
	else
		Scene::getInstance()->ambientLight = Vector3(0.1, 0.1, 0.1);

	for (size_t i = 0; i < this->prefabEntities.size(); i++) {
		renderer->shadow = true;
		this->prefabEntities.at(i)->render(camera, renderer);
		renderer->shadow = false;
		this->prefabEntities.at(i)->render(camera, renderer);
	}
};

void Scene::generateTerrain(float size)
{
	Mesh* floorMesh = new Mesh();
	floorMesh->createPlane(size);

	GTR::Prefab* floorPrefab = new GTR::Prefab();

	floorPrefab->name = "floor";
	floorPrefab->root.mesh = floorMesh;
	floorPrefab->root.material = GTR::Material::Get("asphalt");
	floorPrefab->root.material->tilling_factor = 10;
	PrefabEntity* floorEntity = new PrefabEntity(floorPrefab);
	this->prefabEntities.push_back(floorEntity);
}

void Scene::generateScene(Camera* camera) {

	//generate floor
	//--------------

	generateTerrain(1000);

	//generate lights
	//---------------

	this->ambientLight = Vector3(0.1, 0.1, 0.1);

	Light* light = new Light(lightType::POINT_LIGHT);
	light->setPosition(80, 120, 175);
	light->intensity = 5;
	light->maxDist = 500;

	Light* lightRed = new Light(lightType::POINT_LIGHT);
	lightRed->setPosition(70, 20, -150);
	lightRed->setColor(0.85, 0.1, 0.1);
	lightRed->maxDist = 1000;
	lightRed->intensity = 5;

	Light* lightRed2 = new Light(lightType::POINT_LIGHT);
	lightRed2->setPosition(0, 30, 650);
	lightRed2->setColor(0.85, 0.1, 0.1);
	lightRed2->maxDist = 1000;

	Light* spotLight = new Light(lightType::SPOT);
	spotLight->setPosition(-30, 100, 80);
	spotLight->color = Vector3(1, 0.7, 0.4);
	spotLight->maxDist = 1200;
	spotLight->camera->far_plane = spotLight->maxDist;
	spotLight->camera->lookAt(spotLight->model.getTranslation(), 
		spotLight->model.getTranslation() + spotLight->model.frontVector(), Vector3(0, 1, 0));

	Light* spotLight2 = new Light(lightType::SPOT);
	spotLight2->model.rotate(-90 * DEG2RAD, Vector3(0, 1, 0));
	spotLight2->setPosition(-600, 100, -300);
	spotLight2->color = Vector3(1, 0.7, 0.4);
	spotLight2->maxDist = 1200;
	spotLight2->camera->far_plane = spotLight2->maxDist;
	spotLight2->camera->lookAt(spotLight2->model.getTranslation(),
		spotLight2->model.getTranslation() + spotLight2->model.frontVector(), Vector3(0, 1, 0));

	Light* directionalLight = new Light(lightType::DIRECTIONAL);
	directionalLight->setPosition(450.0f, 600.0f, 0.0f);
	directionalLight->camera->lookAt(directionalLight->model.getTranslation(), camera->eye,
		Vector3(0, 1, 0));
	directionalLight->camera->far_plane = 3000.0f;
	//directionalLight->setColor(0.2, 0.4, 0.6);
	directionalLight->setColor(0.015, 0.04, 0.1);
	directionalLight->intensity = 1;
	directionalLight->target_vector = directionalLight->camera->eye - camera->eye;
	this->lightEntities.push_back(directionalLight);

	this->lightEntities.push_back(light);
	this->lightEntities.push_back(lightRed);
	this->lightEntities.push_back(lightRed2);
	this->lightEntities.push_back(spotLight);
	this->lightEntities.push_back(spotLight2);

	//generate objects
	//----------------

	GTR::Prefab* car_prefab = GTR::Prefab::Get("data/prefabs/gmc/scene.gltf");

	PrefabEntity* car = new PrefabEntity(car_prefab);
	car->pPrefab->root.material = new GTR::Material();
	car->pPrefab->root.material->emissive_texture->Get("data/prefabs/gmc/textures/Material_33_emissive.png");

	PrefabEntity* car2 = new PrefabEntity(car_prefab);
	car2->setPosition(0, 0, 500);
	car2->model.rotate(DEG2RAD * 180, Vector3(0, 1, 0));

	PrefabEntity* car3 = new PrefabEntity(car_prefab);
	car3->setPosition(150, 0, 0);
	car3->model.rotate(DEG2RAD * 30, Vector3(0, 1, 0));

	PrefabEntity* car4 = new PrefabEntity(car_prefab);
	car4->setPosition(-900, 0, 0);
	car4->model.rotate(DEG2RAD * 30, Vector3(0, 1, 0));

	this->prefabEntities.push_back(car);
	this->prefabEntities.push_back(car2);
	this->prefabEntities.push_back(car3);
	this->prefabEntities.push_back(car4);

}

void Scene::generateTestScene() 
{
	generateTerrain(1000);

	Light* spotLight = new Light(lightType::POINT_LIGHT);
	spotLight->setPosition(0.0f, 200.0f, 0.0f);
	spotLight->model.rotate(90 * DEG2RAD, Vector3(-1, 0, 0));
	spotLight->camera->lookAt(spotLight->model.getTranslation(), spotLight->model.frontVector(),
		Vector3(0, 1, 0));
	spotLight->setColor(0.8, 0.3, 0.1);
	spotLight->intensity = 1;
	this->lightEntities.push_back(spotLight);
	

	Light* spotLight2 = new Light(lightType::SPOT);
	spotLight2->setPosition(30.0f, 200.0f, 0.0f);
	spotLight2->model.rotate(90 * DEG2RAD, Vector3(-1, 0, 0));
	spotLight2->camera->lookAt(spotLight2->model.getTranslation(), spotLight2->model.frontVector(), Vector3(0, 1, 0));
	spotLight2->setColor(1, 0, 1);
	spotLight2->intensity = 1;
	//this->lightEntities.push_back(spotLight2);
	

	Light* directionalLight = new Light(lightType::DIRECTIONAL);
	directionalLight->setPosition(100.0f, 750.0f, 0.0f);
	directionalLight->model.rotate(90 * DEG2RAD, Vector3(-1, 0, 0));
	directionalLight->camera->lookAt(directionalLight->model.getTranslation(), directionalLight->model.frontVector(),
		Vector3(0, 1, 0));
	directionalLight->camera->far_plane = 5000;
	directionalLight->setColor(0.1, 0.2, 0.4);
	directionalLight->intensity = 2;
	this->lightEntities.push_back(directionalLight);

	GTR::Prefab* cube = new GTR::Prefab();

	Mesh* cubeMesh = new Mesh;
	cubeMesh->createPyramid();
	cube->root.mesh = cubeMesh;
	cube->root.material = GTR::Material::Get("asphalt");
	
	PrefabEntity* cubeEntity = new PrefabEntity(cube);
	cubeEntity->setPosition(0.0f, 50.0f, 0.0f);
	cubeEntity->model.scale(10.0f, 10.0f, 10.0f);
	this->prefabEntities.push_back(cubeEntity);
}

void Scene::generateDepthMap(GTR::Renderer* renderer)
{
	for (auto light : lightEntities)
	{
		light->renderShadowMap(renderer);
	}
}

void Scene::update(Camera* camera)
{
	for (auto light : lightEntities)
	{
		if (light->light_type == lightType::DIRECTIONAL)
		{
			light->updateDirectional(camera);
		}
	}
}

void Scene::renderDeferred(Camera* camera, GTR::Renderer* renderer)
{
	renderer->renderDeferred(camera);
}