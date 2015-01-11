// http://www.rastertek.com/tertut02.html Used for reference.
// http://www.rastertek.com/tertut14.html Used for reference.


#include "stdafx.h"
#include "TerrainShaderClass.h"


TerrainShaderClass::TerrainShaderClass()
{
	m_effect = 0;
	m_technique = 0;
	m_layout = 0;

	m_worldMatrixPtr = 0;
	m_viewMatrixPtr = 0;
	m_projectionMatrixPtr = 0;
	m_texturePtr = 0;
	m_texturePtr1 = 0;
	m_texturePtr2 = 0;
	m_lightDirectionPtr = 0;
	m_ambientColorPtr = 0;
	m_diffuseColorPtr = 0;
}


TerrainShaderClass::TerrainShaderClass(const TerrainShaderClass& other)
{
}


TerrainShaderClass::~TerrainShaderClass()
{
}


bool TerrainShaderClass::Initialize(ID3D10Device* device)
{
	bool result;


	// Initialize the shader that will be used to draw the triangle.
	result = InitializeShader(device, L"../ModellingInThreeDimensions/data/terrain/terrain.fx");
	if(!result)
	{
		return false;
	}

	return true;
}


void TerrainShaderClass::Shutdown()
{
	// Shutdown the shader effect.
	ShutdownShader();

	return;
}


void TerrainShaderClass::Render(ID3D10Device* device, int indexCount, D3DXMATRIX worldMatrix, D3DXMATRIX viewMatrix, D3DXMATRIX projectionMatrix, 
							  ID3D10ShaderResourceView* texture, ID3D10ShaderResourceView* texture1, ID3D10ShaderResourceView* texture2, D3DXVECTOR3 lightDirection, D3DXVECTOR4 ambientColor, D3DXVECTOR4 diffuseColor,
							  //Shadow
								D3DXMATRIX lightViewMatrix, D3DXMATRIX lightProjectionMatrix,ID3D10ShaderResourceView* depthMapTexture,
								D3DXVECTOR3 lightPosition, D3DXVECTOR3 cameraPosition)
{
	// Set the shader parameters that it will use for rendering.
	SetShaderParameters(worldMatrix, viewMatrix, projectionMatrix, texture, texture1, texture2, lightDirection, ambientColor, diffuseColor,
		//Shadow
								lightViewMatrix, lightProjectionMatrix,depthMapTexture,
								lightPosition, cameraPosition);

	// Now render the prepared buffers with the shader.
	RenderShader(device, indexCount);

	return;
}


bool TerrainShaderClass::InitializeShader(ID3D10Device* device, WCHAR* filename)
{
	HRESULT result;
	ID3D10Blob* errorMessage;
	D3D10_INPUT_ELEMENT_DESC polygonLayout[3];
	unsigned int numElements;
    D3D10_PASS_DESC passDesc;


	// Initialize the error message.
	errorMessage = 0;

	// Load the shader in from the file.
	result = D3DX10CreateEffectFromFile(filename, NULL, NULL, "fx_4_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, 
										device, NULL, NULL, &m_effect, &errorMessage, NULL);
	if(FAILED(result))
	{
		// If the shader failed to compile it should have writen something to the error message.
		if(errorMessage)
		{
			Log::COut("Shader failed to compile");
			Log::COut((char*)(errorMessage->GetBufferPointer()));

			// Release the error message.
			errorMessage->Release();
			errorMessage = 0;

		}
		// If there was nothing in the error message then it simply could not find the shader file itself.
		else
		{
			Log::COut("Missing shader file");
		}

		return false;
	}

	// Get a pointer to the technique inside the shader.
	m_technique = m_effect->GetTechniqueByName("LightTechnique");
	if(!m_technique)
	{
		return false;
	}

	// Now setup the layout of the data that goes into the shader.
	// This setup needs to match the VertexType stucture in the ModelClass and in the shader.
	polygonLayout[0].SemanticName = "POSITION";
	polygonLayout[0].SemanticIndex = 0;
	polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[0].InputSlot = 0;
	polygonLayout[0].AlignedByteOffset = 0;
	polygonLayout[0].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
	polygonLayout[0].InstanceDataStepRate = 0;

	polygonLayout[1].SemanticName = "TEXCOORD";
	polygonLayout[1].SemanticIndex = 0;
	polygonLayout[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	polygonLayout[1].InputSlot = 0;
	polygonLayout[1].AlignedByteOffset = D3D10_APPEND_ALIGNED_ELEMENT;
	polygonLayout[1].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
	polygonLayout[1].InstanceDataStepRate = 0;

	polygonLayout[2].SemanticName = "NORMAL";
	polygonLayout[2].SemanticIndex = 0;
	polygonLayout[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[2].InputSlot = 0;
	polygonLayout[2].AlignedByteOffset = D3D10_APPEND_ALIGNED_ELEMENT;
	polygonLayout[2].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
	polygonLayout[2].InstanceDataStepRate = 0;

	// Get a count of the elements in the layout.
    numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);

	// Get the description of the first pass described in the shader technique.
    m_technique->GetPassByIndex(0)->GetDesc(&passDesc);

	// Create the input layout.
    result = device->CreateInputLayout(polygonLayout, numElements, passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &m_layout);
	if(FAILED(result))
	{
		return false;
	}

	// Get pointers to the three matrices inside the shader so we can update them from this class.
    m_worldMatrixPtr = m_effect->GetVariableByName("worldMatrix")->AsMatrix();
	m_viewMatrixPtr = m_effect->GetVariableByName("viewMatrix")->AsMatrix();
    m_projectionMatrixPtr = m_effect->GetVariableByName("projectionMatrix")->AsMatrix();

	// Get pointer to the texture resource inside the shader.
	m_texturePtr = m_effect->GetVariableByName("grassTexture")->AsShaderResource();
	m_texturePtr1 = m_effect->GetVariableByName("slopeTexture")->AsShaderResource();
	m_texturePtr2 = m_effect->GetVariableByName("rockTexture")->AsShaderResource();

	// Get pointers to the light direction and diffuse color variables inside the shader.
	m_lightDirectionPtr = m_effect->GetVariableByName("lightDirection")->AsVector();
	m_ambientColorPtr = m_effect->GetVariableByName("ambientColor")->AsVector();
	m_diffuseColorPtr = m_effect->GetVariableByName("diffuseColor")->AsVector();

	//Shadowing
	m_lightPositionPtr = m_effect->GetVariableByName("lightPosition")->AsVector();
	m_cameraPositionPtr = m_effect->GetVariableByName("cameraPosition")->AsVector();

	m_lightViewMatrixPtr = m_effect->GetVariableByName("lightViewMatrix")->AsMatrix();
	m_lightProjectionMatrixPtr = m_effect->GetVariableByName("lightProjectionMatrix")->AsMatrix();
	
	m_depthMapTexture = m_effect->GetVariableByName("depthMapTexture")->AsShaderResource();

	return true;
}


void TerrainShaderClass::ShutdownShader()
{
	// Release the light pointers.
	m_lightDirectionPtr = 0;
	m_ambientColorPtr = 0;
	m_diffuseColorPtr = 0;

	// Release the pointer to the texture in the shader file.
	m_texturePtr = 0;
	m_texturePtr1 = 0;
	m_texturePtr2 = 0;

	// Release the pointers to the matrices inside the shader.
	m_worldMatrixPtr = 0;
	m_viewMatrixPtr = 0;
	m_projectionMatrixPtr = 0;

	// Release the pointer to the shader layout.
	if(m_layout)
	{
		m_layout->Release();
		m_layout = 0;
	}

	// Release the pointer to the shader technique.
	m_technique = 0;

	// Release the pointer to the shader.
	if(m_effect)
	{
		m_effect->Release();
		m_effect = 0;
	}

	return;
}

void TerrainShaderClass::SetShaderParameters(D3DXMATRIX worldMatrix, D3DXMATRIX viewMatrix, D3DXMATRIX projectionMatrix, 
										   ID3D10ShaderResourceView* texture, ID3D10ShaderResourceView* texture1, ID3D10ShaderResourceView* texture2, D3DXVECTOR3 lightDirection, D3DXVECTOR4 ambientColor, 
										   D3DXVECTOR4 diffuseColor,
										   //Shadow
											D3DXMATRIX lightViewMatrix, D3DXMATRIX lightProjectionMatrix,ID3D10ShaderResourceView* depthMapTexture,
											D3DXVECTOR3 lightPosition, D3DXVECTOR3 cameraPosition)
{
	// Set the world matrix variable inside the shader.
    m_worldMatrixPtr->SetMatrix((float*)&worldMatrix);

	// Set the view matrix variable inside the shader.
	m_viewMatrixPtr->SetMatrix((float*)&viewMatrix);

	// Set the projection matrix variable inside the shader.
    m_projectionMatrixPtr->SetMatrix((float*)&projectionMatrix);

	// Bind the texture pointers.
	m_texturePtr->SetResource(texture);
	m_texturePtr1->SetResource(texture1);
	m_texturePtr2->SetResource(texture2);

	// Set the direction of the light inside the shader.
	m_lightDirectionPtr->SetFloatVector((float*)&lightDirection);

	// Set the ambient color of the light.
	m_ambientColorPtr->SetFloatVector((float*)&ambientColor);

	// Set the diffuse color of the light inside the shader.
	m_diffuseColorPtr->SetFloatVector((float*)&diffuseColor);

		//Shadowing
	m_lightViewMatrixPtr->SetMatrix((float*)&lightViewMatrix);
	m_lightProjectionMatrixPtr->SetMatrix((float*)&lightProjectionMatrix);
	m_depthMapTexture->SetResource(depthMapTexture);
	m_lightPositionPtr->SetFloatVector((float*)&lightPosition);
	m_cameraPositionPtr->SetFloatVector((float*)&cameraPosition);
	//

	return;
}


void TerrainShaderClass::RenderShader(ID3D10Device* device, int indexCount)
{
    D3D10_TECHNIQUE_DESC techniqueDesc;
	unsigned int i;
	

	// Set the input layout.
	device->IASetInputLayout(m_layout);

	// Get the description structure of the technique from inside the shader so it can be used for rendering.
    m_technique->GetDesc(&techniqueDesc);

    // Go through each pass in the technique (should be just one currently) and render the triangles.
	for(i=0; i<techniqueDesc.Passes; ++i)
    {
        m_technique->GetPassByIndex(i)->Apply(0);
        device->DrawIndexed(indexCount, 0, 0);
    }

	return;
}