// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _mlcio_h
#define _mlcio_h 1

#include <checkstream.h>

Bool legal_attr_char(MLCIStream&, int line, char& c,
		     Bool periodAllowed=FALSE);
void skip_white_comments(MLCIStream& stream, int& line);
Bool skip_white_comments_same_line(MLCIStream& file, int& line);
MStringRC read_word (MLCIStream&, int& line, Bool qMark,
		     Bool periodAllowed = FALSE);
MStringRC read_word_on_same_line(MLCIStream&, int& line, Bool qMark,
		     Bool periodAllowed = FALSE);		      
MString get_temp_file_name();
void remove_file(const MString& file);
MString file_exists(const MString& str, Bool fatalOnFalse = FALSE);
void truncate_file(MLCOStream& stream);
#endif
