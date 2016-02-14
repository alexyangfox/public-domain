// Implementation details for basic_s11n_node.hpp

#define basic_s11n_node_DEFAULT_NAME "unnamed"
#define basic_s11n_node_DEFAULT_CLASS_NAME "s11n_node"

namespace s11n {


	template <typename CharT>
	basic_s11n_node<CharT>::~basic_s11n_node()
	{
		this->clear_children();
	}

	template <typename CharT>
	template < typename T >
	void basic_s11n_node<CharT>::set( const std::string & key,
					  const T & val )
	{
		std::basic_ostringstream<char_type> os;
		os << val;
		this->m_map[key] = os.str();
	}


	template < typename CharT >
	template < typename T >
	T basic_s11n_node<CharT>::get( const std::string & key,
				       const T & defaultval ) const
	{
		typename map_type::const_iterator cit = this->m_map.find( key );
		if( this->m_map.end() != cit )
		{
			std::basic_istringstream<char_type> is( (*cit).second );
			T out;
			is >> out;
			return out;
		}
		return defaultval;
	}

	template <typename CharT>
	void basic_s11n_node<CharT>::unset( const std::string & key )
	{
		typename map_type::iterator iter;
		iter = this->m_map.find( key );
		if ( iter == this->m_map.end() ) return;
		this->m_map.erase( iter );
		return;
	}

	template <typename CharT>
	void basic_s11n_node<CharT>::copy( const basic_s11n_node & rhs )
	{
		if ( &rhs == this ) return;
		this->clear();
		this->name( rhs.name() );
		this->class_name( rhs.class_name() );
		std::copy( rhs.properties().begin(), rhs.properties().end(),
			   std::insert_iterator<map_type>( this->m_map, this->m_map.begin() )
			   );
		std::for_each( rhs.children().begin(),
			       rhs.children().end(),
			       Detail::child_pointer_deep_copier<child_list_type>( this->children() )
			       );
	}


	template <typename CharT>
	void basic_s11n_node<CharT>::clear_properties()
	{
		if ( m_map.empty() ) return;
		m_map.erase( m_map.begin(), m_map.end() );
	}

	template <typename CharT>
	void basic_s11n_node<CharT>::clear_children()
	{
		std::for_each( this->children().begin(),
			       this->children().end(),
			       object_deleter() );
		this->children().clear();
	}

	template <typename CharT>
	const typename basic_s11n_node<CharT>::map_type &
	basic_s11n_node<CharT>::properties() const
	{
		return this->m_map;
	}

	template <typename CharT>
	typename basic_s11n_node<CharT>::map_type &
	basic_s11n_node<CharT>::properties()
	{
		return this->m_map;
	}

	template <typename CharT>
	bool basic_s11n_node<CharT>::is_set( const std::string & key ) const
	{
		return this->m_map.end() != this->m_map.find( key );
	}

	template <typename CharT>
	bool basic_s11n_node<CharT>::empty() const
	{
		return this->children().empty()
			&& this->properties().empty();
	}

	template <typename CharT>
	std::string basic_s11n_node<CharT>::name() const
	{
		return this->m_name;
	}

	template <typename CharT>
	void basic_s11n_node<CharT>::name( const std::string & n )
	{
		this->m_name = n;
	}

	template <typename CharT>
	std::string basic_s11n_node<CharT>::class_name() const
	{
		return this->m_implclass;
	}

	template <typename CharT>
	void basic_s11n_node<CharT>::class_name( const std::string & n )
	{
		this->m_implclass = n;
	}

	template <typename CharT>
	void basic_s11n_node<CharT>::clear()
	{
		this->clear_children();
		this->clear_properties();
	}

	template <typename CharT>
	const typename basic_s11n_node<CharT>::child_list_type &
	basic_s11n_node<CharT>::children() const
	{
		return this->m_children;
	}

	template <typename CharT>
	typename basic_s11n_node<CharT>::child_list_type &
	basic_s11n_node<CharT>::children()
	{
		return this->m_children;
	}

	template <typename CharT>
	basic_s11n_node<CharT>::basic_s11n_node( const basic_s11n_node<CharT> & rhs )
	{
		if( &rhs == this ) return;
		this->copy( rhs );
	}

	template <typename CharT>
	basic_s11n_node<CharT> &
	basic_s11n_node<CharT>::operator=( const basic_s11n_node<CharT> & rhs )
	{
		if( &rhs == this ) return *this;
		this->copy( rhs );
		return *this;
	}


	template <typename CharT>
	void basic_s11n_node<CharT>::swap( basic_s11n_node<CharT> & rhs )
	{
		this->children().swap( rhs.children() );
		this->properties().swap( rhs.properties() );
		this->m_name.swap( rhs.m_name );
		this->m_implclass.swap( rhs.m_implclass );
	}

	template <typename CharT>
	basic_s11n_node<CharT>::basic_s11n_node( const std::string & name,
						 const std::string implclass )
		: m_name(name), m_implclass(implclass)
	{
	}

	template <typename CharT>
	basic_s11n_node<CharT>::basic_s11n_node( const std::string & name )
		: m_name(name), m_implclass(basic_s11n_node_DEFAULT_CLASS_NAME)
	{
	}

	template <typename CharT>
	basic_s11n_node<CharT>::basic_s11n_node()
		: m_name(basic_s11n_node_DEFAULT_NAME),
		  m_implclass(basic_s11n_node_DEFAULT_CLASS_NAME)
	{
	}

} // namespace
#undef basic_s11n_node_DEFAULT_CLASS_NAME
#undef basic_s11n_node_DEFAULT_NAME
