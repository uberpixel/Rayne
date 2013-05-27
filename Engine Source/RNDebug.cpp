//
//  RNDebug.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNDebug.h"
#include "RNMaterial.h"
#include "RNRenderer.h"
#include "RNThread.h"

#define kRNDebugDebugDrawLine3DKey "kRNDebugDebugDrawLine3DKey"

namespace RN
{
	namespace Debug
	{
		struct Point3D
		{
			Point3D(const Vector3& tpoint, const Color& tcolor) :
				point(tpoint),
				color(tcolor)
			{}
			
			Vector3 point;
			Color color;
		};
		
		typedef std::vector<Point3D> Line3D;
		
		// ---------------------
		// MARK: -
		// MARK: Line3D
		// ---------------------
		
		static Material *__Line3DMaterial;
		static Shader *__Line3DShader;
		static std::atomic<bool> __Line3DHandlerState;
		static std::vector<Line3D> __Line3D;
		
		void InstallLine3DHandler()
		{
			if(!__Line3DHandlerState.exchange(true))
			{
				RenderingObject object(RenderingObject::Type::Custom);
				object.material = __Line3DMaterial;
				object.callback = [](Renderer *renderer, const RenderingObject& object) {
					
					GLuint vbo, vao;
					
					glGenVertexArrays(1, &vao);
					glBindVertexArray(vao);
					
					glGenBuffers(1, &vbo);
					glBindBuffer(GL_ARRAY_BUFFER, vbo);
					
					// Enable the VAO state
					ShaderProgram *program = renderer->ActiveProgram();
					
					glEnableVertexAttribArray(program->attPosition);
					glEnableVertexAttribArray(program->attColor0);

					for(const Line3D& line : __Line3D)
					{
						glBufferData(GL_ARRAY_BUFFER, line.size() * sizeof(Point3D), line.data(), GL_STREAM_DRAW);
						
						glVertexAttribPointer(program->attPosition, 3, GL_FLOAT, false, sizeof(Point3D), (void *)0);
						glVertexAttribPointer(program->attColor0, 4, GL_FLOAT, false, sizeof(Point3D), (void *)sizeof(Vector3));
						
						glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(line.size()));
						glBufferData(GL_ARRAY_BUFFER, line.size() * sizeof(Point3D), 0, GL_STREAM_DRAW);
					}
					
					glDisableVertexAttribArray(program->attPosition);
					glDisableVertexAttribArray(program->attColor0);
					
					// Clean up
					glBindBuffer(GL_ARRAY_BUFFER, 0);
					glBindVertexArray(0);
					
					glDeleteVertexArrays(1, &vao);
					glDeleteBuffers(1, &vbo);
					
					__Line3DHandlerState.store(false);
				};
				
				Renderer::SharedInstance()->RenderDebugObject(object, Renderer::Mode::ModeWorld);
			}
		}
		
		void AddLinePoint(const Vector3& point, const Color& color)
		{
			Line3D *line = Thread::CurrentThread()->ObjectForKey<Line3D>(kRNDebugDebugDrawLine3DKey);
			if(!line)
			{
				line = new Line3D();
				Thread::CurrentThread()->SetObjectForKey(line, kRNDebugDebugDrawLine3DKey);
			}

			line->emplace_back(Point3D(point, color));
			
			if((line->size() % 2) == 0)
			{
				line->emplace_back(Point3D(point, color));
			}
		}
		
		void EndLine()
		{
			Line3D *line = Thread::CurrentThread()->ObjectForKey<Line3D>(kRNDebugDebugDrawLine3DKey);
			if(line)
			{
				Line3D temp;
				std::swap(temp, *line);
				
				__Line3D.push_back(std::move(temp));
				delete line;
				
				Thread::CurrentThread()->SetObjectForKey<Line3D>(nullptr, kRNDebugDebugDrawLine3DKey);
				InstallLine3DHandler();
			}
		}
		
		
		// ---------------------
		// MARK: -
		// MARK: Helper
		// ---------------------
		
		void InstallDebugDraw()
		{
			__Line3DHandlerState.store(false);
			
			__Line3DShader = new Shader();
			__Line3DShader->SetShaderForType("shader/rn_DebugDraw.vsh", Shader::ShaderType::VertexShader);
			__Line3DShader->SetShaderForType("shader/rn_DebugDraw.fsh", Shader::ShaderType::FragmentShader);
			
			__Line3DMaterial = new Material(__Line3DShader);
			__Line3DMaterial->blending = false;
			__Line3DMaterial->depthwrite = false;
			__Line3DMaterial->depthtest = true;
			__Line3DMaterial->lighting = false;
		}
	}
}
