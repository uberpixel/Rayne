//
//  RNInputDevice.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_INPUTDEVICE_H_
#define __RAYNE_INPUTDEVICE_H_

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"
#include "../Objects/RNArray.h"
#include "../Objects/RNString.h"
#include "RNInputControl.h"

namespace RN
{
	class InputManager;
	class InputDevice : public InputControl
	{
	public:
		friend class InputManager;
		friend class InputControl;

		RN_OPTIONS(Category, uint32,
				   Mouse = (1 << 0),
				   Keyboard = (1 << 1),
				   Pen = (1 << 2),
				   Gamepad  = (1 << 3),
				   Joystick = (1 << 4));

		struct Descriptor
		{
			Descriptor(Category category) :
				_category(category),
				_name(nullptr),
				_vendor(nullptr),
				_productID(nullptr),
				_vendorID(nullptr)
			{}

			void SetName(const String *name)
			{
				SafeRelease(_name);
				_name = SafeRetain(name);
			}
			void SetVendor(const String *vendor)
			{
				SafeRelease(_vendor);
				_vendor = SafeRetain(vendor);
			}
			void SetVendorID(const Number *vendorID)
			{
				SafeRelease(_vendorID);
				_vendorID = SafeRetain(vendorID);
			}
			void SetProductID(const Number *productID)
			{
				SafeRelease(_productID);
				_productID = SafeRetain(productID);
			}

			Category GetCategory() const { return _category; }
			const String *GetName() const { return _name; }
			const String *GetVendor() const { return _vendor; }
			const Number *GetProductID() const { return _productID; }
			const Number *GetVendorID() const { return _vendorID; }

		private:
			Category _category;
			const String *_name;
			const String *_vendor;
			const Number *_vendorID;
			const Number *_productID;
		};

		RNAPI ~InputDevice() override;

		RNAPI void AddControl(InputControl *control) override;

		RNAPI void Register();
		RNAPI void Unregister();
		bool IsRegistered() const { return (_manager != nullptr); }

		RNAPI bool Activate() RN_NOEXCEPT;
		RNAPI bool Deactivate() RN_NOEXCEPT;
		bool IsActive() const { return _active; }

		Category GetCategory() const { return _category; }
		const String *GetVendor() const { return _vendor; }
		const Number *GetVendorID() const { return _vendorID; }
		const Number *GetProductID() const { return _productID; }

		RNAPI bool SupportsCommand(const String *command) const;
		RNAPI bool SupportsCommand(const String *command, MetaClass *meta) const;

		RNAPI Object *ExecuteCommand(const String *command, Object *argument);

		RNAPI Array *GetSupportedCommands() const;
		RNAPI std::vector<MetaClass *> GetSupportArgumentsForCommand(const String *command) const;

	protected:
		RNAPI InputDevice(const Descriptor &descriptor);

		RNAPI void SetSerialNumber(const String *serialNumber);
		RNAPI void BindCommand(const String *command, std::function<Object * (Object *)> &&action, std::vector<MetaClass *> &&arguments);

		RNAPI virtual bool __Activate() = 0;
		RNAPI virtual bool __Deactivate() = 0;

	private:
		void ControlDidStart(InputControl *control);
		void ControlDidEnd(InputControl *control);
		void ControlDidUpdate(InputControl *control, Object *value);

		struct ExecutionPort
		{
			ExecutionPort(String *tcommand) :
				command(tcommand->Retain())
			{}

			ExecutionPort(const ExecutionPort &other) :
				command(other.command->Retain()),
				supportedArguments(other.supportedArguments),
				action(other.action)
			{}

			ExecutionPort &operator =(const ExecutionPort &other)
			{
				command->Autorelease();
				command = other.command->Retain();

				supportedArguments = other.supportedArguments;
				action = other.action;

				return *this;
			}

			~ExecutionPort()
			{
				command->Release();
			}

			String *command;
			std::function<Object * (Object *)> action;
			std::vector<MetaClass *> supportedArguments;
		};

		const ExecutionPort *GetExecutionPortMatching(const String *command, MetaClass *meta) const;

		Category _category;
		String *_vendor;
		String *_serialNumber;

		Number *_vendorID;
		Number *_productID;

		bool _active;
		InputManager *_manager;
		std::vector<ExecutionPort> _executionPorts;

		__RNDeclareMetaInternal(InputDevice)
	};
}

#endif /* __RAYNE_INPUTDEVICE_H_ */
