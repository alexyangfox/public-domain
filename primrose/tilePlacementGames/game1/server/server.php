<?php



global $ps_version;
$ps_version = "0.1";



// edit settings.php to change server' settings
include( "settings.php" );




// no end-user settings below this point




// enable verbose error reporting to detect uninitialized variables
error_reporting( E_ALL );



// page layout for web-based setup
$setup_header = "
<HTML>
<HEAD><TITLE>Primrose Server Web-based setup</TITLE></HEAD>
<BODY BGCOLOR=#FFFFFF TEXT=#000000 LINK=#0000FF VLINK=#FF0000>

<CENTER>
<TABLE WIDTH=75% BORDER=0 CELLSPACING=0 CELLPADDING=1>
<TR><TD BGCOLOR=#000000>
<TABLE WIDTH=100% BORDER=0 CELLSPACING=0 CELLPADDING=10>
<TR><TD BGCOLOR=#EEEEEE>";

$setup_footer = "
</TD></TR></TABLE>
</TD></TR></TABLE>
</CENTER>
</BODY></HTML>";





// ensure that magic quotes are on (adding slashes before quotes
// so that user-submitted data can be safely submitted in DB queries)
if( !get_magic_quotes_gpc() ) {
    // force magic quotes to be added
    $_GET     = array_map( 'gs_addslashes_deep', $_GET );
    $_POST    = array_map( 'gs_addslashes_deep', $_POST );
    $_REQUEST = array_map( 'gs_addslashes_deep', $_REQUEST );
    $_COOKIE  = array_map( 'gs_addslashes_deep', $_COOKIE );
    }
    





// all calls need to connect to DB, so do it once here
ps_connectToDatabase();

// close connection down below (before function declarations)


// testing:
//sleep( 5 );


// general processing whenver server.php is accessed directly




// grab POST/GET variables
$action = "";
if( isset( $_REQUEST[ "action" ] ) ) {
    $action = $_REQUEST[ "action" ];
    }

$debug = "";
if( isset( $_REQUEST[ "debug" ] ) ) {
    $debug = $_REQUEST[ "debug" ];
    }



if( $action == "version" ) {
    global $ps_version;
    echo "$ps_version";
    }
else if( $action == "show_log" ) {
    ps_showLog();
    }
else if( $action == "clear_log" ) {
    ps_clearLog();
    }
else if( $action == "post_score" ) {
    ps_postScore();
    }
else if( $action == "fetch_scores" ) {
    ps_fetchScores();
    }
else if( $action == "ps_setup" ) {
    global $setup_header, $setup_footer;
    echo $setup_header; 

    echo "<H2>Primrose Server Web-based Setup</H2>";

    echo "Creating tables:<BR>";

    echo "<CENTER><TABLE BORDER=0 CELLSPACING=0 CELLPADDING=1>
          <TR><TD BGCOLOR=#000000>
          <TABLE BORDER=0 CELLSPACING=0 CELLPADDING=5>
          <TR><TD BGCOLOR=#FFFFFF>";

    ps_setupDatabase();

    echo "</TD></TR></TABLE></TD></TR></TABLE></CENTER><BR><BR>";
    
    echo $setup_footer;
    }
else if( preg_match( "/server\.php/", $_SERVER[ "SCRIPT_NAME" ] ) ) {
    // server.php has been called without an action parameter

    // the preg_match ensures that server.php was called directly and
    // not just included by another script
    
    // quick (and incomplete) test to see if we should show instructions
    global $tableNamePrefix;
    
    // check if our "all_time_scores" table exists
    $tableName = $tableNamePrefix . "all_time_scores";
    
    $exists = ps_doesTableExist( $tableName );
        
    if( $exists  ) {
        echo "Primrose server database setup and ready";
        }
    else {
        // start the setup procedure

        global $setup_header, $setup_footer;
        echo $setup_header; 

        echo "<H2>Primrose Server Web-based Setup</H2>";
    
        echo "Primrose Server will walk you through a " .
            "brief setup process.<BR><BR>";
        
        echo "Step 1: ".
            "<A HREF=\"server.php?action=ps_setup\">".
            "create the database tables</A>";

        echo $setup_footer;
        }
    }



// done processing
// only function declarations below

ps_closeDatabase();







/**
 * Creates the database tables needed by primrose server.
 */
function ps_setupDatabase() {
    global $tableNamePrefix;


    $tableName = $tableNamePrefix . "log";
    if( ! ps_doesTableExist( $tableName ) ) {

        // this table contains general info about the server
        // use INNODB engine so table can be locked
        $query =
            "CREATE TABLE $tableName(" .
            "entry TEXT NOT NULL, ".
            "entry_time DATETIME NOT NULL );";

        $result = ps_queryDatabase( $query );

        echo "<B>$tableName</B> table created<BR>";
        }
    else {
        echo "<B>$tableName</B> table already exists<BR>";
        }

    
    
    $tableName = $tableNamePrefix . "all_time_scores";
    if( ! ps_doesTableExist( $tableName ) ) {

        // this table contains general info about each game
        // use INNODB engine so table can be locked
        $query =
            "CREATE TABLE $tableName(" .
            "name VARCHAR(8) NOT NULL," .
            "score INT NOT NULL," .
            "seed INT NOT NULL," .
            "move_history TEXT NOT NULL," .
            "post_date DATETIME NOT NULL );";

        $result = ps_queryDatabase( $query );

        echo "<B>$tableName</B> table created<BR>";
        }
    else {
        echo "<B>$tableName</B> table already exists<BR>";
        }

    
    $tableName = $tableNamePrefix . "today_scores";
    if( ! ps_doesTableExist( $tableName ) ) {

        // this table contains general info about each game
        // use INNODB engine so table can be locked
        $query =
            "CREATE TABLE $tableName(" .
            "name VARCHAR(8) NOT NULL," .
            "score INT NOT NULL," .
            "seed INT NOT NULL," .
            "move_history TEXT NOT NULL," .
            "post_date DATETIME NOT NULL );";

        $result = ps_queryDatabase( $query );

        echo "<B>$tableName</B> table created<BR>";
        }
    else {
        echo "<B>$tableName</B> table already exists<BR>";
        }
    }



function ps_showLog() {
    $password = "";
    if( isset( $_REQUEST[ "password" ] ) ) {
        $password = $_REQUEST[ "password" ];
        }

    global $logAccessPassword, $tableNamePrefix;
    
    if( $password != $logAccessPassword ) {
        echo "Incorrect password.";

        ps_log( "Failed show_log access with password:  $password" );
        }
    else {
        $query = "SELECT * FROM $tableNamePrefix"."log;";
        $result = ps_queryDatabase( $query );

        $numRows = mysql_numrows( $result );

        echo "$numRows log entries:<br><br><br>\n";
        

        for( $i=0; $i<$numRows; $i++ ) {
            $time = mysql_result( $result, $i, "entry_time" );
            $entry = mysql_result( $result, $i, "entry" );

            echo "<b>$time</b>:<br>$entry<hr>\n";
            }
        }
    }



function ps_clearLog() {
    $password = "";
    if( isset( $_REQUEST[ "password" ] ) ) {
        $password = $_REQUEST[ "password" ];
        }

    global $logAccessPassword, $tableNamePrefix;
    
    if( $password != $logAccessPassword ) {
        echo "Incorrect password.";

        ps_log( "Failed clear_log access with password:  $password" );
        }
    else {
        $query = "DELETE FROM $tableNamePrefix"."log;";
        $result = ps_queryDatabase( $query );

        if( $result ) {
            echo "Log cleared.";
            }
        else {
            echo "DELETE operation failed?";
            }
        }
    }



function ps_fetchScores() {
    global $tableNamePrefix, $listSize;
    
    $query = "SELECT * FROM $tableNamePrefix"."all_time_scores ".
        "ORDER BY score DESC LIMIT $listSize;";
    $result = ps_queryDatabase( $query );

    $numRows = mysql_numrows( $result );

    echo "$numRows\n";

    for( $i=0; $i<$numRows; $i++ ) {
        echo mysql_result( $result, $i, "name" );
        echo "#";
        echo mysql_result( $result, $i, "score" );
        echo "#";
        echo mysql_result( $result, $i, "seed" );
        echo "#";
        echo mysql_result( $result, $i, "move_history" );
        echo "\n";
        }


    // drop those older than one day
    $query = "DELETE FROM $tableNamePrefix"."today_scores ".
        "WHERE post_date < ".
        "    SUBTIME( CURRENT_TIMESTAMP, '1 0:00:00.00' );";

    $result = ps_queryDatabase( $query );
    $numRowsRemoved = mysql_affected_rows();
    
    if( $numRowsRemoved ) {
        ps_log( "Removed $numRowsRemoved stale scores from daily list." );
        }

    
    
    $query = "SELECT * FROM $tableNamePrefix"."today_scores ".
        "ORDER BY score DESC LIMIT $listSize;";
    $result = ps_queryDatabase( $query );

    $numRows = mysql_numrows( $result );

    echo "$numRows\n";

    for( $i=0; $i<$numRows; $i++ ) {
        echo mysql_result( $result, $i, "name" );
        echo "#";
        echo mysql_result( $result, $i, "score" );
        echo "#";
        echo mysql_result( $result, $i, "seed" );
        echo "#";
        echo mysql_result( $result, $i, "move_history" );
        echo "\n";
        }
        
    }




function ps_postScore() {

    global $tableNamePrefix, $listSize;
    
    $name = "";
    $score = "";
    $seed = "";
    $move_history = "";
    $hash = "";
    
    
    if( isset( $_REQUEST[ "name" ] ) ) {
        $name = $_REQUEST[ "name" ];
        }
    if( isset( $_REQUEST[ "score" ] ) ) {
        $score = $_REQUEST[ "score" ];
        }
    if( isset( $_REQUEST[ "seed" ] ) ) {
        $seed = $_REQUEST[ "seed" ];
        }
    if( isset( $_REQUEST[ "move_history" ] ) ) {
        $move_history = $_REQUEST[ "move_history" ];
        }
    if( isset( $_REQUEST[ "hash" ] ) ) {
        $hash = strtoupper( $_REQUEST[ "hash" ] );
        }


    
    $postValid = false;


    $numMoves = strlen( $move_history );
    
    if( $numMoves > 48 ) {
        // at least enough moves to fill board (shorter strings are dummies!)

        if( preg_match( "/[A-Za-w]+/", $move_history, $matches ) == 1 ) {
            if( strlen( $matches[0] ) == $numMoves ) {

                // entire move string contains proper base-49 digits

                
                // check hash of all values
                global $secureSalt;
                
                $ourHash = strtoupper( sha1( $name . $score . $seed .
                                             $move_history . $secureSalt ) );
                
                if( $ourHash == $hash ) {
                    $postValid = true;
                    }
                else {
                    ps_log( "Hash mismatch for score post.  Provided $hash,  ".
                            "expecting $ourHash" );
                    }
                }
            }
        }

    
    if( ! $postValid ) {
        ps_log( "Bad score post: $name, $score, $seed, $move_history, $hash" );
        }
    else {
        ps_log( "Score of $score posted by $name" );
        }
    
    
    
    if( $postValid ) {    
        $query = "INSERT INTO $tableNamePrefix"."today_scores VALUES ( " .
            "'$name', '$score', '$seed', '$move_history', ".
            "CURRENT_TIMESTAMP );";
        
        
        $result = ps_queryDatabase( $query );
    

        $query = "SELECT COUNT(*) FROM $tableNamePrefix"."today_scores;";
        
        $result = ps_queryDatabase( $query );
        
        $count = mysql_result( $result, 0, 0 );
        
        
        if( $count > 50 ) {
            // drop lowest one

            $query = "SELECT * FROM $tableNamePrefix"."today_scores ".
                "ORDER BY score ASC LIMIT 1;";
            $result = ps_queryDatabase( $query );
        
            $numRows = mysql_numrows( $result );
        
            if( $numRows > 0 ) {
            
                $dropName = mysql_result( $result, 0, "name" );
                $dropScore = mysql_result( $result, 0, "score" );

                $query = "DELETE FROM $tableNamePrefix"."today_scores ".
                    "WHERE name = '$dropName' ".
                    "AND score = '$dropScore';";
            
                $result = ps_queryDatabase( $query );
                }
        
            }
    
    

        // check for lowest score in all-time highs
        $query = "SELECT * FROM $tableNamePrefix"."all_time_scores ".
            "ORDER BY score ASC;";
        $result = ps_queryDatabase( $query );

        $numRows = mysql_numrows( $result );

        $addToAllTime = false;
    
    
        // maintain only $listSize in all-time-table
        if( $numRows > $listSize - 1 ) {

            // lowest one in table
            $otherScore = mysql_result( $result, 0, "score" );
        
            if( $score > $otherScore ) {

                $otherName = mysql_result( $result, 0, "name" );

                $query = "DELETE FROM $tableNamePrefix"."all_time_scores ".
                    "WHERE name = '$otherName' ".
                    "AND score = '$otherScore';";

                ps_log( "Bumping $otherName from all-time list ".
                        "with score: $otherScore." );

            
                $result = ps_queryDatabase( $query );

                $addToAllTime = true;
                }        
            }
        else {
            // table not full!
            $addToAllTime = true;
            }

        if( $addToAllTime ) {
            $query =
                "INSERT INTO $tableNamePrefix". "all_time_scores VALUES ( " .
                "'$name', '$score', '$seed', '$move_history', ".
                "CURRENT_TIMESTAMP );";

            ps_log( "New score on all-time list by $name: $score." );
        
            $result = ps_queryDatabase( $query );
            }
        }
    
    
    // return same output as fetching score
    ps_fetchScores();
    }




// general-purpose functions down here, many copied from seedBlogs

/**
 * Connects to the database according to the database variables.
 */  
function ps_connectToDatabase() {
    global $databaseServer,
        $databaseUsername, $databasePassword, $databaseName;
    
    
    mysql_connect( $databaseServer, $databaseUsername, $databasePassword )
        or ps_fatalError( "Could not connect to database server: " .
                       mysql_error() );
    
	mysql_select_db( $databaseName )
        or ps_fatalError( "Could not select $databaseName database: " .
                       mysql_error() );
    }


 
/**
 * Closes the database connection.
 */
function ps_closeDatabase() {
    mysql_close();
    }



/**
 * Queries the database, and dies with an error message on failure.
 *
 * @param $inQueryString the SQL query string.
 *
 * @return a result handle that can be passed to other mysql functions.
 */
function ps_queryDatabase( $inQueryString ) {

    $result = mysql_query( $inQueryString )
        or ps_fatalError( "Database query failed:<BR>$inQueryString<BR><BR>" .
                       mysql_error() );

    return $result;
    }



/**
 * Checks whether a table exists in the currently-connected database.
 *
 * @param $inTableName the name of the table to look for.
 *
 * @return 1 if the table exists, or 0 if not.
 */
function ps_doesTableExist( $inTableName ) {
    // check if our table exists
    $tableExists = 0;
    
    $query = "SHOW TABLES";
    $result = ps_queryDatabase( $query );

    $numRows = mysql_numrows( $result );


    for( $i=0; $i<$numRows && ! $tableExists; $i++ ) {

        $tableName = mysql_result( $result, $i, 0 );
        
        if( $tableName == $inTableName ) {
            $tableExists = 1;
            }
        }
    return $tableExists;
    }



function ps_log( $message ) {
    global $enableLog, $tableNamePrefix;

    $slashedMessage = addslashes( $message );
    
    if( $enableLog ) {
        $query = "INSERT INTO $tableNamePrefix"."log VALUES ( " .
            "'$slashedMessage', CURRENT_TIMESTAMP );";
        $result = ps_queryDatabase( $query );
        }
    }



/**
 * Displays the error page and dies.
 *
 * @param $message the error message to display on the error page.
 */
function ps_fatalError( $message ) {
    //global $errorMessage;

    // set the variable that is displayed inside error.php
    //$errorMessage = $message;
    
    //include_once( "error.php" );

    // for now, just print error message
    $logMessage = "Fatal error:  $message";
    
    echo( $logMessage );

    ps_log( $logMessage );
    
    die();
    }



/**
 * Displays the operation error message and dies.
 *
 * @param $message the error message to display.
 */
function ps_operationError( $message ) {
    
    // for now, just print error message
    echo( "ERROR:  $message" );
    die();
    }


/**
 * Recursively applies the addslashes function to arrays of arrays.
 * This effectively forces magic_quote escaping behavior, eliminating
 * a slew of possible database security issues. 
 *
 * @inValue the value or array to addslashes to.
 *
 * @return the value or array with slashes added.
 */
function ps_addslashes_deep( $inValue ) {
    return
        ( is_array( $inValue )
          ? array_map( 'ps_addslashes_deep', $inValue )
          : addslashes( $inValue ) );
    }



/**
 * Recursively applies the stripslashes function to arrays of arrays.
 * This effectively disables magic_quote escaping behavior. 
 *
 * @inValue the value or array to stripslashes from.
 *
 * @return the value or array with slashes removed.
 */
function ps_stripslashes_deep( $inValue ) {
    return
        ( is_array( $inValue )
          ? array_map( 'sb_stripslashes_deep', $inValue )
          : stripslashes( $inValue ) );
    }

?>
