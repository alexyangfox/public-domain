// interface for making sure network is up

// used on iPhone to ensure connectivity before making a socket request... 
// does nothing on always-networked platforms


// true for always-on platforms
//  makeSureNetworkIsUp can be skipped on those platforms
char isNetworkAlwaysOn();




// makes sure the network is up, and tries to bring it up if it is not
// uses inTestURL to check reachability

// note that this call might block
char makeSureNetworkIsUp( char *inTestURL );
