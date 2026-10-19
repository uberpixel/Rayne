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
		WindowGroup {
			RayneMetalView()
				.edgesIgnoringSafeArea(.all)
		}
	}
}

struct RayneMetalView: UIViewRepresentable {

	func makeUIView(context: Context) -> RayneMetalLayerView {
		RayneMetalLayerView()
	}
	
	func updateUIView(_ uiView: RayneMetalLayerView, context: Context) {}
}

final class RayneMetalLayerView: UIView {
	override class var layerClass: AnyClass { CAMetalLayer.self }
	private var hasStarted = false

	override init(frame: CGRect) {
		super.init(frame: frame)
		isMultipleTouchEnabled = true
		NotificationCenter.default.addObserver(self, selector: #selector(inactive), name: UIApplication.willResignActiveNotification, object: nil)
		NotificationCenter.default.addObserver(self, selector: #selector(active), name: UIApplication.didBecomeActiveNotification, object: nil)
	}

	required init?(coder: NSCoder) { fatalError("init(coder:) has not been implemented") }
	deinit { NotificationCenter.default.removeObserver(self) }

	@objc private func inactive() { ios_application_active(0) }
	@objc private func active() { ios_application_active(1) }

	override func layoutSubviews() {
		super.layoutSubviews()
		guard let window, bounds.width > 0, bounds.height > 0 else { return }
		let metalLayer = layer as! CAMetalLayer
		let scale = window.screen.nativeScale
		contentScaleFactor = scale
		metalLayer.drawableSize = CGSize(width: (bounds.width * scale).rounded(), height: (bounds.height * scale).rounded())

		guard !hasStarted else { return }
		hasStarted = true
		ios_application_active(UIApplication.shared.applicationState == .active ? 1 : 0)
		let rayneMainThread = Thread {
			ios_main(metalLayer)
		}
		rayneMainThread.name = "Rayne Main Thread"
		rayneMainThread.start()
	}
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
