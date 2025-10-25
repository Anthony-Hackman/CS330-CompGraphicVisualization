///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
	m_loadedTextures = 0; // Initialize
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST); // AH: Noise Reduction
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glDeleteTextures(1, &m_textureIDs[i].ID); // AH Updated: Gen -> Delete
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;
		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/******************************************************************************/

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
* DefineObjectMaterials()
* Anthony Hackman
***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	//------------------------------------------------------
	OBJECT_MATERIAL		goldMaterial;
	//
	goldMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.1f);
	goldMaterial.ambientStrength = 0.4f;
	goldMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.2f);
	goldMaterial.specularColor = glm::vec3(0.6f, 0.5f, 0.4f);
	goldMaterial.shininess = 22.0;
	goldMaterial.tag = "gold";
	m_objectMaterials.push_back(goldMaterial);
	//------------------------------------------------------
	OBJECT_MATERIAL		cementMaterial;
	//
	cementMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	cementMaterial.ambientStrength = 0.2f;
	cementMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	cementMaterial.specularColor = glm::vec3(0.4f, 0.4f, 0.4f);
	cementMaterial.shininess = 0.5;
	cementMaterial.tag = "cement";
	m_objectMaterials.push_back(cementMaterial);
	//------------------------------------------------------
	OBJECT_MATERIAL		woodMaterial;
	//
	woodMaterial.ambientColor = glm::vec3(0.4f, 0.3f, 0.2f);
	woodMaterial.ambientStrength = 0.2f;
	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.2f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.2f);
	woodMaterial.shininess = 0.3;
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);
	//------------------------------------------------------
	OBJECT_MATERIAL		tileMaterial;
	//
	tileMaterial.ambientColor = glm::vec3(0.2f, 0.3f, 0.4f);
	tileMaterial.ambientStrength = 0.3f;
	tileMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.1f);
	tileMaterial.specularColor = glm::vec3(0.4f, 0.5f, 0.6f);
	tileMaterial.shininess = 25.0;
	tileMaterial.tag = "tile";
	m_objectMaterials.push_back(tileMaterial);
	//------------------------------------------------------
	OBJECT_MATERIAL		glassMaterial;
	//
	glassMaterial.ambientColor = glm::vec3(0.4f, 0.4f, 0.4f);
	glassMaterial.ambientStrength = 0.3f;
	glassMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	glassMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	glassMaterial.shininess = 85.0;
	glassMaterial.tag = "glass";
	m_objectMaterials.push_back(glassMaterial);
	//------------------------------------------------------
	OBJECT_MATERIAL		clayMaterial;
	//
	clayMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.3f);
	clayMaterial.ambientStrength = 0.3f;
	clayMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.5f);
	clayMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.4f);
	clayMaterial.shininess = 0.5;
	clayMaterial.tag = "clay";
	m_objectMaterials.push_back(clayMaterial);
}

/***********************************************************
* SetupSceneLights()
* Adds multiple light sources for realistic illumination
***********************************************************/
void SceneManager::SetupSceneLights()
{
	// Main Light (neutral)
	m_pShaderManager->setVec3Value("lightSources[0].position", 3.0f, 14.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.05f);

	// Key light 1 (neutral)
	m_pShaderManager->setVec3Value("lightSources[1].position", -3.0f, 14.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.05f);

	// Key light 2 (cool tone)
	m_pShaderManager->setVec3Value("lightSources[2].position", 0.6f, 5.0f, 6.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 12.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.5f);

	// Add warm spotlight to mimic sun
	m_pShaderManager->setVec3Value("lightSources[3].position", 3.0f, 2.0f, 1.0f);
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", 0.1f, 0.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 1.0f, 0.3f, 0.4f);
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", 0.5f, 0.2f, 0.3f);
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 1.0f);

	// Enable lighting
	m_pShaderManager->setBoolValue("bUseLighting", true);
}

/**********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	LoadSceneTextures();      // 1
	DefineObjectMaterials();  // 2
	SetupSceneLights();       // 3

	// Load meshes
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadCylinderMesh();
}

/***********************************************************
 *  LoadSceneTextures()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/
	// 
	// @author Anthony Hackman
	// @date 10/5/2025
	// 
	// Ex. Usage;	bReturn = CreateGLTexture("../../<PATH>.ext", "<TAG>");
	bool bReturn = false;
	bReturn = CreateGLTexture("../../Utilities/textures/green_grass.jpg", "green_grass");
	bReturn = CreateGLTexture("../../Utilities/textures/grey_concrete.jpg", "grey_concrete");
	bReturn = CreateGLTexture("../../Utilities/textures/roofing.jpg", "roofing");
	bReturn = CreateGLTexture("../../Utilities/textures/pavers.jpg", "pavers");
	bReturn = CreateGLTexture("../../Utilities/textures/missing_texture.jpg", "missing_texture");
	bReturn = CreateGLTexture("../../Utilities/textures/256_mystic_blue_siding_wood_texture-seamless.jpg", "mystic_blue_siding_wood_texture_seamless");
	bReturn = CreateGLTexture("../../Utilities/textures/52_wood_fence_cut_out_texture.png", "wood_fence_cut_out_texture");
	bReturn = CreateGLTexture("../../Utilities/textures/18_bark_texture-seamless.jpg", "bark_texture_seamless");

	// After the texture image data is loaded into memory, 
	// the loaded textures need to be bound to texture slots using:
	BindGLTextures();
	// 
	// there are a total of 16 available slots for scene textures
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;
	/******************************************************************************/

	// ENVIRONMENT ===========================================================================

	// TEST BOX			**********************************************//
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 1.0f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 1.0f, 8.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.5, 0.5, 0.5, 1); // grey
	SetShaderTexture("missing_texture");
	SetTextureUVScale(1.0f, 1.0f);	// Set tiling

	// Give material for shader/lighting
	SetShaderMaterial("cement");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	SetTextureUVScale(1.0f, 1.0f);	// Reset tiling to default
	/******************************************************************/

	// GROUND PLANE 	**********************************************//
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.3, 0.6, 0.3, 1);
	SetShaderTexture("green_grass");
	SetTextureUVScale(20.0f, 10.0f);	// Set tiling

	// Give material for shader/lighting
	SetShaderMaterial("clay");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	SetTextureUVScale(1.0f, 1.0f);	// Reset tiling to default
	/******************************************************************/

	// Cylinder 	**************************************************//
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 5.0f, 0.3f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.0f, 0.0f, 5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.3, 0.6, 0.3, 1);
	SetShaderTexture("bark_texture_seamless");
	SetTextureUVScale(1.0f, 7.0f);	// Set tiling

	// Give material for shader/lighting
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	SetTextureUVScale(1.0f, 1.0f);	// Reset tiling to default
	/******************************************************************/

	// DRIVEWAY PLANE	**********************************************//
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.5f, 1.0f, 5.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 0.01f, 5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.6, 0.6, 0.6, 1);
	SetShaderTexture("grey_concrete");
	SetTextureUVScale(1.0f, 5.0f);	// Tile

	// Give material for shader/lighting
	SetShaderMaterial("cement");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	SetTextureUVScale(1.0f, 1.0f);	// Reset tiling to default
	/******************************************************************/

	// GARAGE ====================================================================================

	// GARAGE BOX			******************************************//
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.0f, 2.5f, 3.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 1.0f, -1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.5, 0.5, 0.5, 1); // gray
	SetShaderTexture("mystic_blue_siding_wood_texture_seamless");
	SetTextureUVScale(1.0f, 0.75f);	// Tile

	// Give material for shader/lighting
	SetShaderMaterial("clay");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	SetTextureUVScale(1.0f, 1.0f);	// Reset tiling to default
	/******************************************************************/

	// GARAGE BOX: 2 (ABOVE GARAGE) **********************************//
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.99f, 0.99f, 2.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 2.5f, -0.51f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.5, 0.5, 0.5, 1); // gray
	SetShaderTexture("mystic_blue_siding_wood_texture_seamless");
	SetTextureUVScale(1.0f, 0.5f);	// Tile

	// Give material for shader/lighting
	SetShaderMaterial("clay");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	SetTextureUVScale(1.0f, 1.0f);	// Reset tiling to default
	/******************************************************************/
	
	// GARAGE ROOF: (LOWER)	******************************************//
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.0f, 3.0f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 270.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.99f, 2.5f, -0.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.5, 0.5, 0.5, 1); // gray
	SetShaderTexture("roofing");
	SetTextureUVScale(2.0f, 3.0f);	// Set tiling

	// Give material for shader/lighting
	SetShaderMaterial("cement");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPrismMesh();
	SetTextureUVScale(1.0f, 1.0f);	// Reset tiling to default
	/******************************************************************/

	// GARAGE ROOF: 2 (TOP) ******************************************//
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.0f, 3.0f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 270.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.25f, 3.5f, -0.25f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.5, 0.5, 0.5, 1); // gray
	SetShaderTexture("roofing");
	SetTextureUVScale(2.0f, 3.0f); 	// Set tiling

	// Give material for shader/lighting
	SetShaderMaterial("cement");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPrismMesh();
	SetTextureUVScale(1.0f, 1.0f);	// Reset tiling to default
	/******************************************************************/

	// GARAGE ROOF: 2 (TOP INNER) ************************************//
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.01f, 3.01f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 270.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.25f, 3.49f, -0.25); // float to fix clipping

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetShaderColor(0.4f, 0.4f, 0.4f, 1); // gray
	//SetShaderTexture("mystic_blue_siding_wood_texture_seamless");
	//SetTextureUVScale(1.0f, 1.0f);	// Tile

	// Give material for shader/lighting
	SetShaderMaterial("clay");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPrismMesh();
	SetTextureUVScale(1.0f, 1.0f);	// Reset tiling to default
	/******************************************************************/

	// HOUSE	 ====================================================================================

	// HOUSE BOX			******************************************//
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(5.0f, 3.0f, 5.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.0f, 1.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.5, 0.5, 0.5, 1); // gray
	SetShaderTexture("mystic_blue_siding_wood_texture_seamless");
	SetTextureUVScale(1.0f, 1.0f);	// Tile

	// Give material for shader/lighting
	SetShaderMaterial("clay");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	SetTextureUVScale(1.0f, 1.0f);	// Reset tiling to default
	/******************************************************************/

	// HOUSE ROOF - triangular prism		**************************//
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(6.0f, 5.5f, 2.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 270.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.0f, 3.5f, 0.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.5, 0.5, 0.5, 1); // gray
	SetShaderTexture("roofing");
	SetTextureUVScale(2.0f, 3.0f); 	// Set tiling

	// Give material for shader/lighting
	SetShaderMaterial("cement");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPrismMesh();
	SetTextureUVScale(1.0f, 1.0f);	// Reset tiling to default
	/******************************************************************/

	// HOUSE ROOF INNER						**************************//
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(5.99f, 5.51f, 2.0f); // Overhang the roof

	// set the XYZ rotation for the mesh
	XrotationDegrees = 270.0f; // rotate 270 degrees to lay it on its side
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f; // rotate 90 degrees to face the view point

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.0f, 3.49f, 0.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.4f, 0.4f, 0.4f, 1); // gray
	//SetShaderTexture("mystic_blue_siding_wood_texture_seamless");
	//SetTextureUVScale(1.0f, 1.0f);	// Tile

	// Give material for shader/lighting
	SetShaderMaterial("clay");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPrismMesh();
	SetTextureUVScale(1.0f, 1.0f);	// Reset tiling to default
	/******************************************************************/

	// WINDOW							 *****************************//
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.5f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.0f, 1.5f, 2.51f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.4f, 0.4f, 0.4f, 1); // grey
	//SetShaderTexture("");
	//SetTextureUVScale(1.0f, 1.0f); 	// Set tiling

	// Give material for shader/lighting
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	SetTextureUVScale(1.0f, 1.0f);	// Reset tiling to default
	/******************************************************************/

	// DOOR								 *****************************//
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 1.0f, 0.95f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.5f, 1.4f, 2.51f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.4f, 0.4f, 0.4f, 1); // grey
	//SetShaderTexture("");
	//SetTextureUVScale(1.0f, 1.0f); 	// Set tiling

	// Give material for shader/lighting
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	SetTextureUVScale(1.0f, 1.0f);	// Reset tiling to default
	/******************************************************************/

	// GARAGE DOOR						 *****************************//
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 0.75f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 1.0f, 0.51f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.4f, 0.4f, 0.4f, 1); // gray
	//SetShaderTexture("mystic_blue_siding_wood_texture_seamless");
	//SetTextureUVScale(1.0f, 1.0f);	// Tile

	// Give material for shader/lighting
	SetShaderMaterial("clay");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	SetTextureUVScale(1.0f, 1.0f);	// Reset tiling to default
	/******************************************************************/

	// PORCH BOX			******************************************//
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.99f, 1.0f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.0f, 0.0f, 3.0f); // Under the roof Overhang

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.4, 0.3, 0.3, 1); // Red

	//SetShaderTexture("roofing");
	//SetTextureUVScale(2.0f, 3.0f); 	// Set tiling
	
	// Give material for shader/lighting
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	SetTextureUVScale(1.0f, 1.0f);	// Reset tiling to default
	/******************************************************************/

	// PILLAR (LEFT) 		******************************************//
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.1f, 2.6f, 0.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.5f, 1.2f, 3.4f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1); // light

	//SetShaderTexture("roofing");
	//SetTextureUVScale(2.0f, 3.0f); 	// Set tiling

	// Give material for shader/lighting
	SetShaderMaterial("cement");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	SetTextureUVScale(1.0f, 1.0f);	// Reset tiling to default
	/******************************************************************/

	// PILLAR (Middle) 		******************************************//
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.1f, 2.6f, 0.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.5f, 1.2f, 3.4f); // Under the roof Overhang

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1); // light

	//SetShaderTexture("roofing");
	// //SetTextureUVScale(2.0f, 3.0f); 	// Set tiling

	// Give material for shader/lighting
	SetShaderMaterial("cement");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	SetTextureUVScale(1.0f, 1.0f);	// Reset tiling to default
	/******************************************************************/

	// PILLAR (Right) 		******************************************//
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.1f, 2.6f, 0.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.5f, 1.2f, 3.4f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1); // light

	//SetShaderTexture("roofing");
	// //SetTextureUVScale(2.0f, 3.0f); 	// Set tiling

	// Give material for shader/lighting
	SetShaderMaterial("cement");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	SetTextureUVScale(1.0f, 1.0f);	// Reset tiling to default
	/******************************************************************/

	// FENCE ====================================================================================

	// PLANE 1	(Garage side) ****************************************//
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.0f, 3.0f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-6.0f, 0.5f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.4, 0.3, 0.3, 1); // Red
	SetShaderTexture("wood_fence_cut_out_texture");
	SetTextureUVScale(2.0f, 1.0f); 	// Set tiling

	// Give material for shader/lighting
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	SetTextureUVScale(1.0f, 1.0f);	// Reset tiling to default
	/******************************************************************/
	
	// PLANE 2	(Left Connecting 1-3) ********************************//
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(5.0f, 3.0f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-9.0f, 0.5f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.4, 0.3, 0.3, 1); // Red
	SetShaderTexture("wood_fence_cut_out_texture");
	SetTextureUVScale(5.0f, 1.0f); 	// Set tiling

	// Give material for shader/lighting
	SetShaderMaterial("cement");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	SetTextureUVScale(1.0f, 1.0f);	// Reset tiling to default
	/******************************************************************/

	// PLANE 3	(Back Connecting 2-4) ********************************//
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(7.0f, 3.0f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 0.5f, -10.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.4, 0.3, 0.3, 1); // Red
	SetShaderTexture("wood_fence_cut_out_texture");
	SetTextureUVScale(5.0f, 1.0f); 	// Set tiling

	// Give material for shader/lighting
	SetShaderMaterial("cement");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	SetTextureUVScale(1.0f, 1.0f);	// Reset tiling to default
	/******************************************************************/

	// PLANE 4	(Right Connecting 3-5) *******************************//
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(5.0f, 3.0f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.0f, 0.5f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.4, 0.3, 0.3, 1); // Red
	SetShaderTexture("wood_fence_cut_out_texture");
	SetTextureUVScale(5.0f, 1.0f); 	// Set tiling

	// Give material for shader/lighting
	SetShaderMaterial("cement");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	SetTextureUVScale(1.0f, 1.0f);	// Reset tiling to default
	/******************************************************************/
	
	// PLANE 5	(Right Connecting House) *****************************//
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.5f, 1.0f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.5f, 0.5f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.4, 0.3, 0.3, 1); // Red
	SetShaderTexture("wood_fence_cut_out_texture");
	SetTextureUVScale(1.0f, 1.0f); 	// Set tiling

	// Give material for shader/lighting
	SetShaderMaterial("cement");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	SetTextureUVScale(1.0f, 1.0f);	// Reset tiling to default
	/******************************************************************/
}