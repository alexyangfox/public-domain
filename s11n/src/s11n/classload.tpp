#ifndef s11n_net_s11n_CLASSLOAD_TPP_INCLUDED
#define s11n_net_s11n_CLASSLOAD_TPP_INCLUDED
// implementations for non-inlined code declared in classload.hpp

#include <s11n.net/s11n/exception.hpp>

#include <s11n.net/s11n/s11n_config.hpp>
#if s11n_CONFIG_ENABLE_PLUGINS
#    include <s11n.net/s11n/plugin/plugin.hpp>
#endif
#include <s11n.net/s11n/type_traits.hpp> 

template <typename InterfaceBase>
InterfaceBase *
::s11n::cl::object_factory<InterfaceBase>::operator()( const std::string & key ) const
{
	typedef ::s11n::fac::factory_mgr<base_type> FacMgr;
	FacMgr & fm = FacMgr::instance();
#if s11n_CONFIG_ENABLE_PLUGINS
	// Attempt a DLL lookup if the class isn't already registered.
	if( ! fm.provides( key ) )
	{
		using namespace ::s11n::debug;
		S11N_TRACE(TRACE_FACTORY_PLUGINS) << "Factory does not provide '"<<key<<"'. Trying to find plugin...\n";
		std::string where = ::s11n::plugin::open( key );
		if( where.empty()  )
		{
			S11N_TRACE(TRACE_FACTORY_PLUGINS) << "Plugin load failed for '"<<key<<"': " << ::s11n::plugin::dll_error() << '\n';
			return 0;
		}
		S11N_TRACE(TRACE_FACTORY_PLUGINS) << "Opened DLL ["<<where<<"]. Factory provides '"<<key<<"'? == " << fm.provides(key) << '\n';
	}
#endif // s11n_CONFIG_ENABLE_PLUGINS
	return fm.create( key );
}


template <typename InterfaceBase>
InterfaceBase *
::s11n::cl::classload( const std::string key )
{
	using namespace ::s11n::debug;
	S11N_TRACE(TRACE_FACTORY) << "classload<Base>("<<key<<")\n";
	typedef typename ::s11n::type_traits<InterfaceBase>::type IT;
	return object_factory<IT>()( key );
}

template <typename InterfaceBase>
void
::s11n::cl::classloader_register( const std::string & classname, InterfaceBase * (*factory_func)() )
{
	using namespace ::s11n::debug;
	S11N_TRACE(TRACE_FACTORY_REG) << "classloader_register<Base,Sub>("<<classname<<", (FACTORY*))\n";
	::s11n::fac::factory<InterfaceBase>().register_factory( classname, factory_func );
}



#endif // s11n_net_s11n_CLASSLOAD_TPP_INCLUDED
