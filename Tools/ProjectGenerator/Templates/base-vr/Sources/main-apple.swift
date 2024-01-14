//
//  main-apple.swift
//  __TMP_APPLICATION_NAME__
//
//  Copyright __TMP_YEAR__ by __TMP_COMPANY__. All rights reserved.
//

import SwiftUI
import MetalKit
import __TMP_APPLICATION_NAME__Lib

#if os(iOS)
@main
struct Testing: App {
	var body: some Scene {
		let metalView = RayneMetalView()
		WindowGroup {
			metalView
				.edgesIgnoringSafeArea(.all)
		}
	}
}

struct RayneMetalView: UIViewRepresentable {

	let metalView = RayneMetalLayerView()
	func makeUIView(context: Context) -> RayneMetalLayerView {
		metalView.isMultipleTouchEnabled = true
		let metalLayer = metalView.layer as? CAMetalLayer
		
		let rayneMainThread = Thread {
			ios_main(metalLayer)
		}
		rayneMainThread.name = "Rayne Main Thread"
		rayneMainThread.start()
		
		return metalView
	}
	
	func updateUIView(_ uiView: RayneMetalLayerView, context: Context) {}
}

final class RayneMetalLayerView: UIView {
	override class var layerClass: AnyClass { CAMetalLayer.self }
}

#elseif os(visionOS)
import CompositorServices
@main
struct Testing: App {
	var body: some Scene {
		ImmersiveSpace {
			CompositorLayer(configuration: RayneCompositorLayerConfiguration()) { layerRenderer in
				let rayneMainThread = Thread {
					visionos_main(layerRenderer)
				}
				rayneMainThread.name = "Rayne Main Thread"
				rayneMainThread.start()
			}
		}
	}
}

struct RayneCompositorLayerConfiguration: CompositorLayerConfiguration {
	func makeConfiguration(capabilities: LayerRenderer.Capabilities, configuration: inout LayerRenderer.Configuration) {
		let supportsFoveation = capabilities.supportsFoveation
		let supportedLayouts = capabilities.supportedLayouts(options: supportsFoveation ? [.foveationEnabled] : [])

		//Simulator does not support layered! (and only has a single dedicated texture)
		configuration.layout = supportedLayouts.contains(.layered) ? .layered : .dedicated
		configuration.isFoveationEnabled = supportsFoveation

		configuration.colorFormat = .bgra8Unorm_srgb
		configuration.depthFormat = .depth32Float
   }
}
#endif
