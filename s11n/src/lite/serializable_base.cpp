#include "s11nlite.hpp"
#include "serializable_base.hpp"
#include <memory>
namespace s11nlite {
    serializable_base::serializable_base( char const * cn ) :
	m_cname(cn ? cn : std::string("serializable_base")),
	m_ext( cn ? (std::string(".")+cn) : std::string() )
    {
    }

    serializable_base::~serializable_base()
    {
    }

    bool serializable_base::s11n_serialize( serializable_base::node_type & dest ) const
    {
	node_traits::class_name( dest, this->s11n_class() );
	return true;
    }
    bool serializable_base::s11n_deserialize( serializable_base::node_type const & src )
    {
	std::string cname( node_traits::class_name( src ) );
	if( cname != std::string(this->s11n_class()) )
	{
	    throw s11n::s11n_exception("serializable_base[%s]::s11n_deserialize() was asked to desialize data for type [%s].",
				       this->s11n_class(), cname.c_str() );
	}
	return true;
    }

    bool serializable_base::serialize( serializable_base::node_type & dest ) const
    {
	return this->s11n_serialize(dest);
    }
    bool serializable_base::deserialize( serializable_base::node_type const & src )
    {
	return this->s11n_deserialize(src);
    }

    char const * serializable_base::s11n_class() const
    {
	return this->m_cname.c_str();
    }

    char const * serializable_base::s11n_file_extension()
    {
	return m_ext.c_str();
    }
    void serializable_base::s11n_file_extension( std::string const & ext )
    {
	m_ext = ext;
    }

    bool serializable_base::s11n_save( std::string const & dest ) const
    {
	std::string rn( dest );
	if( ! this->m_ext.empty() )
	{
	    if( ! this->filename_matches(dest) )
	    {
		rn = dest + this->m_ext;
	    }
	}
	return s11nlite::save<serializable_base>( *this, rn );
    }

    bool serializable_base::s11n_save( std::ostream & dest ) const
    {
	return s11nlite::save<serializable_base>( *this, dest );
    }

    bool serializable_base::save( std::string const & dest ) const
    {
	return this->s11n_save(dest);
    }

    bool serializable_base::save( std::ostream & dest ) const
    {
	return this->s11n_save(dest);
    }


    bool serializable_base::s11n_load( std::string const & src )
    {
	typedef std::auto_ptr<serializable_base::node_type> NP;
	NP np( s11nlite::load_node( src ) );
	return np.get()
	    ? this->deserialize( *np )
	    : false;
    }
    bool serializable_base::s11n_load( std::istream & src )
    {
	typedef std::auto_ptr<serializable_base::node_type> NP;
	NP np( s11nlite::load_node( src ) );
	return np.get()
	    ? this->deserialize( *np )
	    : false;
    }

    bool serializable_base::load( std::istream & src )
    {
	return this->s11n_load(src);
    }
    bool serializable_base::load( std::string const & src)
    {
	return this->s11n_load(src);
    }

    bool serializable_base::filename_matches( std::string const & src ) const
    {
	if( src.size() >= m_ext.size() )
	{
	    return m_ext == src.substr( src.size() - m_ext.size(), m_ext.size() );
	}
	return false;
    }

    bool serializable_base_s11n::operator()( serializable_base::node_type & dest, serializable_base const & src ) const
    {
	return src.serialize( dest );
    }
    bool serializable_base_s11n::operator()( serializable_base::node_type const & src, serializable_base & dest ) const
    {
	return dest.deserialize( src );
    }
} // namespace
