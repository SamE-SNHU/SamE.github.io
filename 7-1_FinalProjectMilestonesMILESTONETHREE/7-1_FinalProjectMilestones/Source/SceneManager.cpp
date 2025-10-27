///////////////////////////////////////////////////////////////////////////////
//	File: ShaderManager.cpp
// ============
//	Description: Manage the loading and rendering of 3D scenes
//	Purpose: Implements scene and material management and drawing
//	Enhanced: Focused on readability and structure
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for: CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
//  EDITOR: Samantha Ellis, Oct. 23rd, 2025
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// Declaration of global variables
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

	// Initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
	m_lastTextureSlot = -1;
	m_lastMaterialTag.clear();
	m_lastUVScale = glm::vec2(-1.0f, -1.0f);
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

	// Destroy the created OpenGL textures
	DestroyGLTextures();
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

	// Indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// Try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// If the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// Set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	
		// Set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// If the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	
		// If the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// Generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// Free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// Register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_textureSlotByTag[tag] = m_loadedTextures;
		m_textureIdByTag[tag] = textureID;
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
		// Bind textures on corresponding texture units
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
		glGenTextures(1, &m_textureIDs[i].ID);
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
	// Variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// Set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);

	// Set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));

	// Set the translation value in the transform buffer
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
	// Variables for this method
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

        const int textureSlot = FindTextureSlot(textureTag);
        if (textureSlot < 0) return;

        // skip work if we already set this slot last time
        if (textureSlot == m_lastTextureSlot) {
            return;
        }

        m_pShaderManager->setSampler2DValue(g_TextureValueName, textureSlot);
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
        const bool ok = FindMaterial(materialTag, material); // now O(1) via map + fallback
        if (ok)
        {
            m_pShaderManager->setVec3Value("material.ambientColor",    material.ambientColor);
            m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
            m_pShaderManager->setVec3Value("material.diffuseColor",    material.diffuseColor);
            m_pShaderManager->setVec3Value("material.specularColor",   material.specularColor);
            m_pShaderManager->setFloatValue("material.shininess",       material.shininess);
        }
    }
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

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
	bool bReturn = false;

	bReturn = CreateGLTexture(
		"Texture/Christmas_Texture.png", "Christmas");

	bReturn = CreateGLTexture(
		"Texture/Grey_Texture.png", "Grey");

	bReturn = CreateGLTexture(
		"Texture/Ceramic_Texture.png", "Ceramic");

	bReturn = CreateGLTexture(
		"Texture/Glass_Texture.png", "Glass");

	bReturn = CreateGLTexture(
		"Texture/Wood_Texture.png", "Wood");

	bReturn = CreateGLTexture(
		"Texture/Stone_Texture.png", "Stone");

	// After the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	m_objectMaterials.reserve(8);
	OBJECT_MATERIAL goldMaterial;
	goldMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	goldMaterial.ambientStrength = 0.3f;
	goldMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	goldMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	goldMaterial.shininess = 22.0;
	goldMaterial.tag = "metal";

	m_objectMaterials.push_back(goldMaterial);
	m_materialIndex[goldMaterial.tag] = m_objectMaterials.size() - 1;
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting - to use the default rendered 
	// lighting then comment out the following line
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	//White point light
	m_pShaderManager->setVec3Value("lightSources[0].position", -3.0f, 4.0f, 6.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.2f);

	//Blue colored light
	m_pShaderManager->setVec3Value("lightSources[1].position", 3.0f, 4.0f, 6.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.0f, 0.0f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.2f, 0.2f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.5f);

	//Dim white point light
	m_pShaderManager->setVec3Value("lightSources[2].position", 0.0f, 3.0f, 20.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 12.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.2f);
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the textures for the 3D scene
	LoadSceneTextures();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	m_basicMeshes->LoadPlaneMesh();
	
	//Load cylinder
	m_basicMeshes->LoadCylinderMesh();//Mug body, Cookie jar
	
	//Load torus
	m_basicMeshes->LoadTorusMesh();//Mug handle
	
	//Load sphere
	m_basicMeshes->LoadSphereMesh();//Pomegranate
	
	//Load cone
	m_basicMeshes->LoadConeMesh();//Lamps
	
	//Load box
	m_basicMeshes->LoadBoxMesh();//Shelves
	
	//Load tapered cylinder
	m_basicMeshes->LoadTaperedCylinderMesh();//Cake stand base
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

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// Set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// Set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// Set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("Stone");
	SetShaderColor(1, 1, 1, 1);

	// Draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
	
	// Cylinder, mug body
	scaleXYZ = glm::vec3(1.0f, 2.0f, 1.0f);

	// Set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.0f, 7.0f, -5.0f);

	// Set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.5, 1.0, 1.0); //Blue

	SetShaderTexture("Christmas");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// Draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/***************************************************************/

	// Torus, mug handle
	scaleXYZ = glm::vec3(0.6f, 0.6f, 0.6f);

	// Set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// Set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.0f, 8.0f, -5.0f);

	// Set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0f, 0.5f, 0.0f, 1.0f); //Orange 

	SetShaderTexture("Grey");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// Draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	/****************************************************************/

	// Cylinder, cookie jar
	scaleXYZ = glm::vec3(1.0f, 2.0f, 1.0f);

	// Set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.0f, 7.0f, -5.0f);

	// Set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.5, 1.0, 1.0); //Blue

	SetShaderTexture("Ceramic");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// Draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/***************************************************************/

	// Box, Shelf 1
	scaleXYZ = glm::vec3(2.0f, 15.0f, 1.0f);

	// Set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// Set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 6.5f, -5.0f);

	// Set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.647, 0.482, 0.318, 1.0); //Brown

	SetShaderTexture("Wood");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// Draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/***************************************************************/

	// Box, Shelf 2
	scaleXYZ = glm::vec3(2.0f, 15.0f, 1.0f);

	// Set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// Set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 10.5f, -5.0f);

	// Set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.647, 0.482, 0.318, 1.0); //Brown

	SetShaderTexture("Wood");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// Draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/***************************************************************/

	// Cone, lamp 1
	scaleXYZ = glm::vec3(2.0f, 2.0f, 2.0f);

	// Set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.0f, 14.0f, -5.0f);

	// Set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.133, 0.545, 0.133, 1.0); // Forest Green

	SetShaderTexture("Glass");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// Draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();

	/***************************************************************/
	// Cone, lamp 2
	scaleXYZ = glm::vec3(2.0f, 2.0f, 2.0f);

	// Set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 14.0f, -5.0f);

	// Set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.133, 0.545, 0.133, 1.0); // Forest Green

	SetShaderTexture("Glass");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// Draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();

	/***************************************************************/

	// Tapered cylinder, cake base
	scaleXYZ = glm::vec3(1.0f, 1.0f, 1.0f);

	// Set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.0f, 0.0f, -3.0f);

	// Set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.133, 0.545, 0.133, 1.0); // Forest Green

	SetShaderTexture("Ceramic");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// Draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

	/***************************************************************/

	// Cylinder, cake stand top
	scaleXYZ = glm::vec3(2.0f, 0.5f, 2.0f);

	// Set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.0f, 1.0f, -3.0f);

	// Set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.5, 1.0, 1.0); //Blue

	SetShaderTexture("Ceramic");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// Draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/***************************************************************/

	// Sphere, pomegranate
	scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f);

	// Set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.0f, 2.0f, -3.0f);

	// Set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.937, 0.192, 0.290, 1.0); //Red

	// SetShaderTexture("Christmas");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// Draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

	/***************************************************************/
}
