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

#define kRNDebugDebugDrawLine2DKey "kRNDebugDebugDrawLine2DKey"
#define kRNDebugDebugDrawLine3DKey "kRNDebugDebugDrawLine3DKey"

namespace RN
{
	namespace Debug
	{
		struct Point2D
		{
			Point2D(const Vector2& tpoint, const Color& tcolor) :
				point(tpoint),
				color(tcolor)
			{}
			
			Vector2 point;
			Color color;
		};
		
		struct Point3D
		{
			Point3D(const Vector3& tpoint, const Color& tcolor) :
				point(tpoint),
				color(tcolor)
			{}
			
			Vector3 point;
			Color color;
		};
		
		typedef std::vector<Point2D> Line2D;
		typedef std::vector<Point3D> Line3D;
		
		// ---------------------
		// MARK: -
		// MARK: Lines
		// ---------------------
		
		static Material *__Line3DMaterial;
		static Material *__Line2DMaterial;
		static Shader   *__LineShader;
		
		static std::atomic<bool> __Line3DHandlerState;
		static std::atomic<bool> __Line2DHandlerState;
		
		static std::vector<Line3D> __Line3D;
		static std::vector<Line2D> __Line2D;
		
		template<typename Point, typename Vertex, typename Line>
		void DrawLine(Renderer *renderer, const std::vector<Line>& lines)
		{
			GLuint vbo, vao;
			
			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);
			
			glGenBuffers(1, &vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			
			// Enable the VAO state
			ShaderProgram *program = renderer->ActiveProgram();
			
			glEnableVertexAttribArray(program->attPosition);
			glEnableVertexAttribArray(program->attColor0);
			
			for(const Line& line : lines)
			{
				glBufferData(GL_ARRAY_BUFFER, line.size() * sizeof(Point), line.data(), GL_STREAM_DRAW);
				
				glVertexAttribPointer(program->attPosition, 3, GL_FLOAT, false, sizeof(Point), (void *)0);
				glVertexAttribPointer(program->attColor0, 4, GL_FLOAT, false, sizeof(Point), (void *)sizeof(Vertex));
				
				glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(line.size()));
				glBufferData(GL_ARRAY_BUFFER, line.size() * sizeof(Point), 0, GL_STREAM_DRAW);
			}
			
			glDisableVertexAttribArray(program->attPosition);
			glDisableVertexAttribArray(program->attColor0);
			
			// Clean up
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
			
			glDeleteVertexArrays(1, &vao);
			glDeleteBuffers(1, &vbo);
		}
		
		void InstallLine3DHandler()
		{
			if(!__Line3DHandlerState.exchange(true))
			{
				RenderingObject object(RenderingObject::Type::Custom);
				object.material = __Line3DMaterial;
				object.callback = [](Renderer *renderer, const RenderingObject& object) {
					DrawLine<Point3D, Vector3, Line3D>(renderer, __Line3D);
					__Line3DHandlerState.store(false);
				};
				
				Renderer::SharedInstance()->RenderDebugObject(object, Renderer::Mode::ModeWorld);
			}
		}
		
		void InstallLine2DHandler()
		{
			if(!__Line2DHandlerState.exchange(true))
			{
				RenderingObject object(RenderingObject::Type::Custom);
				object.material = __Line2DMaterial;
				object.callback = [](Renderer *renderer, const RenderingObject& object) {
					DrawLine<Point2D, Vector2, Line2D>(renderer, __Line2D);
					__Line2DHandlerState.store(false);
				};
				
				Renderer::SharedInstance()->RenderDebugObject(object, Renderer::Mode::ModeUI);
			}
		}
		
		
		template<typename Vector, typename Line, typename Point>
		void __AddLinePoint(const Vector& point, const Color& color, const char *key)
		{
			Line *line = Thread::CurrentThread()->ObjectForKey<Line>(key);
			if(!line)
			{
				line = new Line();
				Thread::CurrentThread()->SetObjectForKey(line, key);
			}
			
			line->emplace_back(Point(point, color));
			
			if((line->size() % 2) == 0)
			{
				line->emplace_back(Point(point, color));
			}
		}
		
		void AddLinePoint(const Vector3& point, const Color& color)
		{
			__AddLinePoint<Vector3, Line3D, Point3D>(point, color, kRNDebugDebugDrawLine3DKey);
		}
		
		void AddLinePoint(const Vector2& point, const Color& color)
		{
			__AddLinePoint<Vector2, Line2D, Point2D>(point, color, kRNDebugDebugDrawLine2DKey);
		}
		
		void EndLine()
		{
			Line3D *line3D = Thread::CurrentThread()->ObjectForKey<Line3D>(kRNDebugDebugDrawLine3DKey);
			Line2D *line2D = Thread::CurrentThread()->ObjectForKey<Line2D>(kRNDebugDebugDrawLine2DKey);
			
			if(line3D)
			{
				Line3D temp;
				std::swap(temp, *line3D);
				
				__Line3D.push_back(std::move(temp));
				delete line3D;
				
				Thread::CurrentThread()->SetObjectForKey<Line3D>(nullptr, kRNDebugDebugDrawLine3DKey);
				InstallLine3DHandler();
			}
			
			if(line2D)
			{
				Line2D temp;
				std::swap(temp, *line2D);
				
				__Line2D.push_back(std::move(temp));
				delete line2D;
				
				Thread::CurrentThread()->SetObjectForKey<Line2D>(nullptr, kRNDebugDebugDrawLine2DKey);
				InstallLine2DHandler();
			}
		}
		
		
		// ---------------------
		// MARK: -
		// MARK: Helper
		// ---------------------
		
		void InstallDebugDraw()
		{
			__Line3DHandlerState.store(false);
			__Line2DHandlerState.store(false);
			
			__LineShader = new Shader();
			__LineShader->SetShaderForType("shader/rn_DebugDraw.vsh", Shader::ShaderType::VertexShader);
			__LineShader->SetShaderForType("shader/rn_DebugDraw.fsh", Shader::ShaderType::FragmentShader);
			
			__Line3DMaterial = new Material(__LineShader);
			__Line3DMaterial->Define("RN_DEBUG_3D");
			__Line3DMaterial->blending   = false;
			__Line3DMaterial->depthwrite = false;
			__Line3DMaterial->depthtest  = true;
			__Line3DMaterial->lighting   = false;
			
			__Line2DMaterial = new Material(__LineShader);
			__Line2DMaterial->Define("RN_DEBUG_2D");
			__Line2DMaterial->blending   = false;
			__Line2DMaterial->depthwrite = false;
			__Line2DMaterial->depthtest  = false;
			__Line2DMaterial->lighting   = false;
		}
	}
}
