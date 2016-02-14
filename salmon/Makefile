# file "Makefile"
#
# Generated automatically by "generate_makefile.salm".

INSTALL_DIRECTORY=/usr/local
BINARY_INSTALL_DIRECTORY=$(INSTALL_DIRECTORY)/bin
LIBRARY_INSTALL_DIRECTORY=$(INSTALL_DIRECTORY)/lib
DLL_INSTALL_DIRECTORY=$(INSTALL_DIRECTORY)/lib
C_INCLUDE_INSTALL_DIRECTORY=$(INSTALL_DIRECTORY)/include
SALMON_LIBRARY_INSTALL_DIRECTORY=$(INSTALL_DIRECTORY)/salmoninclude
CC=gcc
CFLAGS=-Wall
BUILD_EXECUTABLE=$(CC) $(CFLAGS) -rdynamic -o
BUILD_SHARED_LIBRARY=$(CC) $(CFLAGS) --shared -o
FAST_FLAGS=-O4 -DNDEBUG -Wno-uninitialized -Wno-unused
STANDARD_FLAGS=-g

all: salmoneye salmoneye.check salmoneye.fast libsalmoneye.so libsalmoneye_check.so libsalmoneye_fast.so maybe_pthread library library/platform_dependent.salm

all_pthread: salmoneye.multi salmoneye.check.multi salmoneye.fast.multi libsalmoneye_multi.so libsalmoneye_check_multi.so libsalmoneye_fast_multi.so

maybe_pthread: Makefile.maybe_pthread
	make -f Makefile.maybe_pthread

Makefile.maybe_pthread: salmoneye generate_maybe_pthread_makefile.salm library/platform_dependent.salm test_pthread.c
	./salmoneye generate_maybe_pthread_makefile.salm > Makefile.maybe_pthread

libsalmoneye.dll: libsalmoneye.so
	cp libsalmoneye.so libsalmoneye.dll

library: library/Makefile libsalmoneye.dll
	cd library; make

library/Makefile: salmoneye library/generate_makefile.salm library/test_thread_link.c libsalmoneye.dll
	./salmoneye library/generate_makefile.salm '$(CFLAGS)' > library/Makefile

main.o: main.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) -DSALMONEYE_DLL_NAME=\"libsalmoneye.so\" main.c

main_m_ms.o: main.c
	$(CC) $(CFLAGS) -c -o main_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DSALMONEYE_DLL_NAME=\"libsalmoneye_check.so\" main.c

main_opt.o: main.c
	$(CC) $(CFLAGS) -c -o main_opt.o $(FAST_FLAGS) -DSALMONEYE_DLL_NAME=\"libsalmoneye_fast.so\" main.c

main_multi.o: main.c
	$(CC) $(CFLAGS) -c -o main_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED -DSALMONEYE_DLL_NAME=\"libsalmoneye_multi.so\" main.c

main_m_ms_multi.o: main.c
	$(CC) $(CFLAGS) -c -o main_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED -DSALMONEYE_DLL_NAME=\"libsalmoneye_check_multi.so\" main.c

main_opt_multi.o: main.c
	$(CC) $(CFLAGS) -c -o main_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED -DSALMONEYE_DLL_NAME=\"libsalmoneye_fast_multi.so\" main.c

driver.o: driver.c
	$(CC) $(CFLAGS) -c -DLIBRARY_INSTALL_DIRECTORY=\"$(SALMON_LIBRARY_INSTALL_DIRECTORY)\" $(STANDARD_FLAGS) driver.c

driver_m_ms.o: driver.c
	$(CC) $(CFLAGS) -c -o driver_m_ms.o -DLIBRARY_INSTALL_DIRECTORY=\"$(SALMON_LIBRARY_INSTALL_DIRECTORY)\" $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES driver.c

driver_opt.o: driver.c
	$(CC) $(CFLAGS) -c -o driver_opt.o -DLIBRARY_INSTALL_DIRECTORY=\"$(SALMON_LIBRARY_INSTALL_DIRECTORY)\" $(FAST_FLAGS) driver.c

driver_multi.o: driver.c
	$(CC) $(CFLAGS) -c -o driver_multi.o -DLIBRARY_INSTALL_DIRECTORY=\"$(SALMON_LIBRARY_INSTALL_DIRECTORY)\" $(STANDARD_FLAGS) -DMULTI_THREADED driver.c

driver_m_ms_multi.o: driver.c
	$(CC) $(CFLAGS) -c -o driver_m_ms_multi.o -DLIBRARY_INSTALL_DIRECTORY=\"$(SALMON_LIBRARY_INSTALL_DIRECTORY)\" $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED driver.c

driver_opt_multi.o: driver.c
	$(CC) $(CFLAGS) -c -o driver_opt_multi.o -DLIBRARY_INSTALL_DIRECTORY=\"$(SALMON_LIBRARY_INSTALL_DIRECTORY)\" $(FAST_FLAGS) -DMULTI_THREADED driver.c

source_location.o: source_location.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) source_location.c

source_location_m_ms.o: source_location.c
	$(CC) $(CFLAGS) -c -o source_location_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES source_location.c

source_location_opt.o: source_location.c
	$(CC) $(CFLAGS) -c -o source_location_opt.o $(FAST_FLAGS) source_location.c

source_location_multi.o: source_location.c
	$(CC) $(CFLAGS) -c -o source_location_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED source_location.c

source_location_m_ms_multi.o: source_location.c
	$(CC) $(CFLAGS) -c -o source_location_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED source_location.c

source_location_opt_multi.o: source_location.c
	$(CC) $(CFLAGS) -c -o source_location_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED source_location.c

i_integer.o: i_integer.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) i_integer.c

i_integer_m_ms.o: i_integer.c
	$(CC) $(CFLAGS) -c -o i_integer_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES i_integer.c

i_integer_opt.o: i_integer.c
	$(CC) $(CFLAGS) -c -o i_integer_opt.o $(FAST_FLAGS) i_integer.c

i_integer_multi.o: i_integer.c
	$(CC) $(CFLAGS) -c -o i_integer_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED i_integer.c

i_integer_m_ms_multi.o: i_integer.c
	$(CC) $(CFLAGS) -c -o i_integer_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED i_integer.c

i_integer_opt_multi.o: i_integer.c
	$(CC) $(CFLAGS) -c -o i_integer_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED i_integer.c

o_integer.o: o_integer.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) o_integer.c

o_integer_m_ms.o: o_integer.c
	$(CC) $(CFLAGS) -c -o o_integer_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES o_integer.c

o_integer_opt.o: o_integer.c
	$(CC) $(CFLAGS) -c -o o_integer_opt.o $(FAST_FLAGS) o_integer.c

o_integer_multi.o: o_integer.c
	$(CC) $(CFLAGS) -c -o o_integer_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED o_integer.c

o_integer_m_ms_multi.o: o_integer.c
	$(CC) $(CFLAGS) -c -o o_integer_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED o_integer.c

o_integer_opt_multi.o: o_integer.c
	$(CC) $(CFLAGS) -c -o o_integer_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED o_integer.c

rational.o: rational.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) rational.c

rational_m_ms.o: rational.c
	$(CC) $(CFLAGS) -c -o rational_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES rational.c

rational_opt.o: rational.c
	$(CC) $(CFLAGS) -c -o rational_opt.o $(FAST_FLAGS) rational.c

rational_multi.o: rational.c
	$(CC) $(CFLAGS) -c -o rational_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED rational.c

rational_m_ms_multi.o: rational.c
	$(CC) $(CFLAGS) -c -o rational_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED rational.c

rational_opt_multi.o: rational.c
	$(CC) $(CFLAGS) -c -o rational_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED rational.c

regular_expression.o: regular_expression.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) regular_expression.c

regular_expression_m_ms.o: regular_expression.c
	$(CC) $(CFLAGS) -c -o regular_expression_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES regular_expression.c

regular_expression_opt.o: regular_expression.c
	$(CC) $(CFLAGS) -c -o regular_expression_opt.o $(FAST_FLAGS) regular_expression.c

regular_expression_multi.o: regular_expression.c
	$(CC) $(CFLAGS) -c -o regular_expression_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED regular_expression.c

regular_expression_m_ms_multi.o: regular_expression.c
	$(CC) $(CFLAGS) -c -o regular_expression_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED regular_expression.c

regular_expression_opt_multi.o: regular_expression.c
	$(CC) $(CFLAGS) -c -o regular_expression_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED regular_expression.c

token.o: token.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) token.c

token_m_ms.o: token.c
	$(CC) $(CFLAGS) -c -o token_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES token.c

token_opt.o: token.c
	$(CC) $(CFLAGS) -c -o token_opt.o $(FAST_FLAGS) token.c

token_multi.o: token.c
	$(CC) $(CFLAGS) -c -o token_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED token.c

token_m_ms_multi.o: token.c
	$(CC) $(CFLAGS) -c -o token_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED token.c

token_opt_multi.o: token.c
	$(CC) $(CFLAGS) -c -o token_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED token.c

tokenizer.o: tokenizer.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) tokenizer.c

tokenizer_m_ms.o: tokenizer.c
	$(CC) $(CFLAGS) -c -o tokenizer_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES tokenizer.c

tokenizer_opt.o: tokenizer.c
	$(CC) $(CFLAGS) -c -o tokenizer_opt.o $(FAST_FLAGS) tokenizer.c

tokenizer_multi.o: tokenizer.c
	$(CC) $(CFLAGS) -c -o tokenizer_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED tokenizer.c

tokenizer_m_ms_multi.o: tokenizer.c
	$(CC) $(CFLAGS) -c -o tokenizer_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED tokenizer.c

tokenizer_opt_multi.o: tokenizer.c
	$(CC) $(CFLAGS) -c -o tokenizer_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED tokenizer.c

parser.o: parser.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) parser.c

parser_m_ms.o: parser.c
	$(CC) $(CFLAGS) -c -o parser_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES parser.c

parser_opt.o: parser.c
	$(CC) $(CFLAGS) -c -o parser_opt.o $(FAST_FLAGS) parser.c

parser_multi.o: parser.c
	$(CC) $(CFLAGS) -c -o parser_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED parser.c

parser_m_ms_multi.o: parser.c
	$(CC) $(CFLAGS) -c -o parser_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED parser.c

parser_opt_multi.o: parser.c
	$(CC) $(CFLAGS) -c -o parser_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED parser.c

file_parser.o: file_parser.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) file_parser.c

file_parser_m_ms.o: file_parser.c
	$(CC) $(CFLAGS) -c -o file_parser_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES file_parser.c

file_parser_opt.o: file_parser.c
	$(CC) $(CFLAGS) -c -o file_parser_opt.o $(FAST_FLAGS) file_parser.c

file_parser_multi.o: file_parser.c
	$(CC) $(CFLAGS) -c -o file_parser_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED file_parser.c

file_parser_m_ms_multi.o: file_parser.c
	$(CC) $(CFLAGS) -c -o file_parser_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED file_parser.c

file_parser_opt_multi.o: file_parser.c
	$(CC) $(CFLAGS) -c -o file_parser_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED file_parser.c

value.o: value.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) value.c

value_m_ms.o: value.c
	$(CC) $(CFLAGS) -c -o value_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES value.c

value_opt.o: value.c
	$(CC) $(CFLAGS) -c -o value_opt.o $(FAST_FLAGS) value.c

value_multi.o: value.c
	$(CC) $(CFLAGS) -c -o value_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED value.c

value_m_ms_multi.o: value.c
	$(CC) $(CFLAGS) -c -o value_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED value.c

value_opt_multi.o: value.c
	$(CC) $(CFLAGS) -c -o value_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED value.c

statement.o: statement.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) statement.c

statement_m_ms.o: statement.c
	$(CC) $(CFLAGS) -c -o statement_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES statement.c

statement_opt.o: statement.c
	$(CC) $(CFLAGS) -c -o statement_opt.o $(FAST_FLAGS) statement.c

statement_multi.o: statement.c
	$(CC) $(CFLAGS) -c -o statement_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED statement.c

statement_m_ms_multi.o: statement.c
	$(CC) $(CFLAGS) -c -o statement_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED statement.c

statement_opt_multi.o: statement.c
	$(CC) $(CFLAGS) -c -o statement_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED statement.c

open_statement.o: open_statement.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) open_statement.c

open_statement_m_ms.o: open_statement.c
	$(CC) $(CFLAGS) -c -o open_statement_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES open_statement.c

open_statement_opt.o: open_statement.c
	$(CC) $(CFLAGS) -c -o open_statement_opt.o $(FAST_FLAGS) open_statement.c

open_statement_multi.o: open_statement.c
	$(CC) $(CFLAGS) -c -o open_statement_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED open_statement.c

open_statement_m_ms_multi.o: open_statement.c
	$(CC) $(CFLAGS) -c -o open_statement_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED open_statement.c

open_statement_opt_multi.o: open_statement.c
	$(CC) $(CFLAGS) -c -o open_statement_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED open_statement.c

statement_block.o: statement_block.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) statement_block.c

statement_block_m_ms.o: statement_block.c
	$(CC) $(CFLAGS) -c -o statement_block_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES statement_block.c

statement_block_opt.o: statement_block.c
	$(CC) $(CFLAGS) -c -o statement_block_opt.o $(FAST_FLAGS) statement_block.c

statement_block_multi.o: statement_block.c
	$(CC) $(CFLAGS) -c -o statement_block_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED statement_block.c

statement_block_m_ms_multi.o: statement_block.c
	$(CC) $(CFLAGS) -c -o statement_block_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED statement_block.c

statement_block_opt_multi.o: statement_block.c
	$(CC) $(CFLAGS) -c -o statement_block_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED statement_block.c

open_statement_block.o: open_statement_block.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) open_statement_block.c

open_statement_block_m_ms.o: open_statement_block.c
	$(CC) $(CFLAGS) -c -o open_statement_block_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES open_statement_block.c

open_statement_block_opt.o: open_statement_block.c
	$(CC) $(CFLAGS) -c -o open_statement_block_opt.o $(FAST_FLAGS) open_statement_block.c

open_statement_block_multi.o: open_statement_block.c
	$(CC) $(CFLAGS) -c -o open_statement_block_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED open_statement_block.c

open_statement_block_m_ms_multi.o: open_statement_block.c
	$(CC) $(CFLAGS) -c -o open_statement_block_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED open_statement_block.c

open_statement_block_opt_multi.o: open_statement_block.c
	$(CC) $(CFLAGS) -c -o open_statement_block_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED open_statement_block.c

expression.o: expression.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) expression.c

expression_m_ms.o: expression.c
	$(CC) $(CFLAGS) -c -o expression_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES expression.c

expression_opt.o: expression.c
	$(CC) $(CFLAGS) -c -o expression_opt.o $(FAST_FLAGS) expression.c

expression_multi.o: expression.c
	$(CC) $(CFLAGS) -c -o expression_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED expression.c

expression_m_ms_multi.o: expression.c
	$(CC) $(CFLAGS) -c -o expression_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED expression.c

expression_opt_multi.o: expression.c
	$(CC) $(CFLAGS) -c -o expression_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED expression.c

open_expression.o: open_expression.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) open_expression.c

open_expression_m_ms.o: open_expression.c
	$(CC) $(CFLAGS) -c -o open_expression_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES open_expression.c

open_expression_opt.o: open_expression.c
	$(CC) $(CFLAGS) -c -o open_expression_opt.o $(FAST_FLAGS) open_expression.c

open_expression_multi.o: open_expression.c
	$(CC) $(CFLAGS) -c -o open_expression_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED open_expression.c

open_expression_m_ms_multi.o: open_expression.c
	$(CC) $(CFLAGS) -c -o open_expression_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED open_expression.c

open_expression_opt_multi.o: open_expression.c
	$(CC) $(CFLAGS) -c -o open_expression_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED open_expression.c

type_expression.o: type_expression.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) type_expression.c

type_expression_m_ms.o: type_expression.c
	$(CC) $(CFLAGS) -c -o type_expression_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES type_expression.c

type_expression_opt.o: type_expression.c
	$(CC) $(CFLAGS) -c -o type_expression_opt.o $(FAST_FLAGS) type_expression.c

type_expression_multi.o: type_expression.c
	$(CC) $(CFLAGS) -c -o type_expression_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED type_expression.c

type_expression_m_ms_multi.o: type_expression.c
	$(CC) $(CFLAGS) -c -o type_expression_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED type_expression.c

type_expression_opt_multi.o: type_expression.c
	$(CC) $(CFLAGS) -c -o type_expression_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED type_expression.c

open_type_expression.o: open_type_expression.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) open_type_expression.c

open_type_expression_m_ms.o: open_type_expression.c
	$(CC) $(CFLAGS) -c -o open_type_expression_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES open_type_expression.c

open_type_expression_opt.o: open_type_expression.c
	$(CC) $(CFLAGS) -c -o open_type_expression_opt.o $(FAST_FLAGS) open_type_expression.c

open_type_expression_multi.o: open_type_expression.c
	$(CC) $(CFLAGS) -c -o open_type_expression_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED open_type_expression.c

open_type_expression_m_ms_multi.o: open_type_expression.c
	$(CC) $(CFLAGS) -c -o open_type_expression_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED open_type_expression.c

open_type_expression_opt_multi.o: open_type_expression.c
	$(CC) $(CFLAGS) -c -o open_type_expression_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED open_type_expression.c

basket.o: basket.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) basket.c

basket_m_ms.o: basket.c
	$(CC) $(CFLAGS) -c -o basket_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES basket.c

basket_opt.o: basket.c
	$(CC) $(CFLAGS) -c -o basket_opt.o $(FAST_FLAGS) basket.c

basket_multi.o: basket.c
	$(CC) $(CFLAGS) -c -o basket_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED basket.c

basket_m_ms_multi.o: basket.c
	$(CC) $(CFLAGS) -c -o basket_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED basket.c

basket_opt_multi.o: basket.c
	$(CC) $(CFLAGS) -c -o basket_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED basket.c

open_basket.o: open_basket.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) open_basket.c

open_basket_m_ms.o: open_basket.c
	$(CC) $(CFLAGS) -c -o open_basket_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES open_basket.c

open_basket_opt.o: open_basket.c
	$(CC) $(CFLAGS) -c -o open_basket_opt.o $(FAST_FLAGS) open_basket.c

open_basket_multi.o: open_basket.c
	$(CC) $(CFLAGS) -c -o open_basket_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED open_basket.c

open_basket_m_ms_multi.o: open_basket.c
	$(CC) $(CFLAGS) -c -o open_basket_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED open_basket.c

open_basket_opt_multi.o: open_basket.c
	$(CC) $(CFLAGS) -c -o open_basket_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED open_basket.c

basket_instance.o: basket_instance.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) basket_instance.c

basket_instance_m_ms.o: basket_instance.c
	$(CC) $(CFLAGS) -c -o basket_instance_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES basket_instance.c

basket_instance_opt.o: basket_instance.c
	$(CC) $(CFLAGS) -c -o basket_instance_opt.o $(FAST_FLAGS) basket_instance.c

basket_instance_multi.o: basket_instance.c
	$(CC) $(CFLAGS) -c -o basket_instance_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED basket_instance.c

basket_instance_m_ms_multi.o: basket_instance.c
	$(CC) $(CFLAGS) -c -o basket_instance_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED basket_instance.c

basket_instance_opt_multi.o: basket_instance.c
	$(CC) $(CFLAGS) -c -o basket_instance_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED basket_instance.c

lookup_actual_arguments.o: lookup_actual_arguments.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) lookup_actual_arguments.c

lookup_actual_arguments_m_ms.o: lookup_actual_arguments.c
	$(CC) $(CFLAGS) -c -o lookup_actual_arguments_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES lookup_actual_arguments.c

lookup_actual_arguments_opt.o: lookup_actual_arguments.c
	$(CC) $(CFLAGS) -c -o lookup_actual_arguments_opt.o $(FAST_FLAGS) lookup_actual_arguments.c

lookup_actual_arguments_multi.o: lookup_actual_arguments.c
	$(CC) $(CFLAGS) -c -o lookup_actual_arguments_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED lookup_actual_arguments.c

lookup_actual_arguments_m_ms_multi.o: lookup_actual_arguments.c
	$(CC) $(CFLAGS) -c -o lookup_actual_arguments_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED lookup_actual_arguments.c

lookup_actual_arguments_opt_multi.o: lookup_actual_arguments.c
	$(CC) $(CFLAGS) -c -o lookup_actual_arguments_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED lookup_actual_arguments.c

routine_instance.o: routine_instance.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) routine_instance.c

routine_instance_m_ms.o: routine_instance.c
	$(CC) $(CFLAGS) -c -o routine_instance_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES routine_instance.c

routine_instance_opt.o: routine_instance.c
	$(CC) $(CFLAGS) -c -o routine_instance_opt.o $(FAST_FLAGS) routine_instance.c

routine_instance_multi.o: routine_instance.c
	$(CC) $(CFLAGS) -c -o routine_instance_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED routine_instance.c

routine_instance_m_ms_multi.o: routine_instance.c
	$(CC) $(CFLAGS) -c -o routine_instance_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED routine_instance.c

routine_instance_opt_multi.o: routine_instance.c
	$(CC) $(CFLAGS) -c -o routine_instance_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED routine_instance.c

call.o: call.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) call.c

call_m_ms.o: call.c
	$(CC) $(CFLAGS) -c -o call_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES call.c

call_opt.o: call.c
	$(CC) $(CFLAGS) -c -o call_opt.o $(FAST_FLAGS) call.c

call_multi.o: call.c
	$(CC) $(CFLAGS) -c -o call_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED call.c

call_m_ms_multi.o: call.c
	$(CC) $(CFLAGS) -c -o call_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED call.c

call_opt_multi.o: call.c
	$(CC) $(CFLAGS) -c -o call_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED call.c

open_call.o: open_call.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) open_call.c

open_call_m_ms.o: open_call.c
	$(CC) $(CFLAGS) -c -o open_call_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES open_call.c

open_call_opt.o: open_call.c
	$(CC) $(CFLAGS) -c -o open_call_opt.o $(FAST_FLAGS) open_call.c

open_call_multi.o: open_call.c
	$(CC) $(CFLAGS) -c -o open_call_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED open_call.c

open_call_m_ms_multi.o: open_call.c
	$(CC) $(CFLAGS) -c -o open_call_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED open_call.c

open_call_opt_multi.o: open_call.c
	$(CC) $(CFLAGS) -c -o open_call_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED open_call.c

type.o: type.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) type.c

type_m_ms.o: type.c
	$(CC) $(CFLAGS) -c -o type_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES type.c

type_opt.o: type.c
	$(CC) $(CFLAGS) -c -o type_opt.o $(FAST_FLAGS) type.c

type_multi.o: type.c
	$(CC) $(CFLAGS) -c -o type_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED type.c

type_m_ms_multi.o: type.c
	$(CC) $(CFLAGS) -c -o type_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED type.c

type_opt_multi.o: type.c
	$(CC) $(CFLAGS) -c -o type_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED type.c

quark.o: quark.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) quark.c

quark_m_ms.o: quark.c
	$(CC) $(CFLAGS) -c -o quark_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES quark.c

quark_opt.o: quark.c
	$(CC) $(CFLAGS) -c -o quark_opt.o $(FAST_FLAGS) quark.c

quark_multi.o: quark.c
	$(CC) $(CFLAGS) -c -o quark_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED quark.c

quark_m_ms_multi.o: quark.c
	$(CC) $(CFLAGS) -c -o quark_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED quark.c

quark_opt_multi.o: quark.c
	$(CC) $(CFLAGS) -c -o quark_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED quark.c

object.o: object.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) object.c

object_m_ms.o: object.c
	$(CC) $(CFLAGS) -c -o object_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES object.c

object_opt.o: object.c
	$(CC) $(CFLAGS) -c -o object_opt.o $(FAST_FLAGS) object.c

object_multi.o: object.c
	$(CC) $(CFLAGS) -c -o object_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED object.c

object_m_ms_multi.o: object.c
	$(CC) $(CFLAGS) -c -o object_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED object.c

object_opt_multi.o: object.c
	$(CC) $(CFLAGS) -c -o object_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED object.c

declaration.o: declaration.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) declaration.c

declaration_m_ms.o: declaration.c
	$(CC) $(CFLAGS) -c -o declaration_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES declaration.c

declaration_opt.o: declaration.c
	$(CC) $(CFLAGS) -c -o declaration_opt.o $(FAST_FLAGS) declaration.c

declaration_multi.o: declaration.c
	$(CC) $(CFLAGS) -c -o declaration_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED declaration.c

declaration_m_ms_multi.o: declaration.c
	$(CC) $(CFLAGS) -c -o declaration_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED declaration.c

declaration_opt_multi.o: declaration.c
	$(CC) $(CFLAGS) -c -o declaration_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED declaration.c

variable_declaration.o: variable_declaration.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) variable_declaration.c

variable_declaration_m_ms.o: variable_declaration.c
	$(CC) $(CFLAGS) -c -o variable_declaration_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES variable_declaration.c

variable_declaration_opt.o: variable_declaration.c
	$(CC) $(CFLAGS) -c -o variable_declaration_opt.o $(FAST_FLAGS) variable_declaration.c

variable_declaration_multi.o: variable_declaration.c
	$(CC) $(CFLAGS) -c -o variable_declaration_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED variable_declaration.c

variable_declaration_m_ms_multi.o: variable_declaration.c
	$(CC) $(CFLAGS) -c -o variable_declaration_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED variable_declaration.c

variable_declaration_opt_multi.o: variable_declaration.c
	$(CC) $(CFLAGS) -c -o variable_declaration_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED variable_declaration.c

routine_declaration.o: routine_declaration.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) routine_declaration.c

routine_declaration_m_ms.o: routine_declaration.c
	$(CC) $(CFLAGS) -c -o routine_declaration_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES routine_declaration.c

routine_declaration_opt.o: routine_declaration.c
	$(CC) $(CFLAGS) -c -o routine_declaration_opt.o $(FAST_FLAGS) routine_declaration.c

routine_declaration_multi.o: routine_declaration.c
	$(CC) $(CFLAGS) -c -o routine_declaration_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED routine_declaration.c

routine_declaration_m_ms_multi.o: routine_declaration.c
	$(CC) $(CFLAGS) -c -o routine_declaration_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED routine_declaration.c

routine_declaration_opt_multi.o: routine_declaration.c
	$(CC) $(CFLAGS) -c -o routine_declaration_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED routine_declaration.c

open_routine_declaration.o: open_routine_declaration.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) open_routine_declaration.c

open_routine_declaration_m_ms.o: open_routine_declaration.c
	$(CC) $(CFLAGS) -c -o open_routine_declaration_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES open_routine_declaration.c

open_routine_declaration_opt.o: open_routine_declaration.c
	$(CC) $(CFLAGS) -c -o open_routine_declaration_opt.o $(FAST_FLAGS) open_routine_declaration.c

open_routine_declaration_multi.o: open_routine_declaration.c
	$(CC) $(CFLAGS) -c -o open_routine_declaration_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED open_routine_declaration.c

open_routine_declaration_m_ms_multi.o: open_routine_declaration.c
	$(CC) $(CFLAGS) -c -o open_routine_declaration_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED open_routine_declaration.c

open_routine_declaration_opt_multi.o: open_routine_declaration.c
	$(CC) $(CFLAGS) -c -o open_routine_declaration_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED open_routine_declaration.c

tagalong_declaration.o: tagalong_declaration.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) tagalong_declaration.c

tagalong_declaration_m_ms.o: tagalong_declaration.c
	$(CC) $(CFLAGS) -c -o tagalong_declaration_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES tagalong_declaration.c

tagalong_declaration_opt.o: tagalong_declaration.c
	$(CC) $(CFLAGS) -c -o tagalong_declaration_opt.o $(FAST_FLAGS) tagalong_declaration.c

tagalong_declaration_multi.o: tagalong_declaration.c
	$(CC) $(CFLAGS) -c -o tagalong_declaration_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED tagalong_declaration.c

tagalong_declaration_m_ms_multi.o: tagalong_declaration.c
	$(CC) $(CFLAGS) -c -o tagalong_declaration_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED tagalong_declaration.c

tagalong_declaration_opt_multi.o: tagalong_declaration.c
	$(CC) $(CFLAGS) -c -o tagalong_declaration_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED tagalong_declaration.c

lepton_key_declaration.o: lepton_key_declaration.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) lepton_key_declaration.c

lepton_key_declaration_m_ms.o: lepton_key_declaration.c
	$(CC) $(CFLAGS) -c -o lepton_key_declaration_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES lepton_key_declaration.c

lepton_key_declaration_opt.o: lepton_key_declaration.c
	$(CC) $(CFLAGS) -c -o lepton_key_declaration_opt.o $(FAST_FLAGS) lepton_key_declaration.c

lepton_key_declaration_multi.o: lepton_key_declaration.c
	$(CC) $(CFLAGS) -c -o lepton_key_declaration_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED lepton_key_declaration.c

lepton_key_declaration_m_ms_multi.o: lepton_key_declaration.c
	$(CC) $(CFLAGS) -c -o lepton_key_declaration_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED lepton_key_declaration.c

lepton_key_declaration_opt_multi.o: lepton_key_declaration.c
	$(CC) $(CFLAGS) -c -o lepton_key_declaration_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED lepton_key_declaration.c

quark_declaration.o: quark_declaration.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) quark_declaration.c

quark_declaration_m_ms.o: quark_declaration.c
	$(CC) $(CFLAGS) -c -o quark_declaration_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES quark_declaration.c

quark_declaration_opt.o: quark_declaration.c
	$(CC) $(CFLAGS) -c -o quark_declaration_opt.o $(FAST_FLAGS) quark_declaration.c

quark_declaration_multi.o: quark_declaration.c
	$(CC) $(CFLAGS) -c -o quark_declaration_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED quark_declaration.c

quark_declaration_m_ms_multi.o: quark_declaration.c
	$(CC) $(CFLAGS) -c -o quark_declaration_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED quark_declaration.c

quark_declaration_opt_multi.o: quark_declaration.c
	$(CC) $(CFLAGS) -c -o quark_declaration_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED quark_declaration.c

lock_declaration.o: lock_declaration.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) lock_declaration.c

lock_declaration_m_ms.o: lock_declaration.c
	$(CC) $(CFLAGS) -c -o lock_declaration_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES lock_declaration.c

lock_declaration_opt.o: lock_declaration.c
	$(CC) $(CFLAGS) -c -o lock_declaration_opt.o $(FAST_FLAGS) lock_declaration.c

lock_declaration_multi.o: lock_declaration.c
	$(CC) $(CFLAGS) -c -o lock_declaration_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED lock_declaration.c

lock_declaration_m_ms_multi.o: lock_declaration.c
	$(CC) $(CFLAGS) -c -o lock_declaration_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED lock_declaration.c

lock_declaration_opt_multi.o: lock_declaration.c
	$(CC) $(CFLAGS) -c -o lock_declaration_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED lock_declaration.c

declaration_list.o: declaration_list.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) declaration_list.c

declaration_list_m_ms.o: declaration_list.c
	$(CC) $(CFLAGS) -c -o declaration_list_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES declaration_list.c

declaration_list_opt.o: declaration_list.c
	$(CC) $(CFLAGS) -c -o declaration_list_opt.o $(FAST_FLAGS) declaration_list.c

declaration_list_multi.o: declaration_list.c
	$(CC) $(CFLAGS) -c -o declaration_list_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED declaration_list.c

declaration_list_m_ms_multi.o: declaration_list.c
	$(CC) $(CFLAGS) -c -o declaration_list_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED declaration_list.c

declaration_list_opt_multi.o: declaration_list.c
	$(CC) $(CFLAGS) -c -o declaration_list_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED declaration_list.c

slot_location.o: slot_location.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) slot_location.c

slot_location_m_ms.o: slot_location.c
	$(CC) $(CFLAGS) -c -o slot_location_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES slot_location.c

slot_location_opt.o: slot_location.c
	$(CC) $(CFLAGS) -c -o slot_location_opt.o $(FAST_FLAGS) slot_location.c

slot_location_multi.o: slot_location.c
	$(CC) $(CFLAGS) -c -o slot_location_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED slot_location.c

slot_location_m_ms_multi.o: slot_location.c
	$(CC) $(CFLAGS) -c -o slot_location_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED slot_location.c

slot_location_opt_multi.o: slot_location.c
	$(CC) $(CFLAGS) -c -o slot_location_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED slot_location.c

formal_arguments.o: formal_arguments.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) formal_arguments.c

formal_arguments_m_ms.o: formal_arguments.c
	$(CC) $(CFLAGS) -c -o formal_arguments_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES formal_arguments.c

formal_arguments_opt.o: formal_arguments.c
	$(CC) $(CFLAGS) -c -o formal_arguments_opt.o $(FAST_FLAGS) formal_arguments.c

formal_arguments_multi.o: formal_arguments.c
	$(CC) $(CFLAGS) -c -o formal_arguments_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED formal_arguments.c

formal_arguments_m_ms_multi.o: formal_arguments.c
	$(CC) $(CFLAGS) -c -o formal_arguments_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED formal_arguments.c

formal_arguments_opt_multi.o: formal_arguments.c
	$(CC) $(CFLAGS) -c -o formal_arguments_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED formal_arguments.c

routine_declaration_chain.o: routine_declaration_chain.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) routine_declaration_chain.c

routine_declaration_chain_m_ms.o: routine_declaration_chain.c
	$(CC) $(CFLAGS) -c -o routine_declaration_chain_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES routine_declaration_chain.c

routine_declaration_chain_opt.o: routine_declaration_chain.c
	$(CC) $(CFLAGS) -c -o routine_declaration_chain_opt.o $(FAST_FLAGS) routine_declaration_chain.c

routine_declaration_chain_multi.o: routine_declaration_chain.c
	$(CC) $(CFLAGS) -c -o routine_declaration_chain_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED routine_declaration_chain.c

routine_declaration_chain_m_ms_multi.o: routine_declaration_chain.c
	$(CC) $(CFLAGS) -c -o routine_declaration_chain_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED routine_declaration_chain.c

routine_declaration_chain_opt_multi.o: routine_declaration_chain.c
	$(CC) $(CFLAGS) -c -o routine_declaration_chain_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED routine_declaration_chain.c

routine_instance_chain.o: routine_instance_chain.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) routine_instance_chain.c

routine_instance_chain_m_ms.o: routine_instance_chain.c
	$(CC) $(CFLAGS) -c -o routine_instance_chain_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES routine_instance_chain.c

routine_instance_chain_opt.o: routine_instance_chain.c
	$(CC) $(CFLAGS) -c -o routine_instance_chain_opt.o $(FAST_FLAGS) routine_instance_chain.c

routine_instance_chain_multi.o: routine_instance_chain.c
	$(CC) $(CFLAGS) -c -o routine_instance_chain_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED routine_instance_chain.c

routine_instance_chain_m_ms_multi.o: routine_instance_chain.c
	$(CC) $(CFLAGS) -c -o routine_instance_chain_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED routine_instance_chain.c

routine_instance_chain_opt_multi.o: routine_instance_chain.c
	$(CC) $(CFLAGS) -c -o routine_instance_chain_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED routine_instance_chain.c

semi_labeled_value_list.o: semi_labeled_value_list.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) semi_labeled_value_list.c

semi_labeled_value_list_m_ms.o: semi_labeled_value_list.c
	$(CC) $(CFLAGS) -c -o semi_labeled_value_list_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES semi_labeled_value_list.c

semi_labeled_value_list_opt.o: semi_labeled_value_list.c
	$(CC) $(CFLAGS) -c -o semi_labeled_value_list_opt.o $(FAST_FLAGS) semi_labeled_value_list.c

semi_labeled_value_list_multi.o: semi_labeled_value_list.c
	$(CC) $(CFLAGS) -c -o semi_labeled_value_list_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED semi_labeled_value_list.c

semi_labeled_value_list_m_ms_multi.o: semi_labeled_value_list.c
	$(CC) $(CFLAGS) -c -o semi_labeled_value_list_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED semi_labeled_value_list.c

semi_labeled_value_list_opt_multi.o: semi_labeled_value_list.c
	$(CC) $(CFLAGS) -c -o semi_labeled_value_list_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED semi_labeled_value_list.c

tagalong_key.o: tagalong_key.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) tagalong_key.c

tagalong_key_m_ms.o: tagalong_key.c
	$(CC) $(CFLAGS) -c -o tagalong_key_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES tagalong_key.c

tagalong_key_opt.o: tagalong_key.c
	$(CC) $(CFLAGS) -c -o tagalong_key_opt.o $(FAST_FLAGS) tagalong_key.c

tagalong_key_multi.o: tagalong_key.c
	$(CC) $(CFLAGS) -c -o tagalong_key_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED tagalong_key.c

tagalong_key_m_ms_multi.o: tagalong_key.c
	$(CC) $(CFLAGS) -c -o tagalong_key_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED tagalong_key.c

tagalong_key_opt_multi.o: tagalong_key.c
	$(CC) $(CFLAGS) -c -o tagalong_key_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED tagalong_key.c

lepton_key_instance.o: lepton_key_instance.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) lepton_key_instance.c

lepton_key_instance_m_ms.o: lepton_key_instance.c
	$(CC) $(CFLAGS) -c -o lepton_key_instance_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES lepton_key_instance.c

lepton_key_instance_opt.o: lepton_key_instance.c
	$(CC) $(CFLAGS) -c -o lepton_key_instance_opt.o $(FAST_FLAGS) lepton_key_instance.c

lepton_key_instance_multi.o: lepton_key_instance.c
	$(CC) $(CFLAGS) -c -o lepton_key_instance_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED lepton_key_instance.c

lepton_key_instance_m_ms_multi.o: lepton_key_instance.c
	$(CC) $(CFLAGS) -c -o lepton_key_instance_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED lepton_key_instance.c

lepton_key_instance_opt_multi.o: lepton_key_instance.c
	$(CC) $(CFLAGS) -c -o lepton_key_instance_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED lepton_key_instance.c

unbound.o: unbound.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) unbound.c

unbound_m_ms.o: unbound.c
	$(CC) $(CFLAGS) -c -o unbound_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES unbound.c

unbound_opt.o: unbound.c
	$(CC) $(CFLAGS) -c -o unbound_opt.o $(FAST_FLAGS) unbound.c

unbound_multi.o: unbound.c
	$(CC) $(CFLAGS) -c -o unbound_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED unbound.c

unbound_m_ms_multi.o: unbound.c
	$(CC) $(CFLAGS) -c -o unbound_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED unbound.c

unbound_opt_multi.o: unbound.c
	$(CC) $(CFLAGS) -c -o unbound_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED unbound.c

bind.o: bind.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) bind.c

bind_m_ms.o: bind.c
	$(CC) $(CFLAGS) -c -o bind_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES bind.c

bind_opt.o: bind.c
	$(CC) $(CFLAGS) -c -o bind_opt.o $(FAST_FLAGS) bind.c

bind_multi.o: bind.c
	$(CC) $(CFLAGS) -c -o bind_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED bind.c

bind_m_ms_multi.o: bind.c
	$(CC) $(CFLAGS) -c -o bind_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED bind.c

bind_opt_multi.o: bind.c
	$(CC) $(CFLAGS) -c -o bind_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED bind.c

context.o: context.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) context.c

context_m_ms.o: context.c
	$(CC) $(CFLAGS) -c -o context_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES context.c

context_opt.o: context.c
	$(CC) $(CFLAGS) -c -o context_opt.o $(FAST_FLAGS) context.c

context_multi.o: context.c
	$(CC) $(CFLAGS) -c -o context_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED context.c

context_m_ms_multi.o: context.c
	$(CC) $(CFLAGS) -c -o context_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED context.c

context_opt_multi.o: context.c
	$(CC) $(CFLAGS) -c -o context_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED context.c

instance.o: instance.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) instance.c

instance_m_ms.o: instance.c
	$(CC) $(CFLAGS) -c -o instance_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES instance.c

instance_opt.o: instance.c
	$(CC) $(CFLAGS) -c -o instance_opt.o $(FAST_FLAGS) instance.c

instance_multi.o: instance.c
	$(CC) $(CFLAGS) -c -o instance_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED instance.c

instance_m_ms_multi.o: instance.c
	$(CC) $(CFLAGS) -c -o instance_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED instance.c

instance_opt_multi.o: instance.c
	$(CC) $(CFLAGS) -c -o instance_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED instance.c

variable_instance.o: variable_instance.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) variable_instance.c

variable_instance_m_ms.o: variable_instance.c
	$(CC) $(CFLAGS) -c -o variable_instance_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES variable_instance.c

variable_instance_opt.o: variable_instance.c
	$(CC) $(CFLAGS) -c -o variable_instance_opt.o $(FAST_FLAGS) variable_instance.c

variable_instance_multi.o: variable_instance.c
	$(CC) $(CFLAGS) -c -o variable_instance_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED variable_instance.c

variable_instance_m_ms_multi.o: variable_instance.c
	$(CC) $(CFLAGS) -c -o variable_instance_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED variable_instance.c

variable_instance_opt_multi.o: variable_instance.c
	$(CC) $(CFLAGS) -c -o variable_instance_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED variable_instance.c

static_home.o: static_home.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) static_home.c

static_home_m_ms.o: static_home.c
	$(CC) $(CFLAGS) -c -o static_home_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES static_home.c

static_home_opt.o: static_home.c
	$(CC) $(CFLAGS) -c -o static_home_opt.o $(FAST_FLAGS) static_home.c

static_home_multi.o: static_home.c
	$(CC) $(CFLAGS) -c -o static_home_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED static_home.c

static_home_m_ms_multi.o: static_home.c
	$(CC) $(CFLAGS) -c -o static_home_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED static_home.c

static_home_opt_multi.o: static_home.c
	$(CC) $(CFLAGS) -c -o static_home_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED static_home.c

jump_target.o: jump_target.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) jump_target.c

jump_target_m_ms.o: jump_target.c
	$(CC) $(CFLAGS) -c -o jump_target_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES jump_target.c

jump_target_opt.o: jump_target.c
	$(CC) $(CFLAGS) -c -o jump_target_opt.o $(FAST_FLAGS) jump_target.c

jump_target_multi.o: jump_target.c
	$(CC) $(CFLAGS) -c -o jump_target_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED jump_target.c

jump_target_m_ms_multi.o: jump_target.c
	$(CC) $(CFLAGS) -c -o jump_target_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED jump_target.c

jump_target_opt_multi.o: jump_target.c
	$(CC) $(CFLAGS) -c -o jump_target_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED jump_target.c

jumper.o: jumper.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) jumper.c

jumper_m_ms.o: jumper.c
	$(CC) $(CFLAGS) -c -o jumper_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES jumper.c

jumper_opt.o: jumper.c
	$(CC) $(CFLAGS) -c -o jumper_opt.o $(FAST_FLAGS) jumper.c

jumper_multi.o: jumper.c
	$(CC) $(CFLAGS) -c -o jumper_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED jumper.c

jumper_m_ms_multi.o: jumper.c
	$(CC) $(CFLAGS) -c -o jumper_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED jumper.c

jumper_opt_multi.o: jumper.c
	$(CC) $(CFLAGS) -c -o jumper_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED jumper.c

purity_level.o: purity_level.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) purity_level.c

purity_level_m_ms.o: purity_level.c
	$(CC) $(CFLAGS) -c -o purity_level_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES purity_level.c

purity_level_opt.o: purity_level.c
	$(CC) $(CFLAGS) -c -o purity_level_opt.o $(FAST_FLAGS) purity_level.c

purity_level_multi.o: purity_level.c
	$(CC) $(CFLAGS) -c -o purity_level_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED purity_level.c

purity_level_m_ms_multi.o: purity_level.c
	$(CC) $(CFLAGS) -c -o purity_level_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED purity_level.c

purity_level_opt_multi.o: purity_level.c
	$(CC) $(CFLAGS) -c -o purity_level_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED purity_level.c

lock_instance.o: lock_instance.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) lock_instance.c

lock_instance_m_ms.o: lock_instance.c
	$(CC) $(CFLAGS) -c -o lock_instance_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES lock_instance.c

lock_instance_opt.o: lock_instance.c
	$(CC) $(CFLAGS) -c -o lock_instance_opt.o $(FAST_FLAGS) lock_instance.c

lock_instance_multi.o: lock_instance.c
	$(CC) $(CFLAGS) -c -o lock_instance_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED lock_instance.c

lock_instance_m_ms_multi.o: lock_instance.c
	$(CC) $(CFLAGS) -c -o lock_instance_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED lock_instance.c

lock_instance_opt_multi.o: lock_instance.c
	$(CC) $(CFLAGS) -c -o lock_instance_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED lock_instance.c

lock_chain.o: lock_chain.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) lock_chain.c

lock_chain_m_ms.o: lock_chain.c
	$(CC) $(CFLAGS) -c -o lock_chain_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES lock_chain.c

lock_chain_opt.o: lock_chain.c
	$(CC) $(CFLAGS) -c -o lock_chain_opt.o $(FAST_FLAGS) lock_chain.c

lock_chain_multi.o: lock_chain.c
	$(CC) $(CFLAGS) -c -o lock_chain_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED lock_chain.c

lock_chain_m_ms_multi.o: lock_chain.c
	$(CC) $(CFLAGS) -c -o lock_chain_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED lock_chain.c

lock_chain_opt_multi.o: lock_chain.c
	$(CC) $(CFLAGS) -c -o lock_chain_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED lock_chain.c

virtual_lookup.o: virtual_lookup.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) virtual_lookup.c

virtual_lookup_m_ms.o: virtual_lookup.c
	$(CC) $(CFLAGS) -c -o virtual_lookup_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES virtual_lookup.c

virtual_lookup_opt.o: virtual_lookup.c
	$(CC) $(CFLAGS) -c -o virtual_lookup_opt.o $(FAST_FLAGS) virtual_lookup.c

virtual_lookup_multi.o: virtual_lookup.c
	$(CC) $(CFLAGS) -c -o virtual_lookup_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED virtual_lookup.c

virtual_lookup_m_ms_multi.o: virtual_lookup.c
	$(CC) $(CFLAGS) -c -o virtual_lookup_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED virtual_lookup.c

virtual_lookup_opt_multi.o: virtual_lookup.c
	$(CC) $(CFLAGS) -c -o virtual_lookup_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED virtual_lookup.c

use_instance.o: use_instance.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) use_instance.c

use_instance_m_ms.o: use_instance.c
	$(CC) $(CFLAGS) -c -o use_instance_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES use_instance.c

use_instance_opt.o: use_instance.c
	$(CC) $(CFLAGS) -c -o use_instance_opt.o $(FAST_FLAGS) use_instance.c

use_instance_multi.o: use_instance.c
	$(CC) $(CFLAGS) -c -o use_instance_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED use_instance.c

use_instance_m_ms_multi.o: use_instance.c
	$(CC) $(CFLAGS) -c -o use_instance_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED use_instance.c

use_instance_opt_multi.o: use_instance.c
	$(CC) $(CFLAGS) -c -o use_instance_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED use_instance.c

execute.o: execute.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) execute.c

execute_m_ms.o: execute.c
	$(CC) $(CFLAGS) -c -o execute_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES execute.c

execute_opt.o: execute.c
	$(CC) $(CFLAGS) -c -o execute_opt.o $(FAST_FLAGS) execute.c

execute_multi.o: execute.c
	$(CC) $(CFLAGS) -c -o execute_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED execute.c

execute_m_ms_multi.o: execute.c
	$(CC) $(CFLAGS) -c -o execute_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED execute.c

execute_opt_multi.o: execute.c
	$(CC) $(CFLAGS) -c -o execute_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED execute.c

validator.o: validator.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) validator.c

validator_m_ms.o: validator.c
	$(CC) $(CFLAGS) -c -o validator_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES validator.c

validator_opt.o: validator.c
	$(CC) $(CFLAGS) -c -o validator_opt.o $(FAST_FLAGS) validator.c

validator_multi.o: validator.c
	$(CC) $(CFLAGS) -c -o validator_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED validator.c

validator_m_ms_multi.o: validator.c
	$(CC) $(CFLAGS) -c -o validator_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED validator.c

validator_opt_multi.o: validator.c
	$(CC) $(CFLAGS) -c -o validator_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED validator.c

native_bridge.o: native_bridge.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) native_bridge.c

native_bridge_m_ms.o: native_bridge.c
	$(CC) $(CFLAGS) -c -o native_bridge_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES native_bridge.c

native_bridge_opt.o: native_bridge.c
	$(CC) $(CFLAGS) -c -o native_bridge_opt.o $(FAST_FLAGS) native_bridge.c

native_bridge_multi.o: native_bridge.c
	$(CC) $(CFLAGS) -c -o native_bridge_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED native_bridge.c

native_bridge_m_ms_multi.o: native_bridge.c
	$(CC) $(CFLAGS) -c -o native_bridge_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED native_bridge.c

native_bridge_opt_multi.o: native_bridge.c
	$(CC) $(CFLAGS) -c -o native_bridge_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED native_bridge.c

standard_built_ins.o: standard_built_ins.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) standard_built_ins.c

standard_built_ins_m_ms.o: standard_built_ins.c
	$(CC) $(CFLAGS) -c -o standard_built_ins_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES standard_built_ins.c

standard_built_ins_opt.o: standard_built_ins.c
	$(CC) $(CFLAGS) -c -o standard_built_ins_opt.o $(FAST_FLAGS) standard_built_ins.c

standard_built_ins_multi.o: standard_built_ins.c
	$(CC) $(CFLAGS) -c -o standard_built_ins_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED standard_built_ins.c

standard_built_ins_m_ms_multi.o: standard_built_ins.c
	$(CC) $(CFLAGS) -c -o standard_built_ins_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED standard_built_ins.c

standard_built_ins_opt_multi.o: standard_built_ins.c
	$(CC) $(CFLAGS) -c -o standard_built_ins_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED standard_built_ins.c

unicode.o: unicode.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) unicode.c

unicode_m_ms.o: unicode.c
	$(CC) $(CFLAGS) -c -o unicode_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES unicode.c

unicode_opt.o: unicode.c
	$(CC) $(CFLAGS) -c -o unicode_opt.o $(FAST_FLAGS) unicode.c

unicode_multi.o: unicode.c
	$(CC) $(CFLAGS) -c -o unicode_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED unicode.c

unicode_m_ms_multi.o: unicode.c
	$(CC) $(CFLAGS) -c -o unicode_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED unicode.c

unicode_opt_multi.o: unicode.c
	$(CC) $(CFLAGS) -c -o unicode_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED unicode.c

exceptions.o: exceptions.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) exceptions.c

exceptions_m_ms.o: exceptions.c
	$(CC) $(CFLAGS) -c -o exceptions_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES exceptions.c

exceptions_opt.o: exceptions.c
	$(CC) $(CFLAGS) -c -o exceptions_opt.o $(FAST_FLAGS) exceptions.c

exceptions_multi.o: exceptions.c
	$(CC) $(CFLAGS) -c -o exceptions_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED exceptions.c

exceptions_m_ms_multi.o: exceptions.c
	$(CC) $(CFLAGS) -c -o exceptions_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED exceptions.c

exceptions_opt_multi.o: exceptions.c
	$(CC) $(CFLAGS) -c -o exceptions_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED exceptions.c

reference_cluster.o: reference_cluster.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) reference_cluster.c

reference_cluster_m_ms.o: reference_cluster.c
	$(CC) $(CFLAGS) -c -o reference_cluster_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES reference_cluster.c

reference_cluster_opt.o: reference_cluster.c
	$(CC) $(CFLAGS) -c -o reference_cluster_opt.o $(FAST_FLAGS) reference_cluster.c

reference_cluster_multi.o: reference_cluster.c
	$(CC) $(CFLAGS) -c -o reference_cluster_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED reference_cluster.c

reference_cluster_m_ms_multi.o: reference_cluster.c
	$(CC) $(CFLAGS) -c -o reference_cluster_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED reference_cluster.c

reference_cluster_opt_multi.o: reference_cluster.c
	$(CC) $(CFLAGS) -c -o reference_cluster_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED reference_cluster.c

include.o: include.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) include.c

include_m_ms.o: include.c
	$(CC) $(CFLAGS) -c -o include_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES include.c

include_opt.o: include.c
	$(CC) $(CFLAGS) -c -o include_opt.o $(FAST_FLAGS) include.c

include_multi.o: include.c
	$(CC) $(CFLAGS) -c -o include_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED include.c

include_m_ms_multi.o: include.c
	$(CC) $(CFLAGS) -c -o include_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED include.c

include_opt_multi.o: include.c
	$(CC) $(CFLAGS) -c -o include_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED include.c

thread.o: thread.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) thread.c

thread_m_ms.o: thread.c
	$(CC) $(CFLAGS) -c -o thread_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES thread.c

thread_opt.o: thread.c
	$(CC) $(CFLAGS) -c -o thread_opt.o $(FAST_FLAGS) thread.c

thread_multi.o: thread.c
	$(CC) $(CFLAGS) -c -o thread_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED thread.c

thread_m_ms_multi.o: thread.c
	$(CC) $(CFLAGS) -c -o thread_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED thread.c

thread_opt_multi.o: thread.c
	$(CC) $(CFLAGS) -c -o thread_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED thread.c

trace_channels.o: trace_channels.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) trace_channels.c

trace_channels_m_ms.o: trace_channels.c
	$(CC) $(CFLAGS) -c -o trace_channels_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES trace_channels.c

trace_channels_opt.o: trace_channels.c
	$(CC) $(CFLAGS) -c -o trace_channels_opt.o $(FAST_FLAGS) trace_channels.c

trace_channels_multi.o: trace_channels.c
	$(CC) $(CFLAGS) -c -o trace_channels_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED trace_channels.c

trace_channels_m_ms_multi.o: trace_channels.c
	$(CC) $(CFLAGS) -c -o trace_channels_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED trace_channels.c

trace_channels_opt_multi.o: trace_channels.c
	$(CC) $(CFLAGS) -c -o trace_channels_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED trace_channels.c

utility.o: utility.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) utility.c

utility_m_ms.o: utility.c
	$(CC) $(CFLAGS) -c -o utility_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES utility.c

utility_opt.o: utility.c
	$(CC) $(CFLAGS) -c -o utility_opt.o $(FAST_FLAGS) utility.c

utility_multi.o: utility.c
	$(CC) $(CFLAGS) -c -o utility_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED utility.c

utility_m_ms_multi.o: utility.c
	$(CC) $(CFLAGS) -c -o utility_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED utility.c

utility_opt_multi.o: utility.c
	$(CC) $(CFLAGS) -c -o utility_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED utility.c

profile.o: profile.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) profile.c

profile_m_ms.o: profile.c
	$(CC) $(CFLAGS) -c -o profile_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES profile.c

profile_opt.o: profile.c
	$(CC) $(CFLAGS) -c -o profile_opt.o $(FAST_FLAGS) profile.c

profile_multi.o: profile.c
	$(CC) $(CFLAGS) -c -o profile_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED profile.c

profile_m_ms_multi.o: profile.c
	$(CC) $(CFLAGS) -c -o profile_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED profile.c

profile_opt_multi.o: profile.c
	$(CC) $(CFLAGS) -c -o profile_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED profile.c

platform_dependent.o: platform_dependent.c
	$(CC) $(CFLAGS) -c $(STANDARD_FLAGS) platform_dependent.c

platform_dependent_m_ms.o: platform_dependent.c
	$(CC) $(CFLAGS) -c -o platform_dependent_m_ms.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES platform_dependent.c

platform_dependent_opt.o: platform_dependent.c
	$(CC) $(CFLAGS) -c -o platform_dependent_opt.o $(FAST_FLAGS) platform_dependent.c

platform_dependent_multi.o: platform_dependent.c
	$(CC) $(CFLAGS) -c -o platform_dependent_multi.o $(STANDARD_FLAGS) -DMULTI_THREADED platform_dependent.c

platform_dependent_m_ms_multi.o: platform_dependent.c
	$(CC) $(CFLAGS) -c -o platform_dependent_m_ms_multi.o $(STANDARD_FLAGS) -DCHECK_MEMORY_ALLOCATION -DASSERT_MALLOCED_SIZES -DMULTI_THREADED platform_dependent.c

platform_dependent_opt_multi.o: platform_dependent.c
	$(CC) $(CFLAGS) -c -o platform_dependent_opt_multi.o $(FAST_FLAGS) -DMULTI_THREADED platform_dependent.c

c_foundations/libc_foundations.a:
	cd c_foundations; make CFLAGS='$(CFLAGS)' libc_foundations.a

salmoneye: main.o driver.o source_location.o i_integer.o o_integer.o rational.o regular_expression.o token.o tokenizer.o parser.o file_parser.o value.o statement.o open_statement.o statement_block.o open_statement_block.o expression.o open_expression.o type_expression.o open_type_expression.o basket.o open_basket.o basket_instance.o lookup_actual_arguments.o routine_instance.o call.o open_call.o type.o quark.o object.o declaration.o variable_declaration.o routine_declaration.o open_routine_declaration.o tagalong_declaration.o lepton_key_declaration.o quark_declaration.o lock_declaration.o declaration_list.o slot_location.o formal_arguments.o routine_declaration_chain.o routine_instance_chain.o semi_labeled_value_list.o tagalong_key.o lepton_key_instance.o unbound.o bind.o context.o instance.o variable_instance.o static_home.o jump_target.o jumper.o purity_level.o lock_instance.o lock_chain.o virtual_lookup.o use_instance.o execute.o validator.o native_bridge.o standard_built_ins.o unicode.o exceptions.o reference_cluster.o include.o thread.o trace_channels.o utility.o profile.o platform_dependent.o c_foundations/libc_foundations.a platform_dependent_libs
	$(BUILD_EXECUTABLE) salmoneye main.o driver.o source_location.o i_integer.o o_integer.o rational.o regular_expression.o token.o tokenizer.o parser.o file_parser.o value.o statement.o open_statement.o statement_block.o open_statement_block.o expression.o open_expression.o type_expression.o open_type_expression.o basket.o open_basket.o basket_instance.o lookup_actual_arguments.o routine_instance.o call.o open_call.o type.o quark.o object.o declaration.o variable_declaration.o routine_declaration.o open_routine_declaration.o tagalong_declaration.o lepton_key_declaration.o quark_declaration.o lock_declaration.o declaration_list.o slot_location.o formal_arguments.o routine_declaration_chain.o routine_instance_chain.o semi_labeled_value_list.o tagalong_key.o lepton_key_instance.o unbound.o bind.o context.o instance.o variable_instance.o static_home.o jump_target.o jumper.o purity_level.o lock_instance.o lock_chain.o virtual_lookup.o use_instance.o execute.o validator.o native_bridge.o standard_built_ins.o unicode.o exceptions.o reference_cluster.o include.o thread.o trace_channels.o utility.o profile.o platform_dependent.o -Lc_foundations -lc_foundations`./platform_dependent_libs`

libsalmoneye.so: driver.o source_location.o i_integer.o o_integer.o rational.o regular_expression.o token.o tokenizer.o parser.o file_parser.o value.o statement.o open_statement.o statement_block.o open_statement_block.o expression.o open_expression.o type_expression.o open_type_expression.o basket.o open_basket.o basket_instance.o lookup_actual_arguments.o routine_instance.o call.o open_call.o type.o quark.o object.o declaration.o variable_declaration.o routine_declaration.o open_routine_declaration.o tagalong_declaration.o lepton_key_declaration.o quark_declaration.o lock_declaration.o declaration_list.o slot_location.o formal_arguments.o routine_declaration_chain.o routine_instance_chain.o semi_labeled_value_list.o tagalong_key.o lepton_key_instance.o unbound.o bind.o context.o instance.o variable_instance.o static_home.o jump_target.o jumper.o purity_level.o lock_instance.o lock_chain.o virtual_lookup.o use_instance.o execute.o validator.o native_bridge.o standard_built_ins.o unicode.o exceptions.o reference_cluster.o include.o thread.o trace_channels.o utility.o profile.o platform_dependent.o c_foundations/libc_foundations.a platform_dependent_libs
	$(BUILD_SHARED_LIBRARY) libsalmoneye.so driver.o source_location.o i_integer.o o_integer.o rational.o regular_expression.o token.o tokenizer.o parser.o file_parser.o value.o statement.o open_statement.o statement_block.o open_statement_block.o expression.o open_expression.o type_expression.o open_type_expression.o basket.o open_basket.o basket_instance.o lookup_actual_arguments.o routine_instance.o call.o open_call.o type.o quark.o object.o declaration.o variable_declaration.o routine_declaration.o open_routine_declaration.o tagalong_declaration.o lepton_key_declaration.o quark_declaration.o lock_declaration.o declaration_list.o slot_location.o formal_arguments.o routine_declaration_chain.o routine_instance_chain.o semi_labeled_value_list.o tagalong_key.o lepton_key_instance.o unbound.o bind.o context.o instance.o variable_instance.o static_home.o jump_target.o jumper.o purity_level.o lock_instance.o lock_chain.o virtual_lookup.o use_instance.o execute.o validator.o native_bridge.o standard_built_ins.o unicode.o exceptions.o reference_cluster.o include.o thread.o trace_channels.o utility.o profile.o platform_dependent.o -Lc_foundations -lc_foundations`./platform_dependent_libs`

c_foundations/libc_foundations_check.a:
	cd c_foundations; make CFLAGS='$(CFLAGS)' libc_foundations_check.a

salmoneye.check: main_m_ms.o driver_m_ms.o source_location_m_ms.o i_integer_m_ms.o o_integer_m_ms.o rational_m_ms.o regular_expression_m_ms.o token_m_ms.o tokenizer_m_ms.o parser_m_ms.o file_parser_m_ms.o value_m_ms.o statement_m_ms.o open_statement_m_ms.o statement_block_m_ms.o open_statement_block_m_ms.o expression_m_ms.o open_expression_m_ms.o type_expression_m_ms.o open_type_expression_m_ms.o basket_m_ms.o open_basket_m_ms.o basket_instance_m_ms.o lookup_actual_arguments_m_ms.o routine_instance_m_ms.o call_m_ms.o open_call_m_ms.o type_m_ms.o quark_m_ms.o object_m_ms.o declaration_m_ms.o variable_declaration_m_ms.o routine_declaration_m_ms.o open_routine_declaration_m_ms.o tagalong_declaration_m_ms.o lepton_key_declaration_m_ms.o quark_declaration_m_ms.o lock_declaration_m_ms.o declaration_list_m_ms.o slot_location_m_ms.o formal_arguments_m_ms.o routine_declaration_chain_m_ms.o routine_instance_chain_m_ms.o semi_labeled_value_list_m_ms.o tagalong_key_m_ms.o lepton_key_instance_m_ms.o unbound_m_ms.o bind_m_ms.o context_m_ms.o instance_m_ms.o variable_instance_m_ms.o static_home_m_ms.o jump_target_m_ms.o jumper_m_ms.o purity_level_m_ms.o lock_instance_m_ms.o lock_chain_m_ms.o virtual_lookup_m_ms.o use_instance_m_ms.o execute_m_ms.o validator_m_ms.o native_bridge_m_ms.o standard_built_ins_m_ms.o unicode_m_ms.o exceptions_m_ms.o reference_cluster_m_ms.o include_m_ms.o thread_m_ms.o trace_channels_m_ms.o utility_m_ms.o profile_m_ms.o platform_dependent_m_ms.o c_foundations/libc_foundations_check.a platform_dependent_libs
	$(BUILD_EXECUTABLE) salmoneye.check main_m_ms.o driver_m_ms.o source_location_m_ms.o i_integer_m_ms.o o_integer_m_ms.o rational_m_ms.o regular_expression_m_ms.o token_m_ms.o tokenizer_m_ms.o parser_m_ms.o file_parser_m_ms.o value_m_ms.o statement_m_ms.o open_statement_m_ms.o statement_block_m_ms.o open_statement_block_m_ms.o expression_m_ms.o open_expression_m_ms.o type_expression_m_ms.o open_type_expression_m_ms.o basket_m_ms.o open_basket_m_ms.o basket_instance_m_ms.o lookup_actual_arguments_m_ms.o routine_instance_m_ms.o call_m_ms.o open_call_m_ms.o type_m_ms.o quark_m_ms.o object_m_ms.o declaration_m_ms.o variable_declaration_m_ms.o routine_declaration_m_ms.o open_routine_declaration_m_ms.o tagalong_declaration_m_ms.o lepton_key_declaration_m_ms.o quark_declaration_m_ms.o lock_declaration_m_ms.o declaration_list_m_ms.o slot_location_m_ms.o formal_arguments_m_ms.o routine_declaration_chain_m_ms.o routine_instance_chain_m_ms.o semi_labeled_value_list_m_ms.o tagalong_key_m_ms.o lepton_key_instance_m_ms.o unbound_m_ms.o bind_m_ms.o context_m_ms.o instance_m_ms.o variable_instance_m_ms.o static_home_m_ms.o jump_target_m_ms.o jumper_m_ms.o purity_level_m_ms.o lock_instance_m_ms.o lock_chain_m_ms.o virtual_lookup_m_ms.o use_instance_m_ms.o execute_m_ms.o validator_m_ms.o native_bridge_m_ms.o standard_built_ins_m_ms.o unicode_m_ms.o exceptions_m_ms.o reference_cluster_m_ms.o include_m_ms.o thread_m_ms.o trace_channels_m_ms.o utility_m_ms.o profile_m_ms.o platform_dependent_m_ms.o -Lc_foundations -lc_foundations_check`./platform_dependent_libs`

libsalmoneye_check.so: driver_m_ms.o source_location_m_ms.o i_integer_m_ms.o o_integer_m_ms.o rational_m_ms.o regular_expression_m_ms.o token_m_ms.o tokenizer_m_ms.o parser_m_ms.o file_parser_m_ms.o value_m_ms.o statement_m_ms.o open_statement_m_ms.o statement_block_m_ms.o open_statement_block_m_ms.o expression_m_ms.o open_expression_m_ms.o type_expression_m_ms.o open_type_expression_m_ms.o basket_m_ms.o open_basket_m_ms.o basket_instance_m_ms.o lookup_actual_arguments_m_ms.o routine_instance_m_ms.o call_m_ms.o open_call_m_ms.o type_m_ms.o quark_m_ms.o object_m_ms.o declaration_m_ms.o variable_declaration_m_ms.o routine_declaration_m_ms.o open_routine_declaration_m_ms.o tagalong_declaration_m_ms.o lepton_key_declaration_m_ms.o quark_declaration_m_ms.o lock_declaration_m_ms.o declaration_list_m_ms.o slot_location_m_ms.o formal_arguments_m_ms.o routine_declaration_chain_m_ms.o routine_instance_chain_m_ms.o semi_labeled_value_list_m_ms.o tagalong_key_m_ms.o lepton_key_instance_m_ms.o unbound_m_ms.o bind_m_ms.o context_m_ms.o instance_m_ms.o variable_instance_m_ms.o static_home_m_ms.o jump_target_m_ms.o jumper_m_ms.o purity_level_m_ms.o lock_instance_m_ms.o lock_chain_m_ms.o virtual_lookup_m_ms.o use_instance_m_ms.o execute_m_ms.o validator_m_ms.o native_bridge_m_ms.o standard_built_ins_m_ms.o unicode_m_ms.o exceptions_m_ms.o reference_cluster_m_ms.o include_m_ms.o thread_m_ms.o trace_channels_m_ms.o utility_m_ms.o profile_m_ms.o platform_dependent_m_ms.o c_foundations/libc_foundations_check.a platform_dependent_libs
	$(BUILD_SHARED_LIBRARY) libsalmoneye_check.so driver_m_ms.o source_location_m_ms.o i_integer_m_ms.o o_integer_m_ms.o rational_m_ms.o regular_expression_m_ms.o token_m_ms.o tokenizer_m_ms.o parser_m_ms.o file_parser_m_ms.o value_m_ms.o statement_m_ms.o open_statement_m_ms.o statement_block_m_ms.o open_statement_block_m_ms.o expression_m_ms.o open_expression_m_ms.o type_expression_m_ms.o open_type_expression_m_ms.o basket_m_ms.o open_basket_m_ms.o basket_instance_m_ms.o lookup_actual_arguments_m_ms.o routine_instance_m_ms.o call_m_ms.o open_call_m_ms.o type_m_ms.o quark_m_ms.o object_m_ms.o declaration_m_ms.o variable_declaration_m_ms.o routine_declaration_m_ms.o open_routine_declaration_m_ms.o tagalong_declaration_m_ms.o lepton_key_declaration_m_ms.o quark_declaration_m_ms.o lock_declaration_m_ms.o declaration_list_m_ms.o slot_location_m_ms.o formal_arguments_m_ms.o routine_declaration_chain_m_ms.o routine_instance_chain_m_ms.o semi_labeled_value_list_m_ms.o tagalong_key_m_ms.o lepton_key_instance_m_ms.o unbound_m_ms.o bind_m_ms.o context_m_ms.o instance_m_ms.o variable_instance_m_ms.o static_home_m_ms.o jump_target_m_ms.o jumper_m_ms.o purity_level_m_ms.o lock_instance_m_ms.o lock_chain_m_ms.o virtual_lookup_m_ms.o use_instance_m_ms.o execute_m_ms.o validator_m_ms.o native_bridge_m_ms.o standard_built_ins_m_ms.o unicode_m_ms.o exceptions_m_ms.o reference_cluster_m_ms.o include_m_ms.o thread_m_ms.o trace_channels_m_ms.o utility_m_ms.o profile_m_ms.o platform_dependent_m_ms.o -Lc_foundations -lc_foundations_check`./platform_dependent_libs`

c_foundations/libc_foundations_fast.a:
	cd c_foundations; make CFLAGS='$(CFLAGS)' libc_foundations_fast.a

salmoneye.fast: main_opt.o driver_opt.o source_location_opt.o i_integer_opt.o o_integer_opt.o rational_opt.o regular_expression_opt.o token_opt.o tokenizer_opt.o parser_opt.o file_parser_opt.o value_opt.o statement_opt.o open_statement_opt.o statement_block_opt.o open_statement_block_opt.o expression_opt.o open_expression_opt.o type_expression_opt.o open_type_expression_opt.o basket_opt.o open_basket_opt.o basket_instance_opt.o lookup_actual_arguments_opt.o routine_instance_opt.o call_opt.o open_call_opt.o type_opt.o quark_opt.o object_opt.o declaration_opt.o variable_declaration_opt.o routine_declaration_opt.o open_routine_declaration_opt.o tagalong_declaration_opt.o lepton_key_declaration_opt.o quark_declaration_opt.o lock_declaration_opt.o declaration_list_opt.o slot_location_opt.o formal_arguments_opt.o routine_declaration_chain_opt.o routine_instance_chain_opt.o semi_labeled_value_list_opt.o tagalong_key_opt.o lepton_key_instance_opt.o unbound_opt.o bind_opt.o context_opt.o instance_opt.o variable_instance_opt.o static_home_opt.o jump_target_opt.o jumper_opt.o purity_level_opt.o lock_instance_opt.o lock_chain_opt.o virtual_lookup_opt.o use_instance_opt.o execute_opt.o validator_opt.o native_bridge_opt.o standard_built_ins_opt.o unicode_opt.o exceptions_opt.o reference_cluster_opt.o include_opt.o thread_opt.o trace_channels_opt.o utility_opt.o profile_opt.o platform_dependent_opt.o c_foundations/libc_foundations_fast.a platform_dependent_libs
	$(BUILD_EXECUTABLE) salmoneye.fast main_opt.o driver_opt.o source_location_opt.o i_integer_opt.o o_integer_opt.o rational_opt.o regular_expression_opt.o token_opt.o tokenizer_opt.o parser_opt.o file_parser_opt.o value_opt.o statement_opt.o open_statement_opt.o statement_block_opt.o open_statement_block_opt.o expression_opt.o open_expression_opt.o type_expression_opt.o open_type_expression_opt.o basket_opt.o open_basket_opt.o basket_instance_opt.o lookup_actual_arguments_opt.o routine_instance_opt.o call_opt.o open_call_opt.o type_opt.o quark_opt.o object_opt.o declaration_opt.o variable_declaration_opt.o routine_declaration_opt.o open_routine_declaration_opt.o tagalong_declaration_opt.o lepton_key_declaration_opt.o quark_declaration_opt.o lock_declaration_opt.o declaration_list_opt.o slot_location_opt.o formal_arguments_opt.o routine_declaration_chain_opt.o routine_instance_chain_opt.o semi_labeled_value_list_opt.o tagalong_key_opt.o lepton_key_instance_opt.o unbound_opt.o bind_opt.o context_opt.o instance_opt.o variable_instance_opt.o static_home_opt.o jump_target_opt.o jumper_opt.o purity_level_opt.o lock_instance_opt.o lock_chain_opt.o virtual_lookup_opt.o use_instance_opt.o execute_opt.o validator_opt.o native_bridge_opt.o standard_built_ins_opt.o unicode_opt.o exceptions_opt.o reference_cluster_opt.o include_opt.o thread_opt.o trace_channels_opt.o utility_opt.o profile_opt.o platform_dependent_opt.o -Lc_foundations -lc_foundations_fast`./platform_dependent_libs`

libsalmoneye_fast.so: driver_opt.o source_location_opt.o i_integer_opt.o o_integer_opt.o rational_opt.o regular_expression_opt.o token_opt.o tokenizer_opt.o parser_opt.o file_parser_opt.o value_opt.o statement_opt.o open_statement_opt.o statement_block_opt.o open_statement_block_opt.o expression_opt.o open_expression_opt.o type_expression_opt.o open_type_expression_opt.o basket_opt.o open_basket_opt.o basket_instance_opt.o lookup_actual_arguments_opt.o routine_instance_opt.o call_opt.o open_call_opt.o type_opt.o quark_opt.o object_opt.o declaration_opt.o variable_declaration_opt.o routine_declaration_opt.o open_routine_declaration_opt.o tagalong_declaration_opt.o lepton_key_declaration_opt.o quark_declaration_opt.o lock_declaration_opt.o declaration_list_opt.o slot_location_opt.o formal_arguments_opt.o routine_declaration_chain_opt.o routine_instance_chain_opt.o semi_labeled_value_list_opt.o tagalong_key_opt.o lepton_key_instance_opt.o unbound_opt.o bind_opt.o context_opt.o instance_opt.o variable_instance_opt.o static_home_opt.o jump_target_opt.o jumper_opt.o purity_level_opt.o lock_instance_opt.o lock_chain_opt.o virtual_lookup_opt.o use_instance_opt.o execute_opt.o validator_opt.o native_bridge_opt.o standard_built_ins_opt.o unicode_opt.o exceptions_opt.o reference_cluster_opt.o include_opt.o thread_opt.o trace_channels_opt.o utility_opt.o profile_opt.o platform_dependent_opt.o c_foundations/libc_foundations_fast.a platform_dependent_libs
	$(BUILD_SHARED_LIBRARY) libsalmoneye_fast.so driver_opt.o source_location_opt.o i_integer_opt.o o_integer_opt.o rational_opt.o regular_expression_opt.o token_opt.o tokenizer_opt.o parser_opt.o file_parser_opt.o value_opt.o statement_opt.o open_statement_opt.o statement_block_opt.o open_statement_block_opt.o expression_opt.o open_expression_opt.o type_expression_opt.o open_type_expression_opt.o basket_opt.o open_basket_opt.o basket_instance_opt.o lookup_actual_arguments_opt.o routine_instance_opt.o call_opt.o open_call_opt.o type_opt.o quark_opt.o object_opt.o declaration_opt.o variable_declaration_opt.o routine_declaration_opt.o open_routine_declaration_opt.o tagalong_declaration_opt.o lepton_key_declaration_opt.o quark_declaration_opt.o lock_declaration_opt.o declaration_list_opt.o slot_location_opt.o formal_arguments_opt.o routine_declaration_chain_opt.o routine_instance_chain_opt.o semi_labeled_value_list_opt.o tagalong_key_opt.o lepton_key_instance_opt.o unbound_opt.o bind_opt.o context_opt.o instance_opt.o variable_instance_opt.o static_home_opt.o jump_target_opt.o jumper_opt.o purity_level_opt.o lock_instance_opt.o lock_chain_opt.o virtual_lookup_opt.o use_instance_opt.o execute_opt.o validator_opt.o native_bridge_opt.o standard_built_ins_opt.o unicode_opt.o exceptions_opt.o reference_cluster_opt.o include_opt.o thread_opt.o trace_channels_opt.o utility_opt.o profile_opt.o platform_dependent_opt.o -Lc_foundations -lc_foundations_fast`./platform_dependent_libs`

c_foundations/libc_foundations_multi.a:
	cd c_foundations; make CFLAGS='$(CFLAGS)' libc_foundations_multi.a

salmoneye.multi: main_multi.o driver_multi.o source_location_multi.o i_integer_multi.o o_integer_multi.o rational_multi.o regular_expression_multi.o token_multi.o tokenizer_multi.o parser_multi.o file_parser_multi.o value_multi.o statement_multi.o open_statement_multi.o statement_block_multi.o open_statement_block_multi.o expression_multi.o open_expression_multi.o type_expression_multi.o open_type_expression_multi.o basket_multi.o open_basket_multi.o basket_instance_multi.o lookup_actual_arguments_multi.o routine_instance_multi.o call_multi.o open_call_multi.o type_multi.o quark_multi.o object_multi.o declaration_multi.o variable_declaration_multi.o routine_declaration_multi.o open_routine_declaration_multi.o tagalong_declaration_multi.o lepton_key_declaration_multi.o quark_declaration_multi.o lock_declaration_multi.o declaration_list_multi.o slot_location_multi.o formal_arguments_multi.o routine_declaration_chain_multi.o routine_instance_chain_multi.o semi_labeled_value_list_multi.o tagalong_key_multi.o lepton_key_instance_multi.o unbound_multi.o bind_multi.o context_multi.o instance_multi.o variable_instance_multi.o static_home_multi.o jump_target_multi.o jumper_multi.o purity_level_multi.o lock_instance_multi.o lock_chain_multi.o virtual_lookup_multi.o use_instance_multi.o execute_multi.o validator_multi.o native_bridge_multi.o standard_built_ins_multi.o unicode_multi.o exceptions_multi.o reference_cluster_multi.o include_multi.o thread_multi.o trace_channels_multi.o utility_multi.o profile_multi.o platform_dependent_multi.o c_foundations/libc_foundations_multi.a platform_dependent_libs
	$(BUILD_EXECUTABLE) salmoneye.multi main_multi.o driver_multi.o source_location_multi.o i_integer_multi.o o_integer_multi.o rational_multi.o regular_expression_multi.o token_multi.o tokenizer_multi.o parser_multi.o file_parser_multi.o value_multi.o statement_multi.o open_statement_multi.o statement_block_multi.o open_statement_block_multi.o expression_multi.o open_expression_multi.o type_expression_multi.o open_type_expression_multi.o basket_multi.o open_basket_multi.o basket_instance_multi.o lookup_actual_arguments_multi.o routine_instance_multi.o call_multi.o open_call_multi.o type_multi.o quark_multi.o object_multi.o declaration_multi.o variable_declaration_multi.o routine_declaration_multi.o open_routine_declaration_multi.o tagalong_declaration_multi.o lepton_key_declaration_multi.o quark_declaration_multi.o lock_declaration_multi.o declaration_list_multi.o slot_location_multi.o formal_arguments_multi.o routine_declaration_chain_multi.o routine_instance_chain_multi.o semi_labeled_value_list_multi.o tagalong_key_multi.o lepton_key_instance_multi.o unbound_multi.o bind_multi.o context_multi.o instance_multi.o variable_instance_multi.o static_home_multi.o jump_target_multi.o jumper_multi.o purity_level_multi.o lock_instance_multi.o lock_chain_multi.o virtual_lookup_multi.o use_instance_multi.o execute_multi.o validator_multi.o native_bridge_multi.o standard_built_ins_multi.o unicode_multi.o exceptions_multi.o reference_cluster_multi.o include_multi.o thread_multi.o trace_channels_multi.o utility_multi.o profile_multi.o platform_dependent_multi.o -Lc_foundations -lc_foundations_multi -lpthread`./platform_dependent_libs`

libsalmoneye_multi.so: driver_multi.o source_location_multi.o i_integer_multi.o o_integer_multi.o rational_multi.o regular_expression_multi.o token_multi.o tokenizer_multi.o parser_multi.o file_parser_multi.o value_multi.o statement_multi.o open_statement_multi.o statement_block_multi.o open_statement_block_multi.o expression_multi.o open_expression_multi.o type_expression_multi.o open_type_expression_multi.o basket_multi.o open_basket_multi.o basket_instance_multi.o lookup_actual_arguments_multi.o routine_instance_multi.o call_multi.o open_call_multi.o type_multi.o quark_multi.o object_multi.o declaration_multi.o variable_declaration_multi.o routine_declaration_multi.o open_routine_declaration_multi.o tagalong_declaration_multi.o lepton_key_declaration_multi.o quark_declaration_multi.o lock_declaration_multi.o declaration_list_multi.o slot_location_multi.o formal_arguments_multi.o routine_declaration_chain_multi.o routine_instance_chain_multi.o semi_labeled_value_list_multi.o tagalong_key_multi.o lepton_key_instance_multi.o unbound_multi.o bind_multi.o context_multi.o instance_multi.o variable_instance_multi.o static_home_multi.o jump_target_multi.o jumper_multi.o purity_level_multi.o lock_instance_multi.o lock_chain_multi.o virtual_lookup_multi.o use_instance_multi.o execute_multi.o validator_multi.o native_bridge_multi.o standard_built_ins_multi.o unicode_multi.o exceptions_multi.o reference_cluster_multi.o include_multi.o thread_multi.o trace_channels_multi.o utility_multi.o profile_multi.o platform_dependent_multi.o c_foundations/libc_foundations_multi.a platform_dependent_libs
	$(BUILD_SHARED_LIBRARY) libsalmoneye_multi.so driver_multi.o source_location_multi.o i_integer_multi.o o_integer_multi.o rational_multi.o regular_expression_multi.o token_multi.o tokenizer_multi.o parser_multi.o file_parser_multi.o value_multi.o statement_multi.o open_statement_multi.o statement_block_multi.o open_statement_block_multi.o expression_multi.o open_expression_multi.o type_expression_multi.o open_type_expression_multi.o basket_multi.o open_basket_multi.o basket_instance_multi.o lookup_actual_arguments_multi.o routine_instance_multi.o call_multi.o open_call_multi.o type_multi.o quark_multi.o object_multi.o declaration_multi.o variable_declaration_multi.o routine_declaration_multi.o open_routine_declaration_multi.o tagalong_declaration_multi.o lepton_key_declaration_multi.o quark_declaration_multi.o lock_declaration_multi.o declaration_list_multi.o slot_location_multi.o formal_arguments_multi.o routine_declaration_chain_multi.o routine_instance_chain_multi.o semi_labeled_value_list_multi.o tagalong_key_multi.o lepton_key_instance_multi.o unbound_multi.o bind_multi.o context_multi.o instance_multi.o variable_instance_multi.o static_home_multi.o jump_target_multi.o jumper_multi.o purity_level_multi.o lock_instance_multi.o lock_chain_multi.o virtual_lookup_multi.o use_instance_multi.o execute_multi.o validator_multi.o native_bridge_multi.o standard_built_ins_multi.o unicode_multi.o exceptions_multi.o reference_cluster_multi.o include_multi.o thread_multi.o trace_channels_multi.o utility_multi.o profile_multi.o platform_dependent_multi.o -Lc_foundations -lc_foundations_multi -lpthread`./platform_dependent_libs`

c_foundations/libc_foundations_check_multi.a:
	cd c_foundations; make CFLAGS='$(CFLAGS)' libc_foundations_check_multi.a

salmoneye.check.multi: main_m_ms_multi.o driver_m_ms_multi.o source_location_m_ms_multi.o i_integer_m_ms_multi.o o_integer_m_ms_multi.o rational_m_ms_multi.o regular_expression_m_ms_multi.o token_m_ms_multi.o tokenizer_m_ms_multi.o parser_m_ms_multi.o file_parser_m_ms_multi.o value_m_ms_multi.o statement_m_ms_multi.o open_statement_m_ms_multi.o statement_block_m_ms_multi.o open_statement_block_m_ms_multi.o expression_m_ms_multi.o open_expression_m_ms_multi.o type_expression_m_ms_multi.o open_type_expression_m_ms_multi.o basket_m_ms_multi.o open_basket_m_ms_multi.o basket_instance_m_ms_multi.o lookup_actual_arguments_m_ms_multi.o routine_instance_m_ms_multi.o call_m_ms_multi.o open_call_m_ms_multi.o type_m_ms_multi.o quark_m_ms_multi.o object_m_ms_multi.o declaration_m_ms_multi.o variable_declaration_m_ms_multi.o routine_declaration_m_ms_multi.o open_routine_declaration_m_ms_multi.o tagalong_declaration_m_ms_multi.o lepton_key_declaration_m_ms_multi.o quark_declaration_m_ms_multi.o lock_declaration_m_ms_multi.o declaration_list_m_ms_multi.o slot_location_m_ms_multi.o formal_arguments_m_ms_multi.o routine_declaration_chain_m_ms_multi.o routine_instance_chain_m_ms_multi.o semi_labeled_value_list_m_ms_multi.o tagalong_key_m_ms_multi.o lepton_key_instance_m_ms_multi.o unbound_m_ms_multi.o bind_m_ms_multi.o context_m_ms_multi.o instance_m_ms_multi.o variable_instance_m_ms_multi.o static_home_m_ms_multi.o jump_target_m_ms_multi.o jumper_m_ms_multi.o purity_level_m_ms_multi.o lock_instance_m_ms_multi.o lock_chain_m_ms_multi.o virtual_lookup_m_ms_multi.o use_instance_m_ms_multi.o execute_m_ms_multi.o validator_m_ms_multi.o native_bridge_m_ms_multi.o standard_built_ins_m_ms_multi.o unicode_m_ms_multi.o exceptions_m_ms_multi.o reference_cluster_m_ms_multi.o include_m_ms_multi.o thread_m_ms_multi.o trace_channels_m_ms_multi.o utility_m_ms_multi.o profile_m_ms_multi.o platform_dependent_m_ms_multi.o c_foundations/libc_foundations_check_multi.a platform_dependent_libs
	$(BUILD_EXECUTABLE) salmoneye.check.multi main_m_ms_multi.o driver_m_ms_multi.o source_location_m_ms_multi.o i_integer_m_ms_multi.o o_integer_m_ms_multi.o rational_m_ms_multi.o regular_expression_m_ms_multi.o token_m_ms_multi.o tokenizer_m_ms_multi.o parser_m_ms_multi.o file_parser_m_ms_multi.o value_m_ms_multi.o statement_m_ms_multi.o open_statement_m_ms_multi.o statement_block_m_ms_multi.o open_statement_block_m_ms_multi.o expression_m_ms_multi.o open_expression_m_ms_multi.o type_expression_m_ms_multi.o open_type_expression_m_ms_multi.o basket_m_ms_multi.o open_basket_m_ms_multi.o basket_instance_m_ms_multi.o lookup_actual_arguments_m_ms_multi.o routine_instance_m_ms_multi.o call_m_ms_multi.o open_call_m_ms_multi.o type_m_ms_multi.o quark_m_ms_multi.o object_m_ms_multi.o declaration_m_ms_multi.o variable_declaration_m_ms_multi.o routine_declaration_m_ms_multi.o open_routine_declaration_m_ms_multi.o tagalong_declaration_m_ms_multi.o lepton_key_declaration_m_ms_multi.o quark_declaration_m_ms_multi.o lock_declaration_m_ms_multi.o declaration_list_m_ms_multi.o slot_location_m_ms_multi.o formal_arguments_m_ms_multi.o routine_declaration_chain_m_ms_multi.o routine_instance_chain_m_ms_multi.o semi_labeled_value_list_m_ms_multi.o tagalong_key_m_ms_multi.o lepton_key_instance_m_ms_multi.o unbound_m_ms_multi.o bind_m_ms_multi.o context_m_ms_multi.o instance_m_ms_multi.o variable_instance_m_ms_multi.o static_home_m_ms_multi.o jump_target_m_ms_multi.o jumper_m_ms_multi.o purity_level_m_ms_multi.o lock_instance_m_ms_multi.o lock_chain_m_ms_multi.o virtual_lookup_m_ms_multi.o use_instance_m_ms_multi.o execute_m_ms_multi.o validator_m_ms_multi.o native_bridge_m_ms_multi.o standard_built_ins_m_ms_multi.o unicode_m_ms_multi.o exceptions_m_ms_multi.o reference_cluster_m_ms_multi.o include_m_ms_multi.o thread_m_ms_multi.o trace_channels_m_ms_multi.o utility_m_ms_multi.o profile_m_ms_multi.o platform_dependent_m_ms_multi.o -Lc_foundations -lc_foundations_check_multi -lpthread`./platform_dependent_libs`

libsalmoneye_check_multi.so: driver_m_ms_multi.o source_location_m_ms_multi.o i_integer_m_ms_multi.o o_integer_m_ms_multi.o rational_m_ms_multi.o regular_expression_m_ms_multi.o token_m_ms_multi.o tokenizer_m_ms_multi.o parser_m_ms_multi.o file_parser_m_ms_multi.o value_m_ms_multi.o statement_m_ms_multi.o open_statement_m_ms_multi.o statement_block_m_ms_multi.o open_statement_block_m_ms_multi.o expression_m_ms_multi.o open_expression_m_ms_multi.o type_expression_m_ms_multi.o open_type_expression_m_ms_multi.o basket_m_ms_multi.o open_basket_m_ms_multi.o basket_instance_m_ms_multi.o lookup_actual_arguments_m_ms_multi.o routine_instance_m_ms_multi.o call_m_ms_multi.o open_call_m_ms_multi.o type_m_ms_multi.o quark_m_ms_multi.o object_m_ms_multi.o declaration_m_ms_multi.o variable_declaration_m_ms_multi.o routine_declaration_m_ms_multi.o open_routine_declaration_m_ms_multi.o tagalong_declaration_m_ms_multi.o lepton_key_declaration_m_ms_multi.o quark_declaration_m_ms_multi.o lock_declaration_m_ms_multi.o declaration_list_m_ms_multi.o slot_location_m_ms_multi.o formal_arguments_m_ms_multi.o routine_declaration_chain_m_ms_multi.o routine_instance_chain_m_ms_multi.o semi_labeled_value_list_m_ms_multi.o tagalong_key_m_ms_multi.o lepton_key_instance_m_ms_multi.o unbound_m_ms_multi.o bind_m_ms_multi.o context_m_ms_multi.o instance_m_ms_multi.o variable_instance_m_ms_multi.o static_home_m_ms_multi.o jump_target_m_ms_multi.o jumper_m_ms_multi.o purity_level_m_ms_multi.o lock_instance_m_ms_multi.o lock_chain_m_ms_multi.o virtual_lookup_m_ms_multi.o use_instance_m_ms_multi.o execute_m_ms_multi.o validator_m_ms_multi.o native_bridge_m_ms_multi.o standard_built_ins_m_ms_multi.o unicode_m_ms_multi.o exceptions_m_ms_multi.o reference_cluster_m_ms_multi.o include_m_ms_multi.o thread_m_ms_multi.o trace_channels_m_ms_multi.o utility_m_ms_multi.o profile_m_ms_multi.o platform_dependent_m_ms_multi.o c_foundations/libc_foundations_check_multi.a platform_dependent_libs
	$(BUILD_SHARED_LIBRARY) libsalmoneye_check_multi.so driver_m_ms_multi.o source_location_m_ms_multi.o i_integer_m_ms_multi.o o_integer_m_ms_multi.o rational_m_ms_multi.o regular_expression_m_ms_multi.o token_m_ms_multi.o tokenizer_m_ms_multi.o parser_m_ms_multi.o file_parser_m_ms_multi.o value_m_ms_multi.o statement_m_ms_multi.o open_statement_m_ms_multi.o statement_block_m_ms_multi.o open_statement_block_m_ms_multi.o expression_m_ms_multi.o open_expression_m_ms_multi.o type_expression_m_ms_multi.o open_type_expression_m_ms_multi.o basket_m_ms_multi.o open_basket_m_ms_multi.o basket_instance_m_ms_multi.o lookup_actual_arguments_m_ms_multi.o routine_instance_m_ms_multi.o call_m_ms_multi.o open_call_m_ms_multi.o type_m_ms_multi.o quark_m_ms_multi.o object_m_ms_multi.o declaration_m_ms_multi.o variable_declaration_m_ms_multi.o routine_declaration_m_ms_multi.o open_routine_declaration_m_ms_multi.o tagalong_declaration_m_ms_multi.o lepton_key_declaration_m_ms_multi.o quark_declaration_m_ms_multi.o lock_declaration_m_ms_multi.o declaration_list_m_ms_multi.o slot_location_m_ms_multi.o formal_arguments_m_ms_multi.o routine_declaration_chain_m_ms_multi.o routine_instance_chain_m_ms_multi.o semi_labeled_value_list_m_ms_multi.o tagalong_key_m_ms_multi.o lepton_key_instance_m_ms_multi.o unbound_m_ms_multi.o bind_m_ms_multi.o context_m_ms_multi.o instance_m_ms_multi.o variable_instance_m_ms_multi.o static_home_m_ms_multi.o jump_target_m_ms_multi.o jumper_m_ms_multi.o purity_level_m_ms_multi.o lock_instance_m_ms_multi.o lock_chain_m_ms_multi.o virtual_lookup_m_ms_multi.o use_instance_m_ms_multi.o execute_m_ms_multi.o validator_m_ms_multi.o native_bridge_m_ms_multi.o standard_built_ins_m_ms_multi.o unicode_m_ms_multi.o exceptions_m_ms_multi.o reference_cluster_m_ms_multi.o include_m_ms_multi.o thread_m_ms_multi.o trace_channels_m_ms_multi.o utility_m_ms_multi.o profile_m_ms_multi.o platform_dependent_m_ms_multi.o -Lc_foundations -lc_foundations_check_multi -lpthread`./platform_dependent_libs`

c_foundations/libc_foundations_fast_multi.a:
	cd c_foundations; make CFLAGS='$(CFLAGS)' libc_foundations_fast_multi.a

salmoneye.fast.multi: main_opt_multi.o driver_opt_multi.o source_location_opt_multi.o i_integer_opt_multi.o o_integer_opt_multi.o rational_opt_multi.o regular_expression_opt_multi.o token_opt_multi.o tokenizer_opt_multi.o parser_opt_multi.o file_parser_opt_multi.o value_opt_multi.o statement_opt_multi.o open_statement_opt_multi.o statement_block_opt_multi.o open_statement_block_opt_multi.o expression_opt_multi.o open_expression_opt_multi.o type_expression_opt_multi.o open_type_expression_opt_multi.o basket_opt_multi.o open_basket_opt_multi.o basket_instance_opt_multi.o lookup_actual_arguments_opt_multi.o routine_instance_opt_multi.o call_opt_multi.o open_call_opt_multi.o type_opt_multi.o quark_opt_multi.o object_opt_multi.o declaration_opt_multi.o variable_declaration_opt_multi.o routine_declaration_opt_multi.o open_routine_declaration_opt_multi.o tagalong_declaration_opt_multi.o lepton_key_declaration_opt_multi.o quark_declaration_opt_multi.o lock_declaration_opt_multi.o declaration_list_opt_multi.o slot_location_opt_multi.o formal_arguments_opt_multi.o routine_declaration_chain_opt_multi.o routine_instance_chain_opt_multi.o semi_labeled_value_list_opt_multi.o tagalong_key_opt_multi.o lepton_key_instance_opt_multi.o unbound_opt_multi.o bind_opt_multi.o context_opt_multi.o instance_opt_multi.o variable_instance_opt_multi.o static_home_opt_multi.o jump_target_opt_multi.o jumper_opt_multi.o purity_level_opt_multi.o lock_instance_opt_multi.o lock_chain_opt_multi.o virtual_lookup_opt_multi.o use_instance_opt_multi.o execute_opt_multi.o validator_opt_multi.o native_bridge_opt_multi.o standard_built_ins_opt_multi.o unicode_opt_multi.o exceptions_opt_multi.o reference_cluster_opt_multi.o include_opt_multi.o thread_opt_multi.o trace_channels_opt_multi.o utility_opt_multi.o profile_opt_multi.o platform_dependent_opt_multi.o c_foundations/libc_foundations_fast_multi.a platform_dependent_libs
	$(BUILD_EXECUTABLE) salmoneye.fast.multi main_opt_multi.o driver_opt_multi.o source_location_opt_multi.o i_integer_opt_multi.o o_integer_opt_multi.o rational_opt_multi.o regular_expression_opt_multi.o token_opt_multi.o tokenizer_opt_multi.o parser_opt_multi.o file_parser_opt_multi.o value_opt_multi.o statement_opt_multi.o open_statement_opt_multi.o statement_block_opt_multi.o open_statement_block_opt_multi.o expression_opt_multi.o open_expression_opt_multi.o type_expression_opt_multi.o open_type_expression_opt_multi.o basket_opt_multi.o open_basket_opt_multi.o basket_instance_opt_multi.o lookup_actual_arguments_opt_multi.o routine_instance_opt_multi.o call_opt_multi.o open_call_opt_multi.o type_opt_multi.o quark_opt_multi.o object_opt_multi.o declaration_opt_multi.o variable_declaration_opt_multi.o routine_declaration_opt_multi.o open_routine_declaration_opt_multi.o tagalong_declaration_opt_multi.o lepton_key_declaration_opt_multi.o quark_declaration_opt_multi.o lock_declaration_opt_multi.o declaration_list_opt_multi.o slot_location_opt_multi.o formal_arguments_opt_multi.o routine_declaration_chain_opt_multi.o routine_instance_chain_opt_multi.o semi_labeled_value_list_opt_multi.o tagalong_key_opt_multi.o lepton_key_instance_opt_multi.o unbound_opt_multi.o bind_opt_multi.o context_opt_multi.o instance_opt_multi.o variable_instance_opt_multi.o static_home_opt_multi.o jump_target_opt_multi.o jumper_opt_multi.o purity_level_opt_multi.o lock_instance_opt_multi.o lock_chain_opt_multi.o virtual_lookup_opt_multi.o use_instance_opt_multi.o execute_opt_multi.o validator_opt_multi.o native_bridge_opt_multi.o standard_built_ins_opt_multi.o unicode_opt_multi.o exceptions_opt_multi.o reference_cluster_opt_multi.o include_opt_multi.o thread_opt_multi.o trace_channels_opt_multi.o utility_opt_multi.o profile_opt_multi.o platform_dependent_opt_multi.o -Lc_foundations -lc_foundations_fast_multi -lpthread`./platform_dependent_libs`

libsalmoneye_fast_multi.so: driver_opt_multi.o source_location_opt_multi.o i_integer_opt_multi.o o_integer_opt_multi.o rational_opt_multi.o regular_expression_opt_multi.o token_opt_multi.o tokenizer_opt_multi.o parser_opt_multi.o file_parser_opt_multi.o value_opt_multi.o statement_opt_multi.o open_statement_opt_multi.o statement_block_opt_multi.o open_statement_block_opt_multi.o expression_opt_multi.o open_expression_opt_multi.o type_expression_opt_multi.o open_type_expression_opt_multi.o basket_opt_multi.o open_basket_opt_multi.o basket_instance_opt_multi.o lookup_actual_arguments_opt_multi.o routine_instance_opt_multi.o call_opt_multi.o open_call_opt_multi.o type_opt_multi.o quark_opt_multi.o object_opt_multi.o declaration_opt_multi.o variable_declaration_opt_multi.o routine_declaration_opt_multi.o open_routine_declaration_opt_multi.o tagalong_declaration_opt_multi.o lepton_key_declaration_opt_multi.o quark_declaration_opt_multi.o lock_declaration_opt_multi.o declaration_list_opt_multi.o slot_location_opt_multi.o formal_arguments_opt_multi.o routine_declaration_chain_opt_multi.o routine_instance_chain_opt_multi.o semi_labeled_value_list_opt_multi.o tagalong_key_opt_multi.o lepton_key_instance_opt_multi.o unbound_opt_multi.o bind_opt_multi.o context_opt_multi.o instance_opt_multi.o variable_instance_opt_multi.o static_home_opt_multi.o jump_target_opt_multi.o jumper_opt_multi.o purity_level_opt_multi.o lock_instance_opt_multi.o lock_chain_opt_multi.o virtual_lookup_opt_multi.o use_instance_opt_multi.o execute_opt_multi.o validator_opt_multi.o native_bridge_opt_multi.o standard_built_ins_opt_multi.o unicode_opt_multi.o exceptions_opt_multi.o reference_cluster_opt_multi.o include_opt_multi.o thread_opt_multi.o trace_channels_opt_multi.o utility_opt_multi.o profile_opt_multi.o platform_dependent_opt_multi.o c_foundations/libc_foundations_fast_multi.a platform_dependent_libs
	$(BUILD_SHARED_LIBRARY) libsalmoneye_fast_multi.so driver_opt_multi.o source_location_opt_multi.o i_integer_opt_multi.o o_integer_opt_multi.o rational_opt_multi.o regular_expression_opt_multi.o token_opt_multi.o tokenizer_opt_multi.o parser_opt_multi.o file_parser_opt_multi.o value_opt_multi.o statement_opt_multi.o open_statement_opt_multi.o statement_block_opt_multi.o open_statement_block_opt_multi.o expression_opt_multi.o open_expression_opt_multi.o type_expression_opt_multi.o open_type_expression_opt_multi.o basket_opt_multi.o open_basket_opt_multi.o basket_instance_opt_multi.o lookup_actual_arguments_opt_multi.o routine_instance_opt_multi.o call_opt_multi.o open_call_opt_multi.o type_opt_multi.o quark_opt_multi.o object_opt_multi.o declaration_opt_multi.o variable_declaration_opt_multi.o routine_declaration_opt_multi.o open_routine_declaration_opt_multi.o tagalong_declaration_opt_multi.o lepton_key_declaration_opt_multi.o quark_declaration_opt_multi.o lock_declaration_opt_multi.o declaration_list_opt_multi.o slot_location_opt_multi.o formal_arguments_opt_multi.o routine_declaration_chain_opt_multi.o routine_instance_chain_opt_multi.o semi_labeled_value_list_opt_multi.o tagalong_key_opt_multi.o lepton_key_instance_opt_multi.o unbound_opt_multi.o bind_opt_multi.o context_opt_multi.o instance_opt_multi.o variable_instance_opt_multi.o static_home_opt_multi.o jump_target_opt_multi.o jumper_opt_multi.o purity_level_opt_multi.o lock_instance_opt_multi.o lock_chain_opt_multi.o virtual_lookup_opt_multi.o use_instance_opt_multi.o execute_opt_multi.o validator_opt_multi.o native_bridge_opt_multi.o standard_built_ins_opt_multi.o unicode_opt_multi.o exceptions_opt_multi.o reference_cluster_opt_multi.o include_opt_multi.o thread_opt_multi.o trace_channels_opt_multi.o utility_opt_multi.o profile_opt_multi.o platform_dependent_opt_multi.o -Lc_foundations -lc_foundations_fast_multi -lpthread`./platform_dependent_libs`

./generate_platform_dependent_salm: generate_platform_dependent_salm.c
	$(BUILD_EXECUTABLE) ./generate_platform_dependent_salm -DINSTALL_DIRECTORY='"$(INSTALL_DIRECTORY)"' -DBINARY_INSTALL_DIRECTORY='"$(BINARY_INSTALL_DIRECTORY)"' -DLIBRARY_INSTALL_DIRECTORY='"$(LIBRARY_INSTALL_DIRECTORY)"' -DDLL_INSTALL_DIRECTORY='"$(DLL_INSTALL_DIRECTORY)"' -DC_INCLUDE_INSTALL_DIRECTORY='"$(C_INCLUDE_INSTALL_DIRECTORY)"' -DSALMON_LIBRARY_INSTALL_DIRECTORY='"$(SALMON_LIBRARY_INSTALL_DIRECTORY)"' generate_platform_dependent_salm.c

library/platform_dependent.salm: ./generate_platform_dependent_salm
	./generate_platform_dependent_salm > library/platform_dependent.salm

platform_dependent_libs: platform_dependent_libs.c
	$(BUILD_EXECUTABLE) platform_dependent_libs platform_dependent_libs.c

install: salmoneye salmoneye.fast libsalmoneye.so libsalmoneye_fast.so c_foundations/libc_foundations.a c_foundations/libc_foundations_fast.a maybe_install_pthread driver.h basket.h basket_instance.h bind.h native_bridge.h call.h context.h execute.h validator.h expression.h formal_arguments.h i_integer.h o_integer.h jump_target.h jumper.h purity_level.h lepton_key_declaration.h lock_declaration.h declaration_list.h lock_instance.h lock_chain.h virtual_lookup.h use_instance.h lookup_actual_arguments.h object.h open_basket.h open_call.h open_expression.h open_routine_declaration.h open_statement.h open_statement_block.h open_type_expression.h routine_declaration_chain.h routine_instance_chain.h precedence.h parser.h file_parser.h quark.h quark_declaration.h rational.h regular_expression.h routine_instance.h routine_declaration.h semi_labeled_value_list.h slot_location.h source_location.h standard_built_ins.h unicode.h all_exceptions.h reference_cluster.h include.h thread.h trace_channels.h statement.h statement_block.h tagalong_declaration.h tagalong_key.h lepton_key_instance.h token.h tokenizer.h type.h type_expression.h unbound.h value.h declaration.h variable_declaration.h instance.h variable_instance.h static_home.h utility.h profile.h platform_dependent.h c_foundations/auto_array.h c_foundations/auto_array_implementation.h c_foundations/basic.h c_foundations/buffer_print.h c_foundations/code_point.h c_foundations/diagnostic.h c_foundations/memory_allocation.h c_foundations/memory_allocation_test.h c_foundations/merge_dense_integer_arrays.h c_foundations/print_formatting.h c_foundations/string_aam.h c_foundations/string_index.h c_foundations/trace.h c_foundations/try.h
	mkdir -p $(BINARY_INSTALL_DIRECTORY)
	cp salmoneye $(BINARY_INSTALL_DIRECTORY)
	cp salmoneye.fast $(BINARY_INSTALL_DIRECTORY)
	mkdir -p $(LIBRARY_INSTALL_DIRECTORY)
	cp c_foundations/libc_foundations.a $(LIBRARY_INSTALL_DIRECTORY)
	cp c_foundations/libc_foundations_fast.a $(LIBRARY_INSTALL_DIRECTORY)
	mkdir -p $(DLL_INSTALL_DIRECTORY)
	cp libsalmoneye.so $(DLL_INSTALL_DIRECTORY)
	cp libsalmoneye_fast.so $(DLL_INSTALL_DIRECTORY)
	mkdir -p $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye
	mkdir -p $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/c_foundations
	cp driver.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/driver.h
	cp basket.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/basket.h
	cp basket_instance.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/basket_instance.h
	cp bind.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/bind.h
	cp native_bridge.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/native_bridge.h
	cp call.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/call.h
	cp context.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/context.h
	cp execute.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/execute.h
	cp validator.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/validator.h
	cp expression.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/expression.h
	cp formal_arguments.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/formal_arguments.h
	cp i_integer.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/i_integer.h
	cp o_integer.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/o_integer.h
	cp jump_target.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/jump_target.h
	cp jumper.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/jumper.h
	cp purity_level.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/purity_level.h
	cp lepton_key_declaration.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/lepton_key_declaration.h
	cp lock_declaration.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/lock_declaration.h
	cp declaration_list.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/declaration_list.h
	cp lock_instance.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/lock_instance.h
	cp lock_chain.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/lock_chain.h
	cp virtual_lookup.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/virtual_lookup.h
	cp use_instance.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/use_instance.h
	cp lookup_actual_arguments.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/lookup_actual_arguments.h
	cp object.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/object.h
	cp open_basket.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/open_basket.h
	cp open_call.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/open_call.h
	cp open_expression.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/open_expression.h
	cp open_routine_declaration.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/open_routine_declaration.h
	cp open_statement.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/open_statement.h
	cp open_statement_block.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/open_statement_block.h
	cp open_type_expression.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/open_type_expression.h
	cp routine_declaration_chain.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/routine_declaration_chain.h
	cp routine_instance_chain.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/routine_instance_chain.h
	cp precedence.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/precedence.h
	cp parser.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/parser.h
	cp file_parser.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/file_parser.h
	cp quark.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/quark.h
	cp quark_declaration.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/quark_declaration.h
	cp rational.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/rational.h
	cp regular_expression.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/regular_expression.h
	cp routine_instance.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/routine_instance.h
	cp routine_declaration.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/routine_declaration.h
	cp semi_labeled_value_list.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/semi_labeled_value_list.h
	cp slot_location.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/slot_location.h
	cp source_location.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/source_location.h
	cp standard_built_ins.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/standard_built_ins.h
	cp unicode.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/unicode.h
	cp all_exceptions.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/all_exceptions.h
	cp reference_cluster.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/reference_cluster.h
	cp include.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/include.h
	cp thread.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/thread.h
	cp trace_channels.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/trace_channels.h
	cp statement.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/statement.h
	cp statement_block.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/statement_block.h
	cp tagalong_declaration.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/tagalong_declaration.h
	cp tagalong_key.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/tagalong_key.h
	cp lepton_key_instance.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/lepton_key_instance.h
	cp token.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/token.h
	cp tokenizer.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/tokenizer.h
	cp type.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/type.h
	cp type_expression.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/type_expression.h
	cp unbound.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/unbound.h
	cp value.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/value.h
	cp declaration.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/declaration.h
	cp variable_declaration.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/variable_declaration.h
	cp instance.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/instance.h
	cp variable_instance.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/variable_instance.h
	cp static_home.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/static_home.h
	cp utility.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/utility.h
	cp profile.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/profile.h
	cp platform_dependent.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/platform_dependent.h
	cp c_foundations/auto_array.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/c_foundations/auto_array.h
	cp c_foundations/auto_array_implementation.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/c_foundations/auto_array_implementation.h
	cp c_foundations/basic.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/c_foundations/basic.h
	cp c_foundations/buffer_print.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/c_foundations/buffer_print.h
	cp c_foundations/code_point.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/c_foundations/code_point.h
	cp c_foundations/diagnostic.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/c_foundations/diagnostic.h
	cp c_foundations/memory_allocation.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/c_foundations/memory_allocation.h
	cp c_foundations/memory_allocation_test.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/c_foundations/memory_allocation_test.h
	cp c_foundations/merge_dense_integer_arrays.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/c_foundations/merge_dense_integer_arrays.h
	cp c_foundations/print_formatting.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/c_foundations/print_formatting.h
	cp c_foundations/string_aam.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/c_foundations/string_aam.h
	cp c_foundations/string_index.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/c_foundations/string_index.h
	cp c_foundations/trace.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/c_foundations/trace.h
	cp c_foundations/try.h $(C_INCLUDE_INSTALL_DIRECTORY)/salmoneye/c_foundations/try.h
	mkdir -p $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/short.salm $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/shorter.salm $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/thread.si $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/date_utils.si $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/date_utils.salm $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/convertable.si $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/convertable.salm $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/numeric_operable.si $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/numeric_operable.salm $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/floating_point_interface.si $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/floating_point_interface.salm $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/unlimited_floating_point.si $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/unlimited_floating_point.salm $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/print_floating_point.si $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/print_floating_point.salm $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/scientific_form.si $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/scientific_form.salm $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/limited_floating_point.si $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/limited_floating_point.salm $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/interval.si $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/interval.salm $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/bullseye.si $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/bullseye.salm $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/logarithm.si $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/logarithm.salm $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/ieee754.si $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/ieee754.salm $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/string_char_op.salm $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/string_char_op.si $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/string_utf8_op.salm $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/string_utf8_op.si $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/string_utf16_op.salm $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/string_utf16_op.si $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/string_utf32_op.salm $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/string_utf32_op.si $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/parse.salm $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/parse.si $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/platform_dependent.si $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/module_makefile_generation.salm $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/module_makefile_generation.si $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp build/makefile_generation.salm $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp build/makefile_generation.si $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp build/makefile_generation2.salm $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp build/makefile_generation2.si $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/thread.salm $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	-cp library/thread.so $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	-cp library/unimplemented_thread.so $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	-cp library/convertable.so $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	-cp library/parse.so $(SALMON_LIBRARY_INSTALL_DIRECTORY)
	cp library/platform_dependent.salm $(SALMON_LIBRARY_INSTALL_DIRECTORY)

install_pthread: salmoneye.multi salmoneye.fast.multi libsalmoneye_multi.so libsalmoneye_fast_multi.so c_foundations/libc_foundations_fast_multi.a c_foundations/libc_foundations_multi.a
	mkdir -p $(BINARY_INSTALL_DIRECTORY)
	cp salmoneye.multi $(BINARY_INSTALL_DIRECTORY)
	cp salmoneye.fast.multi $(BINARY_INSTALL_DIRECTORY)
	mkdir -p $(LIBRARY_INSTALL_DIRECTORY)
	cp c_foundations/libc_foundations_fast_multi.a $(LIBRARY_INSTALL_DIRECTORY)
	cp c_foundations/libc_foundations_multi.a $(LIBRARY_INSTALL_DIRECTORY)
	mkdir -p $(DLL_INSTALL_DIRECTORY)
	cp libsalmoneye_multi.so $(DLL_INSTALL_DIRECTORY)
	cp libsalmoneye_fast_multi.so $(DLL_INSTALL_DIRECTORY)

maybe_install_pthread: Makefile.maybe_pthread
	make -f Makefile.maybe_pthread install

other_files: driver.h basket.h basket_instance.h bind.h native_bridge.h call.h context.h execute.h validator.h expression.h formal_arguments.h i_integer.h o_integer.h jump_target.h jumper.h purity_level.h lepton_key_declaration.h lock_declaration.h declaration_list.h lock_instance.h lock_chain.h virtual_lookup.h use_instance.h lookup_actual_arguments.h object.h open_basket.h open_call.h open_expression.h open_routine_declaration.h open_statement.h open_statement_block.h open_type_expression.h routine_declaration_chain.h routine_instance_chain.h precedence.h parser.h file_parser.h quark.h quark_declaration.h rational.h regular_expression.h routine_instance.h routine_declaration.h semi_labeled_value_list.h slot_location.h source_location.h standard_built_ins.h unicode.h all_exceptions.h reference_cluster.h include.h thread.h trace_channels.h statement.h statement_block.h tagalong_declaration.h tagalong_key.h lepton_key_instance.h token.h tokenizer.h type.h type_expression.h unbound.h value.h declaration.h variable_declaration.h instance.h variable_instance.h static_home.h utility.h profile.h platform_dependent.h c_foundations/auto_array.h c_foundations/auto_array_implementation.h c_foundations/basic.h c_foundations/buffer_print.h c_foundations/code_point.h c_foundations/diagnostic.h c_foundations/memory_allocation.h c_foundations/memory_allocation_test.h c_foundations/merge_dense_integer_arrays.h c_foundations/print_formatting.h c_foundations/string_aam.h c_foundations/string_index.h c_foundations/trace.h c_foundations/try.h library/short.salm library/shorter.salm library/thread.si library/date_utils.si library/date_utils.salm library/convertable.si library/convertable.salm library/numeric_operable.si library/numeric_operable.salm library/floating_point_interface.si library/floating_point_interface.salm library/unlimited_floating_point.si library/unlimited_floating_point.salm library/print_floating_point.si library/print_floating_point.salm library/scientific_form.si library/scientific_form.salm library/limited_floating_point.si library/limited_floating_point.salm library/interval.si library/interval.salm library/bullseye.si library/bullseye.salm library/logarithm.si library/logarithm.salm library/ieee754.si library/ieee754.salm library/string_char_op.salm library/string_char_op.si library/string_utf8_op.salm library/string_utf8_op.si library/string_utf16_op.salm library/string_utf16_op.si library/string_utf32_op.salm library/string_utf32_op.si library/parse.salm library/parse.si library/platform_dependent.si library/module_makefile_generation.salm library/module_makefile_generation.si library/nb_thread.c library/unimplemented_thread.c library/thread_implemented.salm library/thread_unimplemented.salm library/convertable.c library/parse.c generate_makefile.salm build/makefile_generation.si build/makefile_generation.salm build/makefile_generation2.si build/makefile_generation2.salm tests/multi.salm tests/multi_include.salm tests/multi_include2.salm tests/multi_include2.si tests/multi_empty.si tests/multi_one_item.si tests/multi_two_items.si tests/multi_name_reference.salm tests/complex.salm tests/expected_multi.txt tests/tester.salm tests/backtick.salm tests/expected_backtick.txt tests/system.salm tests/expected_system.txt tests/write_files.salm tests/expected_write_files.txt tests/hexdump.salm tests/check_and_delete.salm tests/expected_check_and_delete.txt tests/runall.salm tests/testlist.se tests/runlocal.salm tests/directory.salm tests/file_info.salm tests/date.salm tests/print_args.salm tests/non_unicode.c tests/non_unicode.salm tests/expected_non_unicode.txt tests/exceptions.salm tests/expected_exceptions.txt tests/print_types.salm tests/expected_print_types.txt tests/test_short.salm tests/expected_test_short.txt tests/test_shorter.salm tests/expected_test_shorter.txt tests/parse_manual.salm tests/test_reference_manual.salm tests/expected_test_reference_manual.txt tests/alias_warning.salm tests/alias_warning_driver.salm tests/expected_alias_warning.txt tests/invalid.salm tests/expected_invalid.txt tests/type.salm tests/expected_type.txt tests/manual_examples.salm tests/other.salm tests/expected_manual_examples.txt tests/native_bridge_test.c tests/do_native_bridge_test.salm tests/native_bridge_test.salm tests/expected_native_bridge_test.txt tests/native_bridge_include_test.c tests/do_native_bridge_include_test.salm tests/native_bridge_include_test.salm tests/expected_native_bridge_include_test.txt tests/garbage_collection.salm tests/expected_garbage_collection.txt tests/leak.salm tests/expected_leak.txt tests/unlimited_floating_point.salm tests/expected_unlimited_floating_point.txt tests/limited_floating_point.salm tests/expected_limited_floating_point.txt tests/interval.salm tests/expected_interval.txt tests/logarithm.salm tests/expected_logarithm.txt tests/ieee754.salm tests/expected_ieee754.txt tests/parse.salm tests/parse_block1.salm tests/parse_expression1.salm tests/expected_parse.txt tests/thread.salm tests/expected_thread.txt tests/thread_stress.salm tests/expected_thread_stress.txt examples/factorial.salm examples/factorial_no_recursion.salm examples/factorial_block.salm examples/y_combinator.salm examples/thread.salm examples/rename.salm run_medium_test_suite.salm run_very_large_test_suite.salm Makefile docs/Salmon_Reference_Manual.txt docs/Salmon_Tutorial.txt docs/Salmon_Summary_of_Features.txt docs/The_Design_of_Salmon.txt ReleaseNotes.txt status.txt

list_sources:
	@echo generate_maybe_pthread_makefile.salm
	@echo test_pthread.c
	@echo library/generate_makefile.salm
	@echo library/test_thread_link.c
	@echo main.c
	@echo driver.c
	@echo source_location.c
	@echo i_integer.c
	@echo o_integer.c
	@echo rational.c
	@echo regular_expression.c
	@echo token.c
	@echo tokenizer.c
	@echo parser.c
	@echo file_parser.c
	@echo value.c
	@echo statement.c
	@echo open_statement.c
	@echo statement_block.c
	@echo open_statement_block.c
	@echo expression.c
	@echo open_expression.c
	@echo type_expression.c
	@echo open_type_expression.c
	@echo basket.c
	@echo open_basket.c
	@echo basket_instance.c
	@echo lookup_actual_arguments.c
	@echo routine_instance.c
	@echo call.c
	@echo open_call.c
	@echo type.c
	@echo quark.c
	@echo object.c
	@echo declaration.c
	@echo variable_declaration.c
	@echo routine_declaration.c
	@echo open_routine_declaration.c
	@echo tagalong_declaration.c
	@echo lepton_key_declaration.c
	@echo quark_declaration.c
	@echo lock_declaration.c
	@echo declaration_list.c
	@echo slot_location.c
	@echo formal_arguments.c
	@echo routine_declaration_chain.c
	@echo routine_instance_chain.c
	@echo semi_labeled_value_list.c
	@echo tagalong_key.c
	@echo lepton_key_instance.c
	@echo unbound.c
	@echo bind.c
	@echo context.c
	@echo instance.c
	@echo variable_instance.c
	@echo static_home.c
	@echo jump_target.c
	@echo jumper.c
	@echo purity_level.c
	@echo lock_instance.c
	@echo lock_chain.c
	@echo virtual_lookup.c
	@echo use_instance.c
	@echo execute.c
	@echo validator.c
	@echo native_bridge.c
	@echo standard_built_ins.c
	@echo unicode.c
	@echo exceptions.c
	@echo reference_cluster.c
	@echo include.c
	@echo thread.c
	@echo trace_channels.c
	@echo utility.c
	@echo profile.c
	@echo platform_dependent.c
	@echo generate_platform_dependent_salm.c
	@echo platform_dependent_libs.c
	@echo driver.h
	@echo basket.h
	@echo basket_instance.h
	@echo bind.h
	@echo native_bridge.h
	@echo call.h
	@echo context.h
	@echo execute.h
	@echo validator.h
	@echo expression.h
	@echo formal_arguments.h
	@echo i_integer.h
	@echo o_integer.h
	@echo jump_target.h
	@echo jumper.h
	@echo purity_level.h
	@echo lepton_key_declaration.h
	@echo lock_declaration.h
	@echo declaration_list.h
	@echo lock_instance.h
	@echo lock_chain.h
	@echo virtual_lookup.h
	@echo use_instance.h
	@echo lookup_actual_arguments.h
	@echo object.h
	@echo open_basket.h
	@echo open_call.h
	@echo open_expression.h
	@echo open_routine_declaration.h
	@echo open_statement.h
	@echo open_statement_block.h
	@echo open_type_expression.h
	@echo routine_declaration_chain.h
	@echo routine_instance_chain.h
	@echo precedence.h
	@echo parser.h
	@echo file_parser.h
	@echo quark.h
	@echo quark_declaration.h
	@echo rational.h
	@echo regular_expression.h
	@echo routine_instance.h
	@echo routine_declaration.h
	@echo semi_labeled_value_list.h
	@echo slot_location.h
	@echo source_location.h
	@echo standard_built_ins.h
	@echo unicode.h
	@echo all_exceptions.h
	@echo reference_cluster.h
	@echo include.h
	@echo thread.h
	@echo trace_channels.h
	@echo statement.h
	@echo statement_block.h
	@echo tagalong_declaration.h
	@echo tagalong_key.h
	@echo lepton_key_instance.h
	@echo token.h
	@echo tokenizer.h
	@echo type.h
	@echo type_expression.h
	@echo unbound.h
	@echo value.h
	@echo declaration.h
	@echo variable_declaration.h
	@echo instance.h
	@echo variable_instance.h
	@echo static_home.h
	@echo utility.h
	@echo profile.h
	@echo platform_dependent.h
	@echo library/short.salm
	@echo library/shorter.salm
	@echo library/thread.si
	@echo library/date_utils.si
	@echo library/date_utils.salm
	@echo library/convertable.si
	@echo library/convertable.salm
	@echo library/numeric_operable.si
	@echo library/numeric_operable.salm
	@echo library/floating_point_interface.si
	@echo library/floating_point_interface.salm
	@echo library/unlimited_floating_point.si
	@echo library/unlimited_floating_point.salm
	@echo library/print_floating_point.si
	@echo library/print_floating_point.salm
	@echo library/scientific_form.si
	@echo library/scientific_form.salm
	@echo library/limited_floating_point.si
	@echo library/limited_floating_point.salm
	@echo library/interval.si
	@echo library/interval.salm
	@echo library/bullseye.si
	@echo library/bullseye.salm
	@echo library/logarithm.si
	@echo library/logarithm.salm
	@echo library/ieee754.si
	@echo library/ieee754.salm
	@echo library/string_char_op.salm
	@echo library/string_char_op.si
	@echo library/string_utf8_op.salm
	@echo library/string_utf8_op.si
	@echo library/string_utf16_op.salm
	@echo library/string_utf16_op.si
	@echo library/string_utf32_op.salm
	@echo library/string_utf32_op.si
	@echo library/parse.salm
	@echo library/parse.si
	@echo library/platform_dependent.si
	@echo library/module_makefile_generation.salm
	@echo library/module_makefile_generation.si
	@echo library/nb_thread.c
	@echo library/unimplemented_thread.c
	@echo library/thread_implemented.salm
	@echo library/thread_unimplemented.salm
	@echo library/convertable.c
	@echo library/parse.c
	@echo generate_makefile.salm
	@echo build/makefile_generation.si
	@echo build/makefile_generation.salm
	@echo build/makefile_generation2.si
	@echo build/makefile_generation2.salm
	@echo tests/multi.salm
	@echo tests/multi_include.salm
	@echo tests/multi_include2.salm
	@echo tests/multi_include2.si
	@echo tests/multi_empty.si
	@echo tests/multi_one_item.si
	@echo tests/multi_two_items.si
	@echo tests/multi_name_reference.salm
	@echo tests/complex.salm
	@echo tests/expected_multi.txt
	@echo tests/tester.salm
	@echo tests/backtick.salm
	@echo tests/expected_backtick.txt
	@echo tests/system.salm
	@echo tests/expected_system.txt
	@echo tests/write_files.salm
	@echo tests/expected_write_files.txt
	@echo tests/hexdump.salm
	@echo tests/check_and_delete.salm
	@echo tests/expected_check_and_delete.txt
	@echo tests/runall.salm
	@echo tests/testlist.se
	@echo tests/runlocal.salm
	@echo tests/directory.salm
	@echo tests/file_info.salm
	@echo tests/date.salm
	@echo tests/print_args.salm
	@echo tests/non_unicode.c
	@echo tests/non_unicode.salm
	@echo tests/expected_non_unicode.txt
	@echo tests/exceptions.salm
	@echo tests/expected_exceptions.txt
	@echo tests/print_types.salm
	@echo tests/expected_print_types.txt
	@echo tests/test_short.salm
	@echo tests/expected_test_short.txt
	@echo tests/test_shorter.salm
	@echo tests/expected_test_shorter.txt
	@echo tests/parse_manual.salm
	@echo tests/test_reference_manual.salm
	@echo tests/expected_test_reference_manual.txt
	@echo tests/alias_warning.salm
	@echo tests/alias_warning_driver.salm
	@echo tests/expected_alias_warning.txt
	@echo tests/invalid.salm
	@echo tests/expected_invalid.txt
	@echo tests/type.salm
	@echo tests/expected_type.txt
	@echo tests/manual_examples.salm
	@echo tests/other.salm
	@echo tests/expected_manual_examples.txt
	@echo tests/native_bridge_test.c
	@echo tests/do_native_bridge_test.salm
	@echo tests/native_bridge_test.salm
	@echo tests/expected_native_bridge_test.txt
	@echo tests/native_bridge_include_test.c
	@echo tests/do_native_bridge_include_test.salm
	@echo tests/native_bridge_include_test.salm
	@echo tests/expected_native_bridge_include_test.txt
	@echo tests/garbage_collection.salm
	@echo tests/expected_garbage_collection.txt
	@echo tests/leak.salm
	@echo tests/expected_leak.txt
	@echo tests/unlimited_floating_point.salm
	@echo tests/expected_unlimited_floating_point.txt
	@echo tests/limited_floating_point.salm
	@echo tests/expected_limited_floating_point.txt
	@echo tests/interval.salm
	@echo tests/expected_interval.txt
	@echo tests/logarithm.salm
	@echo tests/expected_logarithm.txt
	@echo tests/ieee754.salm
	@echo tests/expected_ieee754.txt
	@echo tests/parse.salm
	@echo tests/parse_block1.salm
	@echo tests/parse_expression1.salm
	@echo tests/expected_parse.txt
	@echo tests/thread.salm
	@echo tests/expected_thread.txt
	@echo tests/thread_stress.salm
	@echo tests/expected_thread_stress.txt
	@echo examples/factorial.salm
	@echo examples/factorial_no_recursion.salm
	@echo examples/factorial_block.salm
	@echo examples/y_combinator.salm
	@echo examples/thread.salm
	@echo examples/rename.salm
	@echo run_medium_test_suite.salm
	@echo run_very_large_test_suite.salm
	@echo Makefile
	@echo docs/Salmon_Reference_Manual.txt
	@echo docs/Salmon_Tutorial.txt
	@echo docs/Salmon_Summary_of_Features.txt
	@echo docs/The_Design_of_Salmon.txt
	@echo ReleaseNotes.txt
	@echo status.txt
	@echo c_foundations

.PHONY: all all_pthread maybe_pthread library install install_pthread maybe_install_pthread other_files list_sources

clean:
	-cd library; make clean
	rm -f Makefile.maybe_pthread libsalmoneye.dll library/Makefile main.o main_m_ms.o main_opt.o main_multi.o main_m_ms_multi.o main_opt_multi.o driver.o driver_m_ms.o driver_opt.o driver_multi.o driver_m_ms_multi.o driver_opt_multi.o source_location.o source_location_m_ms.o source_location_opt.o source_location_multi.o source_location_m_ms_multi.o source_location_opt_multi.o i_integer.o i_integer_m_ms.o i_integer_opt.o i_integer_multi.o i_integer_m_ms_multi.o i_integer_opt_multi.o o_integer.o o_integer_m_ms.o o_integer_opt.o o_integer_multi.o o_integer_m_ms_multi.o o_integer_opt_multi.o rational.o rational_m_ms.o rational_opt.o rational_multi.o rational_m_ms_multi.o rational_opt_multi.o regular_expression.o regular_expression_m_ms.o regular_expression_opt.o regular_expression_multi.o regular_expression_m_ms_multi.o regular_expression_opt_multi.o token.o token_m_ms.o token_opt.o token_multi.o token_m_ms_multi.o token_opt_multi.o tokenizer.o tokenizer_m_ms.o tokenizer_opt.o tokenizer_multi.o tokenizer_m_ms_multi.o tokenizer_opt_multi.o parser.o parser_m_ms.o parser_opt.o parser_multi.o parser_m_ms_multi.o parser_opt_multi.o file_parser.o file_parser_m_ms.o file_parser_opt.o file_parser_multi.o file_parser_m_ms_multi.o file_parser_opt_multi.o value.o value_m_ms.o value_opt.o value_multi.o value_m_ms_multi.o value_opt_multi.o statement.o statement_m_ms.o statement_opt.o statement_multi.o statement_m_ms_multi.o statement_opt_multi.o open_statement.o open_statement_m_ms.o open_statement_opt.o open_statement_multi.o open_statement_m_ms_multi.o open_statement_opt_multi.o statement_block.o statement_block_m_ms.o statement_block_opt.o statement_block_multi.o statement_block_m_ms_multi.o statement_block_opt_multi.o open_statement_block.o open_statement_block_m_ms.o open_statement_block_opt.o open_statement_block_multi.o open_statement_block_m_ms_multi.o open_statement_block_opt_multi.o expression.o expression_m_ms.o expression_opt.o expression_multi.o expression_m_ms_multi.o expression_opt_multi.o open_expression.o open_expression_m_ms.o open_expression_opt.o open_expression_multi.o open_expression_m_ms_multi.o open_expression_opt_multi.o type_expression.o type_expression_m_ms.o type_expression_opt.o type_expression_multi.o type_expression_m_ms_multi.o type_expression_opt_multi.o open_type_expression.o open_type_expression_m_ms.o open_type_expression_opt.o open_type_expression_multi.o open_type_expression_m_ms_multi.o open_type_expression_opt_multi.o basket.o basket_m_ms.o basket_opt.o basket_multi.o basket_m_ms_multi.o basket_opt_multi.o open_basket.o open_basket_m_ms.o open_basket_opt.o open_basket_multi.o open_basket_m_ms_multi.o open_basket_opt_multi.o basket_instance.o basket_instance_m_ms.o basket_instance_opt.o basket_instance_multi.o basket_instance_m_ms_multi.o basket_instance_opt_multi.o lookup_actual_arguments.o lookup_actual_arguments_m_ms.o lookup_actual_arguments_opt.o lookup_actual_arguments_multi.o lookup_actual_arguments_m_ms_multi.o lookup_actual_arguments_opt_multi.o routine_instance.o routine_instance_m_ms.o routine_instance_opt.o routine_instance_multi.o routine_instance_m_ms_multi.o routine_instance_opt_multi.o call.o call_m_ms.o call_opt.o call_multi.o call_m_ms_multi.o call_opt_multi.o open_call.o open_call_m_ms.o open_call_opt.o open_call_multi.o open_call_m_ms_multi.o open_call_opt_multi.o type.o type_m_ms.o type_opt.o type_multi.o type_m_ms_multi.o type_opt_multi.o quark.o quark_m_ms.o quark_opt.o quark_multi.o quark_m_ms_multi.o quark_opt_multi.o object.o object_m_ms.o object_opt.o object_multi.o object_m_ms_multi.o object_opt_multi.o declaration.o declaration_m_ms.o declaration_opt.o declaration_multi.o declaration_m_ms_multi.o declaration_opt_multi.o variable_declaration.o variable_declaration_m_ms.o variable_declaration_opt.o variable_declaration_multi.o variable_declaration_m_ms_multi.o variable_declaration_opt_multi.o routine_declaration.o routine_declaration_m_ms.o routine_declaration_opt.o routine_declaration_multi.o routine_declaration_m_ms_multi.o routine_declaration_opt_multi.o open_routine_declaration.o open_routine_declaration_m_ms.o open_routine_declaration_opt.o open_routine_declaration_multi.o open_routine_declaration_m_ms_multi.o open_routine_declaration_opt_multi.o tagalong_declaration.o tagalong_declaration_m_ms.o tagalong_declaration_opt.o tagalong_declaration_multi.o tagalong_declaration_m_ms_multi.o tagalong_declaration_opt_multi.o lepton_key_declaration.o lepton_key_declaration_m_ms.o lepton_key_declaration_opt.o lepton_key_declaration_multi.o lepton_key_declaration_m_ms_multi.o lepton_key_declaration_opt_multi.o quark_declaration.o quark_declaration_m_ms.o quark_declaration_opt.o quark_declaration_multi.o quark_declaration_m_ms_multi.o quark_declaration_opt_multi.o lock_declaration.o lock_declaration_m_ms.o lock_declaration_opt.o lock_declaration_multi.o lock_declaration_m_ms_multi.o lock_declaration_opt_multi.o declaration_list.o declaration_list_m_ms.o declaration_list_opt.o declaration_list_multi.o declaration_list_m_ms_multi.o declaration_list_opt_multi.o slot_location.o slot_location_m_ms.o slot_location_opt.o slot_location_multi.o slot_location_m_ms_multi.o slot_location_opt_multi.o formal_arguments.o formal_arguments_m_ms.o formal_arguments_opt.o formal_arguments_multi.o formal_arguments_m_ms_multi.o formal_arguments_opt_multi.o routine_declaration_chain.o routine_declaration_chain_m_ms.o routine_declaration_chain_opt.o routine_declaration_chain_multi.o routine_declaration_chain_m_ms_multi.o routine_declaration_chain_opt_multi.o routine_instance_chain.o routine_instance_chain_m_ms.o routine_instance_chain_opt.o routine_instance_chain_multi.o routine_instance_chain_m_ms_multi.o routine_instance_chain_opt_multi.o semi_labeled_value_list.o semi_labeled_value_list_m_ms.o semi_labeled_value_list_opt.o semi_labeled_value_list_multi.o semi_labeled_value_list_m_ms_multi.o semi_labeled_value_list_opt_multi.o tagalong_key.o tagalong_key_m_ms.o tagalong_key_opt.o tagalong_key_multi.o tagalong_key_m_ms_multi.o tagalong_key_opt_multi.o lepton_key_instance.o lepton_key_instance_m_ms.o lepton_key_instance_opt.o lepton_key_instance_multi.o lepton_key_instance_m_ms_multi.o lepton_key_instance_opt_multi.o unbound.o unbound_m_ms.o unbound_opt.o unbound_multi.o unbound_m_ms_multi.o unbound_opt_multi.o bind.o bind_m_ms.o bind_opt.o bind_multi.o bind_m_ms_multi.o bind_opt_multi.o context.o context_m_ms.o context_opt.o context_multi.o context_m_ms_multi.o context_opt_multi.o instance.o instance_m_ms.o instance_opt.o instance_multi.o instance_m_ms_multi.o instance_opt_multi.o variable_instance.o variable_instance_m_ms.o variable_instance_opt.o variable_instance_multi.o variable_instance_m_ms_multi.o variable_instance_opt_multi.o static_home.o static_home_m_ms.o static_home_opt.o static_home_multi.o static_home_m_ms_multi.o static_home_opt_multi.o jump_target.o jump_target_m_ms.o jump_target_opt.o jump_target_multi.o jump_target_m_ms_multi.o jump_target_opt_multi.o jumper.o jumper_m_ms.o jumper_opt.o jumper_multi.o jumper_m_ms_multi.o jumper_opt_multi.o purity_level.o purity_level_m_ms.o purity_level_opt.o purity_level_multi.o purity_level_m_ms_multi.o purity_level_opt_multi.o lock_instance.o lock_instance_m_ms.o lock_instance_opt.o lock_instance_multi.o lock_instance_m_ms_multi.o lock_instance_opt_multi.o lock_chain.o lock_chain_m_ms.o lock_chain_opt.o lock_chain_multi.o lock_chain_m_ms_multi.o lock_chain_opt_multi.o virtual_lookup.o virtual_lookup_m_ms.o virtual_lookup_opt.o virtual_lookup_multi.o virtual_lookup_m_ms_multi.o virtual_lookup_opt_multi.o use_instance.o use_instance_m_ms.o use_instance_opt.o use_instance_multi.o use_instance_m_ms_multi.o use_instance_opt_multi.o execute.o execute_m_ms.o execute_opt.o execute_multi.o execute_m_ms_multi.o execute_opt_multi.o validator.o validator_m_ms.o validator_opt.o validator_multi.o validator_m_ms_multi.o validator_opt_multi.o native_bridge.o native_bridge_m_ms.o native_bridge_opt.o native_bridge_multi.o native_bridge_m_ms_multi.o native_bridge_opt_multi.o standard_built_ins.o standard_built_ins_m_ms.o standard_built_ins_opt.o standard_built_ins_multi.o standard_built_ins_m_ms_multi.o standard_built_ins_opt_multi.o unicode.o unicode_m_ms.o unicode_opt.o unicode_multi.o unicode_m_ms_multi.o unicode_opt_multi.o exceptions.o exceptions_m_ms.o exceptions_opt.o exceptions_multi.o exceptions_m_ms_multi.o exceptions_opt_multi.o reference_cluster.o reference_cluster_m_ms.o reference_cluster_opt.o reference_cluster_multi.o reference_cluster_m_ms_multi.o reference_cluster_opt_multi.o include.o include_m_ms.o include_opt.o include_multi.o include_m_ms_multi.o include_opt_multi.o thread.o thread_m_ms.o thread_opt.o thread_multi.o thread_m_ms_multi.o thread_opt_multi.o trace_channels.o trace_channels_m_ms.o trace_channels_opt.o trace_channels_multi.o trace_channels_m_ms_multi.o trace_channels_opt_multi.o utility.o utility_m_ms.o utility_opt.o utility_multi.o utility_m_ms_multi.o utility_opt_multi.o profile.o profile_m_ms.o profile_opt.o profile_multi.o profile_m_ms_multi.o profile_opt_multi.o platform_dependent.o platform_dependent_m_ms.o platform_dependent_opt.o platform_dependent_multi.o platform_dependent_m_ms_multi.o platform_dependent_opt_multi.o c_foundations/libc_foundations.a salmoneye salmoneye.exe libsalmoneye.so c_foundations/libc_foundations_check.a salmoneye.check salmoneye.check.exe libsalmoneye_check.so c_foundations/libc_foundations_fast.a salmoneye.fast salmoneye.fast.exe libsalmoneye_fast.so c_foundations/libc_foundations_multi.a salmoneye.multi salmoneye.multi.exe libsalmoneye_multi.so c_foundations/libc_foundations_check_multi.a salmoneye.check.multi salmoneye.check.multi.exe libsalmoneye_check_multi.so c_foundations/libc_foundations_fast_multi.a salmoneye.fast.multi salmoneye.fast.multi.exe libsalmoneye_fast_multi.so ./generate_platform_dependent_salm ./generate_platform_dependent_salm.exe library/platform_dependent.salm platform_dependent_libs platform_dependent_libs.exe
	cd c_foundations; make clean
