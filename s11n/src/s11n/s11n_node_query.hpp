#ifndef s11n_S11N_NODE_QUERY_HPP_INCLUDED
#define s11n_S11N_NODE_QUERY_HPP_INCLUDED

////////////////////////////////////////////////////////////////////////
// s11n_node_query.hpp
// Experimental. Don't use.
//
// License: Public Domain
// Author: stephan@s11n.net
////////////////////////////////////////////////////////////////////////
#include <string>

#include <vector>
#include <set>
#include <algorithm>
#include <iterator>
#include <s11n.net/s11n/export.hpp>
#include <s11n.net/s11n/exception.hpp>
#include <s11n.net/s11n/s11n_node.hpp>
#include <s11n.net/s11n/traits.hpp>

namespace s11n {

    /**
       EXPERIMENTAL - do not use!

       This class is for fetching s11n nodes matching certain
       criteria. It is somewhat like using an SQL query builder, but
       is much more limited in what it can do.

       Notable limitations:

       - Can only work on const nodes. It would seem to be impossible
       to consolidate const- and non-const nodes in one API here,
       partly because of the "inherited" constness of child nodes.

       - It does a lot of list copying, and it is conceivable that it
       may use quite a lot of memory. We could use reference counting
       of the internal result sets to reduce this, and may do so if
       this class gets any appreciable use.


       Added in versions 1.3.2/1.2.10.
    */
    template <typename NodeType>
    class node_query
    {
    public:
        /** The templatized S11nNode type. */
        typedef NodeType node_t; 
        /**
           s11n node traits type.
        */
        typedef node_traits<NodeType> traits;

        /**
           We cannot re-use traits::child_list_type here b/c we can
           only work with const NodeType objects from here.
           Additionally, some list operations require sorted lists and
           the associated standard routines require random-access
           iterators.
        */
        typedef std::vector<NodeType const *> node_list;

        /** Internal convenience typedef. */
        typedef std::less<NodeType const *> compare_func;

        /**
           Result set iterator type.
        */
        typedef typename node_list::const_iterator iterator;

        /**
           Creates a new query object which uses src for its
           searching. If src is null then it acts like the no-arg
           ctor.
        */
        explicit node_query( node_t const * src )
            : m_li()
        {
            if( src ) m_li.push_back(src);
        }

        /**
           Creates an empty query object, useful only as the target
           of assignment.
        */
        node_query()
            : m_li()
        {
        }

        /**
           The current result list.
        */
        node_list const & results() const { return m_li; }

        /**
           The current result list. This non-const form
           can be used by non-member algorithms to
           manipulate a result set.
        */
        node_list & results() { return m_li; }

        /**
           Modifies this result set in place to contain all results()
           items for which clause(item) returns true.
        */
        template <typename Ftor>
        node_query & where( Ftor clause )
        {
            node_query res;
            iterator it( m_li.begin() );
            typename node_list::const_iterator et( m_li.end() );
            for( ; et != it; ++it )
            {
                if( clause( *it ) )
                {
                    res.results().push_back(*it);
                }
            }
            m_li.swap( res.m_li );
            return *this;
        }

        template <typename Ftor>
        node_query where( Ftor clause ) const
        {
            node_query res;
            iterator it( m_li.begin() );
            typename node_list::const_iterator et( m_li.end() );
            for( ; et != it; ++it )
            {
                if( clause( *it ) )
                {
                    res.results().push_back(*it);
                }
            }
            return res;
        }

        /**
           Modifies this result set in place to contain all child
           nodes of all items in in the current result set.
        */
        node_query & children()
        {
            node_query res;
            iterator it( m_li.begin() );
            iterator et( m_li.end() );
            node_list & rli( res.results() );
            typedef typename traits::child_list_type NCL;
            for( ; et != it; ++it )
            {
                node_t const * n = *it; 
                NCL const & childs( traits::children( *n ) );
                typename NCL::const_iterator cit( childs.begin() );
                typename NCL::const_iterator cet( childs.end() );
                for( ; cit != cet; ++cit )
                {
                    rli.push_back(*cit);
                }
            }
            m_li.swap( res.m_li );
            return *this;
        }

        /**
           Creates returns a copy of this object. This may be useful
           in certain call-chaining contexts where we don't want to
           edit a given query object in-place.
        */
        node_query copy() const
        {
            return *this;
        }

        /**
           Assigns this object's results to be those from rhs. This
           may be useful in certain complex call-chaining contexts.
        */
        node_query & assign( node_query const & rhs )
        {
            if( this != &rhs ) *this = rhs;
            return *this;
        }

        /**
           Appends rhs.results() to the end of this result set.
        */
        node_query & append( node_query const & rhs )
        {
            std::copy( rhs.m_li.begin(), rhs.m_li.end(),
                       std::back_inserter( this->m_li )
                       );
            return *this;
        }

        /**
           Sorts results() in-place using the given comparison
           operator, which must follow the conventions required by
           std::sort().
        */
        template <typename Compare>
        node_query & sort( Compare cmp )
        {
            std::sort( this->m_li.begin(), this->m_li.end(), cmp );
            return *this;
        }

        /**
           Sorts results() in-place using the default comparison
           algorithm (which simply compares nodes by their pointer
           values).
        */
        node_query & sort()
        {
             return this->sort( compare_func() );
        }

        /**
           Sorts this list in-place and removes any duplicate
           entries. See sort() for the requirements of the Compare
           functor.
        */
        template <typename Compare>
        node_query & unique( Compare cmp )
        {
            this->sort( cmp );
            typedef typename node_list::iterator IT;
            IT it = std::unique( this->m_li.begin(), this->m_li.end() );
            if( m_li.end() != it ) m_li.erase( it, m_li.end() );
            return *this;
        }

        /**
           Equivalent to unique(compre_func()).
        */
        node_query & unique()
        {
            return this->unique( compare_func() );
        }


        /**
           Modifies this result set to include only items which are
           both in this set and in rhs, using cmp to do the comparison
           (which must conform to the requirements of std::sort()
           comparison functions). This set gets sorted as a
           side-effect.
        */
        template <typename CompFunc>
        node_query & intersect( node_query const & rhs, CompFunc cmp )
        {
            if( &rhs == this ) return *this;
            node_list l1( this->m_li );
            this->m_li.clear();
            std::sort( l1.begin(), l1.end(), cmp );
            node_list l2( rhs.m_li );
            std::sort( l2.begin(), l2.end(), cmp );
            std::set_intersection( l1.begin(), l1.end(),
                                   l2.begin(), l2.end(),
                                   std::back_inserter( this->m_li ),
                                   cmp );
            return *this;
        }

        /**
           Equivalent to intersect( rhs, compare_func() ).
        */
        node_query & intersect( node_query const & rhs )
        {
            return this->intersect( rhs, compare_func() );
        }

        /**
           Modifies this result set to include any items which are
           either in this set or rhs (a union). It uses cmp to do the
           comparison (which must conform to the requirements of
           std::sort() comparison functions). This set gets sorted as
           a side-effect.

           The result set may have duplicate entries.

           It is called onion() instead of union() because union()
           is a reserved word in C++.
        */
        template <typename CompFunc>
        node_query & onion( node_query const & rhs, CompFunc cmp )
        {
            if( &rhs == this ) return *this;
            node_list l1( this->m_li );
            this->m_li.clear();
            std::sort( l1.begin(), l1.end(), cmp );
            node_list l2( rhs.m_li );
            std::sort( l2.begin(), l2.end(), cmp );
            std::set_union( l1.begin(), l1.end(),
                            l2.begin(), l2.end(),
                            std::back_inserter( this->m_li ),
                            cmp );
            return *this;
        }

        /**
           Equivalent to union(rhs,compare_func()).
        */
        node_query & onion( node_query const & rhs )
        {
            return this->onion( rhs, compare_func() );
        }

    private:
        /** Result set. */
        node_list m_li;
    };

    /**
       A Concept class which exists only to document the query
       interface required by node_query.
    */
    struct nq_concept
    {
        /**
           Must determine whether n conforms to specific criteria
           and return true on success, else false.
        */
        template <typename NodeType>
        bool operator()( NodeType const * n ) const;
    };

    /**
       An nq_concept implementation for querying nodes
       which have a specific node name.
    */
    struct nq_name_is
    {
        std::string name;
        explicit nq_name_is( std::string const & n ) :name(n)
        {}
        template <typename NodeType>
        bool operator()( NodeType const * n ) const
        {
            typedef node_traits<NodeType> NTR;
            return n
                ? (this->name == NTR::name(*n))
                : false;
        }
    };

    /**
       An nq_concept implementation for querying nodes
       which have a specific property.
    */
    struct nq_has_prop
    {
        std::string key;
        explicit nq_has_prop( std::string const & n ) : key(n)
        {}
        template <typename NodeType>
        bool operator()( NodeType const * n ) const
        {
            typedef node_traits<NodeType> NTR;
            return n
                ? (NTR::is_set( *n, this->key ))
                : false;
        }
    };

    /**
       An nq_concept implementation for querying nodes
       which have a specific s11n class name.
    */
    struct nq_class_name_is
    {
        std::string name;
        explicit nq_class_name_is( std::string const & n ) :name(n)
        {}
#if 0 // awkward to use
        template <typename SerT>
        nq_class_name_is( SerT const * ) : name(s11n_traits<SerT>::class_name(0))
        {
        }
#endif
        template <typename NodeType>
        bool operator()( NodeType const * n ) const
        {
            typedef node_traits<NodeType> NTR;
            return n
                ? (this->name == NTR::class_name(*n))
                : false;
        }

        /**
           Convenience routine. It is 
        */
        template <typename SerT>
        static nq_class_name_is class_name()
        {
            return nq_class_name_is( s11n_traits<SerT>::class_name(0) );
        }
    };


    /**
       Negates another node query functor. Ftor must conform to
    */
    template <typename Ftor>
    struct nq_negate_ftor
    {
        Ftor func;
        nq_negate_ftor( Ftor f ) : func(f)
        {}
        template <typename NodeType>
        bool operator()( NodeType const * n ) const
        {
            typedef node_traits<NodeType> NTR;
            //if( ! n ) throw s11n_exception( S11N_SOURCEINFO, "ng_negate_ftor cannot sensibly react to null nodes!");
            return !func(n);
        }
    };

    /**
       Convenience routine to create an ng_negate_ftor object.
    */
    template <typename Ftor>
    nq_negate_ftor<Ftor> nq_not( Ftor f )
    {
        typedef nq_negate_ftor<Ftor> x;
        return x(f);
    }

    /**
       A node_query search functor which performs a logical
       OR or AND of two nc_concept-compatible functors.
    */
    template <typename Ftor1,typename Ftor2,bool IsAnd>
    struct nq_andor_ftor
    {
        Ftor1 func1;
        Ftor2 func2;
        nq_andor_ftor( Ftor1 f, Ftor2 f2 ) : func1(f),func2(f2)
        {}
        /**
           If IsAnd then returns (func1(n) && func2(n))
           else it returns (func1(n) || func2(n)).
        */
        template <typename NodeType>
        bool operator()( NodeType const * n ) const
        {
            return IsAnd
                ? (func1(n) && func2(n))
                : (func1(n) || func2(n));
        }
    };

    /**
       Convenience function to return a functor for performing
       OR-style searches on node_query objects.
    */
    template <typename Ftor1, typename Ftor2>
    nq_andor_ftor<Ftor1,Ftor2,false> nq_or( Ftor1 f, Ftor2 f2 )
    {
        typedef nq_andor_ftor<Ftor1,Ftor2,false> x;
        return x(f,f2);
    }


    /**
       Convenience function to return a functor for performing
       AND-style searches on node_query objects.
    */
    template <typename Ftor1, typename Ftor2>
    nq_andor_ftor<Ftor1,Ftor2,true> nq_and( Ftor1 f, Ftor2 f2 )
    {
        typedef nq_andor_ftor<Ftor1,Ftor2,true> x;
        return x(f,f2);
    }

} // namespace s11n

#endif // s11n_S11N_NODE_QUERY_HPP_INCLUDED
