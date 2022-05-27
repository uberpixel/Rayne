#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>

#include <fstream>
#include <iterator>
#include <vector>

#include "meshoptimizer.h"

int main(int argc, char *argv[])
{
	if(argc < 3) abort();

	char *inputFileName = argv[1];
	char *outputFileName = argv[2];
	std::cout << "copy and optimize from " << inputFileName << " to " << outputFileName << std::endl;

	//Load file into a buffer
	std::ifstream input(inputFileName, std::ios::binary);
    std::vector<unsigned char> inputBuffer(std::istreambuf_iterator<char>(input), {});

/*  ############################################################
	#Structure of sgm files
	############################################################
	#magic number - uint32 - 352658064
	#version - uint8 - 3
	#number of materials - uint8
	#material id - uint8
	#	number of uv sets - uint8
	#		number of textures - uint8
	#			texture type hint - uint8
	#			filename length - uint16
	#			filename - char*filename length
	#	number of colors - uint8
	#		color type hint - uint8
	#		color rgba - float32*4
	#
	#number of meshs - uint8
	#mesh id - uint8
	#	used materials id - uint8
	#	number of vertices - uint32
	#	texcoord count - uint8
	#	color channel count - uint8 usually 0 or 4
	#	has tangents - uint8 0 if not, 1 otherwise
	#	has bones - uint8 0 if not, 1 otherwise
	#	interleaved vertex data - float32
	#		- position, normal, uvN, color, tangents, weights, bone indices
	#
	#	number of indices - uint32
	#	index size - uint8, usually 2 or 4 bytes
	#	indices - index size
	#
	#has animation - uint8 0 if not, 1 otherwise
	#	animfilename length - uint16
	#	animfilename - char*animfilename length
	*/

	std::vector<unsigned char> outputBuffer;
	size_t currentReadPosition = 0;

	//Magic number
	outputBuffer.push_back(inputBuffer[currentReadPosition++]);
	outputBuffer.push_back(inputBuffer[currentReadPosition++]);
	outputBuffer.push_back(inputBuffer[currentReadPosition++]);
	outputBuffer.push_back(inputBuffer[currentReadPosition++]);

	//Version
	outputBuffer.push_back(inputBuffer[currentReadPosition++]);

	//Number of materials
	unsigned char materialCount = inputBuffer[currentReadPosition++];
	outputBuffer.push_back(materialCount);

	//Materials
	for(unsigned char material = 0; material < materialCount; material++)
	{
		//Material ID
		outputBuffer.push_back(inputBuffer[currentReadPosition++]);

		//Number of UV Sets
		unsigned char uvSetCount = inputBuffer[currentReadPosition++];
		outputBuffer.push_back(uvSetCount);

		//UV Sets
		for(unsigned char uvSet = 0; uvSet < uvSetCount; uvSet++)
		{
			//Number of Textures
			unsigned char textureCount = inputBuffer[currentReadPosition++];
			outputBuffer.push_back(textureCount);

			//Textures
			for(unsigned char uvSet = 0; uvSet < uvSetCount; uvSet++)
			{
				//Texture type hint
				outputBuffer.push_back(inputBuffer[currentReadPosition++]);

				//Texture file name length
				unsigned short filenameLength;
				std::memcpy(&filenameLength, &inputBuffer[currentReadPosition], 2);
				outputBuffer.push_back(inputBuffer[currentReadPosition++]);
				outputBuffer.push_back(inputBuffer[currentReadPosition++]);

				//Texture file name
				for(unsigned short character = 0; character < filenameLength; character++)
				{
					outputBuffer.push_back(inputBuffer[currentReadPosition++]);
				}
			}
		}

		//Number of Colors
		unsigned char colorCount = inputBuffer[currentReadPosition++];
		outputBuffer.push_back(colorCount);

		//Colors
		for(unsigned char color = 0; color < colorCount; color++)
		{
			//Color type hint
			outputBuffer.push_back(inputBuffer[currentReadPosition++]);

			//Color R
			outputBuffer.push_back(inputBuffer[currentReadPosition++]);
			outputBuffer.push_back(inputBuffer[currentReadPosition++]);
			outputBuffer.push_back(inputBuffer[currentReadPosition++]);
			outputBuffer.push_back(inputBuffer[currentReadPosition++]);

			//Color G
			outputBuffer.push_back(inputBuffer[currentReadPosition++]);
			outputBuffer.push_back(inputBuffer[currentReadPosition++]);
			outputBuffer.push_back(inputBuffer[currentReadPosition++]);
			outputBuffer.push_back(inputBuffer[currentReadPosition++]);

			//Color B
			outputBuffer.push_back(inputBuffer[currentReadPosition++]);
			outputBuffer.push_back(inputBuffer[currentReadPosition++]);
			outputBuffer.push_back(inputBuffer[currentReadPosition++]);
			outputBuffer.push_back(inputBuffer[currentReadPosition++]);

			//Color A
			outputBuffer.push_back(inputBuffer[currentReadPosition++]);
			outputBuffer.push_back(inputBuffer[currentReadPosition++]);
			outputBuffer.push_back(inputBuffer[currentReadPosition++]);
			outputBuffer.push_back(inputBuffer[currentReadPosition++]);
		}
	}


	//Number of Meshes
	unsigned char meshCount = inputBuffer[currentReadPosition++];
	outputBuffer.push_back(meshCount);

	for(unsigned char mesh = 0; mesh < meshCount; mesh++)
	{
		//Mesh ID
		outputBuffer.push_back(inputBuffer[currentReadPosition++]);

		//Mesh Material ID
		outputBuffer.push_back(inputBuffer[currentReadPosition++]);

		//Number of Vertices
		unsigned int vertexCount;
		std::memcpy(&vertexCount, &inputBuffer[currentReadPosition], 4);
		outputBuffer.push_back(inputBuffer[currentReadPosition++]);
		outputBuffer.push_back(inputBuffer[currentReadPosition++]);
		outputBuffer.push_back(inputBuffer[currentReadPosition++]);
		outputBuffer.push_back(inputBuffer[currentReadPosition++]);

		//Number of texcoords
		unsigned char texcoordCount = inputBuffer[currentReadPosition++];
		outputBuffer.push_back(texcoordCount);

		//Number of color channels
		unsigned char colorChannelCount = inputBuffer[currentReadPosition++];
		outputBuffer.push_back(colorChannelCount);

		//Has tangents
		unsigned char hasTangents = inputBuffer[currentReadPosition++];
		outputBuffer.push_back(hasTangents);

		//Has bones
		unsigned char hasBones = inputBuffer[currentReadPosition++];
		outputBuffer.push_back(hasBones);

		size_t vertexSize = 3 + 3 + 2 * texcoordCount + colorChannelCount + 4 * hasTangents + 8 * hasBones;
		vertexSize *= sizeof(float);

		std::vector<unsigned char> vertexData;
		vertexData.reserve(vertexCount * vertexSize);

		//Interleaved vertex data
		//Copy the vertex data all at once
		vertexData.insert(std::end(vertexData), std::begin(inputBuffer) + currentReadPosition, std::begin(inputBuffer) + currentReadPosition + vertexSize * vertexCount);
		currentReadPosition += vertexSize * vertexCount;

		//Number of Indices
		unsigned int indexCount;
		std::memcpy(&indexCount, &inputBuffer[currentReadPosition], 4);
		unsigned char indexCountArray[4]; //Store the data here to push into the output vector after processing the mesh, to keep everything in order
		indexCountArray[0] = inputBuffer[currentReadPosition++];
		indexCountArray[1] = inputBuffer[currentReadPosition++];
		indexCountArray[2] = inputBuffer[currentReadPosition++];
		indexCountArray[3] = inputBuffer[currentReadPosition++];

		//Index size
		unsigned char indexSize = inputBuffer[currentReadPosition++];

		//Indices need to be expanded to uint32
		std::vector<unsigned int> indexData;
		indexData.reserve(indexCount);
		for(unsigned int index = 0; index < indexCount; index++)
		{
			if(indexSize == 1) //uint8
			{
				indexData.push_back(inputBuffer[currentReadPosition]);
			}
			else if(indexSize == 2) //uint16
			{
				unsigned short indexValue;
				std::memcpy(&indexValue, &inputBuffer[currentReadPosition], 2);
				indexData.push_back(indexValue);
			}
			else if(indexSize == 4) //uint32
			{
				unsigned int indexValue;
				std::memcpy(&indexValue, &inputBuffer[currentReadPosition], 4);
				indexData.push_back(indexValue);
			}
			else
			{
				std::cout << "Error: Unsupported index size: " << static_cast<unsigned int>(indexSize) << std::endl;
				abort();
			}

			currentReadPosition += indexSize;
		}

		std::cout << "Number of vertices: " << vertexCount << ", number of indices: " << indexCount << ", index size: " << static_cast<unsigned int>(indexSize) << std::endl;

		//TODO: Could remap vertices here, eliminating duplicates, but the exporter already takes care of it, so should not be needed
		//std::vector<unsigned int> remapTable(vertexCount);
		//size_t newVertexCount = meshopt_generateVertexRemap(remapTable.data(), indexData.data(), indexCount, vertexData.data(), vertexCount, vertexSize);
		//TODO: This remapping would then also need to generate new vertex / index buffers here to use going forward and change the vertex count before adding it to the output buffer

		meshopt_VertexCacheStatistics cacheStatsBefore = meshopt_analyzeVertexCache(indexData.data(), indexCount, vertexCount, 16, 0, 0);
		meshopt_VertexFetchStatistics fetchStatsBefore = meshopt_analyzeVertexFetch(indexData.data(), indexCount, vertexCount, vertexSize);

		meshopt_optimizeVertexCache(indexData.data(), indexData.data(), indexCount, vertexCount);
		meshopt_optimizeVertexFetch(vertexData.data(), indexData.data(), indexCount, vertexData.data(), vertexCount, vertexSize);

		meshopt_VertexCacheStatistics cacheStatsAfter = meshopt_analyzeVertexCache(indexData.data(), indexCount, vertexCount, 16, 0, 0);
		meshopt_VertexFetchStatistics fetchStatsAfter = meshopt_analyzeVertexFetch(indexData.data(), indexCount, vertexCount, vertexSize);

		std::cout << "(ACMR, ATVR) before: (" << cacheStatsBefore.acmr << ", " << cacheStatsBefore.atvr << "), after: (" << cacheStatsAfter.acmr << ", " << cacheStatsAfter.atvr << ")" << std::endl;
		std::cout << "Overfetch before: " << fetchStatsBefore.overfetch << ", after: " << fetchStatsAfter.overfetch << std::endl;

		outputBuffer.insert(std::end(outputBuffer), std::begin(vertexData), std::end(vertexData));
		outputBuffer.push_back(indexCountArray[0]);
		outputBuffer.push_back(indexCountArray[1]);
		outputBuffer.push_back(indexCountArray[2]);
		outputBuffer.push_back(indexCountArray[3]);
		outputBuffer.push_back(indexSize);

		for(unsigned int index = 0; index < indexCount; index++)
		{
			if(indexSize == 1) //uint8
			{
				unsigned char indexValue = static_cast<unsigned char>(indexData[index]);
				outputBuffer.push_back(indexValue);
			}
			else if(indexSize == 2) //uint16
			{
				unsigned char indexValue[4];
				std::memcpy(indexValue, &indexData[index], 4);
				outputBuffer.push_back(indexValue[2]);
				outputBuffer.push_back(indexValue[3]);
			}
			else if(indexSize == 4) //uint32
			{
				unsigned char indexValue[4];
				std::memcpy(indexValue, &indexData[index], 4);
				outputBuffer.push_back(indexValue[0]);
				outputBuffer.push_back(indexValue[1]);
				outputBuffer.push_back(indexValue[2]);
				outputBuffer.push_back(indexValue[3]);
			}
		}
	}

	//Has animations
	unsigned char hasAnimations = inputBuffer[currentReadPosition++];
	outputBuffer.push_back(hasAnimations);

	if(hasAnimations)
	{
		//Animation file name length
		unsigned short filenameLength;
		std::memcpy(&filenameLength, &inputBuffer[currentReadPosition], 2);
		outputBuffer.push_back(inputBuffer[currentReadPosition++]);
		outputBuffer.push_back(inputBuffer[currentReadPosition++]);

		//Animation file name
		for(unsigned short character = 0; character < filenameLength; character++)
		{
			outputBuffer.push_back(inputBuffer[currentReadPosition++]);
		}
	}

	std::ofstream output(outputFileName, std::ios::binary | std::ios_base::out);
	std::copy(std::begin(outputBuffer), std::end(outputBuffer), std::ostreambuf_iterator<char>(output));

	std::cout << "Input size: " << inputBuffer.size() << ", output size: " << outputBuffer.size() << std::endl;

	return 0;
}
