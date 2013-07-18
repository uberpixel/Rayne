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
#include "RNString.h"
#include "RNWrappingObject.h"

#define kRNDebugDebugDrawLine2DKey RNCSTR("kRNDebugDebugDrawLine2DKey")
#define kRNDebugDebugDrawLine3DKey RNCSTR("kRNDebugDebugDrawLine3DKey")

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
		static Matrix    __LineTransform;
		
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
			
			renderer->BindVAO(0);
		}
		
		void InstallLine3DHandler()
		{
			if(!__Line3DHandlerState.exchange(true))
			{
				RenderingObject object(RenderingObject::Type::Custom);
				object.material = __Line3DMaterial;
				object.callback = [](Renderer *renderer, const RenderingObject& object) {
					DrawLine<Point3D, Vector3, Line3D>(renderer, __Line3D);
					
					__Line3D.clear();
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
				object.material  = __Line2DMaterial;
				object.transform = &__LineTransform;
				
				object.callback = [](Renderer *renderer, const RenderingObject& object) {
					DrawLine<Point2D, Vector2, Line2D>(renderer, __Line2D);
					
					__Line2D.clear();
					__Line2DHandlerState.store(false);
				};
				
				object.prepare = [](Renderer *renderer, RenderingObject& object) {
					Camera *camera = renderer->ActiveCamera();
					float height = camera->Frame().height - 1.0f;
					
					__LineTransform.MakeTranslate(Vector3(0.0f, height, 0.0f));
				};
				
				Renderer::SharedInstance()->RenderDebugObject(object, Renderer::Mode::ModeUI);
			}
		}
		
		
		template<typename Vector, typename Line, typename Point>
		void __AddLinePoint(const Vector& point, const Color& color, String *key)
		{
			WrappingObject<Line> *line = Thread::CurrentThread()->ObjectForKey<WrappingObject<Line>>(key);
			if(!line)
			{
				line = new WrappingObject<Line>();
				Thread::CurrentThread()->SetObjectForKey(line, key);
				
				line->Release();
			}
			
			Line& data = line->Data();
			
			data.emplace_back(Point(point, color));
			
			if((data.size() % 2) == 0)
			{
				data.emplace_back(Point(point, color));
			}
		}
		
		void AddLinePoint(const Vector3& point, const Color& color)
		{
			__AddLinePoint<Vector3, Line3D, Point3D>(point, color, kRNDebugDebugDrawLine3DKey);
		}
		
		void AddLinePoint(const Vector2& point, const Color& color)
		{
			__AddLinePoint<Vector2, Line2D, Point2D>(Vector2(point.x, -point.y), color, kRNDebugDebugDrawLine2DKey);
		}
		
		void CloseLine()
		{
			WrappingObject<Line3D> *tline3D = Thread::CurrentThread()->ObjectForKey<WrappingObject<Line3D>>(kRNDebugDebugDrawLine3DKey);
			WrappingObject<Line2D> *tline2D = Thread::CurrentThread()->ObjectForKey<WrappingObject<Line2D>>(kRNDebugDebugDrawLine2DKey);
			
			if(tline3D)
			{
				Line3D& line3D = tline3D->Data();
				line3D.erase(line3D.end() - 1);
			}
			
			if(tline2D)
			{
				Line2D& line2D = tline2D->Data();
				line2D.erase(line2D.end() - 1);
			}
		}
		
		void EndLine()
		{
			WrappingObject<Line3D> *tline3D = Thread::CurrentThread()->ObjectForKey<WrappingObject<Line3D>>(kRNDebugDebugDrawLine3DKey);
			WrappingObject<Line2D> *tline2D = Thread::CurrentThread()->ObjectForKey<WrappingObject<Line2D>>(kRNDebugDebugDrawLine2DKey);
			
			if(tline3D)
			{
				Line3D& line3D = tline3D->Data();
				line3D.erase(line3D.end() - 1);
				
				Line3D temp;
				std::swap(temp, line3D);
				
				__Line3D.push_back(std::move(temp));
				
				Thread::CurrentThread()->RemoveObjectForKey(kRNDebugDebugDrawLine3DKey);
				InstallLine3DHandler();
			}
			
			if(tline2D)
			{
				Line2D& line2D = tline2D->Data();
				line2D.erase(line2D.end() - 1);
				
				Line2D temp;
				std::swap(temp, line2D);
				
				__Line2D.push_back(std::move(temp));
				
				Thread::CurrentThread()->RemoveObjectForKey(kRNDebugDebugDrawLine2DKey);
				InstallLine2DHandler();
			}
		}
		
		void DrawBox(const AABB& box, const Color& color)
		{
			Vector3 min = box.Position() + box.minExtend;
			Vector3 max = box.Position() + box.maxExtend;
			
			DrawBox(min, max, color);
		}
		
		void DrawBox(const Vector3& tmin, const Vector3& tmax, const Color& color)
		{
			Vector3 min = Vector3(MIN(tmin.x, tmax.x), MIN(tmin.y, tmax.y), MIN(tmin.z, tmax.z));
			Vector3 max = Vector3(MAX(tmin.x, tmax.x), MAX(tmin.y, tmax.y), MAX(tmin.z, tmax.z));
			
			// Top and bottom
			AddLinePoint(min, color);
			AddLinePoint(Vector3(max.x, min.y, min.z), color);
			AddLinePoint(Vector3(max.x, min.y, max.z), color);
			AddLinePoint(Vector3(min.x, min.y, max.z), color);
			AddLinePoint(min, color);
			CloseLine();
			
			AddLinePoint(Vector3(min.x, max.y, min.z), color);
			AddLinePoint(Vector3(max.x, max.y, min.z), color);
			AddLinePoint(Vector3(max.x, max.y, max.z), color);
			AddLinePoint(Vector3(min.x, max.y, max.z), color);
			AddLinePoint(Vector3(min.x, max.y, min.z), color);
			CloseLine();
			
			// Edges
			AddLinePoint(Vector3(min.x, min.y, min.z), color);
			AddLinePoint(Vector3(min.x, max.y, min.z), color);
			CloseLine();
			
			AddLinePoint(Vector3(max.x, min.y, min.z), color);
			AddLinePoint(Vector3(max.x, max.y, min.z), color);
			CloseLine();
			
			AddLinePoint(Vector3(min.x, min.y, max.z), color);
			AddLinePoint(Vector3(min.x, max.y, max.z), color);
			CloseLine();
			
			AddLinePoint(Vector3(max.x, min.y, max.z), color);
			AddLinePoint(Vector3(max.x, max.y, max.z), color);
			EndLine();
		}
		
		void DrawSphere(const Sphere& sphere, const Color& color, const int tesselation)
		{
			DrawSphere(sphere.Position(), sphere.radius, RN::Color::Yellow(), tesselation);
		}
		
		void DrawSphere(const Vector3 &pos, const float radius, const Color &color, const int tesselation)
		{
			Quaternion rot;
			Vector3 point(0.0f, 0.0f, radius);
			Vector3 temp;
			float offset = 360.0f/tesselation;
			for(float pan = 0.0f; pan <= 180.0f; pan += offset)
			{
				for(float tilt = 0.0f; tilt <= 360.0f; tilt += offset)
				{
					rot.MakeEulerAngle(Vector3(pan, tilt, 0.0f));
					temp = pos+rot.RotateVector(point);
					AddLinePoint(temp, color);
				}
				CloseLine();
			}
			
			for(float tilt = 0.0f; tilt <= 180.0f; tilt += offset)
			{
				for(float pan = 0.0f; pan <= 360.0f; pan += offset)
				{
					rot.MakeEulerAngle(Vector3(pan, tilt, 0.0f));
					temp = pos+rot.RotateVector(point);
					AddLinePoint(temp, color);
				}
				CloseLine();
			}
			
			EndLine();
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
