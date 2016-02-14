<?php

// Basic settings
// You must set these for the server to work

$databaseServer = "localhost";
$databaseUsername = "testUser";
$databasePassword = "testPassword";
$databaseName = "test";

// used to generate a hash of posted values
// must be a file containing:
//
//      < ? php   $secureSalt = "change_me";   ? >
//
// where "change_me" is a value shared with the client
include( "secureSalt.php" );



// The URL of to the server.php script.
$fullServerURL = "http://localhost:8080/jcr13/primrose/server.php";

// End Basic settings



// Customization settings

// Adjust these to change the way the server  works.


// Prefix to use in table names (in case more than one application is using
// the same database).  Two tables are created:  "games" and "columns".
//
// If $tableNamePrefix is "test_" then the tables will be named
// "test_games" and "test_columns".  Thus, more than one server
// installation can use the same database (or the server can share a database
// with another application that uses similar table names).
$tableNamePrefix = "psdemo_";


$enableLog = 1;

// for log view/clear operations
$logAccessPassword = "secret";


// number of score items to return on each list
$listSize = 8;




?>