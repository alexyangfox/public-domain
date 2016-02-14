#ifndef s11n_s11nlite_SERIALIZABLE_BASE_HPP_INCLUDED
#define s11n_s11nlite_SERIALIZABLE_BASE_HPP_INCLUDED
#include <string>
#include <iostream>
namespace s11nlite {
    /**
       A convenience base type for polymorphic serializable types.
       It provides easy access to load/save support for polymorphic
       Serializables which subclass it. It has no public constructor
       and can only be used via subclassing.

       Subclasses must reimplement the protected virtual functions
       s11n_serialize() and s11n_deserialize(). They may optionally
       override the other protected members, but do not need to.
    */
    class serializable_base
    {
    public:
	/** The s11n-compliant serialization node type. */
	typedef s11nlite::node_type node_type;
	/** The s11n-compliant serialization node traits type. */
	typedef ::s11nlite::node_traits node_traits;
	/** Frees internal resources. */
	virtual ~serializable_base();
	/** See the protected member s11n_serialize(). */
	bool serialize( node_type & dest ) const;
	/**
	   See the protected member s11n_deserialize().
	*/
	bool deserialize( node_type const & src );
	/**
	   Returns the class name of this type as used by
	   the de/serialization process. See the protected
	   setter for more information.
	*/
	char const * s11n_class() const;

	/**
	   See the protected member s11n_load().
	*/
	bool load( std::string const & );
	/**
	   Convenience overload.
	*/
	bool load( std::istream & );
	/**
	   See the protected member s11n_save().
	*/
	bool save( std::string const & ) const;
	/**
	   Convenience overload.
	*/
	bool save( std::ostream & ) const;
	/**
	   Returns the file extension associated with this type.
	   If you need it permanently, copy it, because
	   any call to the setter overload of this func will
	   invalidate it.
	*/
	char const * s11n_file_extension();
	
	/**
	   Returns true if the given file name has a trailing suffix
	   equal to s11n_file_extension(). The function may be
	   reimplemented to check against varying extensions, to
	   return true for other filename types (e.g. URLs) handled by
	   subclasses, etc.

	   The intention is that client applications can ask an object
	   if it knows how to handle a specific file before attempting
	   to deserialize it. It's just a half-ass check, as a file
	   extension doesn't have to map to a specific file type, but
	   it's useful in some cases.

	   Note that it does a case-sensitive check.
	*/
	virtual bool filename_matches( std::string const & ) const;

    protected:
	/** Serializes this object to dest. Subclasses must reimplement
	    this to serialize any data they want. They must call
	    this implementation before starting, as this sets
	    the proper polymorphic class name in dest.
	*/
	virtual bool s11n_serialize( node_type & dest ) const;
	/**
	   Deserializes src to this object.

	   If src does not contain data for the same polymorphic type
	   as this object (as determined by comparing s11n_class() to
	   node_traits::class_name(src)), deserialization will fail and
	   an s11n_exception is thrown.

	   Subclasses should call this implementation first, to allow
	   it to do its error checking. If this routine fails,
	   the subclass should pass on the error.
	*/
	virtual bool s11n_deserialize( node_type const & src );

	/**
	   Tries to deserialize the contents of the given file into
	   this object using the deserialize() member.

	   Returns true on success, false on error.
	*/
	virtual bool s11n_load( std::string const & );
	/** See s11n_load(std::string). This functions identically but
	    accepts a stream instead of a string. */
	virtual bool s11n_load( std::istream & );
	/**
	   Serializes this object's state to the given file. If a
	   s11n_file_extension() has been set, it is automatically
	   appended to the name if the name doesn't have that
	   extension already. It is serialized by calling the
	   serialize() member function.

	   Returns true on success, false on error.
	*/
	virtual bool s11n_save( std::string const & ) const;

	/** See s11n_save(std::string). This functions identically but
	    accepts a stream instead of a string. */
	virtual bool s11n_save( std::ostream & ) const;

	/** Sets s11n class name (to be used for de/serialization) and
	    sets s11n_file_extension() to cn prefixed by a
	    '.'. e.g. if cn=="MyType" then s11n_file_extension(".MyType")
	    is set.

	    Note that className gets copied by this object, so it
	    need not be a static string.

	    Subclasses must set className to ensure that polymorphic
	    deserialization works properly. The name must stay
	    constant for the life of the serialized data. Changing
	    this value for a given subclass may break loading of older
	    serialized data.	    
	*/
	explicit serializable_base(char const * className );

	/**
	   Sets the file extension for this type. When
	   save() is called, that file extension is
	   appended to the filename automatically
	   if needed. The extension may be 0.
		
	   Note that the "dot" part of the extension must be
	   explicit. e.g. use s11n_file_extension(".foo") instead of
	   s11n_file_extension("foo").

	   This routine makes a copy of the extension string.
	*/
	void s11n_file_extension( std::string const & );

    private:
	std::string m_cname;
	std::string m_ext;
	
    };
    struct serializable_base_s11n
    {
	/** Returns src.serialize(dest). */
	bool operator()( node_type & dest, serializable_base const & src ) const;
	/** Returns dest.deserialize(src). */
	bool operator()( node_type const & src, serializable_base & dest ) const;
    };
} // namespace
// Register serializable_base with s11n:
#define S11N_TYPE s11nlite::serializable_base
#define S11N_TYPE_NAME "serializable_base"
#define S11N_SERIALIZE_FUNCTOR s11nlite::serializable_base_s11n
#define S11N_ABSTRACT_BASE
#include <s11n.net/s11n/reg_s11n_traits.hpp>
#endif // s11n_s11nlite_SERIALIZABLE_BASE_HPP_INCLUDED
