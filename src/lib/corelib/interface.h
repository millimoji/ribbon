#pragma once
#ifndef _RIBBON_INTERFACE_H_
#define _RIBBON_INTERFACE_H_
namespace Ribbon {

struct IObject
{
	// for the dynamic_cast, we need one virtual mehtod at least
	virtual std::shared_ptr<IObject> GetIObjectSharedPtr() = 0;
};
#define IOBJECT_COMMON_METHODS 	std::shared_ptr<IObject> GetIObjectSharedPtr() override { return std::static_pointer_cast<IObject>(shared_from_this()); }

template <typename IF>
std::shared_ptr<IF> GetSharedPtr(IF* ptr) { return std::dynamic_pointer_cast<IF>(dynamic_cast<IObject*>(ptr)->GetIObjectSharedPtr()); }

struct IProgramMain
{
	virtual int ProgramMain(int, char16_t const* const*) = 0;
};

#define FACTORYEXTERN(CLSNAME) extern std::shared_ptr<I##CLSNAME> (*FactoryNewObject_##CLSNAME)()
#define FACTORYDEFINE(CLSNAME) std::shared_ptr<I##CLSNAME> (*FactoryNewObject_##CLSNAME)() = []() -> std::shared_ptr<I##CLSNAME> { return std::make_shared<CLSNAME>(); }
#define FACTORYEXTERN2(IFNAME,CLSNAME) extern std::shared_ptr<IFNAME> (*FactoryNewObject_##CLSNAME)()
#define FACTORYDEFINE2(IFNAME,CLSNAME) std::shared_ptr<IFNAME> (*FactoryNewObject_##CLSNAME)() = []() -> std::shared_ptr<IFNAME> { return std::make_shared<CLSNAME>(); }

#define FACTORYCREATE(CLSNAME) FactoryNewObject_##CLSNAME()
#define FACTORYCREATENS(NAMESPC,CLSNAME) NAMESPC::FactoryNewObject_##CLSNAME()

#define FACTORYFUNCTION(CLSNAME) FactoryNewObject_##CLSNAME

} // Ribbon
#endif //_RIBBON_INTERFACE_H_
