/*
 * Modification History
 *
 * 2002-April-6   Jason Rohrer
 * Created.
 *
 * 2002-April-8   Jason Rohrer
 * Fixed multiple inclusion bug
 *
 * 2002-May-7   Jason Rohrer
 * Added functions for case-insensitive substring finding and case
 * conversion.
 *
 * 2002-May-9   Jason Rohrer
 * Fixed a bug when string not found.
 *
 * 2002-May-26   Jason Rohrer
 * Added a function for string comparison ignoring cases.
 *
 * 2003-March-28   Jason Rohrer
 * Added split function.
 *
 * 2003-May-1   Jason Rohrer
 * Added replacement functions.
 *
 * 2003-May-4   Jason Rohrer
 * Added list replacement function.
 *
 * 2003-May-10   Jason Rohrer
 * Moved implementations into a .cpp file.  This will break several
 * projects.
 * Added a tokenization function.
 *
 * 2003-June-14   Jason Rohrer
 * Added a join function.
 *
 * 2003-June-22   Jason Rohrer
 * Added an autoSprintf function.
 *
 * 2003-August-12  Jason Rohrer
 * Added a concatonate function.
 *
 * 2003-September-7  Jason Rohrer
 * Improved split comment.
 *
 * 2006-June-2  Jason Rohrer
 * Added a stringStartsWith function.
 */



#include "minorGems/common.h"
#include "minorGems/util/SimpleVector.h"



#ifndef STRING_UTILS_INCLUDED
#define STRING_UTILS_INCLUDED



// ANSI does not support strdup, strcasestr, or strcasecmp
#include <string.h>
#include <ctype.h>
#include <stdio.h>




/**
 * Duplicates a string into a newly allocated string.
 *
 * @param inString the \0-terminated string to duplicate.
 *   Must be destroyed by caller if non-const.
 *
 * @return a \0-terminated duplicate of inString.
 *   Must be destroyed by caller.
 */
inline char *stringDuplicate( const char *inString ) {
    
    char *returnBuffer = new char[ strlen( inString ) + 1 ];

    strcpy( returnBuffer, inString );

    return returnBuffer;    
    }



/**
 * Converts a string to lower case.
 *
 * @param inString the \0-terminated string to convert.
 *   Must be destroyed by caller if non-const.
 *
 * @return a newly allocated \0-terminated string
 *   that is a lowercase version of inString.
 *   Must be destroyed by caller.
 */
char *stringToLowerCase( const char *inString  );



/**
 * Searches for the first occurrence of one string in another.
 *
 * @param inHaystack the \0-terminated string to search in.
 *   Must be destroyed by caller if non-const.
 * @param inNeedle the \0-terminated string to search for.
 *   Must be destroyed by caller if non-const.
 *
 * @return a string pointer into inHaystack where the
 *   first occurrence of inNeedle starts, or NULL if inNeedle is not found.
 */
char *stringLocateIgnoreCase( const char *inHaystack,
                              const char *inNeedle );



/**
 * Compares two strings, ignoring case.
 *
 * @param inStringA the first \0-terminated string.
 *   Must be destroyed by caller if non-const.
 * @param inStringB the second \0-terminated string.
 *   Must be destroyed by caller if non-const.
 *
 * @return an integer less than, equal to, or greater than zero if
 *   inStringA is found,  respectively,  to  be  less  than, to match, or be
 *   greater than inStringB.
 */
int stringCompareIgnoreCase( const char *inStringA,
                             const char *inStringB );



/**
 * Checks if a string starts with a given prefix string.
 *
 * @param inString a \0-terminated string.
 *   Must be destroyed by caller if non-const.
 * @param inPrefix the prefix to look for as a \0-terminated string.
 *   Must be destroyed by caller if non-const.
 *
 * @return true if inString begins with inPrefix, or false otherwise.
 */
char stringStartsWith( const char *inString, const char *inPrefix );



/**
 * Splits a string into parts around a separator string.
 *
 * Note that splitting strings that start and/or end with the separator
 * results in "empty" strings being returned at the start and/or end
 * of the parts array.
 *
 * For example, splitting "a#short#test" around the "#" separator will
 * result in the array { "a", "short", "test" }, but splitting
 * "#another#short#test#" will result in the array
 * { "", "another", "short", "test", "" }.
 *
 * This differs somewhat from the perl version of split, but it gives the
 * caller more information about the string being split.
 *
 * @param inString the string to split.
 *   Must be destroyed by caller if non-const.
 * @param inSeparator the separator string.
 *   Must be destroyed by caller if non-const.
 * @param outNumParts pointer to where the the number of parts (the length of 
 *   the returned array) should be returned.
 *
 * @return an array of split parts.
 *   Must be destroyed by caller.
 */
char **split( char *inString, char *inSeparator, int *outNumParts );



/**
 * Joins a collection of strings using a separator string.
 *
 * @param inStrings the strings to join.
 *   Array and strings must be destroyed by caller.
 * @param inNumParts the number of strings to join.
 * @param inSeparator the separator string.
 *   Must be destroyed by caller if non-const.
 *
 * @return the joined string.
 *   Must be destroyed by caller.
 */
char *join( char **inStrings, int inNumParts, char *inGlue );



/**
 * Concatonates two strings.
 *
 * @param inStringA the first string in the concatonation.
 *   Must be destroyed by caller if non-const.
 * @param inStringB the second string in the concatonation.
 *   Must be destroyed by caller if non-const.
 *
 * @return the concatonation.
 *   Must be destroyed by caller.
 */
char *concatonate( char *inStringA, char *inStringB );



/**
 * Replaces the first occurrence of a target string with
 * a substitute string.
 *
 * All parameters and return value must be destroyed by caller.
 *
 * @param inHaystack the string to search for inTarget in. 
 * @param inTarget the string to search for.
 * @param inSubstitute the string to replace the first occurrence
 *   of the target with.
 * @param outFound a pre-allocated character which will be filled
 *   with true if the target is found, and filled with false
 *   otherwise.
 *
 * @return a newly allocated string with the substitution performed.
 */
char *replaceOnce( char *inHaystack, char *inTarget,
                   char *inSubstitute,
                   char *outFound );



/**
 * Replaces the all occurrences of a target string with
 * a substitute string.
 *
 * Note that this function is not self-insertion-safe:
 * If inSubstitute contains inTarget, this function will
 * enter an infinite loop.
 *         
 * All parameters and return value must be destroyed by caller.
 *
 * @param inHaystack the string to search for inTarget in. 
 * @param inTarget the string to search for.
 * @param inSubstitute the string to replace the all occurrences
 *   of the target with.
 * @param outFound a pre-allocated character which will be filled
 *   with true if the target is found at least once,
 *   and filled with false otherwise.
 *
 * @return a newly allocated string with the substitutions performed.
 */
char *replaceAll( char *inHaystack, char *inTarget,
                  char *inSubstitute,
                  char *outFound );



/**
 * Replaces the all occurrences of each target string on a list with
 * a corresponding substitute string.
 *
 * Note that this function is not self-insertion-safe:
 * If inSubstituteVector contains elements from inTargetVector,
 * this function will enter an infinite loop.
 *         
 * All parameters and return value must be destroyed by caller.
 *
 * @param inHaystack the string to search for targets in. 
 * @param inTargetVector the list of strings to search for.
 *   Vector and contained strings must be destroyed by caller.
 * @param inSubstituteVector the corresponding list of strings to
 *   replace the all occurrences of the targets with.
 *   Vector and contained strings must be destroyed by caller.
 *
 * @return a newly allocated string with the substitutions performed.
 */
char *replaceTargetListWithSubstituteList(
    char *inHaystack,
    SimpleVector<char *> *inTargetVector,
    SimpleVector<char *> *inSubstituteVector );




/**
 * Split a string into tokens using whitespace as separators.
 *
 * @param inString the string to tokenize.
 *   Must be destroyed by caller.
 *
 * @return a vector of extracted strings.
 *   Vector and strings must be destroyed by caller.
 */
SimpleVector<char *> *tokenizeString( char *inString );



/**
 * Prints formatted data elements into a newly allocated string buffer.
 *
 * Similar to sprintf, except that buffer sizing is automatic (and therefore
 * safer than manual sizing).
 *
 * @param inFormatString the format string to print from.
 * @param variable argument list data values to fill in the format string
 *   with (uses same conventions as printf).
 *
 * @return a newly allocated buffer containing the \0-terminated printed
 *   string.
 *   Must be destroyed by caller.
 */
char *autoSprintf( const char* inFormatString, ... );



#endif
